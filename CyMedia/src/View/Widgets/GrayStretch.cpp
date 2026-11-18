#include "GrayStretch.h"

#include "../../Calculate/Calculate.h"
#include "../../common/CyMediaLog.h"
#include "../Widgets/CyCellStepper.h"
#include "../Widgets/CyHistogramView.h"
#include "../Widgets/cycustomwidget.h"

#include <QBoxLayout>
#include <QLabel>
#include <QSignalBlocker>

#include <cmath>

enum grayHistogrameIndex {
    hisI_Gray = 0,
    hisI_R,
    hisI_G,
    hisI_B
};

namespace {
    /// 直方图桶数上限：超过后 Calculate 会分配巨量内存，这里直接拒绝并提示。
    const int kMaxHistogramBits = 20;
}

struct CyMediaDisGrayStretch::PrivateData {
public:
    //UI
    QColor mThemeColor = QColor(0x2a, 0xa3, 0xc6);
    bool mIsInit = false;

    CyHistogramView* mHisView = nullptr;

    CyCellStepper* mYCellStep = nullptr;
    QCheckBox* mAutoYBox = nullptr;
    QCheckBox* mRawBayerBox = nullptr;

    QSpinBox* mStartNumberBox = nullptr;
    QSpinBox* mEndNumberBox = nullptr;

    QStackedWidget* mControlTab = nullptr;

    QLabel* mAutoStretchLab = nullptr;
    CyCustomWidget::CyCheckButton* mAutoStretchBtn = nullptr;

    QLabel* mRGBStretchTypeLab = nullptr;
    QComboBox* mRGBStretchTypeBox = nullptr;

    //histogram
    std::vector<double> mHisData;
    int32_t mImageBit = 8;
    int mLastUnsupportedBit = -1;
    int32_t mXLevels = 256;
    int32_t mYHint = 1000;
    bool mMonoTab = true;

    // stretch
    StretchValue mStretchValue{ 0, 255, 255 };
    CyMedia::StretchType mStretchType = CyMedia::stretch_HSV;
    bool mAutostretch = false;
    bool mRawBayer = false;
    double mSelect_max = 255;
    double mSelect_min_width = 10.0;
};

CyMediaDisGrayStretch::CyMediaDisGrayStretch(QWidget* parent /*= nullptr*/)
    :QWidget(parent) {
    d = new CyMediaDisGrayStretch::PrivateData;
    initGUI();
}

CyMediaDisGrayStretch::~CyMediaDisGrayStretch() {

}

void CyMediaDisGrayStretch::flushTrans() {
    d->mAutoStretchLab->setText(tr("Automatic"));

    d->mRGBStretchTypeLab->setText(tr("Stretching methods"));
    int32_t idx = 0;
    d->mRGBStretchTypeBox->setItemText(idx++, tr("Gray"));
    d->mRGBStretchTypeBox->setItemText(idx++, tr("HSV"));
    d->mRGBStretchTypeBox->setItemText(idx++, tr("Lab"));

    d->mYCellStep->setCaption(tr("Y/cell"));
    d->mAutoYBox->setText(tr("Auto Y"));
    d->mRawBayerBox->setText(tr("Bayer raw data"));

    d->mHisView->setChannelName(CyHistogramView::Ch_Gray, tr("Quantity"));
    updateAxisTitle();
}

void CyMediaDisGrayStretch::setThemeColor(QColor color) {
    d->mHisView->setThemeColor(color);
    d->mHisView->setSeriesColor(CyHistogramView::Ch_Gray, color);
    d->mYCellStep->setThemeColor(color);

    color.setAlpha(0xFF);
    d->mAutoStretchBtn->setEnableCheckColor(color);
}

void CyMediaDisGrayStretch::updateAxisTitle() {
    if (d->mStretchType == CyMedia::stretch_Gray || d->mStretchType == CyMedia::stretch_None) {
        d->mHisView->setAxisTitles(tr("Grayscale"), tr("Quantity"));
    }
    else {
        d->mHisView->setAxisTitles(tr("Brightness"), tr("Quantity"));
    }
}

