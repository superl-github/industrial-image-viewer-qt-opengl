#include "CyMediaDisGrayTest.h"

#include "../CyMediaCalc/CyMediaCalc.h"
#include "cycustomwidget.h"
#include "Histogram/CyQCP.h"

#include "../CyMediaDis.h"
#include "drawItem/CyDisDrawItem.h"

#include <QThread>
#include <QToolBar>
#include <QPushButton>
#include <QToolTip>
#include <QActionGroup>

struct CyMediaDisGrayTest::PrivateData {
public:
	//UI
    CyMedia::CyMediaDis* m_parentDis = nullptr;
	QColor m_ThemeColor = QColor(0x2a, 0xa3, 0xc6);
	bool mIsInit = false;
	QCustomPlot* mCustomPlot = nullptr;
	CyHistogram* mHistogram[4] = { nullptr };
	CyLineChart* mLineChart[4] = { nullptr };
	QColor mHisColor[4];
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
	bool mfirstShow = true;
	std::vector<double> mHistogramData[4];
	QVector<QCPGraphData> mPosHistogramData[4];
	PosHis mPosHisData[4];
	double mPosHisMaxY = 0.0;
	bool mZoomable = true;

	int m_XRangeMax = 255;
	int m_XRangeMin = 0;
	int m_YRangeMax = 1;

	bool mShowPlotTips = true;
	QPoint mLastPlotTipPos;
	int mLastPlotTopX = -1;

	bool mIsGray = false;
	bool mIsPos = false;

	//test
	QUuid mCurrentItemID;
	CyDisDrawItem::ItemType mDrawType = CyDisDrawItem::ItemType::Invalid;

	std::vector<uint8_t> mClacMask;
	bool mMaskHaveData = false;
    bool mMaskIsfullzero = false;

	oneChannelTestInfo mHisTestData;
	threeChannelTestInfo mRGBTestData;
	bool currentIsPosCalc = false;
};

CyMediaDisGrayTest::CyMediaDisGrayTest(QWidget* parent /*= nullptr*/)
    :QWidget(parent) {
    d = new CyMediaDisGrayTest::PrivateData;
    initGUI();

    connect(this, &CyMediaDisGrayTest::uphisVisible, this, &CyMediaDisGrayTest::onUphisVisible);
    connect(this, &CyMediaDisGrayTest::upTestData, this, &CyMediaDisGrayTest::onUpTestData);
    connect(this, &CyMediaDisGrayTest::upHisRange, this, &CyMediaDisGrayTest::onUpHisRange);
    connect(this, &CyMediaDisGrayTest::flushHis, this, [this]() {
        d->mCustomPlot->replot();
        });

    d->mZoomable = !d->mZoomable;
    setZoomble(!d->mZoomable);
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

    d->mCurrentItemID = item->id();
    emit needImage();

    connect(item, &CyDisDrawItem::BaseItem::geometryChanged, this, 
        [this]() {
            emit needImage();
        });
}

