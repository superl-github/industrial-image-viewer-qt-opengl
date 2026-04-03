#include "CyMediaDisGrayTest.h"
#include "CyMediaDis.h"
#include "drawItem/CyDisDrawItem.h"

#include <QToolBar>
#include <QPushButton>
#include <QToolTip>
#include <QActionGroup>

CyMediaDisGrayTest::CyMediaDisGrayTest(QWidget* parent /*= nullptr*/)
    :QWidget(parent) {
    initGUI();

    connect(this, &CyMediaDisGrayTest::uphisVisible, this, &CyMediaDisGrayTest::onUphisVisible);
    connect(this, &CyMediaDisGrayTest::upTestData, this, &CyMediaDisGrayTest::onUpTestData);
    connect(this, &CyMediaDisGrayTest::upHisRange, this, &CyMediaDisGrayTest::onUpHisRange);
    connect(this, &CyMediaDisGrayTest::flushHis, this, [this]() {
        mCustomPlot->replot();
        });

    mZoomable = !mZoomable;
    setZoomble(!mZoomable);
}

CyMediaDisGrayTest::~CyMediaDisGrayTest() {

}

void CyMediaDisGrayTest::Itemdraw(CyDisDrawItem::BaseItem* item) {
    if (false == isVisible() ||
        false == isEnabled() || 
        mDrawType == CyDisDrawItem::ItemType::Invalid) {
        return;
    }

    mCurrentItemID = item->id();
    emit needImage();

    connect(item, &CyDisDrawItem::BaseItem::geometryChanged, this, 
        [this]() {
            emit needImage();
        });
}

