#include "CyMediaDisGrayStretch.h"
#include <QToolTip>

enum grayHistogrameIndex {
    hisI_Gray = 0,
    hisI_R,
    hisI_G,
    hisI_B
};

CyMediaDisGrayStretch::CyMediaDisGrayStretch(QWidget* parent /*= nullptr*/)
    :QWidget(parent) {
    initGUI();
}

CyMediaDisGrayStretch::~CyMediaDisGrayStretch() {
    
}

void CyMediaDisGrayStretch::flushTrans() {
    mAutoStretchLab->setText(tr("Automatic"));

    mRGBStretchTypeLab->setText(tr("Stretching methods"));
    int32_t idx = 0;
    mRGBStretchTypeBox->setItemText(idx++, tr("Gray"));
    mRGBStretchTypeBox->setItemText(idx++, tr("HSV"));
    mRGBStretchTypeBox->setItemText(idx++, tr("Lab"));
}

void CyMediaDisGrayStretch::setThemeColor(QColor color) {
    color.setAlpha(0x50);
    mSelectionRect->setBrush(color);
    color.setAlpha(mSelectionRect->pen().color().alpha());
    mSelectionRect->setPen(QPen(color, 2));

    color.setAlpha(0xFF);
    mAutoStretchBtn->setEnableCheckColor(color);
}

bool CyMediaDisGrayStretch::upImageData(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::DemosaicMethod Method) {
    int currentCtrIndex = mControlTab->currentIndex();
    //切换
    if (info.isMono()) {
        if (mControlTab->currentIndex() != 0) {
            emit transImageType(0);
            currentCtrIndex = 0;
        }
    }
    else {
        if (mControlTab->currentIndex() != 1) {
            emit transImageType(1);
            currentCtrIndex = 1;
        }
    }

    mImageBit = info.bit;
    //计算直方图
    int32_t maxBitValue = 1 << info.bit;
    int stretchMax = maxBitValue - 1;
    double calcHisMinX = -(maxBitValue * 0.05);
    double calcHisMaxX = maxBitValue * 1.05;
    if (mStretchType == CyMedia::stretch_Lab) {
        calcHisMaxX = 120;
        stretchMax = 100;
    }
    //更新拉伸边界范围
    if (stretchMax != mSelect_max) {
        mSelect_max = stretchMax;
        emit upEditRange();
    }

    double calcHisMaxY = 0;
    if (info.isMono()) {
        double maxV, minV, aveV;
        CyMedia::computeGrayHistogram(data, info, 0, false, 
            mHisData, 
            &maxV, &minV, &aveV);
        mHistogram->updateHistogramFromThread(mHisData.data(), mHisData.size());
        //自动拉伸
        if (mAutostretch) {
            int32_t start, end;
            CyMedia::computeGrayStretchPara(mHisData, start, end);
            emit upStretchRange(start, end);
        }
    } 
    else if (info.isBayer()) {
        CyMedia::computeBayerHistogram(data, info,
            mHisData,
            mStretchType,
            Method);

        mHistogram->updateHistogramFromThread(mHisData.data(), mHisData.size());
    }
    else {
        CyMedia::computeRGBHistogram(data, info, 0, false,
            mHisData,
            mStretchType);

        mHistogram->updateHistogramFromThread(mHisData.data(), mHisData.size());
    }
    //更新显示范围
    std::sort(mHisData.begin(), mHisData.end());
    size_t idx97 = static_cast<size_t>(mHisData.size() * 0.97);
    calcHisMaxY = mHisData[idx97];
    emit upHisRange(static_cast<int>(calcHisMinX), static_cast<int>(calcHisMaxX), static_cast<int>(calcHisMaxY));

    return true;
}

bool CyMediaDisGrayStretch::isZoomble() {
    return mZoomable;
}

void CyMediaDisGrayStretch::setZoomble(bool zoom) {
    if (mZoomable == zoom)
        return;
    if (mZoomable != zoom) {
        mZoomable = zoom;
        if (mZoomable) {
            mCustomPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // 可拖动 / 缩放
        }
        else {
            mCustomPlot->setInteractions(QCP::iNone); // 可拖动 / 缩放
        }
    }
}

bool CyMediaDisGrayStretch::axisToolTipVisible() {
    return mShowPlotTips;
}

