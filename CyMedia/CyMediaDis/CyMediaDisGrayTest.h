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
    CyMediaDisGrayTest(QWidget* parent = nullptr);
    ~CyMediaDisGrayTest();
    void setParentDis(CyMedia::CyMediaDis* parentDis);

public:signals:
    void needImage();
    void testModeChange(int drawType);

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


    //histogram
    bool upImageData(CyMedia::ImageShowInfo& info, uint8_t* data);

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
    void initGUI();

    void plotPressEvent();
    void plotwheelEvent();
    
    void onGrayCheckClicked(bool flag);
    void onRedCheckClicked(bool flag);
    void onGreenCheckClicked(bool flag);
    void onBlueCheckClicked(bool flag);

    void onResetShaft();

    void onDrawActTriggered(QAction* act);

    void onUphisVisible();
    void onUpTestData();
    void onUpHisRange(int minX, int maxX, int maxY);

private:
    static const int mPosHisMax = 256;

    enum hisIndex{
        hisI_Gray = 0,
        hisI_R,
        hisI_G,
        hisI_B,
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