bool CyMediaDisGrayTest::upImageData(CyMedia::ImageShowInfo& info, uint8_t* data) {
    //切换
    if (mIsGray != info.isMono() || 
        mIsPos != currentItemIsPos()) {
        mIsGray = info.isMono();
        mIsPos = currentItemIsPos();
        emit uphisVisible();
    }

    //计算直方图
    if (mIsPos != currentIsPosCalc) {
        mPosHisMaxY = 0.0;
        currentIsPosCalc = true;
        mPosHisData[hisI_Gray].init();
        mPosHisData[hisI_R].init();
        mPosHisData[hisI_G].init();
        mPosHisData[hisI_B].init();
        int initHisIndex = 0;
        for (int i = hisI_Gray; i <= hisI_B; i++) {
            if (mPosHistogramData[i].size() != mPosHisMax) {
                mPosHistogramData[i].resize(mPosHisMax);
                initHisIndex++;
            }
        }
        if (initHisIndex) {
            for (int posI = 0; posI < mPosHisMax; posI++) {
                for (int hisI = hisI_Gray; hisI <= hisI_B; hisI++) {
                    mPosHistogramData[hisI][posI].key = posI;
                }
            }
        }
    }
    if (false == mIsPos) {
        currentIsPosCalc = false;
    }

    upMask({info.width, info.height});

    //计算直方图
    int32_t calcXRangePara = 1 << info.bit;

    double calcHisMinX = .0;
    double calcHisMaxX = .0;
    double calcHisMaxY = 0.0;

    if (info.isMono()) {
        if (mIsPos) {
            //计算像素点
            double temR, tempG, tempB;
            auto pos = getCurrentItem()->pos();
            CyMedia::calcCoordinateColor(info, data, pos.x(), pos.y(),
                &temR, &tempG, &tempB);
            //入队
            mPosHisData[hisI_Gray].addData(temR);
            mPosHisData[hisI_Gray].getLinearData(mHistogramData[hisI_Gray]);
            if (mPosHisMaxY <= temR) {
                mPosHisMaxY = temR;
            }
            mHisTestData.currentGray = temR;
            calcHisMaxY = mPosHisMaxY;
            calcXRangePara = mPosHisMax;
            //更新数据
            for (int i = 0; i < mHistogramData[hisI_Gray].size(); i++) {
                mPosHistogramData[hisI_Gray][i].value = mHistogramData[hisI_Gray][i];
            }
            mLineChart[hisI_Gray]->setGraphData(mPosHistogramData[hisI_Gray]);
        }
        else {
            CyMedia::computeGrayHistogram(data, info, &mClacMask, mMaskHaveData,
                mHistogramData[hisI_Gray],
                &mHisTestData.max, &mHisTestData.min, &mHisTestData.ave);
            //更新数据
            mHistogram[hisI_Gray]->updateHistogramFromThread(mHistogramData[hisI_Gray].data(), mHistogramData[hisI_Gray].size());

            std::sort(mHistogramData[hisI_Gray].begin(), mHistogramData[hisI_Gray].end());
            size_t idx97 = static_cast<size_t>(mHistogramData[hisI_Gray].size() * 0.97);
            calcHisMaxY = mHistogramData[hisI_Gray][idx97];
        }
        //计算参数
        CyMedia::computerUniformity(mHistogramData[hisI_Gray], mHisTestData.ave, mHisTestData.max,
            &mHisTestData.std, &mHisTestData.Uniformity);
    }
    else if (info.isBayer()) {
        return false;
    }
    else {
        if (mIsPos) {
            //计算像素点
            double tempRGB[4];
            auto pos = getCurrentItem()->pos();
            CyMedia::calcCoordinateColor(info, data, pos.x(), pos.y(),
                &tempRGB[hisI_R], &tempRGB[hisI_G], &tempRGB[hisI_B]);
            //入队
            for (int i = hisI_R; i <= hisI_B; i++) {
                mPosHisData[i].addData(tempRGB[i]);
                mPosHisData[i].getLinearData(mHistogramData[i]);
            }
            //更新数据
            for (int posI = 0; posI < mHistogramData[hisI_R].size(); posI++) {
                for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
                    mPosHistogramData[hisI][posI].value = mHistogramData[hisI][posI];
                }
            }
            for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
                mLineChart[hisI]->setGraphData(mPosHistogramData[hisI]);
            }
            
            if (mPosHisMaxY <= tempRGB[hisI_R]) {
                mPosHisMaxY = tempRGB[hisI_R];
            }
            if (mPosHisMaxY <= tempRGB[hisI_G]) {
                mPosHisMaxY = tempRGB[hisI_G];
            }
            if (mPosHisMaxY <= tempRGB[hisI_B]) {
                mPosHisMaxY = tempRGB[hisI_B];
            }
            if (mRGBTestData.currentGray.size() != 3) {
                mRGBTestData.currentGray.resize(3);
            }
            mRGBTestData.currentGray[0] = tempRGB[hisI_R];
            mRGBTestData.currentGray[1] = tempRGB[hisI_G];
            mRGBTestData.currentGray[2] = tempRGB[hisI_B];
            calcHisMaxY = mPosHisMaxY;
            calcXRangePara = mPosHisMax;
        }
        else {
            CyMedia::computeRGBHistogram(data, info, &mClacMask, mMaskHaveData,
                mHistogramData[hisI_R], mHistogramData[hisI_G], mHistogramData[hisI_B],
                mRGBTestData.max, mRGBTestData.min, mRGBTestData.ave);
            //更新数据
            for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
                mHistogram[hisI]->updateHistogramFromThread(mHistogramData[hisI].data(), mHistogramData[hisI].size());
            }
        }
        //计算参数
        CyMedia::computerThreeUniformity(
            mHistogramData[hisI_R], mHistogramData[hisI_G], mHistogramData[hisI_B],
            mRGBTestData.ave, mRGBTestData.max,
            mRGBTestData.std, mRGBTestData.Uniformity);
        for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
            std::sort(mHistogramData[hisI].begin(), mHistogramData[hisI].end());
        }

        size_t idx97 = static_cast<size_t>(mHistogramData[hisI_R].size() * 0.97);
        calcHisMaxY = mHistogramData[hisI_R][idx97] > mHistogramData[hisI_G][idx97] ? std::max(mHistogramData[hisI_R][idx97], mHistogramData[hisI_B][idx97]) : std::max(mHistogramData[hisI_G][idx97], mHistogramData[hisI_B][idx97]);
    }

    calcHisMinX = -(calcXRangePara * 0.05);
    calcHisMaxX = calcXRangePara * 1.05;
    emit upHisRange(static_cast<int>(calcHisMinX), static_cast<int>(calcHisMaxX), static_cast<int>(calcHisMaxY));
    emit upTestData();
    emit flushHis();
    return true;
}