void CyMediaDisGrayTest::ItemRemoved(QUuid id) {
    if (id != d->mCurrentItemID)
        return;
    if (QThread::currentThread() != this->thread()) {
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
    return d->m_parentDis->getItem(d->mCurrentItemID);
}

bool CyMediaDisGrayTest::upImageData(CyMedia::ImageShowInfo& info, uint8_t* data) {
    if (false == this->isVisible()) return false;

    if (false == d->m_parentDis->isSingleItemMode()) return false;
    if (d->m_parentDis->isDrawing() && !getCurrentItem()) return false;

    //切换
    if (d->mIsGray != info.isMono() ||
        d->mIsPos != currentItemIsPos()) {
        d->mIsGray = info.isMono();
        d->mIsPos = currentItemIsPos();
        emit uphisVisible();
    }

    //计算直方图
    if (d->mIsPos != d->currentIsPosCalc) {
        d->mPosHisMaxY = 0.0;
        d->currentIsPosCalc = true;
        d->mPosHisData[hisI_Gray].init();
        d->mPosHisData[hisI_R].init();
        d->mPosHisData[hisI_G].init();
        d->mPosHisData[hisI_B].init();
        int initHisIndex = 0;
        for (int i = hisI_Gray; i <= hisI_B; i++) {
            if (d->mPosHistogramData[i].size() != mPosHisMax) {
                d->mPosHistogramData[i].resize(mPosHisMax);
                initHisIndex++;
            }
        }
        if (initHisIndex) {
            for (int posI = 0; posI < mPosHisMax; posI++) {
                for (int hisI = hisI_Gray; hisI <= hisI_B; hisI++) {
                    d->mPosHistogramData[hisI][posI].key = posI;
                }
            }
        }
    }
    if (false == d->mIsPos) {
        d->currentIsPosCalc = false;
    }

    upMask({info.width, info.height});

    //计算直方图
    int32_t calcXRangePara = 1 << info.bit;

    double calcHisMinX = .0;
    double calcHisMaxX = .0;
    double calcHisMaxY = 0.0;

    if (info.isMono()) {
        if (d->mIsPos) {
            //计算像素点
            double temR, tempG, tempB;
            auto pos = getCurrentItem()->pos();
            CyMedia::calcCoordinateColor(info, data, pos.x(), pos.y(),
                &temR, &tempG, &tempB);
            //入队
            d->mPosHisData[hisI_Gray].addData(temR);
            d->mPosHisData[hisI_Gray].getLinearData(d->mHistogramData[hisI_Gray]);
            if (d->mPosHisMaxY <= temR) {
                d->mPosHisMaxY = temR;
            }
            d->mHisTestData.currentGray = temR;
            calcHisMaxY = d->mPosHisMaxY;
            calcXRangePara = mPosHisMax;
            //更新数据
            for (int i = 0; i < d->mHistogramData[hisI_Gray].size(); i++) {
                d->mPosHistogramData[hisI_Gray][i].value = d->mHistogramData[hisI_Gray][i];
            }
            d->mLineChart[hisI_Gray]->setGraphData(d->mPosHistogramData[hisI_Gray]);
        }
        else {
            if (d->mMaskIsfullzero) {
                d->mHistogramData[hisI_Gray].assign(d->mHistogramData[hisI_Gray].size(), 0.0);
            }
            else {
                CyMedia::computeGrayHistogram(data, info, &d->mClacMask, d->mMaskHaveData,
                    d->mHistogramData[hisI_Gray],
                    &d->mHisTestData.max, &d->mHisTestData.min, &d->mHisTestData.ave);
            }
            //更新数据
            d->mHistogram[hisI_Gray]->updateHistogramFromThread(d->mHistogramData[hisI_Gray].data(), d->mHistogramData[hisI_Gray].size());

            if (false == d->mMaskIsfullzero) {
                double maxY_threshold = 0.97;
                std::sort(d->mHistogramData[hisI_Gray].begin(), d->mHistogramData[hisI_Gray].end());
                size_t idx_maxY = static_cast<size_t>(d->mHistogramData[hisI_Gray].size() * maxY_threshold);
                calcHisMaxY = d->mHistogramData[hisI_Gray][idx_maxY];
				while (calcHisMaxY <= 0.0) {
					idx_maxY++;
					if (idx_maxY >= d->mHistogramData[hisI_Gray].size()) break;
                    calcHisMaxY = d->mHistogramData[hisI_Gray][idx_maxY];
				}
            }
        }
        //计算参数
        CyMedia::computerUniformity(d->mHistogramData[hisI_Gray], d->mHisTestData.ave, d->mHisTestData.max,
            &d->mHisTestData.std, &d->mHisTestData.Uniformity);
    }
    else if (info.isBayer()) {
        return false;
    }
    else {
        if (d->mIsPos) {
            //计算像素点
            double tempRGB[4];
            auto pos = getCurrentItem()->pos();
            CyMedia::calcCoordinateColor(info, data, pos.x(), pos.y(),
                &tempRGB[hisI_R], &tempRGB[hisI_G], &tempRGB[hisI_B]);
            //入队
            for (int i = hisI_R; i <= hisI_B; i++) {
                d->mPosHisData[i].addData(tempRGB[i]);
                d->mPosHisData[i].getLinearData(d->mHistogramData[i]);
            }
            //更新数据
            for (int posI = 0; posI < d->mHistogramData[hisI_R].size(); posI++) {
                for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
                    d->mPosHistogramData[hisI][posI].value = d->mHistogramData[hisI][posI];
                }
            }
            for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
                d->mLineChart[hisI]->setGraphData(d->mPosHistogramData[hisI]);
            }
            
            if (d->mPosHisMaxY <= tempRGB[hisI_R]) {
                d->mPosHisMaxY = tempRGB[hisI_R];
            }
            if (d->mPosHisMaxY <= tempRGB[hisI_G]) {
                d->mPosHisMaxY = tempRGB[hisI_G];
            }
            if (d->mPosHisMaxY <= tempRGB[hisI_B]) {
                d->mPosHisMaxY = tempRGB[hisI_B];
            }
            if (d->mRGBTestData.currentGray.size() != 3) {
                d->mRGBTestData.currentGray.resize(3);
            }
            d->mRGBTestData.currentGray[0] = tempRGB[hisI_R];
            d->mRGBTestData.currentGray[1] = tempRGB[hisI_G];
            d->mRGBTestData.currentGray[2] = tempRGB[hisI_B];
            calcHisMaxY = d->mPosHisMaxY;
            calcXRangePara = mPosHisMax;
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
                CyMedia::computeRGBHistogram(data, info, &d->mClacMask, d->mMaskHaveData,
                    d->mHistogramData[hisI_R], d->mHistogramData[hisI_G], d->mHistogramData[hisI_B],
                    d->mRGBTestData.max, d->mRGBTestData.min, d->mRGBTestData.ave);
            }
            //更新数据
            for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
                d->mHistogram[hisI]->updateHistogramFromThread(d->mHistogramData[hisI].data(), d->mHistogramData[hisI].size());
            }
        }

        if (false == d->mMaskIsfullzero) {
            //计算参数
            CyMedia::computerThreeUniformity(
                d->mHistogramData[hisI_R], d->mHistogramData[hisI_G], d->mHistogramData[hisI_B],
                d->mRGBTestData.ave, d->mRGBTestData.max,
                d->mRGBTestData.std, d->mRGBTestData.Uniformity);
            for (int hisI = hisI_R; hisI <= hisI_B; hisI++) {
                std::sort(d->mHistogramData[hisI].begin(), d->mHistogramData[hisI].end());
            }
            size_t idx97 = static_cast<size_t>(d->mHistogramData[hisI_R].size() * 0.97);
            calcHisMaxY = d->mHistogramData[hisI_R][idx97] > d->mHistogramData[hisI_G][idx97] ? std::max(d->mHistogramData[hisI_R][idx97], d->mHistogramData[hisI_B][idx97]) : std::max(d->mHistogramData[hisI_G][idx97], d->mHistogramData[hisI_B][idx97]);
        }
    }

    calcHisMinX = -(calcXRangePara * 0.05);
    calcHisMaxX = calcXRangePara * 1.05;
    emit upHisRange(static_cast<int>(calcHisMinX), static_cast<int>(calcHisMaxX), static_cast<int>(calcHisMaxY));
    emit upTestData();
    emit flushHis();
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

