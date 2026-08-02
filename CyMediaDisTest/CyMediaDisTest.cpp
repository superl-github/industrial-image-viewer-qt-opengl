#include "CyMediaDisTest.h"

#include <random>
#include <chrono>
#include <ppl.h>

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

CyMediaDisTest::CyMediaDisTest(QWidget* parent)
    : QMainWindow(parent) {
    ui.setupUi(this);
    this->setWindowIcon(QIcon(":/CyMediaDisTest/CyMedia.png"));
    setAcceptDrops(true);
    resize(1000, 800);
    //OpenGL检查
    int openglV_main, openglV_sub;
    bool suportOpenGl = CyMedia::CyMediaDis::supportsOpenGL(openglV_main, openglV_sub);
    if (openglV_main >= 3 && openglV_sub >= 3) {
        printf("OpenGL yes!!!\n");
    }

    m_Setting = new QSettings(QString("CyMediaDisTest.ini"), QSettings::IniFormat, this);
    initGUI();
    flushTranslate();
    initcap();

    //QTimer::singleShot(300, this, [this] {
    //    QFile imgFile("G:\\image\\industrial_camera\\Raw\\IMG_3423_RGGB_5184_3456_16.raw");
    //    CyMedia::ImageShowInfo imgInfo;
    //    imgInfo.width = 5184;
    //    imgInfo.height = 3456;
    //    imgInfo.bit = 16;
    //    //imgInfo.format = CyMedia::BAYERRG;
    //    imgInfo.format = CyMedia::MONO;
    //    imgInfo.upLenth();
    //    if (imgFile.open(QIODevice::ReadOnly)) {
    //        auto readCode = imgFile.readAll();
    //        imgFile.close();
    //        m_view->upImageData(imgInfo, (uint8_t*)readCode.data());
    //    }
    //    });
}

CyMediaDisTest::~CyMediaDisTest() {
    if (m_AnalogAcquisitionThread && m_AnalogAcquisitionThread->isRunning()) {
        m_bIsAcuistion = false;
        m_AnalogAcquisitionThread->wait();
        m_AnalogAcquisitionThread->terminate();
    }
}

QString CyMediaDisTest::getAnalogImageTypeName(CyMediaTest::eAnalogImageType type) {
    switch (type) {
    case CyMediaTest::AnalogImage_RandomColor: return tr("Random Color");
    case CyMediaTest::AnalogImage_CheckerBoard: return tr("Checker Board");
    case CyMediaTest::AnalogImage_MovingStripes: return tr("Moving Stripes");
    case CyMediaTest::AnalogImage_Plasma: return tr("Plasma");
    }
    return QString("");
}

