#include "GrayMeasurement.h"

#include "../../Calculate/Calculate.h"
#include "../../common/CyMediaLog.h"
#include "../Widgets/CyCellStepper.h"
#include "../Widgets/CyHistogramView.h"

#include "../Display.h"
#include "../Annotation/DrawItem.h"

#include <QThread>
#include <QToolBar>
#include <QPushButton>
#include <QActionGroup>
#include <QSignalBlocker>
#include <QLabel>
#include <QGridLayout>
#include <QTimer>

#include <cmath>

namespace {
    // 1<<31 用 int 表示是未定义行为；这里统一用 double 计算位深范围。
    inline double bitRangeValue(int bit) {
        return (bit >= 31) ? 2147483648.0 : double(1u << bit);
    }

    inline double bitMaxValue(int bit) {
        return bitRangeValue(bit) - 1.0;
    }

    // 点模式的统计量：对已采样的历史序列求值。
    struct PosStatistics {
        double ave = 0.0;
        double max = 0.0;
        double min = 0.0;
        double std = 0.0;
        double uniformity = 0.0;
    };

    inline PosStatistics calcPosStatistics(const std::vector<double>& samples, double bitMax) {
        PosStatistics st;
        if (samples.empty()) return st;

        const double count = static_cast<double>(samples.size());
        st.max = samples.front();
        st.min = samples.front();

        double sum = 0.0;
        for (double v : samples) {
            sum += v;
            if (v > st.max) st.max = v;
            if (v < st.min) st.min = v;
        }
        st.ave = sum / count;

        double variance = 0.0;
        for (double v : samples) {
            variance += (v - st.ave) * (v - st.ave);
        }
        st.std = std::sqrt(variance / count);
        // 与 CyMediaCalc::computerUniformity 保持同一公式，避免两种模式口径不一致
        st.uniformity = (st.std > 0.0 && bitMax > 0.0)
            ? 20.0 * std::log10(bitMax / st.std)
            : 0.0;
        return st;
    }
}

struct CyMediaDisGrayTest::PrivateData {
public:
    //UI
    CyMedia::CyMediaDis* m_parentDis = nullptr;
    QColor m_ThemeColor = QColor(0x2a, 0xa3, 0xc6);
    bool mIsInit = false;

    CyHistogramView* mHisView = nullptr;
    CyCellStepper* mXCellStep = nullptr;
    CyCellStepper* mYCellStep = nullptr;
    QCheckBox* mAutoYBox = nullptr;

    QWidget* nTestInfoEditWidget[4] = { nullptr };
    QLabel* mCahnnelLabel = nullptr;
    QCheckBox* mChannelCtlLab[4] = { nullptr };
    QLabel* mAveageLabel[5] = { nullptr };
    QLabel* mMaximumLabel[5] = { nullptr };
    QLabel* mMinimumLabel[5] = { nullptr };
    QLabel* mStdLabel[5] = { nullptr };
    QLabel* mUniformityLabel[5] = { nullptr };

    QActionGroup* mDrawBtnGroup = nullptr;
    QAction* mDrawAct[6];

    QPushButton* mResetBtn = nullptr;

    //histogram
    std::vector<double> mHistogramData[4];
    PosHis mPosHisData[4];
    double mPosHisMaxY = 0.0;

    int mXLevels = 256;   ///< X 轴上可显示的总灰度级数（点模式下为采样点数）
    int mYHint = 1;       ///< 自动 Y 的参考峰值

    bool mIsGray = false;
    bool mIsPos = false;

    //test
    QUuid mCurrentItemID;
    CyDisDrawItem::ItemType mDrawType = CyDisDrawItem::ItemType::Invalid;
    QPainterPath mOldItemPath;

    std::vector<uint8_t> mClacMask;
    bool mMaskHaveData = false;
    bool mMaskIsfullzero = false;

    oneChannelTestInfo mHisTestData;
    threeChannelTestInfo mRGBTestData;
    bool currentIsPosCalc = false;
    int mLastUnsupportedBit = -1;
    int mLastUnsupportedFormat = -1;
};

CyMediaDisGrayTest::CyMediaDisGrayTest(QWidget* parent /*= nullptr*/)
    :QWidget(parent) {
    d = new CyMediaDisGrayTest::PrivateData;
    initGUI();
    qRegisterMetaType<CyMediaDisGrayTest::testModeChangeType>("CyMediaDisGrayTest::testModeChangeType");

    connect(this, &CyMediaDisGrayTest::uphisVisible, this, &CyMediaDisGrayTest::onUphisVisible);
    connect(this, &CyMediaDisGrayTest::upTestData, this, &CyMediaDisGrayTest::onUpTestData);
    connect(this, &CyMediaDisGrayTest::upHisRange, this, &CyMediaDisGrayTest::onUpHisRange);
}

CyMediaDisGrayTest::~CyMediaDisGrayTest() {

}

void CyMediaDisGrayTest::setParentDis(CyMedia::CyMediaDis* parentDis) {
    d->m_parentDis = parentDis;
}

void CyMediaDisGrayTest::Itemdraw(CyDisDrawItem::BaseItem* item) {
    if (false == isVisible() ||
        false == isEnabled() ||
        d->mDrawType == CyDisDrawItem::ItemType::Invalid) {
        return;
    }
    if (!item) {
        CyMediaLog::instance().log_printf_Warning(
            "[GrayTest] Itemdraw called with null item");
        return;
    }
    d->mCurrentItemID = item->id();
    if (d->mDrawType != item->itemType()) {
        // 切换测试形状后重新允许 Y 轴自动调整
        d->mHisView->setAutoY(true);
        syncAxisControls();
    }
    emit needImage();

    connect(item, &CyDisDrawItem::BaseItem::geometryChanged, this, [this]() {
            emit needImage();
        });
}

void CyMediaDisGrayTest::ItemRemoved(QUuid id) {
    if (id != d->mCurrentItemID) return;
    if (QThread::currentThread() == this->thread()) {
        d->mDrawAct[CyDisDrawItem::Invalid]->setChecked(true);
    }
    else {
        QTimer::singleShot(0, this, [this]() {
            d->mDrawAct[CyDisDrawItem::Invalid]->setChecked(true);
            });
    }
    emit needImage();
}

