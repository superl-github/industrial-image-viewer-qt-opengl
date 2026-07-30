#pragma once

#include "ui_CyMediaDisTest.h"
#include "CyMediaTestCommon.h"
#include "CyMediaDis.h"
#include "CyMediaParse/CyMediaImageParse.h"

#include <QtWidgets/QMainWindow>
#include <QThread>
#include <QElapsedTimer>
#include <QLabel>
#include <QDragEnterEvent>
#include <QmimeData>
#include <QSettings>

class CyMediaDisTest : public QMainWindow {
    Q_OBJECT

public:
    CyMediaDisTest(QWidget *parent = nullptr);
    ~CyMediaDisTest();

public:
    QString getAnalogImageTypeName(CyMediaTest::eAnalogImageType type);
    void genAnalogImage(CyMediaTest::eAnalogImageType type, QImage& img, int frameIdx);
    QString geyBayerMethodStr(CyMedia::DemosaicingMethod methord);
    QString geyYUVMethodStr(CyMedia::YUVTransMethod methord);

    void openFile(QString filePath);

private:
    void initGUI();
    void initStatus();
    void initMenu();
    void initContexMenu();
    void flushTranslate();
    void initcap();

    //Menu
    void on_act_file_open();
    void on_act_image_save();
    void on_act_video_rec_start();
    void on_act_video_rec_pause();
    void on_act_video_rec_stop();

    void on_act_acq_img_type(QAction* act);
    void on_act_acq_analog();
    void on_act_acq_stop();

    void on_act_view_colormap_type(QAction* act);
    void on_act_view_bayer_rebuild_type(QAction* act);
    void on_act_view_yuv_rebuild_type(QAction* act);
    void on_act_gray_stretch();

    void on_act_grayscale_measure();

    void on_act_about();

    void on_status_timerout();

    //replay
    void closeReplay();

private:
    void onFileOpen(QString filepath);
    void onViewUpPosPix(qint32 x, qint32 y, double r, double g, double b, bool signlR);
    void onImageSizeChanged(quint32 w, quint32 h, int bit);

private:
    void dragEnterEvent(QDragEnterEvent* event)override;
    void dropEvent(QDropEvent* event)override;
    void urlsDropOpe(QList<QUrl> urls);

private:
    void thread_acquisition();

private:
    QSettings* m_Setting;
    const QString m_app_name = QString("CyMediaTest");
    CyMedia::eLanguage m_language = CyMedia::ENGLISH;
    Ui::CyMediaDisTestClass ui;
    CyMedia::CyMediaDis* m_view = nullptr;
    CyMedia::CyMediaDis_GetRawInfoDialog* m_rawHeaderW = nullptr;
    CyMedia::CyMediaImageParse* m_image_func;

    //status
    QLabel* ui_ImageSizeLabel = nullptr;
    QLabel* ui_ImageSize_NameLabel = nullptr;
    QLabel* ui_PosColorLabel = nullptr;
    QLabel* ui_PosColor_NameLabel = nullptr;
    QLabel* ui_CapFpsLabel = nullptr;
    QLabel* ui_CapFps_NameLabel = nullptr;
    QLabel* ui_DisFpsLabel = nullptr;
    QLabel* ui_DisFps_NameLabel = nullptr;
    //Menu
    QMenuBar* ui_menu_bar = nullptr;
    //Menu/File
    QMenu* ui_menu_file = nullptr;
    QAction* ui_act_file_open = nullptr;
    QAction* ui_act_image_save = nullptr;
    QAction* ui_act_video_rec_start = nullptr;
    QAction* ui_act_video_rec_pause = nullptr;
    QAction* ui_act_video_rec_stot = nullptr;
    //Menu/capture
    QMenu* ui_menu_acq = nullptr;
    QMenu* ui_menu_acq_img_type = nullptr;
    QVector<QAction*> ui_act_acq_img_type_list;
    QActionGroup* ui_act_group_acq_img_type = nullptr;
    QAction* ui_act_acq_analog = nullptr;
    QAction* ui_act_acq_stop = nullptr;
    //Menu/view
    QMenu* ui_menu_view = nullptr;
    QMenu* ui_menu_colormaping = nullptr;
    QVector<QAction*> ui_act_view_colomap_list;
    QActionGroup* ui_act_group_view_colomap = nullptr;
    QMenu* ui_menu_bayer_rebuild = nullptr;
    QVector<QAction*> ui_act_view_bayer_rebuild_list;
    QActionGroup* ui_act_group_view_bayer_rebuild = nullptr;
    QMenu* ui_menu_yuv_rebuild = nullptr;
    QVector<QAction*> ui_act_view_yuv_rebuild_list;
    QActionGroup* ui_act_group_view_yuv_rebuild = nullptr;
    QAction* ui_grayscaleStretchAct = nullptr;
    //Menu/Tool
    QMenu* ui_menu_tool = nullptr;
    QAction* ui_grayscaleMeasurementAct = nullptr;
    //Menu/About
    QMenu* ui_menu_help = nullptr;
    QAction* ui_act_about = nullptr;

    //acquistion
    CyMediaTest::eAnalogImageType m_analog_img_type = CyMediaTest::AnalogImage_RandomColor;
    CyMedia::ImageShowInfo m_ImageInfo;
    bool m_bIsAcuistion = false;
    bool m_bStopView = false;
    QThread* m_AnalogAcquisitionThread = nullptr;
    double                      m_CapFps = 0.0;

    QThread* mUpImageThread = nullptr;
    bool mUpImageThreadFlag = false;

    QTimer* m_statusTimer = nullptr;
};