void CyMediaDisTest::genAnalogImage(CyMediaTest::eAnalogImageType type, QImage& img, int frameIdx) {
    uint8_t* pData = img.bits();
    int width = img.width();
    int height = img.height();

    switch (type) {
        // ---------- 随机彩色 ----------
    case CyMediaTest::AnalogImage_RandomColor: {
        static std::random_device rd;
        static std::default_random_engine eng(rd());
        std::uniform_int_distribution<int> distr(0, 255);
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                int idx = (h * width + w) * 3;
                pData[idx + 0] = distr(eng); // R
                pData[idx + 1] = distr(eng); // G
                pData[idx + 2] = distr(eng); // B
            }
        }
        break;
    }
                                             // ---------- 移动棋盘格 ----------
    case CyMediaTest::AnalogImage_CheckerBoard: {
        int tileSize = 40;
        // 让棋盘随帧索引移动
        int offsetX = (frameIdx % (tileSize * 2)) - tileSize;
        int offsetY = (frameIdx / 2) % (tileSize * 2); // 竖直移动稍慢
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                bool white = (((w + offsetX) / tileSize) + ((h + offsetY) / tileSize)) % 2 == 0;
                uint8_t val = white ? 255 : 0;
                int idx = (h * width + w) * 3;
                pData[idx + 0] = val;
                pData[idx + 1] = val;
                pData[idx + 2] = val;
            }
        }
        break;
    }

                                              // ---------- 动态正弦条纹（灰度） ----------
    case CyMediaTest::AnalogImage_MovingStripes: {
        double freq = 0.02;
        double phase = frameIdx * 0.1;
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                double val = sin((w + h) * freq + phase) * 0.5 + 0.5; // [0,1]
                uint8_t gray = static_cast<uint8_t>(val * 255);
                int idx = (h * width + w) * 3;
                pData[idx + 0] = gray;
                pData[idx + 1] = gray;
                pData[idx + 2] = gray;
            }
        }
    }break;

                                               // ---------- 等离子分形效果 ----------
    case CyMediaTest::AnalogImage_Plasma: {
        double t = frameIdx * 0.05;
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                double u = (double)w / width;
                double v = (double)h / height;
                double val = sin(u * 10.0 + t) +
                    sin(v * 8.0 + t * 1.3) +
                    sin((u + v) * 6.0 + t * 0.7) +
                    sin(sqrt((u - 0.5) * (u - 0.5) + (v - 0.5) * (v - 0.5)) * 15.0 + t);
                val = (val + 4.0) / 8.0;
                uint8_t c = static_cast<uint8_t>(std::min(255.0, val * 255.0));
                int idx = (h * width + w) * 3;
                pData[idx + 0] = static_cast<uint8_t>(std::min(255.0, (sin(val * 3.14) + 1) * 127));
                pData[idx + 1] = static_cast<uint8_t>(std::min(255.0, (cos(val * 3.14) + 1) * 127));
                pData[idx + 2] = c;
            }
        }
    }break;
    }
}

QString CyMediaDisTest::geyBayerMethodStr(CyMedia::DemosaicingMethod methord) {
    switch (methord) {
    case CyMedia::DEMOSAIC_NONE:return tr("Source");
    case CyMedia::DEMOSAIC_BILINEAR:return tr("Bilinear");
    case CyMedia::DEMOSAIC_MALVA:return tr("Malva");
    case CyMedia::DEMOSAIC_AHD:return tr("AHD");
    }

    return QString("unknown");
}

QString CyMediaDisTest::geyYUVMethodStr(CyMedia::YUVTransMethod methord) {
    switch (methord) {
    case CyMedia::YUVTRANS_NORMAL:return tr("rgb");
    case CyMedia::YUVTRANS_Y:return tr("Only Y");
    }

    return QString("unknown");
}

void CyMediaDisTest::openFile(QString filePath) {
    urlsDropOpe(QList<QUrl>{QUrl::fromUserInput(filePath)});
}

void CyMediaDisTest::initGUI() {
    m_view = new CyMedia::CyMediaDis(this);
    m_view->setAcceptDrops(acceptDrops());
    m_view->setSceneAcceptDrop(acceptDrops());
    m_view->setDirectUpImage(true);
    connect(m_view, &CyMedia::CyMediaDis::upPosPix, this, &CyMediaDisTest::onViewUpPosPix);
    connect(m_view, &CyMedia::CyMediaDis::imageSizeChanged, this, &CyMediaDisTest::onImageSizeChanged);
    connect(m_view, &CyMedia::CyMediaDis::urlsDrop, this, &CyMediaDisTest::urlsDropOpe);

    m_view->setYUVMethod(CyMedia::YUVTRANS_Y);

    m_rawHeaderW = new CyMedia::CyMediaDis_GetRawInfoDialog(this);
    m_image_func = new CyMedia::CyMediaImageParse();

    QVBoxLayout* mainLyout = new QVBoxLayout(ui.centralWidget);
    mainLyout->setContentsMargins(0, 0, 0, 0);
    mainLyout->setSpacing(0);
    mainLyout->addWidget(m_view);

    initStatus();
    initMenu();
    initContexMenu();
    initMenu();

    ui_act_acq_stop->setEnabled(false);
}