bool CyMediaDisGrayStretch::upImageData(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe colorOpe) {
    if (false == this->isVisible()) {
        if (false == d->mAutostretch || false == info.isMono()) {
            return false;
        }
    }
    if (!data) {
        CyMediaLog::instance().log_printf_Error(
            "[GrayStretch] null image data: %dx%d bit=%d format=%d",
            info.width, info.height, int(info.bit), int(info.format));
        return false;
    }
    if (info.bit < 1 || info.bit > kMaxHistogramBits) {
        if (d->mLastUnsupportedBit != info.bit) {
            d->mLastUnsupportedBit = info.bit;
            CyMediaLog::instance().log_printf_Warning(
                "[GrayStretch] unsupported bit depth %d (supported 1..%d), histogram update skipped",
                int(info.bit), int(kMaxHistogramBits));
        }
        return false;
    }
    d->mLastUnsupportedBit = -1;

    //切换单色/彩色页签（工作线程只改镜像状态，界面通过排队信号更新）
    const bool tMono = info.isMono(colorOpe);
    if (tMono != d->mMonoTab) {
        d->mMonoTab = tMono;
        emit transImageType(tMono ? 0 : 1);
    }

    d->mImageBit = info.bit;
    //计算直方图（31 位时 1<<31 不适合用 int 表示）
    const double maxBitValue = (info.bit >= 31) ? 2147483648.0 : double(1u << info.bit);
    int stretchMax = int(maxBitValue - 1);
    if (d->mStretchType == CyMedia::stretch_Lab) {
        stretchMax = 100;
    }

    //更新拉伸边界范围
    if (stretchMax != d->mSelect_max) {
        d->mSelect_max = stretchMax;
        emit upEditRange(true);
    }

    //更新直方图
    if (info.isMono()) {
        double maxV, minV, aveV;
        CyMediaCalc::computeGrayHistogram(data, info, 0, false,
            d->mHisData,
            &maxV, &minV, &aveV);
    }
    else if (info.isBayer()) {
        //按原始数据统计：跳过去马赛克，直接统计 CFA 像素，耗时大幅下降
        const CyMedia::DemosaicingMethod func = d->mRawBayer
            ? CyMedia::DEMOSAIC_NONE : colorOpe.bayerFunc;
        const CyMedia::StretchType type = d->mRawBayer
            ? CyMedia::stretch_Gray : d->mStretchType;
        CyMediaCalc::computeBayerHistogram(data, info,
            d->mHisData,
            type,
            func);
    }
    else if (info.isYUV()) {
        CyMediaCalc::computerYUVHistogram(data, info,
            d->mHisData,
            colorOpe.stretchType,
            colorOpe.YUVFunc);
    }
    else {
        CyMediaCalc::computeRGBHistogram(data, info,
            d->mHisData,
            d->mStretchType);
    }

    if (d->mHisData.empty()) {
        //未支持的格式或不支持的类型：清空曲线，避免显示上一帧
        d->mHisView->setSeriesData(CyHistogramView::Ch_Gray, nullptr, 0);
        return false;
    }

    d->mHisView->setSeriesData(CyHistogramView::Ch_Gray,
        d->mHisData.data(), int(d->mHisData.size()));

    //X 轴：0 ~ 最大 bit 值（Lab 为 0~100）
    d->mXLevels = (d->mStretchType == CyMedia::stretch_Lab) ? 101 : int(maxBitValue);
    if (d->mXLevels < 2) d->mXLevels = 2;

    //计算Y轴显示范围：优先显示全部数据，极端尖峰做截断
    const double calcHisMaxY = CyMediaCalc::determineYAxisMax(d->mHisData, int(d->mHisData.size() * 0.02));
    d->mYHint = qMax(1, int(calcHisMaxY));

    emit upHisRange(0, d->mXLevels - 1, d->mYHint);

    //自动拉伸（单色直方图才有意义）
    if (d->mAutostretch && d->mMonoTab) {
        int32_t start, end;
        CyMediaCalc::computeGrayStretchPara(d->mHisData, start, end);
        emit upStretchRange(start, end);
    }

    return true;
}