CyDisDrawItem::BaseItem* CyMediaDisGrayTest::getCurrentItem() {
    if (d->mCurrentItemID.isNull()) {
        return nullptr;
    }
    if (!d->m_parentDis) {
        CyMediaLog::instance().log_printf_Error(
            "[GrayTest] parent CyMediaDis is null, cannot resolve current item");
        return nullptr;
    }
    return d->m_parentDis->getItem(d->mCurrentItemID);
}

QPainterPath CyMediaDisGrayTest::oldItemPpath() {
    return d->mOldItemPath;
}

bool CyMediaDisGrayTest::upImage(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe) {
    if (false == this->isVisible()) return false;

    if (!d->m_parentDis) {
        CyMediaLog::instance().log_printf_Error(
            "[GrayTest] parent CyMediaDis is null, histogram update skipped");
        return false;
    }
    if (!data) {
        CyMediaLog::instance().log_printf_Error(
            "[GrayTest] null image data: %dx%d bit=%d format=%d",
            info.width, info.height, int(info.bit), int(info.format));
        return false;
    }
    if (info.bit < 1 || info.bit > mMaxHistogramBits) {
        if (d->mLastUnsupportedBit != info.bit) {
            d->mLastUnsupportedBit = info.bit;
            CyMediaLog::instance().log_printf_Warning(
                "[GrayTest] unsupported bit depth %d (supported 1..%d), histogram update skipped",
                int(info.bit), int(mMaxHistogramBits));
        }
        return false;
    }
    d->mLastUnsupportedBit = -1;

    if (false == d->m_parentDis->isSingleItemMode()) return false;
    if (d->m_parentDis->isDrawing() && !getCurrentItem()) return false;

    //切换
    bool tIsMono = info.isMono() ||
        (info.isBayer() && formatOpe.bayerFunc == DEMOSAIC_NONE) ||
        (info.isYUV() && formatOpe.YUVFunc == YUVTRANS_Y);
    if (d->mIsGray != tIsMono ||
        d->mIsPos != currentItemIsPos()) {
        d->mIsGray = tIsMono;
        d->mIsPos = currentItemIsPos();
        emit uphisVisible();
    }

    //清空点统计（点模式的曲线按有效样本重建，这里只需要清空历史序列）
    if (d->mIsPos != d->currentIsPosCalc) {
        d->mPosHisMaxY = 0.0;
        d->currentIsPosCalc = true;
        for (int i = hisI_Gray; i <= hisI_B; i++) {
            d->mPosHisData[i].init();
            d->mHistogramData[i].clear();
        }
    }
    if (false == d->mIsPos) {
        d->currentIsPosCalc = false;
    }

    //更新计算掩码
    //点模式不需要掩码：点只采样自身坐标处的像素，不能套用区域掩码的"未绘制"判断，
    //否则 upMask 会因为 Point 类型把 mMaskHaveData 置 false，直接 return，点模式就永远没有数据。
    if (d->mIsPos) {
        d->mMaskHaveData = false;
        d->mMaskIsfullzero = false;
    }
    else if (d->mDrawType == CyDisDrawItem::ItemType::Invalid) {
        d->mMaskHaveData = false;
        d->mMaskIsfullzero = false;
    }
    else {
        upMask({ info.width, info.height });
        //选择了图形项目但未绘制
        if (false == d->mMaskHaveData) return false;
    }
    //计算直方图
    calcImageHisFlag cancHisFlag;
    if (info.isMono() || tIsMono) {
        cancHisFlag = upImage_Mono(info, data, formatOpe);
    }
    else if (info.isRGB()) {
        cancHisFlag = upImage_RGB(info, data, formatOpe);
    }
    else if (info.isBayer()) {
        cancHisFlag = upImage_Bayer(info, data, formatOpe);
    }
    else if (info.isYUV()) {
        cancHisFlag = upImage_YUV(info, data, formatOpe);
    }
    else {
        //未支持的格式（例如 RGBA）：清空曲线，避免继续显示上一帧的旧数据
        if (d->mLastUnsupportedFormat != int(info.format)) {
            d->mLastUnsupportedFormat = int(info.format);
            CyMediaLog::instance().log_printf_Warning(
                "[GrayTest] unsupported pixel format %d, histogram cleared", int(info.format));
        }
        for (int i = hisI_Gray; i <= hisI_B; i++) {
            d->mHistogramData[i].clear();
            publishSeries(i);
        }
        return false;
    }

    //X 轴范围：点模式为采样点数，其余为完整位深范围（便于查看每一个灰度级）
    d->mXLevels = d->mIsPos ? mPosHisMax : int(bitRangeValue(info.bit));
    if (d->mXLevels < 2) d->mXLevels = 2;
    d->mYHint = qMax(1, int(cancHisFlag.calcHisMaxY));

    emit upHisRange(0, d->mXLevels - 1, d->mYHint);
    emit upTestData();
    return true;
}

bool CyMediaDisGrayTest::currentTestDataIsGray() {
    return d->mIsGray;
}

CyMediaDisGrayTest::oneChannelTestInfo& CyMediaDisGrayTest::getGrayTestData() {
    return d->mHisTestData;
}

CyMediaDisGrayTest::threeChannelTestInfo& CyMediaDisGrayTest::getRGBTestData() {
    return d->mRGBTestData;
}

void CyMediaDisGrayTest::closeEvent(QCloseEvent* event) {
    QWidget::closeEvent(event);
    if (parent()) {
        hide();
        auto citem = getCurrentItem();
        if (citem) {
            d->mOldItemPath = citem->pathInScene();
        }
        else {
            d->mOldItemPath.clear();
        }
        emit testModeChange(int(CyDisDrawItem::ItemType::Invalid), CyMediaDisGrayTest::ModeChange_hide);
    }
    else {
        close();
    }
}

void CyMediaDisGrayTest::showEvent(QShowEvent* event) {
    if (!d->m_parentDis || true == d->m_parentDis->isSingleItemMode())
        emit testModeChange(d->mDrawType, CyMediaDisGrayTest::QPainterPath_show);
    //重新显示时恢复 Y 轴自动调整
    d->mHisView->setAutoY(true);
    syncAxisControls();
    emit needImage();
    QWidget::showEvent(event);
}