void CyMediaDisTest::initStatus() {
    ui_ImageSizeLabel = new QLabel(this);
    ui_ImageSize_NameLabel = new QLabel(this);
    ui_PosColorLabel = new QLabel(this);
    ui_PosColor_NameLabel = new QLabel(this);
    ui_CapFpsLabel = new QLabel(this);
    ui_CapFps_NameLabel = new QLabel(this);
    ui_DisFpsLabel = new QLabel(this);
    ui_DisFps_NameLabel = new QLabel(this);

    ui.statusBar->addPermanentWidget(ui_ImageSize_NameLabel);
    ui.statusBar->addPermanentWidget(ui_ImageSizeLabel);
    ui.statusBar->addPermanentWidget(ui_PosColor_NameLabel);
    ui.statusBar->addPermanentWidget(ui_PosColorLabel);
    ui.statusBar->addPermanentWidget(ui_CapFps_NameLabel);
    ui.statusBar->addPermanentWidget(ui_CapFpsLabel);
    ui.statusBar->addPermanentWidget(ui_DisFps_NameLabel);
    ui.statusBar->addPermanentWidget(ui_DisFpsLabel);

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &CyMediaDisTest::on_status_timerout);
    m_statusTimer->start(1000);
}

void CyMediaDisTest::initMenu() {
    ui_menu_bar = this->menuBar();
    QAction* t_act = nullptr;
    //file
    ui_menu_file = new QMenu(this);

    ui_act_file_open = new QAction(ui_menu_file);
    ui_menu_file->addAction(ui_act_file_open);
    connect(ui_act_file_open, &QAction::triggered, this, &CyMediaDisTest::on_act_file_open);

    ui_act_image_save = new QAction(ui_menu_file);
    ui_menu_file->addAction(ui_act_image_save);
    connect(ui_act_image_save, &QAction::triggered, this, &CyMediaDisTest::on_act_image_save);

    ui_act_video_rec_start = new QAction(ui_menu_file);
    ui_menu_file->addAction(ui_act_video_rec_start);
    connect(ui_act_video_rec_start, &QAction::triggered, this, &CyMediaDisTest::on_act_video_rec_start);

    ui_act_video_rec_pause = new QAction(ui_menu_file);
    ui_menu_file->addAction(ui_act_video_rec_pause);
    connect(ui_act_video_rec_pause, &QAction::triggered, this, &CyMediaDisTest::on_act_video_rec_pause);

    ui_act_video_rec_stot = new QAction(ui_menu_file);
    ui_menu_file->addAction(ui_act_video_rec_stot);
    connect(ui_act_video_rec_stot, &QAction::triggered, this, &CyMediaDisTest::on_act_video_rec_stop);

    //capture
    ui_menu_acq = new QMenu(this);

    ui_menu_acq_img_type = new QMenu(ui_menu_acq);
    ui_act_group_acq_img_type = new QActionGroup(this);
    for (int i = CyMediaTest::AnalogImage_Begin + 1; i < CyMediaTest::AnalogImage_End; i++) {
        t_act = new QAction(ui_menu_acq_img_type);
        t_act->setCheckable(true);
        t_act->setData(i);
        ui_menu_acq_img_type->addAction(t_act);
        ui_act_group_acq_img_type->addAction(t_act);
        ui_act_acq_img_type_list.append(t_act);
        if (i == m_analog_img_type) {
            t_act->setChecked(true);
        }
    }
    ui_menu_acq->addMenu(ui_menu_acq_img_type);
    connect(ui_act_group_acq_img_type, &QActionGroup::triggered, this, &CyMediaDisTest::on_act_acq_img_type);

    ui_act_acq_analog = new QAction(ui_menu_acq);
    ui_menu_acq->addAction(ui_act_acq_analog);
    connect(ui_act_acq_analog, &QAction::triggered, this, &CyMediaDisTest::on_act_acq_analog);

    ui_act_acq_stop = new QAction(ui_menu_acq);
    ui_menu_acq->addAction(ui_act_acq_stop);
    connect(ui_act_acq_stop, &QAction::triggered, this, &CyMediaDisTest::on_act_acq_stop);

    //view
    ui_menu_view = new QMenu(this);

    ui_menu_colormaping = new QMenu(ui_menu_view);
    ui_act_group_view_colomap = new QActionGroup(this);
    auto colormaplist = m_view->ColorMapList();
    for (int i = 0; i < colormaplist.size(); i++) {
        t_act = new QAction(ui_menu_colormaping);
        t_act->setCheckable(true);
        t_act->setData(i);
        t_act->setText(colormaplist[i]);
        ui_act_view_colomap_list.append(t_act);
        ui_act_group_view_colomap->addAction(t_act);
        ui_menu_colormaping->addAction(t_act);
        if (i == m_view->colorMapIndex()) {
            t_act->setChecked(true);
        }
    }
    ui_menu_view->addMenu(ui_menu_colormaping);
    connect(ui_act_group_view_colomap, &QActionGroup::triggered, this, &CyMediaDisTest::on_act_view_colormap_type);

    ui_menu_bayer_rebuild = new QMenu(ui_menu_view);
    ui_act_group_view_bayer_rebuild = new QActionGroup(this);
    for (int i = CyMedia::DEMOSAIC_NONE; i <= CyMedia::DEMOSAIC_AHD; i++) {
        t_act = new QAction(ui_menu_bayer_rebuild);
        t_act->setCheckable(true);
        t_act->setData(i);
        ui_act_view_bayer_rebuild_list.append(t_act);
        ui_act_group_view_bayer_rebuild->addAction(t_act);
        ui_menu_bayer_rebuild->addAction(t_act);
        if (i == m_view->Demosaic()) {
            t_act->setChecked(true);
        }
    }
    ui_menu_view->addMenu(ui_menu_bayer_rebuild);
    connect(ui_act_group_view_bayer_rebuild, &QActionGroup::triggered, this, &CyMediaDisTest::on_act_view_bayer_rebuild_type);

    ui_menu_yuv_rebuild = new QMenu(ui_menu_view);
    ui_act_group_view_yuv_rebuild = new QActionGroup(this);
    for (int i = CyMedia::YUVTRANS_NORMAL; i <= CyMedia::YUVTRANS_Y; i++) {
        t_act = new QAction(ui_menu_yuv_rebuild);
        t_act->setCheckable(true);
        t_act->setData(i);
        ui_act_view_yuv_rebuild_list.append(t_act);
        ui_act_group_view_yuv_rebuild->addAction(t_act);
        ui_menu_yuv_rebuild->addAction(t_act);
        if (i == m_view->YUVMethod()) {
            t_act->setChecked(true);
        }
    }
    ui_menu_view->addMenu(ui_menu_yuv_rebuild);
    connect(ui_act_group_view_yuv_rebuild, &QActionGroup::triggered, this, &CyMediaDisTest::on_act_view_yuv_rebuild_type);

    ui_grayscaleStretchAct = new QAction(ui_menu_view);
    ui_menu_view->addAction(ui_grayscaleStretchAct);
    connect(ui_grayscaleStretchAct, &QAction::triggered, this, &CyMediaDisTest::on_act_gray_stretch);

    //Tool
    ui_menu_tool = new QMenu(this);

    ui_grayscaleMeasurementAct = new QAction(ui_menu_tool);
    ui_menu_tool->addAction(ui_grayscaleMeasurementAct);
    connect(ui_grayscaleMeasurementAct, &QAction::triggered, this, &CyMediaDisTest::on_act_grayscale_measure);

    //About
    ui_menu_help = new QMenu(this);

    ui_act_about = new QAction(this);
    ui_menu_help->addAction(ui_act_about);
    connect(ui_act_about, &QAction::triggered, this, &CyMediaDisTest::on_act_about);

    ui_menu_bar->addMenu(ui_menu_file);
    ui_menu_bar->addMenu(ui_menu_acq);
    ui_menu_bar->addMenu(ui_menu_view);
    ui_menu_bar->addMenu(ui_menu_tool);
    ui_menu_bar->addMenu(ui_menu_help);
}