bool CyMediaDisGrayTest::isZoomble() {
    return d->mZoomable;
}

void CyMediaDisGrayTest::setZoomble(bool zoom) {
    if (d->mZoomable == zoom)
        return;
    if (d->mZoomable != zoom) {
        d->mZoomable = zoom;
        if (d->mZoomable) {
            d->mCustomPlot->setInteractions(
                QCP::iRangeDrag |
                QCP::iRangeZoom |
                QCP::iSelectAxes |
                QCP::iSelectLegend |
                QCP::iSelectPlottables); // 可拖动 / 缩放
        }
        else {
            d->mCustomPlot->setInteractions(QCP::iNone); // 可拖动 / 缩放
        }
    }
}

bool CyMediaDisGrayTest::axisToolTipVisible() {
    return d->mShowPlotTips;
}

void CyMediaDisGrayTest::setAxisToolTipVisible(bool visi) {
    d->mShowPlotTips = visi;
    if (!visi)
        QToolTip::hideText();
}

bool CyMediaDisGrayTest::eventFilter(QObject* watched, QEvent* event) {
    if (watched == d->mCustomPlot && event->type() == QEvent::MouseMove) {
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
    d->mfirstShow = true;
    if (!d->m_parentDis || true == d->m_parentDis->isSingleItemMode())
        emit testModeChange(d->mDrawType);
    emit needImage();
    QWidget::showEvent(event);
}

void CyMediaDisGrayTest::plotMouseMove(QPoint mousePos) {
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
    if (plotToolTips(dValueX, dValueY))
        d->mLastPlotTipPos = mousePos;
}

bool CyMediaDisGrayTest::plotToolTips(double xValue, double yValue) {
    double calcXvalue = 0;
    double calcYvalue = 0.0;
    //X像素比
    double dRatioX = d->mCustomPlot->xAxis->axisRect()->width() / (d->mCustomPlot->xAxis->range().upper - d->mCustomPlot->xAxis->range().lower);
    CyHistogram* mainBar = nullptr;
    CyLineChart* mainLinChart = nullptr;
    for (int i = hisI_Gray; i < hisI_B; i++) {
        if (d->mHistogram[i]->visible()) {
            mainBar = d->mHistogram[i];
            break;
        }
    }
    if (!mainBar) {
        for (int i = hisI_Gray; i < hisI_B; i++) {
            if (d->mLineChart[i]->visible()) {
                mainLinChart = d->mLineChart[i];
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
                d->mLastPlotTopX = calcXvalue;
                if (d->mHistogram[hisI_Gray]->visible()) {
                    calcYvalue = d->mHistogram[hisI_Gray]->data()->at(iPointIdx)->value;
                    tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_Gray)).arg(QString::number(calcYvalue));
                }
                else {
                    if (d->mHistogram[hisI_R]->visible()) {
                        calcYvalue = d->mHistogram[hisI_R]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_R)).arg(QString::number(calcYvalue));
                    }
                    if (d->mHistogram[hisI_G]->visible()) {
                        calcYvalue = d->mHistogram[hisI_G]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_G)).arg(QString::number(calcYvalue));
                    }
                    if (d->mHistogram[hisI_B]->visible()) {
                        calcYvalue = d->mHistogram[hisI_B]->data()->at(iPointIdx)->value;
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
                d->mLastPlotTopX = calcXvalue;
                if (d->mLineChart[hisI_Gray]->visible()) {
                    calcYvalue = d->mLineChart[hisI_Gray]->data()->at(iPointIdx)->value;
                    tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_Gray)).arg(QString::number(calcYvalue));
                }
                else {
                    if (d->mLineChart[hisI_R]->visible()) {
                        calcYvalue = d->mLineChart[hisI_R]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_R)).arg(QString::number(calcYvalue));
                    }
                    if (d->mLineChart[hisI_G]->visible()) {
                        calcYvalue = d->mLineChart[hisI_G]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_G)).arg(QString::number(calcYvalue));
                    }
                    if (d->mLineChart[hisI_B]->visible()) {
                        calcYvalue = d->mLineChart[hisI_B]->data()->at(iPointIdx)->value;
                        tooltipStr += QString(u8"%1:%2\n").arg(getPosToolTip_YStr(hisI_B)).arg(QString::number(calcYvalue));
                    }
                }
            }
        }
    }
    if (tooltipStr.size() > 0) {
        tooltipStr.remove(tooltipStr.size() - 1, 1);
        tooltipStr = QString(u8"%1:%2\n").arg(getPosToolTip_XStr(d->mHistogram[hisI_Gray]->visible())).arg(calcXvalue) + tooltipStr;
        QToolTip::showText(cursor().pos(), tooltipStr, d->mCustomPlot);
        return true;
    }

    QToolTip::hideText();
    return false;
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

    //plot
    d->mCustomPlot = new QCustomPlot(this);
    d->mCustomPlot->installEventFilter(this);
    d->mCustomPlot->xAxis->setRange(d->m_XRangeMin, d->m_XRangeMax);
    d->mCustomPlot->yAxis->setRange(0, d->m_YRangeMax);
    connect(d->mCustomPlot, &QCustomPlot::mousePress, this, &CyMediaDisGrayTest::plotPressEvent);
    connect(d->mCustomPlot, &QCustomPlot::mouseWheel, this, &CyMediaDisGrayTest::plotwheelEvent);

    //His
    d->mHisColor[hisI_Gray] = QColor(Qt::darkGray);
    d->mHisColor[hisI_R] = QColor(200, 30, 30, 255);
    d->mHisColor[hisI_G] = QColor(30, 200, 30, 200);
    d->mHisColor[hisI_B] = QColor(30, 30, 200, 150);

    for (int i = hisI_Gray; i <= hisI_B; i++) {
        d->mHistogram[i] = new CyHistogram(d->mCustomPlot->xAxis, d->mCustomPlot->yAxis);
        d->mHistogram[i]->setWidthType(QCPBars::wtPlotCoords);
        d->mHistogram[i]->setPen(d->mHisColor[i]);
        d->mHistogram[i]->setVisible(false);

        d->mLineChart[i] = new CyLineChart(d->mCustomPlot->xAxis, d->mCustomPlot->yAxis);
        d->mLineChart[i]->setLineStyle(QCPGraph::lsLine);
        d->mLineChart[i]->setScatterStyle(QCPScatterStyle::ssDot/*QCPScatterStyle::ssDisc*/);
        d->mLineChart[i]->setAdaptiveSampling(true);
        d->mLineChart[i]->setScatterSkip(0);
        d->mLineChart[i]->setPen(d->mHisColor[i]);
        d->mLineChart[i]->setVisible(false);
    }

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

    QWidget* tCtlW = new QWidget(this);
    QHBoxLayout* tCtrlLayout = new QHBoxLayout(tCtlW);
    tCtrlLayout->addWidget(itemtypeToolbar);
    tCtrlLayout->addWidget(d->mResetBtn);
    tCtrlLayout->setContentsMargins(0, 0, 0, 0);
    tCtlW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->mResetBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QVBoxLayout* mainLyout = new QVBoxLayout(this);
    mainLyout->addWidget(d->mCustomPlot);
    mainLyout->addWidget(testInfoW);
    mainLyout->addWidget(tCtlW);
    mainLyout->setContentsMargins(0, 0, 0, 0);
    mainLyout->setSpacing(2);

    flushTrans();
    setThemeColor(d->m_ThemeColor);

    onUphisVisible();

    //connect
    connect(d->mChannelCtlLab[hisI_Gray], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onGrayCheckClicked);
    connect(d->mChannelCtlLab[hisI_R], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onRedCheckClicked);
    connect(d->mChannelCtlLab[hisI_G], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onGreenCheckClicked);
    connect(d->mChannelCtlLab[hisI_B], &QCheckBox::clicked, this, &CyMediaDisGrayTest::onBlueCheckClicked);

    connect(d->mDrawBtnGroup, &QActionGroup::triggered, this, &CyMediaDisGrayTest::onDrawActTriggered);
}