void CyMediaDisGrayTest::initGUI() {
    if (d->mIsInit)
        return;

    d->mIsInit = true;

    const char testLabHeadStyle[] =
        "QLabel{\n"
        "    background-color:#AAAAAA;\n"
        "    font-family:Microsoft YaHei UI;\n"
        "    font-size:13px;\n"
        "    font-weight:Bold;\n"
        "}\n";
    const char testLabInfoStyle[] =
        "QLabel{\n"
        "   background-color:#D2D2D2;\n"
        "}\n"
        "QCheckBox{\n"
        "   background-color:#D2D2D2;\n"
        "}\n"
        "QCheckBox::indicator {\n"
        "   qproperty-alignment:AlignHCenter AlignVCenter;\n"
        "}\n";

    //histogram view
    d->mHisView = new CyHistogramView(this);
    d->mHisView->setDisplayStyle(CyHistogramView::Style_Area);
    d->mHisView->setCellPixelSize(QSize(58, 34));
    d->mHisView->setXMaxRange(255);
    d->mHisView->setSeriesColor(CyHistogramView::Ch_Gray, QColor(Qt::darkGray));
    d->mHisView->setSeriesColor(CyHistogramView::Ch_Red, QColor(200, 30, 30, 255));
    d->mHisView->setSeriesColor(CyHistogramView::Ch_Green, QColor(30, 160, 30, 255));
    d->mHisView->setSeriesColor(CyHistogramView::Ch_Blue, QColor(30, 60, 200, 255));
    d->mHisView->setSeriesVisible(CyHistogramView::Ch_Gray, true);
    d->mHisView->setSeriesVisible(CyHistogramView::Ch_Red, false);
    d->mHisView->setSeriesVisible(CyHistogramView::Ch_Green, false);
    d->mHisView->setSeriesVisible(CyHistogramView::Ch_Blue, false);

    d->mXCellStep = new CyCellStepper(CyCellStepper::Horizontal, this);
    d->mXCellStep->setRange(1, 4096);
    d->mXCellStep->setValue(16);

    d->mYCellStep = new CyCellStepper(CyCellStepper::Vertical, this);
    d->mYCellStep->setRange(1, 1000000000);
    d->mYCellStep->setValue(1);

    d->mAutoYBox = new QCheckBox(this);
    d->mAutoYBox->setChecked(true);

    d->mCahnnelLabel = new QLabel(this);
    d->mCahnnelLabel->setAlignment(Qt::AlignHCenter);
    for (int i = 0; i < 5; i++) {
        if (i <= 3) {
            d->mChannelCtlLab[i] = new QCheckBox(this);
            d->mChannelCtlLab[i]->setChecked(true);
            connect(d->mChannelCtlLab[i], &QCheckBox::stateChanged, this,
                [this](int state) {
                    onUphisVisible();
                });
        }
        d->mAveageLabel[i] = new QLabel(this);
        d->mAveageLabel[i]->setAlignment(Qt::AlignHCenter);

        d->mMaximumLabel[i] = new QLabel(this);
        d->mMaximumLabel[i]->setAlignment(Qt::AlignHCenter);

        d->mMinimumLabel[i] = new QLabel(this);
        d->mMinimumLabel[i]->setAlignment(Qt::AlignHCenter);

        d->mStdLabel[i] = new QLabel(this);
        d->mStdLabel[i]->setAlignment(Qt::AlignHCenter);

        d->mUniformityLabel[i] = new QLabel(this);
        d->mUniformityLabel[i]->setAlignment(Qt::AlignHCenter);
    }
    d->mChannelCtlLab[0]->setChecked(true);

    QToolBar* itemtypeToolbar = new QToolBar(this);
    itemtypeToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly); // 仅显示图标
    itemtypeToolbar->setIconSize(QSize(24, 24));

    d->mDrawBtnGroup = new QActionGroup(this);
    for (int i = CyDisDrawItem::ItemType::Invalid; i <= CyDisDrawItem::ItemType::Polygon; i++) {
        d->mDrawAct[i] = new QAction(QIcon(), "", itemtypeToolbar);
        d->mDrawAct[i]->setData(QVariant(i));
        d->mDrawAct[i]->setCheckable(true);
        itemtypeToolbar->addAction(d->mDrawAct[i]);

        d->mDrawBtnGroup->addAction(d->mDrawAct[i]);
    }
    d->mDrawAct[CyDisDrawItem::ItemType::Invalid]->setChecked(true);
    d->mResetBtn = new QPushButton(QIcon(":/CyMediaDis/ICONS/ResetShaft.png"), "", this);
    connect(d->mResetBtn, &QPushButton::clicked, this, &CyMediaDisGrayTest::onResetShaft);

    QWidget* testInfoHeadWidget = new QWidget(this);
    QHBoxLayout* tastInfoTitleLayout = new QHBoxLayout(testInfoHeadWidget);
    tastInfoTitleLayout->addWidget(d->mCahnnelLabel);
    tastInfoTitleLayout->addWidget(d->mAveageLabel[4]);
    tastInfoTitleLayout->addWidget(d->mMaximumLabel[4]);
    tastInfoTitleLayout->addWidget(d->mMinimumLabel[4]);
    tastInfoTitleLayout->addWidget(d->mStdLabel[4]);
    tastInfoTitleLayout->addWidget(d->mUniformityLabel[4]);
    tastInfoTitleLayout->setContentsMargins(0, 0, 0, 0);
    tastInfoTitleLayout->setSpacing(2);
    testInfoHeadWidget->setStyleSheet(testLabHeadStyle);

    for (int i = hisI_Gray; i <= hisI_B; i++) {
        d->nTestInfoEditWidget[i] = new QWidget(this);
        QGridLayout* testInfoEditLayout = new QGridLayout(d->nTestInfoEditWidget[i]);
        testInfoEditLayout->setContentsMargins(0, 0, 0, 0);
        testInfoEditLayout->setSpacing(2);
        int columI = 0;
        testInfoEditLayout->addWidget(d->mChannelCtlLab[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(d->mAveageLabel[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(d->mMaximumLabel[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(d->mMinimumLabel[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(d->mStdLabel[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(d->mUniformityLabel[i], 0, columI++, 1, 1);
        d->nTestInfoEditWidget[i]->setStyleSheet(testLabInfoStyle);
    }

    QWidget* testInfoW = new QWidget(this);
    QVBoxLayout* testInfoLyout = new QVBoxLayout(testInfoW);
    testInfoLyout->addWidget(testInfoHeadWidget);
    for (int i = hisI_Gray; i <= hisI_B; i++) {
        testInfoLyout->addWidget(d->nTestInfoEditWidget[i]);
    }
    testInfoLyout->setContentsMargins(0, 0, 0, 0);
    testInfoLyout->setSpacing(2);
    testInfoW->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    //坐标轴格子控制行
    QWidget* axisCtlW = new QWidget(this);
    QHBoxLayout* axisLayout = new QHBoxLayout(axisCtlW);
    axisLayout->setContentsMargins(0, 0, 0, 0);
    axisLayout->addSpacing(8);
    axisLayout->addWidget(d->mXCellStep);
    axisLayout->addSpacing(8);
    axisLayout->addWidget(d->mYCellStep);
    axisLayout->addWidget(d->mAutoYBox);
    axisLayout->addItem(new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
    axisCtlW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QWidget* tCtlW = new QWidget(this);
    QHBoxLayout* tCtrlLayout = new QHBoxLayout(tCtlW);
    tCtrlLayout->addWidget(itemtypeToolbar);
    tCtrlLayout->addItem(new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
    //重置轴按钮放在工具行右侧，避免坐标轴行在窄窗口/高 DPI 下溢出
    tCtrlLayout->addWidget(d->mResetBtn);
    tCtrlLayout->setContentsMargins(0, 0, 0, 0);
    tCtlW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout* mainLyout = new QVBoxLayout(this);
    mainLyout->addWidget(d->mHisView, 1);
    mainLyout->addWidget(axisCtlW);
    mainLyout->addWidget(testInfoW);
    mainLyout->addWidget(tCtlW);
    mainLyout->setContentsMargins(0, 0, 0, 0);
    mainLyout->setSpacing(2);

    flushTrans();
    setThemeColor(d->m_ThemeColor);

    onUphisVisible();
    updateAxisRanges();

    //connect
    connect(d->mChannelCtlLab[hisI_Gray], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onGrayCheckClicked);
    connect(d->mChannelCtlLab[hisI_R], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onRedCheckClicked);
    connect(d->mChannelCtlLab[hisI_G], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onGreenCheckClicked);
    connect(d->mChannelCtlLab[hisI_B], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onBlueCheckClicked);

    connect(d->mDrawBtnGroup, &QActionGroup::triggered, this, &CyMediaDisGrayTest::onDrawActTriggered);

    //直方图风格/坐标轴格子
    connect(d->mXCellStep, &CyCellStepper::valueChanged, this,
        [this](int value) {
            d->mHisView->setXPerCell(value, true);
            syncAxisControls();
        });
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
}

void CyMediaDisGrayTest::onGrayCheckClicked(bool flag) {
    Q_UNUSED(flag);
    d->mChannelCtlLab[hisI_Gray]->setChecked(true);
    return;
}

void CyMediaDisGrayTest::onRedCheckClicked(bool flag) {
    Q_UNUSED(flag);
    onUphisVisible();
}

void CyMediaDisGrayTest::onGreenCheckClicked(bool flag) {
    Q_UNUSED(flag);
    onUphisVisible();
}

void CyMediaDisGrayTest::onBlueCheckClicked(bool flag) {
    Q_UNUSED(flag);
    onUphisVisible();
}

void CyMediaDisGrayTest::onResetShaft() {
    d->mHisView->resetView();
    syncAxisControls();
}

void CyMediaDisGrayTest::onDrawActTriggered(QAction* act) {
    d->mDrawType = CyDisDrawItem::ItemType(act->data().toInt());
    if (!d->m_parentDis || true == d->m_parentDis->isSingleItemMode())
        emit testModeChange(int(d->mDrawType), CyMediaDisGrayTest::ModeChange_normal);
}

void CyMediaDisGrayTest::publishSeries(int channel) {
    if (!d->mHisView) return;
    if (channel < hisI_Gray || channel > hisI_B) return;

    const std::vector<double>& data = d->mHistogramData[channel];
    //size 为 0 时表示清空，控件内部同样走排队通道
    d->mHisView->setSeriesData(channel, data.data(), int(data.size()));
}

CyMediaDisGrayTest::calcImageHisFlag CyMediaDisGrayTest::upImage_Mono(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe) {
    calcImageHisFlag calcRe;

    if (d->mIsPos) {
        auto item = getCurrentItem();
        if (!item) {
            return calcRe;
        }
        //计算像素点
        double temR = 0.0, tempG = 0.0, tempB = 0.0;
        auto pos = item->pos();
        CyMediaCalc::calcCoordinateColor(info, data, static_cast<int>(pos.x()), static_cast<int>(pos.y()),
            &temR, &tempG, &tempB, formatOpe);
        //入队
        d->mPosHisData[hisI_Gray].addData(temR);
        d->mPosHisData[hisI_Gray].getLinearData(d->mHistogramData[hisI_Gray]);
        if (d->mPosHisMaxY < temR) {
            d->mPosHisMaxY = temR;
        }
        d->mHisTestData.currentGray = static_cast<int32_t>(std::lround(temR));
        //点模式统计：对采样序列求值，否则界面上的统计值会一直停在 0
        const PosStatistics st = calcPosStatistics(d->mHistogramData[hisI_Gray], bitMaxValue(info.bit));
        d->mHisTestData.ave = st.ave;
        d->mHisTestData.max = st.max;
        d->mHisTestData.min = st.min;
        d->mHisTestData.std = st.std;
        d->mHisTestData.Uniformity = st.uniformity;
        //更新折线
        publishSeries(hisI_Gray);
        calcRe.calcHisMaxY = std::max(1.0, d->mPosHisMaxY);
        calcRe.calcXRangePara = mPosHisMax;
    }
    else {
        if (d->mMaskIsfullzero) {
            d->mHistogramData[hisI_Gray].assign(d->mHistogramData[hisI_Gray].size(), 0.0);
        }
        else {
            if (info.isBayer()) {
                CyMediaCalc::computeGrayHistogram(data, info, &d->mClacMask, d->mMaskHaveData,
                    d->mHistogramData[hisI_Gray],
                    &d->mHisTestData.max, &d->mHisTestData.min, &d->mHisTestData.ave);
            }
            else if (info.isYUV()) {
                CyMediaCalc::computerYUVHistogram(data, info, &d->mClacMask, d->mMaskHaveData,
                    d->mHistogramData[hisI_Gray], d->mHistogramData[hisI_Gray], d->mHistogramData[hisI_Gray],
                    d->mRGBTestData.max, d->mRGBTestData.min, d->mRGBTestData.ave, formatOpe.YUVFunc);
                d->mHisTestData.max = d->mRGBTestData.max[0];
                d->mHisTestData.min = d->mRGBTestData.min[0];
                d->mHisTestData.ave = d->mRGBTestData.ave[0];
            }
            else {
                CyMediaCalc::computeGrayHistogram(data, info, &d->mClacMask, d->mMaskHaveData,
                    d->mHistogramData[hisI_Gray],
                    &d->mHisTestData.max, &d->mHisTestData.min, &d->mHisTestData.ave);
            }
        }
        //更新数据
        publishSeries(hisI_Gray);
    }
    //计算参数（点模式的统计量在采样时已经算好，这里只处理直方图模式）
    if (false == d->mIsPos) {
        int maxXrange = 0;
        CyMediaCalc::computerUniformity(d->mHistogramData[hisI_Gray], d->mHisTestData.ave, bitMaxValue(info.bit),
            &d->mHisTestData.std, &d->mHisTestData.Uniformity, &maxXrange);
        calcRe.calcXRangePara = maxXrange;
    }
    //计算Y轴显示范围
    if (false == d->mMaskIsfullzero && false == d->mIsPos) {
        calcRe.calcHisMaxY = CyMediaCalc::determineYAxisMax(d->mHistogramData[hisI_Gray]);
    }

    return calcRe;
}

CyMediaDisGrayTest::calcImageHisFlag CyMediaDisGrayTest::upImage_RGB(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe) {
    calcImageHisFlag calcRe;
    if (d->mIsPos) {
        auto item = getCurrentItem();
        if (!item) {
            return calcRe;
        }
        //计算像素点
        double tempRGB[4] = { 0.0, 0.0, 0.0, 0.0 };
        auto pos = item->pos();
        CyMediaCalc::calcCoordinateColor(info, data, static_cast<int>(pos.x()), static_cast<int>(pos.y()),
            &tempRGB[hisI_R], &tempRGB[hisI_G], &tempRGB[hisI_B], formatOpe);
        if (d->mRGBTestData.currentGray.size() != 3) {
            d->mRGBTestData.currentGray.resize(3);
        }
        //入队
        for (int i = hisI_R; i <= hisI_B; i++) {
            d->mPosHisData[i].addData(tempRGB[i]);
            d->mPosHisData[i].getLinearData(d->mHistogramData[i]);
            if (d->mPosHisMaxY < tempRGB[i]) {
                d->mPosHisMaxY = tempRGB[i];
            }
            //点模式统计：对采样序列求值，否则界面上的统计值会一直停在 0
            const PosStatistics st = calcPosStatistics(d->mHistogramData[i], bitMaxValue(info.bit));
            d->mRGBTestData.ave[i - 1] = st.ave;
            d->mRGBTestData.max[i - 1] = st.max;
            d->mRGBTestData.min[i - 1] = st.min;
            d->mRGBTestData.std[i - 1] = st.std;
            d->mRGBTestData.Uniformity[i - 1] = st.uniformity;
            //更新折线
            publishSeries(i);
        }
        d->mRGBTestData.currentGray[0] = static_cast<int32_t>(std::lround(tempRGB[hisI_R]));
        d->mRGBTestData.currentGray[1] = static_cast<int32_t>(std::lround(tempRGB[hisI_G]));
        d->mRGBTestData.currentGray[2] = static_cast<int32_t>(std::lround(tempRGB[hisI_B]));
        calcRe.calcHisMaxY = std::max(1.0, d->mPosHisMaxY);
        calcRe.calcXRangePara = mPosHisMax;
    }
    else {
        if (d->mMaskIsfullzero) {
            d->mHistogramData[hisI_R].assign(d->mHistogramData[hisI_R].size(), 0.0);
            d->mHistogramData[hisI_G].assign(d->mHistogramData[hisI_G].size(), 0.0);
            d->mHistogramData[hisI_B].assign(d->mHistogramData[hisI_B].size(), 0.0);

            d->mRGBTestData.ave.assign(d->mRGBTestData.ave.size(), 0.0);
            d->mRGBTestData.max.assign(d->mRGBTestData.max.size(), 0.0);
            d->mRGBTestData.min.assign(d->mRGBTestData.min.size(), 0.0);
            d->mRGBTestData.std.assign(d->mRGBTestData.std.size(), 0.0);
            d->mRGBTestData.Uniformity.assign(d->mRGBTestData.Uniformity.size(), 0.0);
        }
        else {
            CyMediaCalc::computeRGBHistogram(data, info, &d->mClacMask, d->mMaskHaveData,
                d->mHistogramData[hisI_R], d->mHistogramData[hisI_G], d->mHistogramData[hisI_B],
                d->mRGBTestData.max, d->mRGBTestData.min, d->mRGBTestData.ave);
        }
        //更新数据
        for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
            publishSeries(hisI);
            calcRe.calcXRangePara = bitRangeValue(info.bit);
        }
        if (false == d->mMaskIsfullzero) {
            //计算参数
            std::vector<int> maxXrange(3);
            CyMediaCalc::computerThreeUniformity(
                d->mHistogramData[hisI_R], d->mHistogramData[hisI_G], d->mHistogramData[hisI_B],
                d->mRGBTestData.ave, bitMaxValue(info.bit),
                d->mRGBTestData.std, d->mRGBTestData.Uniformity, maxXrange);
            //计算Y轴显示范围
            for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
                //注意：upImage* 在管线工作线程执行，不能访问控件的 GUI 状态，
                //这里对所有通道取最大值，显示范围略保守但线程安全。
                calcRe.calcHisMaxY = std::max(calcRe.calcHisMaxY,
                    CyMediaCalc::determineYAxisMax(d->mHistogramData[hisI], int(d->mHistogramData[hisI].size() * 0.02)));
            }
            calcRe.calcXRangePara = (maxXrange[0] > maxXrange[1]) ? maxXrange[0] : qMax(maxXrange[1], maxXrange[2]);
        }
    }

    return calcRe;
}

CyMediaDisGrayTest::calcImageHisFlag CyMediaDisGrayTest::upImage_Bayer(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe) {
    calcImageHisFlag calcRe;

    if (d->mIsPos) {
        auto item = getCurrentItem();
        if (!item) {
            return calcRe;
        }
        double tempRGB[4] = { 0.0, 0.0, 0.0, 0.0 };
        auto pos = item->pos();
        CyMediaCalc::calcCoordinateColor(info, data, static_cast<int>(pos.x()), static_cast<int>(pos.y()),
            &tempRGB[hisI_R], &tempRGB[hisI_G], &tempRGB[hisI_B], formatOpe);
        if (d->mRGBTestData.currentGray.size() != 3) {
            d->mRGBTestData.currentGray.resize(3);
        }
        //入队
        for (int i = hisI_R; i <= hisI_B; i++) {
            d->mPosHisData[i].addData(tempRGB[i]);
            d->mPosHisData[i].getLinearData(d->mHistogramData[i]);
            if (d->mPosHisMaxY < tempRGB[i]) {
                d->mPosHisMaxY = tempRGB[i];
            }
            //点模式统计：对采样序列求值，否则界面上的统计值会一直停在 0
            const PosStatistics st = calcPosStatistics(d->mHistogramData[i], bitMaxValue(info.bit));
            d->mRGBTestData.ave[i - 1] = st.ave;
            d->mRGBTestData.max[i - 1] = st.max;
            d->mRGBTestData.min[i - 1] = st.min;
            d->mRGBTestData.std[i - 1] = st.std;
            d->mRGBTestData.Uniformity[i - 1] = st.uniformity;
            //更新折线
            publishSeries(i);
        }
        d->mRGBTestData.currentGray[0] = static_cast<int32_t>(std::lround(tempRGB[hisI_R]));
        d->mRGBTestData.currentGray[1] = static_cast<int32_t>(std::lround(tempRGB[hisI_G]));
        d->mRGBTestData.currentGray[2] = static_cast<int32_t>(std::lround(tempRGB[hisI_B]));
        calcRe.calcHisMaxY = std::max(1.0, d->mPosHisMaxY);
        calcRe.calcXRangePara = mPosHisMax;
    }
    else {
        if (d->mMaskIsfullzero) {
            d->mHistogramData[hisI_R].assign(d->mHistogramData[hisI_R].size(), 0.0);
            d->mHistogramData[hisI_G].assign(d->mHistogramData[hisI_G].size(), 0.0);
            d->mHistogramData[hisI_B].assign(d->mHistogramData[hisI_B].size(), 0.0);

            d->mRGBTestData.ave.assign(d->mRGBTestData.ave.size(), 0.0);
            d->mRGBTestData.max.assign(d->mRGBTestData.max.size(), 0.0);
            d->mRGBTestData.min.assign(d->mRGBTestData.min.size(), 0.0);
            d->mRGBTestData.std.assign(d->mRGBTestData.std.size(), 0.0);
            d->mRGBTestData.Uniformity.assign(d->mRGBTestData.Uniformity.size(), 0.0);
        }
        else {
            CyMediaCalc::computeBayerHistogram(data, info, &d->mClacMask, d->mMaskHaveData,
                d->mHistogramData[hisI_R], d->mHistogramData[hisI_G], d->mHistogramData[hisI_B],
                d->mRGBTestData.max, d->mRGBTestData.min, d->mRGBTestData.ave, formatOpe.bayerFunc);
        }
        //更新数据
        for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
            publishSeries(hisI);
        }
    }
    if (false == d->mIsPos && false == d->mMaskIsfullzero) {
        //计算参数
        std::vector<int> maxXrange(3);
        CyMediaCalc::computerThreeUniformity(
            d->mHistogramData[hisI_R], d->mHistogramData[hisI_G], d->mHistogramData[hisI_B],
            d->mRGBTestData.ave, bitMaxValue(info.bit),
            d->mRGBTestData.std, d->mRGBTestData.Uniformity, maxXrange);
        calcRe.calcXRangePara = (maxXrange[0] > maxXrange[1]) ? maxXrange[0] : qMax(maxXrange[1], maxXrange[2]);
        //计算Y轴显示范围
        for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
            if (d->mHisView->isSeriesVisible(hisI)) {
                calcRe.calcHisMaxY = std::max(calcRe.calcHisMaxY,
                    CyMediaCalc::determineYAxisMax(d->mHistogramData[hisI], int(d->mHistogramData[hisI].size() * 0.02)));
            }
        }
    }

    return calcRe;
}

CyMediaDisGrayTest::calcImageHisFlag CyMediaDisGrayTest::upImage_YUV(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe) {
    calcImageHisFlag calcRe;

    if (d->mIsPos) {
        auto item = getCurrentItem();
        if (!item) {
            return calcRe;
        }
        //计算像素点
        double tempRGB[4] = { 0.0, 0.0, 0.0, 0.0 };
        auto pos = item->pos();
        CyMediaCalc::calcCoordinateColor(info, data, static_cast<int>(pos.x()), static_cast<int>(pos.y()),
            &tempRGB[hisI_R], &tempRGB[hisI_G], &tempRGB[hisI_B], formatOpe);
        if (d->mRGBTestData.currentGray.size() != 3) {
            d->mRGBTestData.currentGray.resize(3);
        }
        d->mRGBTestData.currentGray[0] = static_cast<int32_t>(std::lround(tempRGB[hisI_R]));
        d->mRGBTestData.currentGray[1] = static_cast<int32_t>(std::lround(tempRGB[hisI_G]));
        d->mRGBTestData.currentGray[2] = static_cast<int32_t>(std::lround(tempRGB[hisI_B]));

        //只取 Y 分量时等同灰度（正常流程会走 upImage_Mono，这里兜底），否则按 RGB 三通道显示
        if (formatOpe.YUVFunc == YUVTRANS_Y) {
            const double gray = tempRGB[hisI_Gray];
            d->mPosHisData[hisI_Gray].addData(gray);
            d->mPosHisData[hisI_Gray].getLinearData(d->mHistogramData[hisI_Gray]);
            if (d->mPosHisMaxY < gray) {
                d->mPosHisMaxY = gray;
            }
            d->mHisTestData.currentGray = static_cast<int32_t>(std::lround(gray));
            //点模式统计：对采样序列求值，否则界面上的统计值会一直停在 0
            const PosStatistics st = calcPosStatistics(d->mHistogramData[hisI_Gray], bitMaxValue(info.bit));
            d->mHisTestData.ave = st.ave;
            d->mHisTestData.max = st.max;
            d->mHisTestData.min = st.min;
            d->mHisTestData.std = st.std;
            d->mHisTestData.Uniformity = st.uniformity;
            //更新折线
            publishSeries(hisI_Gray);
        }
        else {
            for (int i = hisI_R; i <= hisI_B; i++) {
                d->mPosHisData[i].addData(tempRGB[i]);
                d->mPosHisData[i].getLinearData(d->mHistogramData[i]);
                if (d->mPosHisMaxY < tempRGB[i]) {
                    d->mPosHisMaxY = tempRGB[i];
                }
                const PosStatistics st = calcPosStatistics(d->mHistogramData[i], bitMaxValue(info.bit));
                d->mRGBTestData.ave[i - 1] = st.ave;
                d->mRGBTestData.max[i - 1] = st.max;
                d->mRGBTestData.min[i - 1] = st.min;
                d->mRGBTestData.std[i - 1] = st.std;
                d->mRGBTestData.Uniformity[i - 1] = st.uniformity;
                //更新折线
                publishSeries(i);
            }
        }
        calcRe.calcHisMaxY = std::max(1.0, d->mPosHisMaxY);
        calcRe.calcXRangePara = mPosHisMax;
    }
    else {
        if (d->mMaskIsfullzero) {
            d->mHistogramData[hisI_R].assign(d->mHistogramData[hisI_R].size(), 0.0);
            d->mHistogramData[hisI_G].assign(d->mHistogramData[hisI_G].size(), 0.0);
            d->mHistogramData[hisI_B].assign(d->mHistogramData[hisI_B].size(), 0.0);

            d->mRGBTestData.ave.assign(d->mRGBTestData.ave.size(), 0.0);
            d->mRGBTestData.max.assign(d->mRGBTestData.max.size(), 0.0);
            d->mRGBTestData.min.assign(d->mRGBTestData.min.size(), 0.0);
            d->mRGBTestData.std.assign(d->mRGBTestData.std.size(), 0.0);
            d->mRGBTestData.Uniformity.assign(d->mRGBTestData.Uniformity.size(), 0.0);
        }
        else {
            CyMediaCalc::computerYUVHistogram(data, info, &d->mClacMask, d->mMaskHaveData,
                d->mHistogramData[hisI_R], d->mHistogramData[hisI_G], d->mHistogramData[hisI_B],
                d->mRGBTestData.max, d->mRGBTestData.min, d->mRGBTestData.ave, formatOpe.YUVFunc);
        }
        //更新数据
        for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
            publishSeries(hisI);
        }
    }
    if (false == d->mIsPos && false == d->mMaskIsfullzero) {
        //计算参数
        std::vector<int> maxXrange(3);
        CyMediaCalc::computerThreeUniformity(
            d->mHistogramData[hisI_R], d->mHistogramData[hisI_G], d->mHistogramData[hisI_B],
            d->mRGBTestData.ave, bitMaxValue(info.bit),
            d->mRGBTestData.std, d->mRGBTestData.Uniformity, maxXrange);
        calcRe.calcXRangePara = (maxXrange[0] > maxXrange[1]) ? maxXrange[0] : qMax(maxXrange[1], maxXrange[2]);
        //计算Y轴显示范围
        for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
            if (d->mHisView->isSeriesVisible(hisI)) {
                calcRe.calcHisMaxY = std::max(calcRe.calcHisMaxY,
                    CyMediaCalc::determineYAxisMax(d->mHistogramData[hisI], int(d->mHistogramData[hisI].size() * 0.02)));
            }
        }
    }

    return calcRe;
}

void CyMediaDisGrayTest::flushTrans() {
    d->mCahnnelLabel->setText(tr("Channel"));
    d->mAveageLabel[4]->setText(tr("Average"));
    d->mMaximumLabel[4]->setText(tr("Maximum"));
    d->mMinimumLabel[4]->setText(tr("Minimum"));
    d->mStdLabel[4]->setText(tr("std"));
    d->mUniformityLabel[4]->setText(tr("Uniformity"));

    d->mChannelCtlLab[0]->setText(tr("Gray"));
    d->mChannelCtlLab[1]->setText(tr("Red"));
    d->mChannelCtlLab[2]->setText(tr("Green"));
    d->mChannelCtlLab[3]->setText(tr("Blue"));

    d->mDrawAct[CyDisDrawItem::ItemType::Invalid]->setText(tr("None"));
    d->mDrawAct[CyDisDrawItem::ItemType::Point]->setText(tr("Point"));
    d->mDrawAct[CyDisDrawItem::ItemType::Rectangle]->setText(tr("Rectangle"));
    d->mDrawAct[CyDisDrawItem::ItemType::Line]->setText(tr("Line"));
    d->mDrawAct[CyDisDrawItem::ItemType::Ellipse]->setText(tr("Ellipse"));
    d->mDrawAct[CyDisDrawItem::ItemType::Polygon]->setText(tr("Polygon"));

    d->mResetBtn->setText(tr("Reset shaft"));

    //直方图控件
    d->mXCellStep->setCaption(tr("X/cell"));
    d->mYCellStep->setCaption(tr("Y/cell"));
    d->mAutoYBox->setText(tr("Auto Y"));

    d->mHisView->setChannelName(CyHistogramView::Ch_Gray, (!d->mIsPos && d->mIsGray) ? tr("Quantity") : tr("Gray"));
    d->mHisView->setChannelName(CyHistogramView::Ch_Red, tr("Red"));
    d->mHisView->setChannelName(CyHistogramView::Ch_Green, tr("Green"));
    d->mHisView->setChannelName(CyHistogramView::Ch_Blue, tr("Blue"));
    d->mHisView->setAxisTitles(
        (d->mIsPos || !d->mIsGray) ? tr("Horizontal axis") : tr("Grayscale"), tr("Quantity"));
}

void CyMediaDisGrayTest::setThemeColor(QColor color) {
    d->mHisView->setThemeColor(color);
    d->mXCellStep->setThemeColor(color);
    d->mYCellStep->setThemeColor(color);

    d->mDrawAct[CyDisDrawItem::ItemType::Invalid]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Invalid, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Point]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Point, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Rectangle]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Rectangle, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Line]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Line, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Ellipse]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Ellipse, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Polygon]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Polygon, 32, color));
}