void CyMediaDisTest::initContexMenu() {

}

void CyMediaDisTest::flushTranslate() {
    //status
    ui_ImageSize_NameLabel->setText(tr("Image size: "));
    ui_PosColor_NameLabel->setText(tr("Coordinate color: "));
    ui_CapFps_NameLabel->setText(tr("Acquisition fps: "));
    ui_DisFps_NameLabel->setText(tr("Display fps: "));

    //menu
    ui_menu_file->setTitle(tr("File"));
    ui_act_file_open->setText(tr("Open file"));
    ui_act_image_save->setText(tr("save frame"));
    ui_act_video_rec_start->setText(tr("Start recording"));
    ui_act_video_rec_pause->setText(tr("Pause recording"));
    ui_act_video_rec_stot->setText(tr("Stop recording"));

    ui_menu_acq->setTitle(tr("Capture"));
    ui_menu_acq_img_type->setTitle(tr("Capture image type"));
    for (auto oneact : ui_act_acq_img_type_list) {
        oneact->setText(getAnalogImageTypeName(CyMediaTest::eAnalogImageType(oneact->data().toUInt())));
    }
    ui_act_acq_analog->setText(tr("Analog capture"));
    ui_act_acq_stop->setText(tr("Stop analog capture"));

    ui_menu_view->setTitle(tr("Image"));
    ui_menu_colormaping->setTitle(tr("Color mapping"));
    ui_menu_bayer_rebuild->setTitle(tr("Demosaicing Method"));
    for (auto oneact : ui_act_view_bayer_rebuild_list) {
        oneact->setText(geyBayerMethodStr(CyMedia::DemosaicingMethod(oneact->data().toUInt())));
    }
    ui_menu_yuv_rebuild->setTitle(tr("YUV Method"));
    for (auto oneact : ui_act_view_yuv_rebuild_list) {
        oneact->setText(geyYUVMethodStr(CyMedia::YUVTransMethod(oneact->data().toUInt())));
    }
    ui_grayscaleStretchAct->setText(tr("Grayscale stretching"));

    ui_menu_tool->setTitle(tr("Tool"));
    ui_grayscaleMeasurementAct->setText(tr("Grayscale measurement"));

    ui_menu_help->setTitle(tr("Help"));
    ui_act_about->setText(tr("about ") + m_app_name);
}


