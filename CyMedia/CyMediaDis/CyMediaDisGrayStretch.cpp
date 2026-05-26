#include "CyMediaDisGrayStretch.h"

#include "../CyMediaCalc/CyMediaCalc.h"
#include "Histogram/CyQCP.h"
#include "cycustomwidget.h"

#include <QToolTip>

#define debug_msg(fmt, ...) printf("[CyMediaDisGrayStretch(%d)  " fmt, __LINE__, ##__VA_ARGS__)

enum grayHistogrameIndex {
    hisI_Gray = 0,
    hisI_R,
    hisI_G,
    hisI_B
};

struct CyMediaDisGrayStretch::PrivateData {
public:
	//UI
	QColor mThemeColor = QColor(0x2a, 0xa3, 0xc6);
	bool mIsInit = false;
	QCustomPlot* mCustomPlot = nullptr;
	CyHistogram* mHistogram = nullptr;
	QCPItemRect* mSelectionRect = nullptr;

	QSpinBox* mStartNumberBox = nullptr;
	QSpinBox* mEndNumberBox = nullptr;

	QTabWidget* mControlTab;

	QLabel* mAutoStretchLab = nullptr;
	CyCustomWidget::CyCheckButton* mAutoStretchBtn = nullptr;

	QLabel* mRGBStretchTypeLab = nullptr;
	QComboBox* mRGBStretchTypeBox = nullptr;

	//TooTip··················································
	bool mShowPlotTips = true;
	QPoint mLastPlotTipPos;
	int mLastPlotTopX = -1;

	//histogram
	std::vector<double> mHisData;
	int32_t mImageBit = 8;
	int32_t mXrangeMin = 0;
	int32_t mXRange = 255;
	int32_t mYRange = 1000;
	bool mZoomable = false;

	// stretch
	StretchValue mStretchValue;
	CyMedia::StretchType mStretchType = CyMedia::stretch_HSV;
	bool mAutostretch = false;
	bool mIsDragging = false;
	bool mIsResizingLeft = false;
	bool mIsResizingRight = false;
	QPointF mLastMousePos;
	double mSelect_min = 0;
	double mSelect_max = 255;
	double mSelect_min_width = 10.0;
	const int mRESIZE_THRESHOLD = 8;
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
}

void CyMediaDisGrayStretch::setThemeColor(QColor color) {
    color.setAlpha(0x50);
    d->mSelectionRect->setBrush(color);
    color.setAlpha(d->mSelectionRect->pen().color().alpha());
    d->mSelectionRect->setPen(QPen(color, 2));

    color.setAlpha(0xFF);
    d->mAutoStretchBtn->setEnableCheckColor(color);
}

bool CyMediaDisGrayStretch::upImageData(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::DemosaicMethod Method) {
    QElapsedTimer eTimer;

    eTimer.restart();
    int currentCtrIndex = d->mControlTab->currentIndex();
    //切换
    if (info.isMono()) {
        if (d->mControlTab->currentIndex() != 0) {
            emit transImageType(0);
            currentCtrIndex = 0;
        }
    }
    else {
        if (d->mControlTab->currentIndex() != 1) {
            emit transImageType(1);
            currentCtrIndex = 1;
        }
    }
    //debug_msg("切换界面 耗时：%lldms\n", eTimer.elapsed());

    eTimer.restart();
    d->mImageBit = info.bit;
    //计算直方图
    int32_t maxBitValue = 1 << info.bit;
    int stretchMax = maxBitValue - 1;
    double calcHisMinX = -(maxBitValue * 0.05);
    double calcHisMaxX = maxBitValue * 1.05;
    if (d->mStretchType == CyMedia::stretch_Lab) {
        calcHisMaxX = 120;
        stretchMax = 100;
    }
    //debug_msg("计算直方图 耗时：%lldms\n", eTimer.elapsed());

    eTimer.restart();
    //更新拉伸边界范围
    if (stretchMax != d->mSelect_max) {
        d->mSelect_max = stretchMax;
        emit upEditRange();
    }
    //debug_msg("更新拉伸边界范围 耗时：%lldms\n", eTimer.elapsed());

    eTimer.restart();
    //更新直方图
    double calcHisMaxY = 0;
    if (info.isMono()) {
        double maxV, minV, aveV;
        CyMedia::computeGrayHistogram(data, info, 0, false, 
            d->mHisData,
            &maxV, &minV, &aveV);
        d->mHistogram->updateHistogramFromThread(d->mHisData.data(), d->mHisData.size());
        //自动拉伸
        if (d->mAutostretch) {
            int32_t start, end;
            CyMedia::computeGrayStretchPara(d->mHisData, start, end);
            emit upStretchRange(start, end);
        }
    } 
    else if (info.isBayer()) {
        CyMedia::computeBayerHistogram(data, info,
            d->mHisData,
            d->mStretchType,
            Method);

        d->mHistogram->updateHistogramFromThread(d->mHisData.data(), d->mHisData.size());
    }
    else {
        CyMedia::computeRGBHistogram(data, info, 0, false,
            d->mHisData,
            d->mStretchType);

        d->mHistogram->updateHistogramFromThread(d->mHisData.data(), d->mHisData.size());
    }
    //debug_msg("更新直方图 耗时：%lldms\n", eTimer.elapsed());

    eTimer.restart();
    //更新显示范围
    std::sort(d->mHisData.begin(), d->mHisData.end());
    size_t idx97 = static_cast<size_t>(d->mHisData.size() * 0.97);
    calcHisMaxY = d->mHisData[idx97];
    emit upHisRange(static_cast<int>(calcHisMinX), static_cast<int>(calcHisMaxX), static_cast<int>(calcHisMaxY));
    //debug_msg("更新显示范围 耗时：%lldms\n", eTimer.elapsed());

    return true;
}