bool CyMediaDisGrayTest::isZoomble() {
    return mZoomable;
}

void CyMediaDisGrayTest::setZoomble(bool zoom) {
    if (mZoomable == zoom)
        return;
    if (mZoomable != zoom) {
        mZoomable = zoom;
        if (mZoomable) {
            mCustomPlot->setInteractions(
                QCP::iRangeDrag |
                QCP::iRangeZoom |
                QCP::iSelectAxes |
                QCP::iSelectLegend |
                QCP::iSelectPlottables); // 可拖动 / 缩放
        }
        else {
            mCustomPlot->setInteractions(QCP::iNone); // 可拖动 / 缩放
        }
    }
}

bool CyMediaDisGrayTest::axisToolTipVisible() {
    return mShowPlotTips;
}

void CyMediaDisGrayTest::setAxisToolTipVisible(bool visi) {
    mShowPlotTips = visi;
    if (!visi)
        QToolTip::hideText();
}

bool CyMediaDisGrayTest::eventFilter(QObject* watched, QEvent* event) {
    if (watched == mCustomPlot && event->type() == QEvent::MouseMove) {
        plotMouseMove(static_cast<QMouseEvent*>(event)->pos());
    }

    return QWidget::eventFilter(watched, event);
}

void CyMediaDisGrayTest::closeEvent(QCloseEvent* event) {
    QWidget::closeEvent(event);
    if (parent()) {
        hide();
        emit testModeChange(int(CyDisDrawItem::ItemType::Invalid));
    }
    else {
        close();
    }
}

void CyMediaDisGrayTest::showEvent(QShowEvent* event) {
    mfirstShow = true;
    emit testModeChange(mDrawType);
    emit needImage();
    QWidget::showEvent(event);
}

