#pragma once
#include "CyMediaBaseDef.h"
#include "CyMediaDis/CyMediaRecTimeW.h"
#include "CyMediaDis/CyMediaDisGrayStretch.h"
#include "CyMediaDis/CyMediaDisGrayTest.h"
#include "CyMediaDis/drawItem/CyDisDrawItem.h"

#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QLayout>
#include <QUrl>

namespace CyMedia {
    class CYMEDIA_LIB CyMediaDis : public QFrame {
        Q_OBJECT

    public:
        using CyMediaDisImageCallBack = std::function<void(CyMedia::ImageShowInfo&, uint8_t*, void*)>;

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

        void itemDrawed(CyDisDrawItem::BaseItem* item);

    public:
        static bool supportsOpenGL(int& mainV, int& subV);
        static bool supportsOpenGLForCyMedia();
        static QString pixelFormatStr(CyMedia::ePixType format);

        CyMedia::eLanguage currentLanguage();
        bool setLanguage(CyMedia::eLanguage lang);

        void setPrintLog(bool flag);
        void setLogCallback(CyMedia::LogCallback cb, void* pUser = nullptr);

        //Widget
        void setSceneAcceptDrop(bool accept);
    
        //image Process
        void setImageStackNum(uint32_t num);
        bool upImageData(CyMedia::ImageShowInfo info, uint8_t* data, bool force = false);
        void registerImageCallBack(CyMediaDisImageCallBack func, void*pUser);
        
        bool haveDate(void);
        void clearImage(void);

        CyMedia::ImageShowInfo& imageinfo();
        double displayFps(void);

        CyMedia::StretchType stretchType();
        void setStretchType(CyMedia::StretchType type);
        void setStreaChPara(uint32_t start = 0, uint32_t end = 0);

        CyMedia::DemosaicMethod Demosaic();
        void setDemosaic(CyMedia::DemosaicMethod method);

        QStringList ColorMapList() const;
        quint32 colorMapIndex() const;
        bool setColorMap(quint32 index);
        bool setColorMap(const QString& mapName);

        void zoomAuto();

        //Tools/UI
        void setDrawMode(CyDisDrawItem::ItemType mode);

        void setThemeColor(QColor color);

        bool toolBarVisible(void);
        void setToolBarVisible(bool show);

        bool zoomScrollBarVisible(void);
        void setZoomScrollBarVisible(bool show);

		// DrawItem
        QUuid addItem(CyDisDrawItem::ItemType itemType);
        QUuid addItem(CyDisDrawItem::ItemType itemType, QPainterPath path);
		CyDisDrawItem::BaseItem* getItem(QUuid& id);
        void clearItem();

        bool isSingleItemMode();
        void setSingleItemMode(bool flag);

        QUuid getLaseItem();

        bool isDrawing();

        //  RecTime
        bool recTimeVisible();
        void setRecTimeVisible(bool visi);

		void upRecTime(uint64_t time);
		void upRecTime(uint64_t saved, uint64_t sum);
		void upRecTime_Timed(uint64_t saved, uint64_t sum);

        //  GraStretch
        CyMediaDisGrayStretch* stretchWidget();
        void setGrayStretchVisible(bool visible);

        //  GrayTest
        CyMediaDisGrayTest* grayTestWidget();
        void setGrayTestVisible(bool visible);

    private:
        class privateData; 
        privateData* d = nullptr;
    };



	class CYMEDIA_LIB CyMediaDis_GetRawInfoDialog : public QDialog {
		Q_OBJECT
	public:
		explicit CyMediaDis_GetRawInfoDialog(CyMedia::eLanguage language = CyMedia::CHINESE, QWidget* parent = nullptr);

		QString openFileName();
		quint32 imageWidth();
		quint32 imageHeight();
		quint32 imagenBit();
		quint32 imageColorChannels();
        CyMedia::ePixType imagePixelType();
		quint32 imageOffset();

		void setLanguage(CyMedia::eLanguage language);
		void setOpenFileName(QString name);
		void setOpenInfo(quint32 w, quint32 h, quint32 bit, CyMedia::ePixType pixelType, quint32 ch, quint32 offset);

	protected:
		void initGUI(bool first = false);

	private slots:
		void onOkClicked();
		void onCanCelClicked();

	private:
		class PrivateData;
		PrivateData* p_data;
	};
};