void CyMediaDisGrayStretch::setAxisToolTipVisible(bool visi) {
    mShowPlotTips = visi;
    if (!visi)
        QToolTip::hideText();
}

CyMedia::StretchType CyMediaDisGrayStretch::stretchtype() {
    if (false == isVisible() || (
        mStretchValue.start == 0 &&
        mStretchValue.end == mStretchValue.max)) {
        return CyMedia::stretch_None;
    }
    return mStretchType;
}

CyMediaDisGrayStretch::StretchValue CyMediaDisGrayStretch::stretchValue() {
    return mStretchValue;
}

bool CyMediaDisGrayStretch::isAutoStretch() {
    return mAutoStretchBtn->isChecked() && mControlTab->currentIndex() == 0;
}

bool CyMediaDisGrayStretch::eventFilter(QObject* watched, QEvent* event) {
    if (watched == mCustomPlot) {
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
        if (mIsDragging || mIsResizingLeft || mIsResizingRight) {
            double currentX = int(mCustomPlot->xAxis->pixelToCoord(mouseEvent->pos().x()));
            currentX = qBound(mSelect_min, currentX, mSelect_max);

            double rectLeft = mSelectionRect->topLeft->coords().x();
            double rectRight = mSelectionRect->bottomRight->coords().x();
            double rectWidth = rectRight - rectLeft;


            double newLeft = rectLeft;
            double newRight = rectRight;
            if (mIsDragging) {
                // 计算移动偏移量
                double deltaX = currentX - mLastMousePos.x();
                newLeft = rectLeft + deltaX;
                newRight = rectRight + deltaX;

                // 边界限制
                if (newLeft < mSelect_min) {
                    newLeft = mSelect_min;
                    newRight = mSelect_min + rectWidth;
                }
                if (newRight > mSelect_max) {
                    newRight = mSelect_max;
                    newLeft = mSelect_max - rectWidth;
                }
            }
            else if (mIsResizingLeft) {
                // 调整左边界
                newLeft = qBound(mSelect_min, currentX, rectRight - mSelect_min_width);
            }
            else if (mIsResizingRight) {
                // 调整右边界
                newRight = qBound(rectLeft + mSelect_min_width, currentX, mSelect_max);
            }

            upSelectRectChange(int32_t(newLeft), int32_t(newRight));

            mLastMousePos = QPointF(currentX, 0);
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            double xCoord = mCustomPlot->xAxis->pixelToCoord(mouseEvent->pos().x());
            double rectLeft = mSelectionRect->topLeft->coords().x();
            double rectRight = mSelectionRect->bottomRight->coords().x();

            // 转换为像素进行边缘检测
            double leftEdgePixel = mCustomPlot->xAxis->coordToPixel(rectLeft);
            double rightEdgePixel = mCustomPlot->xAxis->coordToPixel(rectRight);

            if (qAbs(mouseEvent->pos().x() - leftEdgePixel) <= mRESIZE_THRESHOLD &&
                xCoord >= rectLeft - 5 && xCoord <= rectRight) {
                mIsResizingLeft = true;
                mLastMousePos = QPointF(xCoord, 0);
                mCustomPlot->setCursor(Qt::SizeHorCursor);
                return true;
            }

            if (qAbs(mouseEvent->pos().x() - rightEdgePixel) <= mRESIZE_THRESHOLD &&
                xCoord >= rectLeft && xCoord <= rectRight + 5) {
                mIsResizingRight = true;
                mLastMousePos = QPointF(xCoord, 0);
                mCustomPlot->setCursor(Qt::SizeHorCursor);
                return true;
            }

            if (xCoord > rectLeft && xCoord < rectRight) {
                mIsDragging = true;
                mLastMousePos = QPointF(xCoord, 0);
                mCustomPlot->setCursor(Qt::SizeAllCursor);
                return true;
            }
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            mIsDragging = false;
            mIsResizingLeft = false;
            mIsResizingRight = false;
            mCustomPlot->setCursor(Qt::ArrowCursor);

            // 确保选择区域在边界内
            double rectLeft = mSelectionRect->topLeft->coords().x();
            double rectRight = mSelectionRect->bottomRight->coords().x();

            double newLeft = rectLeft;
            double newRight = rectRight;
            if (rectLeft < mSelect_min) {
                newLeft = mSelect_min;
            }
            if (rectRight > mSelect_max) {
                newRight = mSelect_max;
            }
            if (rectRight - rectLeft < mSelect_min_width) {
                if (rectLeft < mSelect_min + mSelect_min_width) {
                    newRight = mSelect_max;
                }
                else if (rectRight > mSelect_max - mSelect_min_width) {
                    newLeft = mSelect_max - mSelect_min_width;
                }
            }

            upSelectRectChange(int32_t(newLeft), int32_t(newRight));
            return true;
        }
    }
    else if (event->type() == QEvent::Leave) {
        mCustomPlot->setCursor(Qt::ArrowCursor);
    }

    return false;
}