void CyMediaDisGrayTest::plotPressEvent() {
    auto xSelectedParts = d->mCustomPlot->xAxis->selectedParts();
    auto ySelectedParts = d->mCustomPlot->yAxis->selectedParts();
    if (xSelectedParts.testFlag(QCPAxis::spAxis) || xSelectedParts.testFlag(QCPAxis::spTickLabels)) {
        d->mCustomPlot->axisRect()->setRangeDrag(d->mCustomPlot->xAxis->orientation());
    }
    else if (ySelectedParts.testFlag(QCPAxis::spAxis) || ySelectedParts.testFlag(QCPAxis::spTickLabels)) {
        d->mCustomPlot->axisRect()->setRangeDrag(d->mCustomPlot->yAxis->orientation());
    }
    else {
        d->mCustomPlot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    }
}

void CyMediaDisGrayTest::plotwheelEvent() {
    auto xSelectedParts = d->mCustomPlot->xAxis->selectedParts();
    auto ySelectedParts = d->mCustomPlot->yAxis->selectedParts();
    if (xSelectedParts.testFlag(QCPAxis::spAxis) || xSelectedParts.testFlag(QCPAxis::spTickLabels)) {
        d->mCustomPlot->axisRect()->setRangeZoom(d->mCustomPlot->xAxis->orientation());
    }
    else if (ySelectedParts.testFlag(QCPAxis::spAxis) || ySelectedParts.testFlag(QCPAxis::spTickLabels)) {
        d->mCustomPlot->axisRect()->setRangeZoom(d->mCustomPlot->yAxis->orientation());
    }
    else {
        d->mCustomPlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    }
}