bool CyMediaDisGrayStretch::isZoomble() {
    return d->mZoomable;
}

void CyMediaDisGrayStretch::setZoomble(bool zoom) {
    if (d->mZoomable == zoom)
        return;
    if (d->mZoomable != zoom) {
        d->mZoomable = zoom;
        if (d->mZoomable) {
            d->mCustomPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // 可拖动 / 缩放
        }
        else {
            d->mCustomPlot->setInteractions(QCP::iNone); // 可拖动 / 缩放
        }
    }
}

bool CyMediaDisGrayStretch::axisToolTipVisible() {
    return d->mShowPlotTips;
}

void CyMediaDisGrayStretch::setAxisToolTipVisible(bool visi) {
    d->mShowPlotTips = visi;
    if (!visi)
        QToolTip::hideText();
}

CyMedia::StretchType CyMediaDisGrayStretch::stretchtype() {
    if (false == isVisible()/* || (mStretchValue.start == 0 && mStretchValue.end == mStretchValue.max)*/) {
        return CyMedia::stretch_None;
    }
    return d->mStretchType;
}

CyMediaDisGrayStretch::StretchValue CyMediaDisGrayStretch::stretchValue() {
    return d->mStretchValue;
}

bool CyMediaDisGrayStretch::isAutoStretch() {
    return d->mAutoStretchBtn->isChecked() && d->mControlTab->currentIndex() == 0;
}

bool CyMediaDisGrayStretch::eventFilter(QObject* watched, QEvent* event) {
    if (watched == d->mCustomPlot) {
        bool selectMove = mouseOnSelectRect(event);
        if (event->type() == QEvent::MouseMove && false == selectMove) {
            plotMouseMove(static_cast<QMouseEvent*>(event)->pos());
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            plotMouseMove(static_cast<QMouseEvent*>(event)->pos());
        }
    }

    return QWidget::eventFilter(watched, event);
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
    emit needImage();
    QWidget::showEvent(event);
}