void CyMediaDisTest::initcap() {
    m_AnalogAcquisitionThread = new QThread(this);
    connect(m_AnalogAcquisitionThread, &QThread::started, this, &CyMediaDisTest::thread_acquisition, Qt::DirectConnection);
}

void CyMediaDisTest::on_act_file_open() {
    //停止采集
    if (m_bIsAcuistion == true) {
        on_act_acq_analog();
        QTimer::singleShot(300, this,
            [=]() {
                on_act_file_open();
            });
        return;
    }

    auto selFilePath = QFileDialog::getOpenFileName(
        this,
        tr("Select the file to open"),
        "",
        "RAW(*.raw)"
    );
    if (selFilePath.isEmpty()) return;
    onFileOpen(selFilePath);
}

void CyMediaDisTest::on_act_image_save() {

}

void CyMediaDisTest::on_act_video_rec_start() {

}

void CyMediaDisTest::on_act_video_rec_pause() {

}

void CyMediaDisTest::on_act_video_rec_stop() {

}

void CyMediaDisTest::on_act_acq_img_type(QAction* act) {
    m_analog_img_type = CyMediaTest::eAnalogImageType(act->data().toUInt());
}

void CyMediaDisTest::on_act_acq_analog() {
    if (m_bIsAcuistion) return;

    //启动采集线程
    m_bIsAcuistion = true;
    m_AnalogAcquisitionThread->start();
    
    //UI
    ui_act_acq_analog->setEnabled(false);
    ui_act_acq_stop->setEnabled(true);
}

void CyMediaDisTest::on_act_acq_stop() {
    if (false == m_bIsAcuistion) return;

    //停止采集线程
    m_bIsAcuistion = false;
    m_AnalogAcquisitionThread->quit();
    m_AnalogAcquisitionThread->wait(3000);
    m_AnalogAcquisitionThread->terminate();

    //UI
    ui_act_acq_analog->setEnabled(true);
    ui_act_acq_stop->setEnabled(false);
}

void CyMediaDisTest::on_act_view_colormap_type(QAction* act) {
    m_view->setColorMap(act->data().toUInt());
}

