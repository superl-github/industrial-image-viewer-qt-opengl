#pragma once

#include <QtCore/qglobal.h>

# if defined(CYMEDIA_EXPORT)
#  define CYMEDIA_LIB Q_DECL_EXPORT
# else
#  define CYMEDIA_LIB Q_DECL_IMPORT
# endif

#include <QWidget>
#include <QPushButton>
#include <QLayout>
#include <QUrl>

#include "CyMediaBaseDef.h"
#include "CyMediaDis/CyMediaRecTimeW.h"
#include "CyMediaDis/CyMediaDisGrayStretch.h"
#include "CyMediaDis/CyMediaDisGrayTest.h"
#include "CyMediaDis/drawItem/CyDisDrawItem.h"

class CYMEDIA_LIB CyMediaDis : public QWidget {
    Q_OBJECT

public:
    CyMediaDis(QWidget* parent = nullptr);
    ~CyMediaDis();

public:signals:
    void urlsDrop(QList<QUrl> urls);
    void PressOnView();
    void DoubleClickOnView();

    void upPosPix(int32_t x, int32_t y, double r, double g, double b, bool signlR);
    void zoomValueChange(double value);
    void pressOnView();

public:
    static bool supportsOpenGL(int& mainV, int& subV);
    static bool supportsOpenGLForCyMedia();

    void setPrintLog(bool flag);
    void setLogCallback(CyMedia::LogCallback cb, void* pUser = nullptr);
    
    //image Process
    bool upImageData(CyMedia::ImageShowInfo info, uint8_t* data, bool force = false);
    bool haveDate(void);
    void clearImage(void);

    CyMedia::StretchType stretchType();
    void setStretchType(CyMedia::StretchType type);
    void setStreaChPara(uint32_t start = 0, uint32_t end = 0);

    CyMedia::DemosaicMethod Demosaic();
    void setDemosaic(CyMedia::DemosaicMethod method);

    QStringList ColorMapList() const;
    quint32 colorMapIndex() const;
    bool setColorMap(quint32 index);
    bool setColorMap(const QString& mapName);

    //Tools/UI
    void setDrawMode(CyDisDrawItem::ItemType mode);

    void setThemeColor(QColor color);

    CyDisDrawItem::BaseItem* getItem(QUuid& id);

    bool toolBarVisible(void);
    void setToolBarVisible(bool show);

    bool zoomScrollBarVisible(void);
    void setZoomScrollBarVisible(bool show);

    CyMediaRecTimeW* rectimeItem();

    CyMediaDisGrayStretch* stretchWidget();
    void setGrayStretchVisible(bool visible);

    CyMediaDisGrayTest* grayTestWidget();
    void setGrayTestVisible(bool visible);

private:
    class privateData; 
    privateData* d = nullptr;
};