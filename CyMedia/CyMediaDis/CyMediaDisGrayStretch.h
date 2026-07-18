#pragma once
#include "../CyMediaBaseDef.h"

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
    void upEditRange(bool bitChange = false);

public:
    void flushTrans();
    void setThemeColor(QColor color);

    //histogram
    bool upImageData(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::DemosaicingMethod Method);

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
private:
    void initGUI();
    QString getPosToolTip(double xValue, double yValue);

    void onTransImageType(int index);
    void onUpStretchRange(int start, int end);
    void onUpHisRange(int minX, int maxX, int maxY);
    void onUpEditRange(bool bitChange = false);

private:
    struct PrivateData;
    PrivateData* d = nullptr;
};