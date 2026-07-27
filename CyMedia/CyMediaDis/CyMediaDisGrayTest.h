/*
 * @file        CyMediaDisGrayTest.cpp/.h
 * @brief       图像灰度测试与直方图统计组件
 * @details     本文件实现了 CyMediaDisGrayTest 类，作为 CyMediaDis 显示模块的辅助工具。
 *              主要功能包括：
 *              - 支持灰度、RGB、Bayer、YUV 等多种图像格式的直方图计算与显示；
 *              - 提供单通道（灰度）或三通道（RGB）的统计信息：平均值、最大值、最小值、标准差、均匀性；
 *              - 支持通过绘制图形（点、矩形、线、椭圆、多边形）选择感兴趣区域（ROI），并实时更新统计结果；
 *              - 提供交互式图表，支持鼠标悬停提示、坐标轴缩放、通道切换及重置坐标轴；
 *              - 适配点选模式（Pos）下的时序数据折线图显示。
 * @note        依赖 QCustomPlot 及内部 CyMediaCalc 计算库。
 * 
 * @author LLF
 * @date   July 2026
 * @version 1.0
 */
#pragma once
#include "../CyMediaBaseDef.h"
#include "drawItem/BaseItem.h"

#include <QWidget>
#include <QTabWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QBoxLayout>
#include <QRadioButton>
#include <QAction>
#include <QCheckBox>

namespace CyMedia {
    class CyMediaDis;
}
class CyMediaDisGrayTest : public QWidget {
    Q_OBJECT

public:
    enum hisIndex {
        hisI_Gray = 0,
        hisI_R,
        hisI_G,
        hisI_B,
    };
    enum testModeChangeType {
        ModeChange_normal = 0,
        ModeChange_hide = 1,
        QPainterPath_show = 2,
    };
    struct oneChannelTestInfo {
        double ave = 0.0;
        double max = 0.0;
        double min = 0.0;
        double std = 0.0;
        double Uniformity = 0.0;

        int32_t currentGray = 0;
    };
    struct threeChannelTestInfo {
        std::vector<double> ave = std::vector<double>(3);
        std::vector<double> max = std::vector<double>(3);
        std::vector<double> min = std::vector<double>(3);
        std::vector<double> std = std::vector<double>(3);
        std::vector<double> Uniformity = std::vector<double>(3);

        std::vector<int32_t> currentGray = std::vector<int32_t>(3);

        ~threeChannelTestInfo() {
            ave.clear();
            ave.shrink_to_fit();

            max.clear();
            max.shrink_to_fit();

            min.clear();
            min.shrink_to_fit();

            std.clear();
            std.shrink_to_fit();

            Uniformity.clear();
            Uniformity.shrink_to_fit();
        }
    };

public:
    CyMediaDisGrayTest(QWidget* parent = nullptr);
    ~CyMediaDisGrayTest();
    void setParentDis(CyMedia::CyMediaDis* parentDis);

public:signals:
    void needImage();
    void testModeChange(int drawType, CyMediaDisGrayTest::testModeChangeType emitType);

private:signals:
    void uphisVisible();
    void upTestData();
    void upHisRange(int minX, int maxX, int maxY);
    void flushHis();

public:
    void flushTrans();
    void setThemeColor(QColor color);
    //testGraphics
    void Itemdraw(CyDisDrawItem::BaseItem* item);
    void ItemRemoved(QUuid id);
    CyDisDrawItem::BaseItem* getCurrentItem();
    QPainterPath oldItemPpath();


    //histogram
    bool upImage(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);
    bool currentTestDataIsGray();
    CyMediaDisGrayTest::oneChannelTestInfo& getGrayTestData();
    CyMediaDisGrayTest::threeChannelTestInfo& getRGBTestData();

    bool isZoomble();
    void setZoomble(bool zoom);

    bool axisToolTipVisible();
    void setAxisToolTipVisible(bool visi);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event)override;
    void showEvent(QShowEvent* event)override;

    void plotMouseMove(QPoint mousePos);
    bool plotToolTips(double xValue, double yValue);

private:
    struct calcImageHisFlag {
        double calcHisMaxY = 0.0;
        double calcXRangePara = 0.0;
    };
    void initGUI();

    void plotPressEvent();
    void plotwheelEvent();
    
    void onGrayCheckClicked(bool flag);
    void onRedCheckClicked(bool flag);
    void onGreenCheckClicked(bool flag);
    void onBlueCheckClicked(bool flag);

    void onResetShaft();

    void onDrawActTriggered(QAction* act);

    CyMediaDisGrayTest::calcImageHisFlag upImage_Mono(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);
    CyMediaDisGrayTest::calcImageHisFlag upImage_RGB(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);
    CyMediaDisGrayTest::calcImageHisFlag upImage_Bayer(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);
    CyMediaDisGrayTest::calcImageHisFlag upImage_YUV(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);

    void onUphisVisible();
    void onUpTestData();
    void onUpHisRange(int minX, int maxX, int maxY);

private:
    static const int mPosHisMax = 256;

    class PosHis {
    public:
        PosHis() = default;

        void init() {
            mHead = 0;
            mCount = 0;
        }

        void addData(double value) {
            mBuffer[mHead] = value;          // Overwrite oldest data (Eviction)
            mHead = (mHead + 1) % mPosHisMax;   // Move pointer (Loop)
            if (mCount < mPosHisMax) mCount++;  // Count during initial population
        }

        void getLinearData(std::vector<double>& output) {
            if (mCount == 0) {
                output.clear();
                return;
            }

            if (output.size() != mCount) {
                output.resize(mCount);
            }
            double* pOut = output.data();
            int start = (mHead - mCount + mPosHisMax) % mPosHisMax;

            if (start + mCount <= mPosHisMax) {
                std::copy(mBuffer + start, mBuffer + start + mCount, pOut);
            }
            else {
                std::copy(mBuffer + start, mBuffer + mPosHisMax, pOut);
                std::copy(mBuffer, mBuffer + (start + mCount - mPosHisMax), pOut + (mPosHisMax - start));
            }
        }

        int count() const { return mCount; };
    private:
        int mCount = 0;           // Current Volume of Valid Data
        double mBuffer[mPosHisMax];      // Single Array
        int mHead = 0;            // Points to the earliest data
    };

    QString getPosToolTip_XStr(bool Mono);
    QString getPosToolTip_YStr(CyMediaDisGrayTest::hisIndex color);

    bool currentItemIsPos();

    void upMask(QSize imgSize);

private:
    struct PrivateData;
    PrivateData* d = nullptr;
};