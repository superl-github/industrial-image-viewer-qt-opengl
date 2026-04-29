#pragma once
#include "../CyMediaBaseDef.h"
#include "../CyMediaCalc/CyMediaCalc.h"
#include "Histogram/CyQCP.h"
#include "cycustomwidget.h"
#include <QWidget>
#include <QTabWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QBoxLayout>

class CyMediaDisGrayStretch : public QWidget {
    Q_OBJECT

public:
    struct StretchValue {
        int32_t start;
        int32_t end;
        int32_t max;
    };

public:
    CyMediaDisGrayStretch(QWidget* parent = nullptr);
    ~CyMediaDisGrayStretch();

public:signals:
    void needImage();
    void stretchParaChange();

private:signals:
    void transImageType(int index);
    void upStretchRange(int start, int end);
    void upHisRange(int minX, int maxX, int maxY);
    void upEditRange();

public:
    void flushTrans();
    void setThemeColor(QColor color);

    //histogram
    bool upImageData(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::DemosaicMethod Method);

    bool isZoomble();
    void setZoomble(bool zoom);

    bool axisToolTipVisible();
    void setAxisToolTipVisible(bool visi);

    //stretch
    CyMedia::StretchType stretchtype();

    StretchValue stretchValue();

    bool isAutoStretch();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event)override;
    void showEvent(QShowEvent* event)override;

    bool mouseOnSelectRect(QEvent* event);

    void plotMouseMove(QPoint mousePos);
    bool plotToolTips(double xValue, double yValue);

private:
    void onStartValueEdit();
    void onEndValueEdit();

    void onAutoStretchChange(bool enable);
    void onStretchTypeChange(int idx);

    void upSelectRectChange(int32_t start, int32_t end, bool needSignals = true);
    void onUpSelectRectRange(int32_t start, int32_t end);
    void onUpEditorNumber(int32_t  start, int32_t end);

    void updateCursor(const QPointF& pos);
    void onPlotRangeChange(const QCPRange& range);
private:
    void initGUI();
    QString getPosToolTip(double xValue, double yValue);

    void onTransImageType(int index);
    void onUpStretchRange(int start, int end);
    void onUpHisRange(int minX, int maxX, int maxY);
    void onUpEditRange();
    //UI
    QColor mThemeColor = QColor(0x2a, 0xa3, 0xc6);
    bool mIsInit = false;
    QCustomPlot* mCustomPlot = nullptr;
    CyHistogram* mHistogram  = nullptr;
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