CyMedia::StretchType CyMediaDisGrayStretch::stretchtype() {
    return d->mStretchType;
}

CyMediaDisGrayStretch::StretchValue CyMediaDisGrayStretch::stretchValue() {
    return d->mStretchValue;
}

bool CyMediaDisGrayStretch::isAutoStretch() {
    return d->mAutoStretchBtn->isChecked() && d->mControlTab->currentIndex() == 0;
}

void CyMediaDisGrayStretch::setStretchValue(CyMedia::StretchType type, int32_t start, int32_t end, int32_t max) {
    if (max <= 0) return;
    if (type == CyMedia::stretch_None) type = CyMedia::stretch_Gray;

    const int32_t minW = int32_t(d->mSelect_min_width);
    max = qMax(max, minW + 1);
    start = qBound(0, start, max - minW);
    end = qBound(minW, end, max);
    if (end <= start) end = qMin(max, start + minW);

    d->mStretchType = type;
    d->mSelect_max = double(max);
    d->mStretchValue = { start, end, max };

    {
        QSignalBlocker b(d->mRGBStretchTypeBox);
        d->mRGBStretchTypeBox->setCurrentIndex(int(type) - 1);
    }
    updateAxisTitle();

    {
        QSignalBlocker b(d->mStartNumberBox);
        d->mStartNumberBox->setRange(0, int(max - minW));
        d->mStartNumberBox->setValue(start);
    }
    {
        QSignalBlocker b(d->mEndNumberBox);
        d->mEndNumberBox->setRange(int(minW), int(max));
        d->mEndNumberBox->setValue(end);
    }

    d->mHisView->setXMaxRange(max);
    d->mHisView->setSelection(start, end);
    syncAxisControls();
}

void CyMediaDisGrayStretch::setAutoStretch(bool on) {
    if (d->mAutostretch == on) return;
    d->mAutostretch = on;
    QSignalBlocker b(d->mAutoStretchBtn);
    d->mAutoStretchBtn->setChecked(on);
}

void CyMediaDisGrayStretch::closeEvent(QCloseEvent* event) {
    QWidget::closeEvent(event);
    if (parent()) {
        hide();
        emit stretchParaChange();
    }
    else {
        close();
    }
}

void CyMediaDisGrayStretch::showEvent(QShowEvent* event) {
    d->mHisView->setAutoY(true);
    syncAxisControls();
    emit needImage();
    QWidget::showEvent(event);
}


bool CyMediaDisGrayStretch::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Wheel && (obj->inherits("QSpinBox") || obj->inherits("QComboBox"))) return true;

    return QWidget::eventFilter(obj, event);
}

void CyMediaDisGrayStretch::onStartValueEdit(int start) {
    if (d->mStretchValue.start == start) {
        return;
    }
    if (start >= d->mStretchValue.end) {
        start = qMax(0, d->mStretchValue.end - 1);
    }
    upSelectRectChange(start, d->mStretchValue.end);
}

void CyMediaDisGrayStretch::onEndValueEdit(int end) {
    if (d->mStretchValue.end == end) {
        return;
    }
    if (end <= d->mStretchValue.start) {
        end = qMin(int(d->mSelect_max), d->mStretchValue.start + 1);
    }
    upSelectRectChange(d->mStretchValue.start, end);
}

void CyMediaDisGrayStretch::onAutoStretchChange(bool enable) {
    if (d->mAutostretch == enable)
        return;
    d->mAutostretch = enable;
    emit needImage();
}

void CyMediaDisGrayStretch::onStretchTypeChange(int idx) {
    d->mStretchType = CyMedia::StretchType(idx + 1);
    updateAxisTitle();
    if (d->mStretchType == CyMedia::stretch_Lab && d->mSelect_max != 100) {
        d->mSelect_max = 100;
        onUpEditRange();
    }
    emit stretchTypeChanged();
    emit needImage();
}