void CyMediaDisGrayTest::plotMouseMove(QPoint mousePos) {
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

bool CyMediaDisGrayTest::plotToolTips(double xValue, double yValue) {
    double calcXvalue = 0;
    double calcYvalue = 0.0;
    //X像素比
    double dRatioX = mCustomPlot->xAxis->axisRect()->width() / (mCustomPlot->xAxis->range().upper - mCustomPlot->xAxis->range().lower);
    CyHistogram* mainBar = nullptr;
    CyLineChart* mainLinChart = nullptr;
    for (int i = hisI_Gray; i < hisI_B; i++) {
        if (mHistogram[i]->visible()) {
            mainBar = mHistogram[i];
            break;
        }
    }
    if (!mainBar) {
        for (int i = hisI_Gray; i < hisI_B; i++) {
            if (mLineChart[i]->visible()) {
                mainLinChart = mLineChart[i];
                break;
            }
        }
    }
    QString tooltipStr;
    static double adjugeWidth = 20.0;
    if (mainBar || mainLinChart) {
        //直方图
        if (mainBar) {
            //取得最接近的坐标
            int index_left = mainBar->findBegin(xValue, true);
            int index_right = mainBar->findEnd(xValue, true);
            double dif_left = fabs(mainBar->data()->at(index_left)->key - xValue);
            double dif_right = fabs(mainBar->data()->at(index_right)->key - xValue);
            int iPointIdx = ((dif_left < dif_right) ? index_left : index_right);
            calcXvalue = mainBar->data()->at(iPointIdx)->key;
            //鼠标值和最近值的差值
            double dx = fabs(calcXvalue - xValue);
            if (dx * dRatioX <= adjugeWidth) {
                /*if (mLastPlotTopX == calcXvalue) {
                    return true;
                }*/
                mLastPlotTopX = calcXvalue;
                if (mHistogram[hisI_Gray]->visible()) {
                    calcYvalue = mHistogram[hisI_Gray]->data()->at(iPointIdx)->value;
                    tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_Gray)).arg(QString::number(calcYvalue));
                }
                else {
                    if (mHistogram[hisI_R]->visible()) {
                        calcYvalue = mHistogram[hisI_R]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_R)).arg(QString::number(calcYvalue));
                    }
                    if (mHistogram[hisI_G]->visible()) {
                        calcYvalue = mHistogram[hisI_G]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_G)).arg(QString::number(calcYvalue));
                    }
                    if (mHistogram[hisI_B]->visible()) {
                        calcYvalue = mHistogram[hisI_B]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_B)).arg(QString::number(calcYvalue));
                    }
                }
            }
        }
        //折线图
        else {
            //取得最接近的坐标
            int index_left = mainLinChart->findBegin(xValue, true);
            int index_right = mainLinChart->findEnd(xValue, true);
            double dif_left = fabs(mainLinChart->data()->at(index_left)->key - xValue);
            double dif_right = fabs(mainLinChart->data()->at(index_right)->key - xValue);
            int iPointIdx = ((dif_left < dif_right) ? index_left : index_right);
            calcXvalue = mainLinChart->data()->at(iPointIdx)->key;
            //鼠标值和最近值的差值
            double dx = fabs(calcXvalue - xValue);
            if (dx * dRatioX <= adjugeWidth) {
                /*if (mLastPlotTopX == calcXvalue) {
                    return true;
                }*/
                mLastPlotTopX = calcXvalue;
                if (mLineChart[hisI_Gray]->visible()) {
                    calcYvalue = mLineChart[hisI_Gray]->data()->at(iPointIdx)->value;
                    tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_Gray)).arg(QString::number(calcYvalue));
                }
                else {
                    if (mLineChart[hisI_R]->visible()) {
                        calcYvalue = mLineChart[hisI_R]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_R)).arg(QString::number(calcYvalue));
                    }
                    if (mLineChart[hisI_G]->visible()) {
                        calcYvalue = mLineChart[hisI_G]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_G)).arg(QString::number(calcYvalue));
                    }
                    if (mLineChart[hisI_B]->visible()) {
                        calcYvalue = mLineChart[hisI_B]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_B)).arg(QString::number(calcYvalue));
                    }
                }
            }
        }
    }
    if (tooltipStr.size() > 0) {
        tooltipStr.remove(tooltipStr.size() - 1, 1);
        tooltipStr = QString(u8"%1:%2\n").arg(getPosToolTip_XStr(mHistogram[hisI_Gray]->visible())).arg(calcXvalue) + tooltipStr;
        QToolTip::showText(cursor().pos(), tooltipStr, mCustomPlot);
        return true;
    }

    QToolTip::hideText();
    return false;
}