bool CyMediaDisGrayStretch::mouseOnSelectRect(QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        updateCursor(mouseEvent->pos());
        if (d->mIsDragging || d->mIsResizingLeft || d->mIsResizingRight) {
            double currentX = int(d->mCustomPlot->xAxis->pixelToCoord(mouseEvent->pos().x()));
            currentX = qBound(d->mSelect_min, currentX, d->mSelect_max);

            double rectLeft = d->mSelectionRect->topLeft->coords().x();
            double rectRight = d->mSelectionRect->bottomRight->coords().x();
            double rectWidth = rectRight - rectLeft;


            double newLeft = rectLeft;
            double newRight = rectRight;
            if (d->mIsDragging) {
                // 计算移动偏移量
                double deltaX = currentX - d->mLastMousePos.x();
                newLeft = rectLeft + deltaX;
                newRight = rectRight + deltaX;

                // 边界限制
                if (newLeft < d->mSelect_min) {
                    newLeft = d->mSelect_min;
                    newRight = d->mSelect_min + rectWidth;
                }
                if (newRight > d->mSelect_max) {
                    newRight = d->mSelect_max;
                    newLeft = d->mSelect_max - rectWidth;
                }
            }
            else if (d->mIsResizingLeft) {
                // 调整左边界
                newLeft = qBound(d->mSelect_min, currentX, rectRight - d->mSelect_min_width);
            }
            else if (d->mIsResizingRight) {
                // 调整右边界
                newRight = qBound(rectLeft + d->mSelect_min_width, currentX, d->mSelect_max);
            }

            upSelectRectChange(int32_t(newLeft), int32_t(newRight));

            d->mLastMousePos = QPointF(currentX, 0);
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            double xCoord = d->mCustomPlot->xAxis->pixelToCoord(mouseEvent->pos().x());
            double rectLeft = d->mSelectionRect->topLeft->coords().x();
            double rectRight = d->mSelectionRect->bottomRight->coords().x();

            // 转换为像素进行边缘检测
            double leftEdgePixel = d->mCustomPlot->xAxis->coordToPixel(rectLeft);
            double rightEdgePixel = d->mCustomPlot->xAxis->coordToPixel(rectRight);

            if (qAbs(mouseEvent->pos().x() - leftEdgePixel) <= d->mRESIZE_THRESHOLD &&
                xCoord >= rectLeft - 5 && xCoord <= rectRight) {
                d->mIsResizingLeft = true;
                d->mLastMousePos = QPointF(xCoord, 0);
                d->mCustomPlot->setCursor(Qt::SizeHorCursor);
                return true;
            }

            if (qAbs(mouseEvent->pos().x() - rightEdgePixel) <= d->mRESIZE_THRESHOLD &&
                xCoord >= rectLeft && xCoord <= rectRight + 5) {
                d->mIsResizingRight = true;
                d->mLastMousePos = QPointF(xCoord, 0);
                d->mCustomPlot->setCursor(Qt::SizeHorCursor);
                return true;
            }

            if (xCoord > rectLeft && xCoord < rectRight) {
                d->mIsDragging = true;
                d->mLastMousePos = QPointF(xCoord, 0);
                d->mCustomPlot->setCursor(Qt::SizeAllCursor);
                return true;
            }
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            d->mIsDragging = false;
            d->mIsResizingLeft = false;
            d->mIsResizingRight = false;
            d->mCustomPlot->setCursor(Qt::ArrowCursor);

            // 确保选择区域在边界内
            double rectLeft = d->mSelectionRect->topLeft->coords().x();
            double rectRight = d->mSelectionRect->bottomRight->coords().x();

            double newLeft = rectLeft;
            double newRight = rectRight;
            if (rectLeft < d->mSelect_min) {
                newLeft = d->mSelect_min;
            }
            if (rectRight > d->mSelect_max) {
                newRight = d->mSelect_max;
            }
            if (rectRight - rectLeft < d->mSelect_min_width) {
                if (rectLeft < d->mSelect_min + d->mSelect_min_width) {
                    newRight = d->mSelect_max;
                }
                else if (rectRight > d->mSelect_max - d->mSelect_min_width) {
                    newLeft = d->mSelect_max - d->mSelect_min_width;
                }
            }

            upSelectRectChange(int32_t(newLeft), int32_t(newRight));
            return true;
        }
    }
    else if (event->type() == QEvent::Leave) {
        d->mCustomPlot->setCursor(Qt::ArrowCursor);
    }

    return false;
}

void CyMediaDisGrayStretch::plotMouseMove(QPoint mousePos) {
    if (false == d->mShowPlotTips)
        return;
    //鼠标坐标转化CustomPlot内部坐标
    double dValueX = d->mCustomPlot->xAxis->pixelToCoord(mousePos.x());
    double dValueY = d->mCustomPlot->yAxis->pixelToCoord(mousePos.y());
    ////判断是否超出坐标轴范围
    //if (dValueX < mCustomPlot->xAxis->range().lower || dValueX > mCustomPlot->xAxis->range().upper ||
    //    dValueY < mCustomPlot->yAxis->range().lower || dValueY > mCustomPlot->yAxis->range().upper) {
    //    QToolTip::hideText();
    //    return;
    //}
    //逐个图像判断符合条件的曲线点
    if (plotToolTips(dValueX, dValueY)) {
        d->mLastPlotTipPos = mousePos;
    }
}