void CyMediaDisGrayStretch::onRawBayerChange(bool enable) {
    if (d->mRawBayer != enable) {
        d->mRawBayer = enable;
    }
    emit needImage();
}

void CyMediaDisGrayStretch::upSelectRectChange(int32_t start, int32_t end, bool needSignals /* = true*/) {
    const int32_t maxV = int32_t(d->mSelect_max);
    if (start < 0) start = 0;
    if (end > maxV) end = maxV;
    if (end - start < int32_t(d->mSelect_min_width)) {
        end = qMin(maxV, start + int32_t(d->mSelect_min_width));
        if (end - start < int32_t(d->mSelect_min_width)) {
            start = qMax(0, end - int32_t(d->mSelect_min_width));
        }
    }

    d->mStretchValue.start = start;
    d->mStretchValue.end = end;

    d->mHisView->setSelection(start, end);
    onUpEditorNumber(start, end);

    if (needSignals) {
        emit stretchParaChange();
    }
}

void CyMediaDisGrayStretch::onUpSelectRectRange(int32_t start, int32_t end) {
    d->mHisView->setSelection(start, end);
    emit stretchParaChange();
}

void CyMediaDisGrayStretch::onUpEditorNumber(int32_t start, int32_t end) {
    {
        QSignalBlocker blocker(d->mStartNumberBox);
        d->mStartNumberBox->setValue(start);
    }
    {
        QSignalBlocker blocker(d->mEndNumberBox);
        d->mEndNumberBox->setValue(end);
    }
}