void CyMediaDisGrayStretch::plotMouseMove(QPoint mousePos) {
    if (false == mShowPlotTips)
        return;
    //鼠标坐标转化CustomPlot内部坐标
    double dValueX = mCustomPlot->xAxis->pixelToCoord(mousePos.x());
    double dValueY = mCustomPlot->yAxis->pixelToCoord(mousePos.y());
    ////判断是否超出坐标轴范围
    //if (dValueX < mCustomPlot->xAxis->range().lower || dValueX > mCustomPlot->xAxis->range().upper ||
    //    dValueY < mCustomPlot->yAxis->range().lower || dValueY > mCustomPlot->yAxis->range().upper) {
    //    QToolTip::hideText();
    //    return;
    //}
    //逐个图像判断符合条件的曲线点
    if (plotToolTips(dValueX, dValueY))
        mLastPlotTipPos = mousePos;
}

bool CyMediaDisGrayStretch::plotToolTips(double xValue, double yValue) {
    double calcXvalue = 0;
    double calcYvalue = 0.0;
    //X像素比
    double dRatioX = mCustomPlot->xAxis->axisRect()->width() / (mCustomPlot->xAxis->range().upper - mCustomPlot->xAxis->range().lower);

    //取得最接近的坐标
    int index_left = mHistogram->findBegin(xValue, true);
    int index_right = mHistogram->findEnd(xValue, true);
    double dif_left = fabs(mHistogram->data()->at(index_left)->key - xValue);
    double dif_right = fabs(mHistogram->data()->at(index_right)->key - xValue);
    int iPointIdx = ((dif_left < dif_right) ? index_left : index_right);
    calcXvalue = mHistogram->data()->at(iPointIdx)->key;
    //鼠标值和最近值的差值
    double dx = fabs(calcXvalue - xValue);
    //判断离坐标的距离是否在范围内
    static double adjugeWidth = 20.0;
    if (dx * dRatioX <= adjugeWidth) {
        /*if (mLastPlotTopX == calcXvalue) {
            return true;
        }*/
        mLastPlotTopX = calcXvalue;
        calcYvalue = mHistogram->data()->at(iPointIdx)->value;
        QToolTip::showText(cursor().pos(), getPosToolTip(calcXvalue, calcYvalue), mCustomPlot);
        return true;
    }

    QToolTip::hideText();
    return false;
}

void CyMediaDisGrayStretch::onStartValueEdit() {
    int start = mStartNumberBox->value();
    if (mStretchValue.start == start) {
        return;
    }

    mStretchValue.start = start;
    onUpSelectRectRange(mStretchValue.start, mStretchValue.end);
}

void CyMediaDisGrayStretch::onEndValueEdit() {
    int end = mEndNumberBox->value();
    if (mStretchValue.end == end) {
        return;
    }

    mStretchValue.end = end;
    onUpSelectRectRange(mStretchValue.start, mStretchValue.end);
}

void CyMediaDisGrayStretch::onAutoStretchChange(bool enable) {
    if (mAutostretch == enable)
        return;
    mAutostretch = enable;
    emit needImage();
}

void CyMediaDisGrayStretch::onStretchTypeChange(int idx) {
    mStretchType = CyMedia::StretchType(idx + 1);
    emit needImage();
}

void CyMediaDisGrayStretch::upSelectRectChange(int32_t start, int32_t end, bool needSignals/* = true*/) {
    int32_t newS = int32_t(start);
    int32_t newE = int32_t(end);

    mStretchValue.start = int32_t(newS);
    mStretchValue.end = int32_t(newE);
    onUpSelectRectRange(mStretchValue.start, mStretchValue.end);
    onUpEditorNumber(mStretchValue.start, mStretchValue.end);
    emit stretchParaChange();
}