bool CyMediaDisGrayStretch::plotToolTips(double xValue, double yValue) {
    double calcXvalue = 0;
    double calcYvalue = 0.0;
    //X像素比
    double dRatioX = d->mCustomPlot->xAxis->axisRect()->width() / (d->mCustomPlot->xAxis->range().upper - d->mCustomPlot->xAxis->range().lower);

    //取得最接近的坐标
    int index_left = d->mHistogram->findBegin(xValue, true);
    int index_right = d->mHistogram->findEnd(xValue, true);
    double dif_left = fabs(d->mHistogram->data()->at(index_left)->key - xValue);
    double dif_right = fabs(d->mHistogram->data()->at(index_right)->key - xValue);
    int iPointIdx = ((dif_left < dif_right) ? index_left : index_right);
    calcXvalue = d->mHistogram->data()->at(iPointIdx)->key;
    //鼠标值和最近值的差值
    double dx = fabs(calcXvalue - xValue);
    //判断离坐标的距离是否在范围内
    static double adjugeWidth = 20.0;
    if (dx * dRatioX <= adjugeWidth) {
        /*if (mLastPlotTopX == calcXvalue) {
            return true;
        }*/
        d->mLastPlotTopX = calcXvalue;
        calcYvalue = d->mHistogram->data()->at(iPointIdx)->value;
        QToolTip::showText(cursor().pos(), getPosToolTip(calcXvalue, calcYvalue), d->mCustomPlot);
        return true;
    }

    QToolTip::hideText();
    return false;
}

void CyMediaDisGrayStretch::onStartValueEdit() {
    int start = d->mStartNumberBox->value();
    if (d->mStretchValue.start == start) {
        return;
    }

    d->mStretchValue.start = start;
    onUpSelectRectRange(d->mStretchValue.start, d->mStretchValue.end);
}

void CyMediaDisGrayStretch::onEndValueEdit() {
    int end = d->mEndNumberBox->value();
    if (d->mStretchValue.end == end) {
        return;
    }

    d->mStretchValue.end = end;
    onUpSelectRectRange(d->mStretchValue.start, d->mStretchValue.end);
}

void CyMediaDisGrayStretch::onAutoStretchChange(bool enable) {
    if (d->mAutostretch == enable)
        return;
    d->mAutostretch = enable;
    emit needImage();
}

void CyMediaDisGrayStretch::onStretchTypeChange(int idx) {
    d->mStretchType = CyMedia::StretchType(idx + 1);
    if (d->mStretchType == CyMedia::stretch_Lab && d->mSelect_max != 100) {
        d->mSelect_max = 100;
        onUpEditRange();
    }
    emit needImage();
}

void CyMediaDisGrayStretch::upSelectRectChange(int32_t start, int32_t end, bool needSignals/* = true*/) {
    int32_t newS = int32_t(start);
    int32_t newE = int32_t(end);

    d->mStretchValue.start = int32_t(newS);
    d->mStretchValue.end = int32_t(newE);
    onUpSelectRectRange(d->mStretchValue.start, d->mStretchValue.end);
    onUpEditorNumber(d->mStretchValue.start, d->mStretchValue.end);
    emit stretchParaChange();
}

void CyMediaDisGrayStretch::onUpSelectRectRange(int32_t start, int32_t end) {
    d->mSelectionRect->topLeft->setCoords(start, d->mCustomPlot->yAxis->range().upper);
    d->mSelectionRect->bottomRight->setCoords(end, 0);

    d->mCustomPlot->replot();
    emit stretchParaChange();
}

void CyMediaDisGrayStretch::onUpEditorNumber(int32_t  start, int32_t end) {
    d->mStartNumberBox->setValue(start);
    d->mEndNumberBox->setValue(end);
}