void CyMediaDisGrayTest::initGUI() {
    if (mIsInit)
        return;

    mIsInit = true;

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

    //plot
    mCustomPlot = new QCustomPlot(this);
    mCustomPlot->installEventFilter(this);
    mCustomPlot->xAxis->setRange(m_XRangeMin, m_XRangeMax);
    mCustomPlot->yAxis->setRange(0, m_YRangeMax);
    connect(mCustomPlot, &QCustomPlot::mousePress, this, &CyMediaDisGrayTest::plotPressEvent);
    connect(mCustomPlot, &QCustomPlot::mouseWheel, this, &CyMediaDisGrayTest::plotwheelEvent);

    //His
    mHisColor[hisI_Gray] = QColor(Qt::darkGray);
    mHisColor[hisI_R] = QColor(200, 30, 30, 255);
    mHisColor[hisI_G] = QColor(30, 200, 30, 200);
    mHisColor[hisI_B] = QColor(30, 30, 200, 150);

    for (int i = hisI_Gray; i <= hisI_B; i++) {
        mHistogram[i] = new CyHistogram(mCustomPlot->xAxis, mCustomPlot->yAxis);
        mHistogram[i]->setWidthType(QCPBars::wtPlotCoords);
        mHistogram[i]->setPen(mHisColor[i]);
        mHistogram[i]->setVisible(false);

        mLineChart[i] = new CyLineChart(mCustomPlot->xAxis, mCustomPlot->yAxis);
        mLineChart[i]->setLineStyle(QCPGraph::lsLine);
        mLineChart[i]->setScatterStyle(QCPScatterStyle::ssDot/*QCPScatterStyle::ssDisc*/);
        mLineChart[i]->setAdaptiveSampling(true);
        mLineChart[i]->setScatterSkip(0);
        mLineChart[i]->setPen(mHisColor[i]);
        mLineChart[i]->setVisible(false);
    }

    mCahnnelLabel = new QLabel(this);
    mCahnnelLabel->setAlignment(Qt::AlignHCenter);
    for (int i = 0; i < 5; i++) {
        if (i <= 3) {
            mChannelCtlLab[i] = new QCheckBox(this);
            mChannelCtlLab[i]->setChecked(true);
            connect(mChannelCtlLab[i], &QCheckBox::stateChanged, this, 
                [this](int state) {
                    onUphisVisible();
                });
        }
        mAveageLabel[i] = new QLabel(this);
        mAveageLabel[i]->setAlignment(Qt::AlignHCenter);

        mMaximumLabel[i] = new QLabel(this);
        mMaximumLabel[i]->setAlignment(Qt::AlignHCenter);

        mMinimumLabel[i] = new QLabel(this);
        mMinimumLabel[i]->setAlignment(Qt::AlignHCenter);

        mStdLabel[i] = new QLabel(this);
        mStdLabel[i]->setAlignment(Qt::AlignHCenter);

        mUniformityLabel[i] = new QLabel(this);
        mUniformityLabel[i]->setAlignment(Qt::AlignHCenter);
    }
    mChannelCtlLab[0]->setChecked(true);

    QToolBar* itemtypeToolbar = new QToolBar(this);
    itemtypeToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly); // 仅显示图标
    itemtypeToolbar->setIconSize(QSize(24, 24));

    mDrawBtnGroup = new QActionGroup(this);
    for (int i = CyDisDrawItem::ItemType::Invalid; i <= CyDisDrawItem::ItemType::Ellipse; i++) {
        mDrawAct[i] = new QAction(QIcon(), "", itemtypeToolbar);
        mDrawAct[i]->setData(QVariant(i));
        mDrawAct[i]->setCheckable(true);
        itemtypeToolbar->addAction(mDrawAct[i]);

        mDrawBtnGroup->addAction(mDrawAct[i]);
    }
    mDrawAct[CyDisDrawItem::ItemType::Invalid]->setChecked(true);
    mResetBtn = new QPushButton(QIcon(":/CyMediaDis/ICONS/ResetShaft.png"), "", this);
    connect(mResetBtn, &QPushButton::clicked, this, &CyMediaDisGrayTest::onResetShaft);

    QWidget* testInfoHeadWidget = new QWidget(this);
    QHBoxLayout* tastInfoTitleLayout = new QHBoxLayout(testInfoHeadWidget);
    tastInfoTitleLayout->addWidget(mCahnnelLabel);
    tastInfoTitleLayout->addWidget(mAveageLabel[4]);
    tastInfoTitleLayout->addWidget(mMaximumLabel[4]);
    tastInfoTitleLayout->addWidget(mMinimumLabel[4]);
    tastInfoTitleLayout->addWidget(mStdLabel[4]);
    tastInfoTitleLayout->addWidget(mUniformityLabel[4]);
    tastInfoTitleLayout->setContentsMargins(0, 0, 0, 0);
    tastInfoTitleLayout->setSpacing(2);
    testInfoHeadWidget->setStyleSheet(testLabHeadStyle);

    for (int i = hisI_Gray; i <= hisI_B; i++) {
        nTestInfoEditWidget[i] = new QWidget(this);
        QGridLayout* testInfoEditLayout = new QGridLayout(nTestInfoEditWidget[i]);
        testInfoEditLayout->setContentsMargins(0, 0, 0, 0);
        testInfoEditLayout->setSpacing(2);
        int columI = 0;
        testInfoEditLayout->addWidget(mChannelCtlLab[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(mAveageLabel[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(mMaximumLabel[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(mMinimumLabel[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(mStdLabel[i], 0, columI++, 1, 1);
        testInfoEditLayout->addWidget(mUniformityLabel[i], 0, columI++, 1, 1);
        nTestInfoEditWidget[i]->setStyleSheet(testLabInfoStyle);
    }

    QWidget* testInfoW = new QWidget(this);
    QVBoxLayout* testInfoLyout = new QVBoxLayout(testInfoW);
    testInfoLyout->addWidget(testInfoHeadWidget);
    for (int i = hisI_Gray; i <= hisI_B; i++) {
        testInfoLyout->addWidget(nTestInfoEditWidget[i]);
    }
    testInfoLyout->setContentsMargins(0, 0, 0, 0);
    testInfoLyout->setSpacing(2);
    testInfoW->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QWidget* tCtlW = new QWidget(this);
    QHBoxLayout* tCtrlLayout = new QHBoxLayout(tCtlW);
    tCtrlLayout->addWidget(itemtypeToolbar);
    tCtrlLayout->addWidget(mResetBtn);
    tCtrlLayout->setContentsMargins(0, 0, 0, 0);
    tCtlW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mResetBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QVBoxLayout* mainLyout = new QVBoxLayout(this);
    mainLyout->addWidget(mCustomPlot);
    mainLyout->addWidget(testInfoW);
    mainLyout->addWidget(tCtlW);
    mainLyout->setContentsMargins(0, 0, 0, 0);
    mainLyout->setSpacing(2);

    flushTrans();
    setThemeColor(m_ThemeColor);

    onUphisVisible();

    //connect
    connect(mChannelCtlLab[hisI_Gray], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onGrayCheckClicked);
    connect(mChannelCtlLab[hisI_R], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onRedCheckClicked);
    connect(mChannelCtlLab[hisI_G], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onGreenCheckClicked);
    connect(mChannelCtlLab[hisI_B], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onBlueCheckClicked);

    connect(mDrawBtnGroup, &QActionGroup::triggered, this, &CyMediaDisGrayTest::onDrawActTriggered);
}

void CyMediaDisGrayTest::plotPressEvent() {
    auto xSelectedParts = mCustomPlot->xAxis->selectedParts();
    auto ySelectedParts = mCustomPlot->yAxis->selectedParts();
    if (xSelectedParts.testFlag(QCPAxis::spAxis) || xSelectedParts.testFlag(QCPAxis::spTickLabels)) {
        mCustomPlot->axisRect()->setRangeDrag(mCustomPlot->xAxis->orientation());
    }
    else if (ySelectedParts.testFlag(QCPAxis::spAxis) || ySelectedParts.testFlag(QCPAxis::spTickLabels)) {
        mCustomPlot->axisRect()->setRangeDrag(mCustomPlot->yAxis->orientation());
    }
    else {
        mCustomPlot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    }
}

void CyMediaDisGrayTest::plotwheelEvent() {
    auto xSelectedParts = mCustomPlot->xAxis->selectedParts();
    auto ySelectedParts = mCustomPlot->yAxis->selectedParts();
    if (xSelectedParts.testFlag(QCPAxis::spAxis) || xSelectedParts.testFlag(QCPAxis::spTickLabels)) {
        mCustomPlot->axisRect()->setRangeZoom(mCustomPlot->xAxis->orientation());
    }
    else if (ySelectedParts.testFlag(QCPAxis::spAxis) || ySelectedParts.testFlag(QCPAxis::spTickLabels)) {
        mCustomPlot->axisRect()->setRangeZoom(mCustomPlot->yAxis->orientation());
    }
    else {
        mCustomPlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    }
}

void CyMediaDisGrayTest::onGrayCheckClicked(bool flag) {
    Q_UNUSED(flag);
    mChannelCtlLab[hisI_Gray]->setChecked(true);
    return;
    onUphisVisible();
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
    mCustomPlot->xAxis->setRange(m_XRangeMin, m_XRangeMax);
    mCustomPlot->yAxis->setRange(0, m_YRangeMax);
    emit flushHis();
}

void CyMediaDisGrayTest::onDrawActTriggered(QAction* act) {
    mDrawType = CyDisDrawItem::ItemType(act->data().toInt());
    emit testModeChange(int(mDrawType));
}

void CyMediaDisGrayTest::flushTrans() {
    mCahnnelLabel->setText(tr("Channel"));
    mAveageLabel[4]->setText(tr("Average"));
    mMaximumLabel[4]->setText(tr("Maximum"));
    mMinimumLabel[4]->setText(tr("Minimum"));
    mStdLabel[4]->setText(tr("std"));
    mUniformityLabel[4]->setText(tr("Uniformity"));

    mChannelCtlLab[0]->setText(tr("Gray"));
    mChannelCtlLab[1]->setText(tr("Red"));
    mChannelCtlLab[2]->setText(tr("Green"));
    mChannelCtlLab[3]->setText(tr("Blue"));

    mDrawAct[CyDisDrawItem::ItemType::Invalid]->setText(tr("None"));
    mDrawAct[CyDisDrawItem::ItemType::Point]->setText(tr("Point"));
    mDrawAct[CyDisDrawItem::ItemType::Rectangle]->setText(tr("Rectangle"));
    mDrawAct[CyDisDrawItem::ItemType::Line]->setText(tr("Line"));
    mDrawAct[CyDisDrawItem::ItemType::Ellipse]->setText(tr("Ellipse"));

    mResetBtn->setText(tr("Reset shaft"));
}

void CyMediaDisGrayTest::setThemeColor(QColor color) {
    mCustomPlot->xAxis->setSelectedBasePen(QPen(color, 2));
    mCustomPlot->xAxis->setSelectedTickPen(QPen(color, 2));
    mCustomPlot->xAxis->setSelectedSubTickPen(QPen(color, 1));
    mCustomPlot->xAxis->setSelectedTickLabelColor(color);

    mCustomPlot->yAxis->setSelectedBasePen(QPen(color, 2));
    mCustomPlot->yAxis->setSelectedTickPen(QPen(color, 2));
    mCustomPlot->yAxis->setSelectedSubTickPen(QPen(color, 1));
    mCustomPlot->yAxis->setSelectedTickLabelColor(color);

    mDrawAct[CyDisDrawItem::ItemType::Invalid]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Invalid, 32, color));
    mDrawAct[CyDisDrawItem::ItemType::Point]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Point, 32, color));
    mDrawAct[CyDisDrawItem::ItemType::Rectangle]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Rectangle, 32, color));
    mDrawAct[CyDisDrawItem::ItemType::Line]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Line, 32, color));
    mDrawAct[CyDisDrawItem::ItemType::Ellipse]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Ellipse, 32, color));
}