void CyMediaDisGrayTest::onUphisVisible() {
    //折线图 / 柱形图由统一控件切换风格；点模式固定使用折线，且按样本点连线而非阶梯线
    if (d->mIsPos) {
        d->mHisView->setDisplayStyle(CyHistogramView::Style_Line);
        d->mHisView->setStepLine(false);
    }
    else {
        d->mHisView->setDisplayStyle(CyHistogramView::Style_Bars);
        d->mHisView->setStepLine(true);
    }

    for (int i = hisI_Gray; i <= hisI_B; i++) {
        if (i == hisI_Gray) {
            d->mHisView->setSeriesVisible(i, d->mIsGray);
        }
        else {
            d->mHisView->setSeriesVisible(i,
                !d->mIsGray && d->mChannelCtlLab[i]->isChecked());
        }
    }

    d->nTestInfoEditWidget[hisI_Gray]->setVisible(d->mIsGray);
    d->nTestInfoEditWidget[hisI_R]->setVisible(!d->mIsGray);
    d->nTestInfoEditWidget[hisI_G]->setVisible(!d->mIsGray);
    d->nTestInfoEditWidget[hisI_B]->setVisible(!d->mIsGray);

    d->mHisView->setChannelName(CyHistogramView::Ch_Gray, (!d->mIsPos && d->mIsGray) ? tr("Quantity") : tr("Gray"));
    d->mHisView->setAxisTitles(
        (d->mIsPos || !d->mIsGray) ? tr("Horizontal axis") : tr("Grayscale"), tr("Quantity"));

    d->mHisView->update();
}