void CyMediaDisGrayTest::onGrayCheckClicked(bool flag) {
    Q_UNUSED(flag);
    d->mChannelCtlLab[hisI_Gray]->setChecked(true);
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
    d->mCustomPlot->xAxis->setRange(d->m_XRangeMin, d->m_XRangeMax);
    d->mCustomPlot->yAxis->setRange(0, d->m_YRangeMax);
    emit flushHis();
}

void CyMediaDisGrayTest::onDrawActTriggered(QAction* act) {
    d->mDrawType = CyDisDrawItem::ItemType(act->data().toInt());
    if (!d->m_parentDis || true == d->m_parentDis->isSingleItemMode())
        emit testModeChange(int(d->mDrawType));
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
}

void CyMediaDisGrayTest::setThemeColor(QColor color) {
    d->mCustomPlot->xAxis->setSelectedBasePen(QPen(color, 2));
    d->mCustomPlot->xAxis->setSelectedTickPen(QPen(color, 2));
    d->mCustomPlot->xAxis->setSelectedSubTickPen(QPen(color, 1));
    d->mCustomPlot->xAxis->setSelectedTickLabelColor(color);

    d->mCustomPlot->yAxis->setSelectedBasePen(QPen(color, 2));
    d->mCustomPlot->yAxis->setSelectedTickPen(QPen(color, 2));
    d->mCustomPlot->yAxis->setSelectedSubTickPen(QPen(color, 1));
    d->mCustomPlot->yAxis->setSelectedTickLabelColor(color);

    d->mDrawAct[CyDisDrawItem::ItemType::Invalid]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Invalid, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Point]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Point, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Rectangle]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Rectangle, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Line]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Line, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Ellipse]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Ellipse, 32, color));
    d->mDrawAct[CyDisDrawItem::ItemType::Polygon]->setIcon(CyDisDrawItem::drawItemIcon(CyDisDrawItem::Polygon, 32, color));
}