void CyMediaDisGrayStretch::onUpSelectRectRange(int32_t start, int32_t end) {
    mSelectionRect->topLeft->setCoords(start, mCustomPlot->yAxis->range().upper);
    mSelectionRect->bottomRight->setCoords(end, 0);

    mCustomPlot->replot();
    emit stretchParaChange();
}

void CyMediaDisGrayStretch::onUpEditorNumber(int32_t  start, int32_t end) {
    mStartNumberBox->setValue(start);
    mEndNumberBox->setValue(end);
}

void CyMediaDisGrayStretch::updateCursor(const QPointF& pos) {
    if (mIsDragging || mIsResizingLeft || mIsResizingRight) {
        return;
    }

    double rectLeft = mSelectionRect->topLeft->coords().x();
    double rectRight = mSelectionRect->bottomRight->coords().x();

    // 转换为像素坐标进行边界检测
    double leftEdgePixel = mCustomPlot->xAxis->coordToPixel(rectLeft);
    double rightEdgePixel = mCustomPlot->xAxis->coordToPixel(rectRight);

    // 检查鼠标是否在左边界或右边界附近
    if (qAbs(pos.x() - leftEdgePixel) <= mRESIZE_THRESHOLD ||
        qAbs(pos.x() - rightEdgePixel) <= mRESIZE_THRESHOLD) {
        mCustomPlot->setCursor(Qt::SizeHorCursor);
    }
    // 检查鼠标是否在矩形内部
    else if (pos.x() > mCustomPlot->xAxis->coordToPixel(rectLeft) &&
        pos.x() < mCustomPlot->xAxis->coordToPixel(rectRight)) {
        mCustomPlot->setCursor(Qt::SizeAllCursor);
    }
    // 默认光标
    else {
        mCustomPlot->setCursor(Qt::ArrowCursor);
    }
}

void CyMediaDisGrayStretch::onPlotRangeChange(const QCPRange& range) {
    // 保持选择矩形覆盖整个Y轴范围
    onUpSelectRectRange(mStretchValue.start, mStretchValue.end);
    mCustomPlot->replot();
}