void CyMediaDisGrayTest::onUpTestData() {
    bool isPoint = d->mIsPos;
    if (d->mIsGray) {
        if (isPoint) {
            d->mAveageLabel[hisI_Gray]->setText(QString("%1(%2)")
            .arg(QString::number(d->mHisTestData.ave, 'f', 3))
            .arg(QString::number(d->mHisTestData.currentGray)));
        }
        else {
            d->mAveageLabel[hisI_Gray]->setText(QString::number(d->mHisTestData.ave, 'f', 3));
        }
        d->mMaximumLabel[hisI_Gray]->setText(QString::number(d->mHisTestData.max, 'f', 3));
        d->mMinimumLabel[hisI_Gray]->setText(QString::number(d->mHisTestData.min, 'f', 3));
        d->mStdLabel[hisI_Gray]->setText(QString::number(d->mHisTestData.std, 'f', 3));
        d->mUniformityLabel[hisI_Gray]->setText(QString::number(d->mHisTestData.Uniformity, 'f', 3));
    }
    else{
        for (int i = hisI_R; i <= hisI_B; i++) {
            if (isPoint) {
                d->mAveageLabel[i]->setText(QString("%1(%2)")
                    .arg(QString::number(d->mRGBTestData.ave[i - 1], 'f', 3))
                    .arg(QString::number(d->mRGBTestData.currentGray[i - 1])));
            }
            else {
                d->mAveageLabel[i]->setText(QString::number(d->mRGBTestData.ave[i - 1], 'f', 3));
            }
            d->mMaximumLabel[i]->setText(QString::number(d->mRGBTestData.max[i - 1], 'f', 3));
            d->mMinimumLabel[i]->setText(QString::number(d->mRGBTestData.min[i - 1], 'f', 3));
            d->mStdLabel[i]->setText(QString::number(d->mRGBTestData.std[i - 1], 'f', 3));
            d->mUniformityLabel[i]->setText(QString::number(d->mRGBTestData.Uniformity[i - 1], 'f', 3));
        }
    }
}