void CyMediaDisTest::on_act_view_bayer_rebuild_type(QAction* act) {
    m_view->setDemosaic(CyMedia::DemosaicingMethod(act->data().toUInt()));
}

void CyMediaDisTest::on_act_view_yuv_rebuild_type(QAction* act) {
    m_view->setYUVMethod(CyMedia::YUVTransMethod(act->data().toUInt()));
}

void CyMediaDisTest::on_act_gray_stretch() {
    auto w = m_view->stretchWidget();
    if (!w) return;
    if (false == w->isVisible()) {
        w->setVisible(true);
    }
    else {
        w->raise();
        auto parentPos = pos();
        w->move(parentPos.x() + (width() - w->width()) / 2, parentPos.y() + (height() - w->height()) / 2);
    }
}

void CyMediaDisTest::on_act_grayscale_measure() {
    auto w = m_view->grayTestWidget();
    if (!w) return;
    if (false == w->isVisible()) {
        w->setVisible(true);
    }
    else {
        w->raise();
        auto parentPos = pos();
        w->move(parentPos.x() + (width() - w->width()) / 2, parentPos.y() + (height() - w->height()) / 2);
    }
}

void CyMediaDisTest::on_act_about() {

}

void CyMediaDisTest::on_status_timerout() {
    ui_CapFpsLabel->setText(QString::number(m_CapFps, 'f', 2));
    ui_DisFpsLabel->setText(QString::number(m_view->displayFps(), 'f', 2));
}

void CyMediaDisTest::closeReplay() {

}

void CyMediaDisTest::onFileOpen(QString filepath) {
    auto fileType = m_image_func->getTypeByPath(filepath.toStdString());
    if (fileType == CyMedia::IMAGE_SUFFIX_INVALID) {
        QMessageBox::warning(
            this,
            tr("error"),
            tr("Invalid file or unsupported format : %1").arg(filepath),
            QMessageBox::Ok
        );
        return;
    }
    //先按视频解析
    if (fileType == CyMedia::IMAGE_SUFFIX_RAW) {

    }

    CyMedia::ImageShowInfo info;
    std::vector<uint8_t> data;
    QString errStr;
    int openRet = m_image_func->openImage(filepath.toStdWString(), fileType, info, data);
    if (openRet == 3) {
        m_rawHeaderW->setOpenFileName(QFileInfo(filepath).completeBaseName());

        quint32 width = m_rawHeaderW->imageWidth();
        quint32 height = m_rawHeaderW->imageHeight();
        quint32 nbit = m_rawHeaderW->imagenBit();
        CyMedia::ePixType pixelFormat = m_rawHeaderW->imagePixelType();
        CyMedia::ePixelValueType pix_val_type = m_rawHeaderW->specialPixe();
        quint32 offset = m_rawHeaderW->imageOffset();
        
        QVariant readValue;
        readValue = m_Setting->value(QString("ImgageInfo/imageWidth"));
        if (readValue.isValid())
            width = readValue.toUInt();
        readValue = m_Setting->value(QString("ImgageInfo/imageHeight"));
        if (readValue.isValid())
            height = readValue.toUInt();
        readValue = m_Setting->value(QString("ImgageInfo/imagenBit"));
        if (readValue.isValid())
            nbit = readValue.toUInt();
        readValue = m_Setting->value(QString("ImgageInfo/imagePixelValueType"));
        if (readValue.isValid())
            pix_val_type = CyMedia::ePixelValueType(readValue.toUInt());
        readValue = m_Setting->value(QString("ImgageInfo/imagePixelType"));
        if (readValue.isValid())
            pixelFormat = CyMedia::ePixType(readValue.toUInt());
        readValue = m_Setting->value(QString("ImgageInfo/imageOffset"));
        if (readValue.isValid())
            offset = readValue.toUInt();

        m_rawHeaderW->setOpenInfo(width, height, nbit, pixelFormat, pix_val_type, offset);
        auto re = m_rawHeaderW->exec();
        if (CyMedia::CyMediaDis_GetRawInfoDialog::Accepted != re) {
            return;
        }

        info.width = m_rawHeaderW->imageWidth();
        info.height = m_rawHeaderW->imageHeight();
        info.bit = m_rawHeaderW->imagenBit();
        info.format = m_rawHeaderW->imagePixelType();
        info.special_pixel = m_rawHeaderW->specialPixe();
        offset = m_rawHeaderW->imageOffset();

        m_Setting->setValue(QString("ImgageInfo/imageWidth"), info.width);
        m_Setting->setValue(QString("ImgageInfo/imageHeight"), info.height);
        m_Setting->setValue(QString("ImgageInfo/imagenBit"), info.bit);
        m_Setting->setValue(QString("ImgageInfo/imagePixelValueType"), quint32(info.special_pixel));
        m_Setting->setValue(QString("ImgageInfo/imagePixelType"), quint32(info.format));
        m_Setting->setValue(QString("ImgageInfo/imageOffset"), offset);
        m_Setting->sync();

        info.upLenth();
        openRet = m_image_func->openImage_NotHeaderRaw(filepath.toLocal8Bit().data(), m_rawHeaderW->imageOffset(), info, data);
    }
    if (0 != openRet) {
        QString errStr;
        if (openRet == 1) {
            errStr = tr("file error");
        }
        else if (openRet == 2) {
            errStr = tr("Invalid file format");
        }
        else if (openRet == 3) {
            errStr = tr("image header error");
        }
        QMessageBox::warning(
            this,
            tr("error"),
            errStr,
            QMessageBox::Ok
        );
        return;
    }

    //更新图像
    m_view->upImageData(info, data.data());
}