void CyMediaDisGrayStretch::updateCursor(const QPointF& pos) {
    if (d->mIsDragging || d->mIsResizingLeft || d->mIsResizingRight) {
        return;
    }

    double rectLeft = d->mSelectionRect->topLeft->coords().x();
    double rectRight = d->mSelectionRect->bottomRight->coords().x();

    // 转换为像素坐标进行边界检测
    double leftEdgePixel = d->mCustomPlot->xAxis->coordToPixel(rectLeft);
    double rightEdgePixel = d->mCustomPlot->xAxis->coordToPixel(rectRight);

    // 检查鼠标是否在左边界或右边界附近
    if (qAbs(pos.x() - leftEdgePixel) <= d->mRESIZE_THRESHOLD ||
        qAbs(pos.x() - rightEdgePixel) <= d->mRESIZE_THRESHOLD) {
        d->mCustomPlot->setCursor(Qt::SizeHorCursor);
    }
    // 检查鼠标是否在矩形内部
    else if (pos.x() > d->mCustomPlot->xAxis->coordToPixel(rectLeft) &&
        pos.x() < d->mCustomPlot->xAxis->coordToPixel(rectRight)) {
        d->mCustomPlot->setCursor(Qt::SizeAllCursor);
    }
    // 默认光标
    else {
        d->mCustomPlot->setCursor(Qt::ArrowCursor);
    }
}

void CyMediaDisGrayStretch::initGUI() {
    if (d->mIsInit)
        return;
    d->mIsInit = true;

    d->mStretchValue = { 0, 255, 255 };

    //plot
    d->mCustomPlot = new QCustomPlot(this);
    d->mCustomPlot->installEventFilter(this);
    //select
    d->mSelectionRect = new QCPItemRect(d->mCustomPlot);
    d->mSelectionRect->topLeft->setType(QCPItemPosition::ptPlotCoords);
    d->mSelectionRect->bottomRight->setType(QCPItemPosition::ptPlotCoords);

    // 设置初始选择区域 (50-150)
    onUpSelectRectRange(d->mStretchValue.start, d->mStretchValue.end);

    //histogram
    d->mHistogram = new CyHistogram(d->mCustomPlot->xAxis, d->mCustomPlot->yAxis);
    d->mHistogram->setPen(QColor(Qt::lightGray));
    d->mHistogram->setWidthType(QCPBars::wtPlotCoords);

    //xAxis
    auto xAxis = d->mCustomPlot->xAxis;
    xAxis->setRange(-1, d->mXRange * 1.1);
    xAxis->grid()->setZeroLinePen(Qt::NoPen);
    xAxis->ticker()->setTickCount(11);
    xAxis->ticker()->setTickStepStrategy(QCPAxisTicker::tssReadability);
    //yAxis
    auto yAxis = d->mCustomPlot->yAxis;
    yAxis->grid()->setZeroLinePen(Qt::NoPen);
    yAxis->setTickLabels(false);
    yAxis->setRange(0, d->mYRange);

    //Ctroll
    d->mStartNumberBox = new QSpinBox(this);
    d->mStartNumberBox->setRange(0, d->mSelect_max);
    d->mStartNumberBox->setValue(d->mStretchValue.start);
    d->mEndNumberBox = new QSpinBox(this);
    d->mEndNumberBox->setRange(d->mSelect_min_width, d->mSelect_max);
    d->mEndNumberBox->setValue(d->mStretchValue.end);

    d->mControlTab = new QTabWidget(this);
    d->mControlTab->tabBar()->setVisible(false);

    d->mAutoStretchLab = new QLabel(this);
    d->mAutoStretchBtn = new CyCustomWidget::CyCheckButton(this);
    d->mAutoStretchBtn->setChecked(d->mAutostretch);

    d->mRGBStretchTypeLab = new QLabel(this);
    d->mRGBStretchTypeBox = new QComboBox(this);
    d->mRGBStretchTypeBox->addItem("");
    d->mRGBStretchTypeBox->addItem("");
    d->mRGBStretchTypeBox->addItem("");
    d->mRGBStretchTypeBox->setCurrentIndex(1);

    //layout
    QWidget* grayCtrW = new QWidget(this);
    QHBoxLayout* grayCtrLayout = new QHBoxLayout(grayCtrW);
    grayCtrLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    grayCtrLayout->addWidget(d->mAutoStretchLab);
    grayCtrLayout->addWidget(d->mAutoStretchBtn);

    QWidget* rgbCtrW = new QWidget(this);
    QHBoxLayout* rgbCtrLyout = new QHBoxLayout(rgbCtrW);
    rgbCtrLyout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    rgbCtrLyout->addWidget(d->mRGBStretchTypeLab);
    rgbCtrLyout->addWidget(d->mRGBStretchTypeBox);
    d->mControlTab->clear();
    d->mControlTab->addTab(grayCtrW, "");
    d->mControlTab->addTab(rgbCtrW, "");
    grayCtrW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rgbCtrW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QGridLayout* mainLayou = new QGridLayout(this);
    mainLayou->addWidget(d->mCustomPlot, 0, 0, 1, 3);
    mainLayou->addWidget(d->mStartNumberBox, 1, 0, 1, 1);
    mainLayou->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 1, 1, 1);
    mainLayou->addWidget(d->mEndNumberBox, 1, 2, 1, 1);
    mainLayou->addWidget(d->mControlTab, 2, 0, 1, 3);
    
    d->mStartNumberBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->mEndNumberBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->mControlTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->mCustomPlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    flushTrans();
    setThemeColor(d->mThemeColor);

    //Connection
    connect(d->mCustomPlot->xAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged), this, [this](const QCPRange& range) {
		// 保持选择矩形覆盖整个Y轴范围
		onUpSelectRectRange(d->mStretchValue.start, d->mStretchValue.end);
		d->mCustomPlot->replot();
        });
	connect(d->mCustomPlot->yAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged), this, [this](const QCPRange& range) {
		// 保持选择矩形覆盖整个Y轴范围
		onUpSelectRectRange(d->mStretchValue.start, d->mStretchValue.end);
		d->mCustomPlot->replot();
		});

    connect(d->mStartNumberBox, &QSpinBox::editingFinished, this, &CyMediaDisGrayStretch::onStartValueEdit);
    connect(d->mEndNumberBox, &QSpinBox::editingFinished, this, &CyMediaDisGrayStretch::onEndValueEdit);
    connect(d->mAutoStretchBtn, &CyCustomWidget::CyCheckButton::stateChanged, this, &CyMediaDisGrayStretch::onAutoStretchChange);
    connect(d->mRGBStretchTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CyMediaDisGrayStretch::onStretchTypeChange);

    connect(this, &CyMediaDisGrayStretch::transImageType, this, &CyMediaDisGrayStretch::onTransImageType);
    connect(this, &CyMediaDisGrayStretch::upStretchRange, this, &CyMediaDisGrayStretch::onUpStretchRange);
    connect(this, &CyMediaDisGrayStretch::upHisRange, this, &CyMediaDisGrayStretch::onUpHisRange);
    connect(this, &CyMediaDisGrayStretch::upEditRange, this, &CyMediaDisGrayStretch::onUpEditRange);
}