void CyMediaDisGrayTest::onUphisVisible() {
    //折线图
    if (mIsPos) {
        for (int i = hisI_Gray; i <= hisI_B; i++) {
            mHistogram[i]->setVisible(false);
        }
        if (mIsGray) {
            mLineChart[hisI_Gray]->setVisible(true);
            for (int i = hisI_Gray + 1; i <= hisI_B; i++) {
                mLineChart[i]->setVisible(false);
            }
        }
        else {
            for (int i = hisI_Gray + 1; i <= hisI_B; i++) {
                mLineChart[i]->setVisible(mChannelCtlLab[i]->isChecked());
            }
        }
    }
    else {
        for (int i = hisI_Gray; i <= hisI_B; i++) {
            mLineChart[i]->setVisible(false);
        }
        if (mIsGray) {
            mHistogram[hisI_Gray]->setVisible(true);
            for (int i = hisI_Gray + 1; i <= hisI_B; i++) {
                mHistogram[i]->setVisible(false);
            }
        }
        else {
            for (int i = hisI_Gray + 1; i <= hisI_B; i++) {
                mHistogram[i]->setVisible(mChannelCtlLab[i]->isChecked());
            }
        }
    }

    nTestInfoEditWidget[hisI_Gray]->setVisible(mIsGray);
    nTestInfoEditWidget[hisI_R]->setVisible(!mIsGray);
    nTestInfoEditWidget[hisI_G]->setVisible(!mIsGray);
    nTestInfoEditWidget[hisI_B]->setVisible(!mIsGray);

    mCustomPlot->replot();
}