void CyMediaDisTest::onViewUpPosPix(qint32 x, qint32 y, double r, double g, double b, bool signlR) {
    if (signlR) {
        ui_PosColorLabel->setText(QString("(%1, %2) %3 %4")
            .arg(x)
            .arg(y)
            .arg(tr("Gray"))
            .arg(r));
    }
    else {
        ui_PosColorLabel->setText(QString("(%1, %2) RGB %3 %4 %5")
            .arg(x)
            .arg(y)
            .arg(r)
            .arg(g)
            .arg(b));
    }
}

void CyMediaDisTest::onImageSizeChanged(quint32 w, quint32 h, int bit) {
    ui_ImageSizeLabel->setText(QString("%1 * %2 (%3bit)")
        .arg(w)
        .arg(h)
        .arg(bit));
}

void CyMediaDisTest::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void CyMediaDisTest::dropEvent(QDropEvent* event) {
    QList<QUrl> urls = event->mimeData()->urls();
    urlsDropOpe(urls);
}

void CyMediaDisTest::urlsDropOpe(QList<QUrl> urls) {
    QString fileName;
    QFileInfo oneFileInfo;
    for (QUrl var : urls) {
        fileName = var.toLocalFile();
        oneFileInfo.setFile(fileName);
        if (oneFileInfo.exists()) {
            onFileOpen(fileName);
            break;
        }
    }
}

void CyMediaDisTest::thread_acquisition() {
    QElapsedTimer t_CapFpsTimer;
    uint64_t t_CapFpsCount = 0;

    CyMedia::ImageShowInfo info;
    info.width = 1000;
    info.height = 1000;
    info.bit = 8;
    info.format = CyMedia::RGB;
    /*info.format = CyMedia::BAYERRG;
    info.width *= 3;*/
    info.upLenth();
    QImage img(info.width, info.height, QImage::Format_RGB888);
    int framId = 0;
    while (m_bIsAcuistion) {
        t_CapFpsCount++;
        if (false == t_CapFpsTimer.isValid()) {
            t_CapFpsTimer.start();
        }
        else if (t_CapFpsTimer.elapsed() >= 1000){
            m_CapFps = t_CapFpsCount * 1000.0 / t_CapFpsTimer.elapsed();
            t_CapFpsCount = 0;
            t_CapFpsTimer.restart();
        }
        genAnalogImage(m_analog_img_type, img, framId);
        m_view->upImageData(info, img.bits());
        framId++;if (framId == INT_MAX) framId = 0;
        QThread::msleep(1);
    }

    m_AnalogAcquisitionThread->quit();
}

