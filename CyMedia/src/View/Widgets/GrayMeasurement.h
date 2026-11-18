/*
 * @file        GrayMeasurement.h
 * @brief       图像灰度测试与直方图统计组件
 * @details     本文件实现了 CyMediaDisGrayTest 类，作为 CyMedia 显示模块的辅助工具。
 *              主要功能包括：
 *              - 支持灰度、RGB、Bayer、YUV 等多种图像格式的直方图计算与显示；
 *              - 提供单通道（灰度）或三通道（RGB）的统计信息：平均值、最大值、最小值、标准差、均匀性；
 *              - 支持通过绘制图形（点、矩形、线、椭圆、多边形）选择感兴趣区域（ROI），并实时更新统计结果；
 *              - 直方图由自绘控件 CyHistogramView 渲染，支持柱形/折线/半透明轮廓三种风格，
 *                横轴"每格灰度级"、纵轴"每格计数"可在界面上精确设置；
 *              - 鼠标位置读数固定显示在绘图区左上角，不再使用跟随鼠标的 ToolTip；
 *              - 适配点选模式（Pos）下的时序数据折线图显示。
 * @note        依赖内部 CyMediaCalc 计算库与自绘直方图控件 CyHistogramView。
 *
 * @author LLF
 * @date   July 2026
 * @version 2.0
 */
#pragma once
#include "../../common/ImageTypes.h"
#include "../Annotation/BaseItem.h"

#include <QWidget>
#include <QTabWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QBoxLayout>
#include <QRadioButton>
#include <QAction>
#include <QCheckBox>
#include <vector>

namespace CyMedia {
    class CyMediaDis;
}
class CyHistogramView;

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

protected:
    void closeEvent(QCloseEvent* event)override;
    void showEvent(QShowEvent* event)override;

private:
    struct calcImageHisFlag {
        double calcHisMaxY = 0.0;
        double calcXRangePara = 0.0;
    };
    void initGUI();

    void onResetShaft();

    void onDrawActTriggered(QAction* act);

    void onGrayCheckClicked(bool flag);
    void onRedCheckClicked(bool flag);
    void onGreenCheckClicked(bool flag);
    void onBlueCheckClicked(bool flag);

    CyMediaDisGrayTest::calcImageHisFlag upImage_Mono(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);
    CyMediaDisGrayTest::calcImageHisFlag upImage_RGB(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);
    CyMediaDisGrayTest::calcImageHisFlag upImage_Bayer(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);
    CyMediaDisGrayTest::calcImageHisFlag upImage_YUV(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe formatOpe);

    void onUphisVisible();
    void onUpTestData();
    void onUpHisRange(int minX, int maxX, int maxY);

    /// 把控件当前的"每格"数值同步到编辑框（不触发信号）。
    void syncAxisControls();
    /// 按当前数据范围刷新编辑框的取值范围。
    void updateAxisRanges();
    /// 数据变化后推送到自绘控件（线程安全）。
    void publishSeries(int channel);

private:
    static const int mPosHisMax = 256;
    /// 直方图桶数上限：超过后 Calculate 会分配巨量内存，这里直接拒绝并提示。
    static const int mMaxHistogramBits = 20;

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

    bool currentItemIsPos();

    void upMask(QSize imgSize);

private:
    struct PrivateData;
    PrivateData* d = nullptr;
};