void CyMediaDisGrayStretch::initGUI() {
    if (mIsInit)
        return;
    mIsInit = true;

    mStretchValue = { 0, 255, 255 };

    //plot
    mCustomPlot = new QCustomPlot(this);
    mCustomPlot->installEventFilter(this);
    //select
    mSelectionRect = new QCPItemRect(mCustomPlot);
    mSelectionRect->topLeft->setType(QCPItemPosition::ptPlotCoords);
    mSelectionRect->bottomRight->setType(QCPItemPosition::ptPlotCoords);

    // 设置初始选择区域 (50-150)
    onUpSelectRectRange(mStretchValue.start, mStretchValue.end);

    //histogram
    mHistogram = new CyHistogram(mCustomPlot->xAxis, mCustomPlot->yAxis);
    mHistogram->setPen(QColor(Qt::lightGray));
    mHistogram->setWidthType(QCPBars::wtPlotCoords);

    //xAxis
    auto xAxis = mCustomPlot->xAxis;
    xAxis->setRange(-1, mXRange * 1.1);
    xAxis->grid()->setZeroLinePen(Qt::NoPen);
    xAxis->ticker()->setTickCount(11);
    xAxis->ticker()->setTickStepStrategy(QCPAxisTicker::tssReadability);
    //yAxis
    auto yAxis = mCustomPlot->yAxis;
    yAxis->grid()->setZeroLinePen(Qt::NoPen);
    yAxis->setTickLabels(false);
    yAxis->setRange(0, mYRange);

    //Ctroll
    mStartNumberBox = new QSpinBox(this);
    mStartNumberBox->setRange(0, mSelect_max);
    mStartNumberBox->setValue(mStretchValue.start);
    mEndNumberBox = new QSpinBox(this);
    mEndNumberBox->setRange(mSelect_min_width, mSelect_max);
    mEndNumberBox->setValue(mStretchValue.end);

    mControlTab = new QTabWidget(this);
    mControlTab->tabBar()->setVisible(false);

    mAutoStretchLab = new QLabel(this);
    mAutoStretchBtn = new CyCustomWidget::CyCheckButton(this);
    mAutoStretchBtn->setChecked(mAutostretch);

    mRGBStretchTypeLab = new QLabel(this);
    mRGBStretchTypeBox = new QComboBox(this);
    mRGBStretchTypeBox->addItem("");
    mRGBStretchTypeBox->addItem("");
    mRGBStretchTypeBox->addItem("");
    mRGBStretchTypeBox->setCurrentIndex(1);

    //layout
    QWidget* grayCtrW = new QWidget(this);
    QHBoxLayout* grayCtrLayout = new QHBoxLayout(grayCtrW);
    grayCtrLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    grayCtrLayout->addWidget(mAutoStretchLab);
    grayCtrLayout->addWidget(mAutoStretchBtn);
    grayCtrW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QWidget* rgbCtrW = new QWidget(this);
    QHBoxLayout* rgbCtrLyout = new QHBoxLayout(rgbCtrW);
    rgbCtrLyout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    rgbCtrLyout->addWidget(mRGBStretchTypeLab);
    rgbCtrLyout->addWidget(mRGBStretchTypeBox);
    rgbCtrW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mControlTab->clear();
    mControlTab->addTab(grayCtrW, "");
    mControlTab->addTab(rgbCtrW, "");

    QGridLayout* mainLayou = new QGridLayout(this);
    mainLayou->addWidget(mCustomPlot, 0, 0, 1, 3);
    mainLayou->addWidget(mStartNumberBox, 1, 0, 1, 1);
    mainLayou->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 1, 1, 1);
    mainLayou->addWidget(mEndNumberBox, 1, 2, 1, 1);
    mainLayou->addWidget(mControlTab, 2, 0, 1, 3);
    
    mStartNumberBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mEndNumberBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mControlTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mCustomPlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    flushTrans();
    setThemeColor(mThemeColor);

    //Connection
    connect(mCustomPlot->xAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged), this, &CyMediaDisGrayStretch::onPlotRangeChange);
    connect(mCustomPlot->yAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged), this, &CyMediaDisGrayStretch::onPlotRangeChange);

    connect(mStartNumberBox, &QSpinBox::editingFinished, this, &CyMediaDisGrayStretch::onStartValueEdit);
    connect(mEndNumberBox, &QSpinBox::editingFinished, this, &CyMediaDisGrayStretch::onEndValueEdit);
    connect(mAutoStretchBtn, &CyCustomWidget::CyCheckButton::stateChanged, this, &CyMediaDisGrayStretch::onAutoStretchChange);
    connect(mRGBStretchTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CyMediaDisGrayStretch::onStretchTypeChange);

    connect(this, &CyMediaDisGrayStretch::transImageType, this, &CyMediaDisGrayStretch::onTransImageType);
    connect(this, &CyMediaDisGrayStretch::upStretchRange, this, &CyMediaDisGrayStretch::onUpStretchRange);
    connect(this, &CyMediaDisGrayStretch::upHisRange, this, &CyMediaDisGrayStretch::onUpHisRange);
    connect(this, &CyMediaDisGrayStretch::upEditRange, this, &CyMediaDisGrayStretch::onUpBoxCahnge);
}

QString CyMediaDisGrayStretch::getPosToolTip(double xValue, double yValue) {
    QString reStr;
    switch (mStretchType) {
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
    mControlTab->setCurrentIndex(index);
}

void CyMediaDisGrayStretch::onUpStretchRange(int start, int end) {
    upSelectRectChange(start, end, false);
}

void CyMediaDisGrayStretch::onUpHisRange(int minX, int maxX, int maxY) {
    if (mXRange != maxX || 
        mXrangeMin != minX) {
        mXRange = maxX;
        mXrangeMin = minX;
        mCustomPlot->xAxis->setRange(mXrangeMin, mXRange);
    }

    float yChange = abs(mYRange - maxY) / mYRange;
    if (yChange > 0.2) {
        mYRange = maxY;
        mCustomPlot->yAxis->setRange(0, mYRange);
    }
}

void CyMediaDisGrayStretch::onUpBoxCahnge() {
    auto oldS = mStartNumberBox->value();
    auto oldE = mEndNumberBox->value();

    mStartNumberBox->setRange(0, mSelect_max - mSelect_min_width);
    mEndNumberBox->setRange(mSelect_min_width, mSelect_max);

    mStartNumberBox->setValue(oldS);
    mEndNumberBox->setValue(oldE);

    onStartValueEdit();
    onEndValueEdit();

    mStretchValue.max = mSelect_max;
}