void CyMediaDisGrayTest::onUpTestData() {
    bool isPoint = mIsPos;
    if (mIsGray) {
        if (isPoint) {
            mAveageLabel[hisI_Gray]->setText(QString("%1(%2)")
            .arg(QString::number(mHisTestData.ave, 'f', 3))
            .arg(QString::number(mHisTestData.currentGray)));
        }
        else {
            mAveageLabel[hisI_Gray]->setText(QString::number(mHisTestData.ave, 'f', 3));
        }
        mMaximumLabel[hisI_Gray]->setText(QString::number(mHisTestData.max, 'f', 3));
        mMinimumLabel[hisI_Gray]->setText(QString::number(mHisTestData.min, 'f', 3));
        mStdLabel[hisI_Gray]->setText(QString::number(mHisTestData.std, 'f', 3));
        mUniformityLabel[hisI_Gray]->setText(QString::number(mHisTestData.Uniformity, 'f', 3));
    }
    else{
        for (int i = hisI_R; i <= hisI_B; i++) {
            if (isPoint) {
                mAveageLabel[i]->setText(QString("%1(%2)")
                    .arg(QString::number(mRGBTestData.ave[i - 1], 'f', 3))
                    .arg(QString::number(mRGBTestData.currentGray[i - 1])));
            }
            else {
                mAveageLabel[i]->setText(QString::number(mRGBTestData.ave[i - 1], 'f', 3));
            }
            mMaximumLabel[i]->setText(QString::number(mRGBTestData.max[i - 1], 'f', 3));
            mMinimumLabel[i]->setText(QString::number(mRGBTestData.min[i - 1], 'f', 3));
            mStdLabel[i]->setText(QString::number(mRGBTestData.std[i - 1], 'f', 3));
            mUniformityLabel[i]->setText(QString::number(mRGBTestData.Uniformity[i - 1], 'f', 3));
        }
    }
}