void CyMediaDisGrayTest::onUphisVisible() {
    //折线图
    if (d->mIsPos) {
        for (int i = hisI_Gray; i <= hisI_B; i++) {
            d->mHistogram[i]->setVisible(false);
        }
        if (d->mIsGray) {
            d->mLineChart[hisI_Gray]->setVisible(true);
            for (int i = hisI_Gray + 1; i <= hisI_B; i++) {
                d->mLineChart[i]->setVisible(false);
            }
        }
        else {
            for (int i = hisI_Gray + 1; i <= hisI_B; i++) {
                d->mLineChart[i]->setVisible(d->mChannelCtlLab[i]->isChecked());
            }
        }
    }
    else {
        for (int i = hisI_Gray; i <= hisI_B; i++) {
            d->mLineChart[i]->setVisible(false);
        }
        if (d->mIsGray) {
            d->mHistogram[hisI_Gray]->setVisible(true);
            for (int i = hisI_Gray + 1; i <= hisI_B; i++) {
                d->mHistogram[i]->setVisible(false);
            }
        }
        else {
            for (int i = hisI_Gray + 1; i <= hisI_B; i++) {
                d->mHistogram[i]->setVisible(d->mChannelCtlLab[i]->isChecked());
            }
        }
    }

    d->nTestInfoEditWidget[hisI_Gray]->setVisible(d->mIsGray);
    d->nTestInfoEditWidget[hisI_R]->setVisible(!d->mIsGray);
    d->nTestInfoEditWidget[hisI_G]->setVisible(!d->mIsGray);
    d->nTestInfoEditWidget[hisI_B]->setVisible(!d->mIsGray);

    d->mCustomPlot->replot();
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
    if (minX != d->m_XRangeMin || maxX != d->m_XRangeMax) {
        d->m_XRangeMin = minX;
        d->m_XRangeMax = maxX;
        d->mCustomPlot->xAxis->setRange(d->m_XRangeMin, d->m_XRangeMax);
    }

    float change = abs(d->m_YRangeMax - maxY) * 1.0 / d->m_YRangeMax;
    if (change > 0.20 || d->mfirstShow) {
        if (d->mfirstShow)
            d->mfirstShow = false;
        d->m_YRangeMax = maxY;
        d->mCustomPlot->yAxis->setRange(0, d->m_YRangeMax);
    }
}
 
QString CyMediaDisGrayTest::getPosToolTip_XStr(bool Mono) {
    if (false == Mono || 
        d->mIsPos) {
        return tr("Horizontal axis");
    }

    return tr("Grayscale");
}

QString CyMediaDisGrayTest::getPosToolTip_YStr(hisIndex color) {
    if (color == hisI_Gray) {
        if (d->mIsPos) {
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