void CyMediaDisGrayTest::onUpHisRange(int minX, int maxX, int maxY) {
    if (d->mHisView->xMaxRange() != maxX || d->mHisView->xMinRange() != minX) {
        d->mHisView->setXMinRange(minX);
        d->mHisView->setXMaxRange(maxX);
        updateAxisRanges();
    }

    if (maxY > 0) {
        //限制手动缩放的 Y 上限，同时给自动模式一个参考峰值
        d->mHisView->setYMaxRange(qMax(1.0, double(maxY) * 4.0));
        d->mHisView->setYHintMax(maxY);
    }
    syncAxisControls();
}

void CyMediaDisGrayTest::updateAxisRanges() {
    syncAxisControls();
}

void CyMediaDisGrayTest::syncAxisControls() {
    if (!d->mHisView) return;

    const int maxX = qMax(1, d->mHisView->xMaxRange() - d->mHisView->xMinRange() + 1);
    const double yBound = std::ceil(d->mHisView->yMaxRange());
    const int maxY = (yBound >= 2000000000.0) ? 2000000000 : qMax(1, int(yBound));

    d->mXCellStep->setRange(1, maxX);
    d->mXCellStep->setValue(d->mHisView->xPerCell());
    d->mYCellStep->setRange(1, maxY);
    d->mYCellStep->setValue(d->mHisView->yPerCell());

    {
        QSignalBlocker blocker(d->mAutoYBox);
        d->mAutoYBox->setChecked(d->mHisView->autoY());
    }
}

bool CyMediaDisGrayTest::currentItemIsPos() {
    auto item = getCurrentItem();
    if (!item)
        return false;
    return item->itemType() == CyDisDrawItem::Point;
}

void CyMediaDisGrayTest::upMask(QSize imgSize) {
    d->mMaskIsfullzero = false;

    auto item = getCurrentItem();
    if (!item) {
        d->mMaskHaveData = false;
        return;
    }
    if (item->isPreViewMode()) {
        d->mMaskHaveData = false;
        return;
    }

    if (item->itemType() == CyDisDrawItem::Point || item->itemType() == CyDisDrawItem::Invalid) {
        d->mMaskHaveData = false;
        return;
    }

    d->mMaskIsfullzero = !CyDisDrawItem::pathToMask(item->pathInScene(), imgSize, d->mClacMask);
    d->mMaskHaveData = true;
}