void CyMediaDisGrayTest::onUpHisRange(int minX, int maxX, int maxY) {
    if (minX != m_XRangeMin || maxX != m_XRangeMax) {
        m_XRangeMin = minX;
        m_XRangeMax = maxX;
        mCustomPlot->xAxis->setRange(m_XRangeMin, m_XRangeMax);
    }

    float change = abs(m_YRangeMax - maxY) * 1.0 / m_YRangeMax;
    if (change > 0.20 || mfirstShow) {
        if (mfirstShow)
            mfirstShow = false;
        m_YRangeMax = maxY;
        mCustomPlot->yAxis->setRange(0, m_YRangeMax);
    }
}
 
QString CyMediaDisGrayTest::getPosToolTip_XStr(bool Mono) {
    if (false == Mono || 
        mIsPos) {
        return tr("Horizontal axis");
    }

    return tr("Grayscale");
}

QString CyMediaDisGrayTest::getPosToolTip_YStr(hisIndex color) {
    if (color == hisI_Gray) {
        if (mIsPos) {
            return tr("Grayscale");
        }
        return tr("Quantity");
    }
    
    switch (color) {
        case CyMediaDisGrayTest::hisI_R:
            return tr("Red");
        case CyMediaDisGrayTest::hisI_G:
            return tr("Green");
        case CyMediaDisGrayTest::hisI_B:
            return tr(u8"Blue");
    }

    return QString();
}

CyDisDrawItem::BaseItem* CyMediaDisGrayTest::getCurrentItem() {
    if (mCurrentItemID.isNull()) {
        return nullptr;
    }
    return ((CyMediaDis*)parent())->getItem(mCurrentItemID);
}

bool CyMediaDisGrayTest::currentItemIsPos() {
    auto item = getCurrentItem();
    if (!item)
        return false;
    return item->itemType() == CyDisDrawItem::Point;
}

void CyMediaDisGrayTest::upMask(QSize imgSize) {
    auto item = getCurrentItem();
    if (!item) {
        mMaskHaveData = false;
        return;
    }
    if (item->itemType() == CyDisDrawItem::Point) {
        mMaskHaveData = false;
    }

    CyDisDrawItem::pathToMask(item->pathInScene(), imgSize, mClacMask);

    mMaskHaveData = true;
}