void CyMediaDisGrayStretch::initGUI() {
    if (d->mIsInit)
        return;
    d->mIsInit = true;

    //histogram
    d->mHisView = new CyHistogramView(this);
    d->mHisView->setDisplayStyle(CyHistogramView::Style_Area);
    d->mHisView->setCellPixelSize(QSize(58, 34));
    d->mHisView->setXMaxRange(255);
    d->mHisView->setSelectionEnabled(true);
    d->mHisView->setSelectionMinWidth(d->mSelect_min_width);
    d->mHisView->setSelection(d->mStretchValue.start, d->mStretchValue.end);
    d->mHisView->setSeriesColor(CyHistogramView::Ch_Gray, d->mThemeColor);
    d->mHisView->setSeriesVisible(CyHistogramView::Ch_Gray, true);
    //X 轴固定为全量程（0 ~ 最大 bit 值），不提供缩放/平移
    d->mHisView->setXAdjustable(false);

    //格子
    d->mYCellStep = new CyCellStepper(CyCellStepper::Vertical, this);
    d->mYCellStep->setRange(1, 1000000000);
    d->mYCellStep->setValue(1);
    d->mAutoYBox = new QCheckBox(this);
    d->mAutoYBox->setChecked(true);

    d->mRawBayerBox = new QCheckBox(this);
    d->mRawBayerBox->setChecked(false);

    //Ctroll
    d->mStartNumberBox = new QSpinBox(this);
    d->mStartNumberBox->setMouseTracking(false);
    d->mStartNumberBox->setKeyboardTracking(false);
    d->mStartNumberBox->setRange(0, int(d->mSelect_max - d->mSelect_min_width));
    d->mStartNumberBox->setValue(d->mStretchValue.start);
    d->mStartNumberBox->installEventFilter(this);
    d->mEndNumberBox = new QSpinBox(this);
    d->mEndNumberBox->setMouseTracking(false);
    d->mEndNumberBox->setKeyboardTracking(false);
    d->mEndNumberBox->setRange(int(d->mSelect_min_width), int(d->mSelect_max));
    d->mEndNumberBox->setValue(d->mStretchValue.end);
    d->mEndNumberBox->installEventFilter(this);

    d->mControlTab = new QStackedWidget(this);

    d->mAutoStretchLab = new QLabel(this);
    d->mAutoStretchBtn = new CyCustomWidget::CyCheckButton(this);
    d->mAutoStretchBtn->setChecked(d->mAutostretch);

    d->mRGBStretchTypeLab = new QLabel(this);
    d->mRGBStretchTypeBox = new QComboBox(this);
    d->mRGBStretchTypeBox->installEventFilter(this);
    d->mRGBStretchTypeBox->addItem("");
    d->mRGBStretchTypeBox->addItem("");
    d->mRGBStretchTypeBox->addItem("");
    d->mRGBStretchTypeBox->setCurrentIndex(1);

    //layout
    QWidget* grayCtrW = new QWidget(this);
    QHBoxLayout* grayCtrLayout = new QHBoxLayout(grayCtrW);
    grayCtrLayout->setContentsMargins(0, 0, 0, 0);
    grayCtrLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    grayCtrLayout->addWidget(d->mAutoStretchLab);
    grayCtrLayout->addWidget(d->mAutoStretchBtn);

    QWidget* rgbCtrW = new QWidget(this);
    QHBoxLayout* rgbCtrLyout = new QHBoxLayout(rgbCtrW);
    rgbCtrLyout->setContentsMargins(0, 0, 0, 0);
    rgbCtrLyout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    rgbCtrLyout->addWidget(d->mRGBStretchTypeLab);
    rgbCtrLyout->addWidget(d->mRGBStretchTypeBox);
    d->mControlTab->addWidget(grayCtrW);
    d->mControlTab->addWidget(rgbCtrW);
    grayCtrW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    rgbCtrW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    //拉伸区间编辑行
    QWidget* rangeW = new QWidget(this);
    QHBoxLayout* rangeLayout = new QHBoxLayout(rangeW);
    rangeLayout->setContentsMargins(0, 0, 0, 0);
    rangeLayout->setSpacing(4);
    rangeLayout->addWidget(d->mStartNumberBox);
    rangeLayout->addItem(new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
    rangeLayout->addWidget(d->mEndNumberBox);
    rangeW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    //坐标轴/风格控制行
    QWidget* axisW = new QWidget(this);
    QHBoxLayout* axisLayout = new QHBoxLayout(axisW);
    axisLayout->setContentsMargins(0, 0, 0, 0);
    //X 轴固定为全量程，只保留 Y 轴格子调节
    axisLayout->addWidget(d->mYCellStep);
    axisLayout->addWidget(d->mAutoYBox);
    axisLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
    axisLayout->addWidget(d->mRawBayerBox);
    axisW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout* mainLayou = new QVBoxLayout(this);
    mainLayou->setContentsMargins(0, 0, 0, 0);
    mainLayou->setSpacing(2);
    mainLayou->addWidget(d->mHisView, 1);
    mainLayou->addWidget(rangeW);
    mainLayou->addWidget(axisW);
    mainLayou->addWidget(d->mControlTab);
    d->mHisView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->mControlTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    flushTrans();
    setThemeColor(d->mThemeColor);
    updateAxisRanges();

    //Connection
    connect(d->mStartNumberBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &CyMediaDisGrayStretch::onStartValueEdit);
    connect(d->mEndNumberBox, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &CyMediaDisGrayStretch::onEndValueEdit);
    connect(d->mAutoStretchBtn, &CyCustomWidget::CyCheckButton::stateChanged, this, &CyMediaDisGrayStretch::onAutoStretchChange);
    connect(d->mRGBStretchTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CyMediaDisGrayStretch::onStretchTypeChange);
    connect(d->mRawBayerBox, &QCheckBox::toggled, this, &CyMediaDisGrayStretch::onRawBayerChange);

    connect(d->mYCellStep, &CyCellStepper::valueChanged, this,
        [this](int value) {
            d->mHisView->setYPerCell(value, true);
            syncAxisControls();
        });
    connect(d->mAutoYBox, &QCheckBox::toggled, this,
        [this](bool checked) {
            d->mHisView->setAutoY(checked);
            syncAxisControls();
        });
    connect(d->mHisView, &CyHistogramView::autoYChanged, this,
        [this](bool on) {
            QSignalBlocker blocker(d->mAutoYBox);
            d->mAutoYBox->setChecked(on);
        });
    connect(d->mHisView, &CyHistogramView::autoXChanged, this,
        [this](bool) {
            syncAxisControls();
        });
    connect(d->mHisView, &CyHistogramView::selectionChanged, this,
        [this](double start, double end) {
            upSelectRectChange(int32_t(std::lround(start)), int32_t(std::lround(end)));
        });

    connect(this, &CyMediaDisGrayStretch::transImageType, this, &CyMediaDisGrayStretch::onTransImageType);
    connect(this, &CyMediaDisGrayStretch::upStretchRange, this, &CyMediaDisGrayStretch::onUpStretchRange);
    connect(this, &CyMediaDisGrayStretch::upHisRange, this, &CyMediaDisGrayStretch::onUpHisRange);
    connect(this, &CyMediaDisGrayStretch::upEditRange, this, &CyMediaDisGrayStretch::onUpEditRange);
}

void CyMediaDisGrayStretch::onTransImageType(int index) {
    d->mControlTab->setCurrentIndex(index);
    d->mMonoTab = (index == 0);
}

void CyMediaDisGrayStretch::onUpStretchRange(int start, int end) {
    if (start == d->mStretchValue.start && end == d->mStretchValue.end) return;
    upSelectRectChange(start, end, false);
}

void CyMediaDisGrayStretch::onUpHisRange(int minX, int maxX, int maxY) {
    if (d->mHisView->xMaxRange() != maxX || d->mHisView->xMinRange() != minX) {
        d->mHisView->setXMinRange(minX);
        d->mHisView->setXMaxRange(maxX);
        updateAxisRanges();
        //位深变化后重新应用一次选区，避免选区被旧的坐标范围截断
        d->mHisView->setSelection(d->mStretchValue.start, d->mStretchValue.end);
    }

    if (maxY > 0) {
        d->mHisView->setYMaxRange(qMax(1.0, double(maxY) * 4.0));
        d->mHisView->setYHintMax(maxY);
    }
    syncAxisControls();
}

void CyMediaDisGrayStretch::onUpEditRange(bool bitChange /* = false*/) {
    d->mStretchValue.max = int32_t(d->mSelect_max);

    int start = d->mStretchValue.start;
    int end = d->mStretchValue.end;

    if (bitChange) {
        // 位深变化只 clamp；统一由 Display::updateStretchForImageInfo 决定策略
        start = qBound(0, start, int(d->mSelect_max - d->mSelect_min_width));
        end = qBound(int(d->mSelect_min_width), end, int(d->mSelect_max));
    }

    {
        QSignalBlocker b(d->mStartNumberBox);
        d->mStartNumberBox->setRange(0, int(d->mSelect_max - d->mSelect_min_width));
    }
    {
        QSignalBlocker b(d->mEndNumberBox);
        d->mEndNumberBox->setRange(int(d->mSelect_min_width), int(d->mSelect_max));
    }

    d->mStretchValue.start = int32_t(start);
    d->mStretchValue.end = int32_t(end);
    onUpEditorNumber(start, end);
    d->mHisView->setSelection(start, end);
}

void CyMediaDisGrayStretch::updateAxisRanges() {
    syncAxisControls();
}

void CyMediaDisGrayStretch::syncAxisControls() {
    if (!d->mHisView) return;

    const double yBound = std::ceil(d->mHisView->yMaxRange());
    const int maxY = (yBound >= 2000000000.0) ? 2000000000 : qMax(1, int(yBound));
    d->mYCellStep->setRange(1, maxY);
    d->mYCellStep->setValue(d->mHisView->yPerCell());

    {
        QSignalBlocker blocker(d->mAutoYBox);
        d->mAutoYBox->setChecked(d->mHisView->autoY());
    }
}