QString CyMediaDisGrayStretch::getPosToolTip(double xValue, double yValue) {
    QString reStr;
    switch (d->mStretchType) {
        case CyMedia::stretch_Gray: {
            reStr = QString("%1:%2\n%3:%4")
                .arg(tr("Grayscale")).arg(xValue)
                .arg(tr("Quantity")).arg(yValue);
            }break;

        case CyMedia::stretch_HSV: 
        case CyMedia::stretch_Lab: {
            reStr = QString("%1:%2\n%3:%4")
                .arg(tr("Brightness")).arg(xValue)
                .arg(tr("Quantity")).arg(yValue);
            }break;
    }

    return reStr;
}

void CyMediaDisGrayStretch::onTransImageType(int index) {
    d->mControlTab->setCurrentIndex(index);
}

void CyMediaDisGrayStretch::onUpStretchRange(int start, int end) {
    upSelectRectChange(start, end, false);
}

void CyMediaDisGrayStretch::onUpHisRange(int minX, int maxX, int maxY) {
    if (d->mXRange != maxX ||
        d->mXrangeMin != minX) {
        d->mXRange = maxX;
        d->mXrangeMin = minX;
        d->mCustomPlot->xAxis->setRange(d->mXrangeMin, d->mXRange);
    }

    if (maxY <= 0) return;
    float yChange = abs(d->mYRange - maxY) / d->mYRange;
    if (yChange > 0.2) {
        d->mYRange = maxY;
        d->mCustomPlot->yAxis->setRange(0, d->mYRange);
    }
}

void CyMediaDisGrayStretch::onUpEditRange() {
    d->mStretchValue.max = d->mSelect_max;

    auto oldS = d->mStartNumberBox->value();
    auto oldE = d->mEndNumberBox->value();

    d->mStartNumberBox->setRange(0, d->mSelect_max - d->mSelect_min_width);
    d->mEndNumberBox->setRange(d->mSelect_min_width, d->mSelect_max);

    d->mStartNumberBox->setValue(oldS);
    d->mEndNumberBox->setValue(oldE);

    onStartValueEdit();
    onEndValueEdit();
}

