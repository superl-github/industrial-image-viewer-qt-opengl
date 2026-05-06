#include "CyMediaDis.h"

#include "CyMediaDis/CyMediaDisView.h"
#include "CyMediaDis/CyDMediaDisScen.h"
#include "CyMediaDis/CyDMediaDisBack.h"
#include "CyMediaCalc/CyMediaCalc.h"
#include "CyMediaDis/drawItem/CyDisDrawItem.h"
#include "CyMediaDis/drawItem/DrawItemTool.h"

#include <queue>

#include <QElapsedTimer>
#include <QOffscreenSurface>
#include <QTranslator>
#include <QFormLayout>
#include <QLabel>

#define ImageStackMaxSize 3

namespace CyMedia {
    class CyMediaDis::privateData : public QObject {
        Q_OBJECT

    public:
        typedef struct oneFrameBuffer {
            CyMedia::ImageShowInfo info;                        // 图像信息
            unsigned char* pdata = nullptr;                     // 图像数据
            bool            bisUpData = false;                  // 是否是新数据需要更新
            bool            bIsSource = false;                  // 是否是模块存储的原始数据
            bool            bUpImage = true;                    // 是否更新显示图像
            bool            bUpStretch = true;                  // 是否更新灰度拉伸
            bool            bIsAddFps = true;                   // 是否统计帧频
        }oneFrameBuffer_;

        typedef struct opeFrameThreadPara {
            QElapsedTimer dataFpsTimer;
            int dataFpsTimePointMs = 3000;
            int dataFpsFramePoint = 30;

            uint8_t* pAnalyImage = 0;
            uint32_t analyImageLen = 0;

            uint8_t* pAlignImage = 0;
            CyMedia::ImageShowInfo pAlignImage_info;

            bool openglOpeBayer = false;

            void init() {
                dataFpsTimer.invalidate();
                if (pAnalyImage) {
                    delete[] pAnalyImage;
                    pAnalyImage = nullptr;
                }
                analyImageLen = 0;
                if (pAlignImage) {
                    delete[] pAlignImage;
                    pAlignImage = nullptr;
                }
                pAlignImage_info.length = 0;

                openglOpeBayer = false;
            }
        }_opeFrameThreadPara;

    public:
        explicit privateData(CyMediaDis* parent = nullptr);
        ~privateData();

    public:signals:
        void ImageDataDone(bool InfoChange, CyMedia::ImageShowInfo info);
        void startDataFpsTimer();

    public:
        void initGUI();

        void log_printf(const char* fmt, ...);
        void log_printf_Sub(const std::string& msg, void* pUser);

        oneFrameBuffer* getBuffer(bool force = false);
        void upImageToBuffer(oneFrameBuffer* buffer, CyMedia::ImageShowInfo& info, uint8_t* data);
        bool addData(CyMedia::ImageShowInfo info, uint8_t* data, bool isSource = false, bool force = false);
        void addOneGrayData(bool upImage, bool force = false);
        void clearImage();
        bool upDataIsSlow();

        void stopImageOpeThread();

        void setDrawMode(CyDisDrawItem::ItemType mode);

    private:
        void Thread_ImageData();
        void Thread_ImageData_processOne(oneFrameBuffer& img, opeFrameThreadPara& opePara);
        void Thread_ImageData_SpecialOpe(
            CyMedia::ImageShowInfo& srcInfo, uint8_t* srcData, opeFrameThreadPara& opePara);
        void Thread_ImageData_UpDis(CyMedia::ImageShowInfo& info, uint8_t* data, opeFrameThreadPara& opePara, bool isSource);
        void ImageDataDoneReceive(bool InfoChange, CyMedia::ImageShowInfo info);

        void onAddItem(QUuid id);
        void onRemoveItem(QUuid id);
        void onGrayToolNeedImage();

    private:
        void initScene();
        void initToolBar();
        void initItem();
        void initLayout();

    public:
        QMutex  m_dataLock;
        CyMedia::eLanguage m_language = ENGLISH;
        QTranslator* m_trans = nullptr;
        bool m_bIsPrintDebug = false;
        CyMedia::LogCallback m_logCallback = nullptr;
        void* m_logCallback_user = nullptr;

        //=====BaseUI/Item=====
        QColor mThemeColor = QColor(0x2a, 0xa3, 0xc6);
        CyMediaDis* m_parent = nullptr;
        CyMediaDisView* view = nullptr;
        CyDMediaDisScen* scene = nullptr;
        CyDMediaDisBack* imageItem = nullptr;
        CyDisDrawItem::ItemManager* drawmanager = nullptr;
        CyDisDrawItem::DrawItemTool* drawingTool = nullptr;
        QThread* pImageDataThread = 0;        ///< 图像处理线程句柄
        bool     bImageDataThread_flag = true;   ///< 图像处理线程循环标志
        std::queue<oneFrameBuffer*>  threadPare_ImageDataStack;      ///< 图像线程所使用的数据栈
        oneFrameBuffer threadPare_ImageDataArray[ImageStackMaxSize];
        quint32 ImageStackMaxNum = ImageStackMaxSize;
        quint32 ImageDataStack_currentOpe = 0;

        //=====Image=====
        CyMediaDisImageCallBack mImageCallBack = nullptr;
        void* mImageCallBackUser = nullptr;
        CyMedia::ImageShowInfo m_imageinfo;
        int32_t posX = 0;
        int32_t posY = 0;

        bool bHaveData = false;              ///< 当前是否有图像数据
        bool m_bFistUpImage = true;          ///< 是否首次更新图像

        CyMedia::ImageShowInfo  m_rawInfo;        ///< 接收数据原始信息
        unsigned char* m_rawImageData = 0;        ///< 接收数据原始图像

        //=====数据统计=====
        int64_t nDataFpsCount = 0;
        double   DataFps = 0.0;
        QElapsedTimer           calcolorTimer;  ///< 计算鼠标位置颜色定时器
        QElapsedTimer           flushtimer;     ///< 显示帧频计时器
        QElapsedTimer clearImageTime;

        //=====UI/Tool=====
        QWidget* toolWidget = 0;      ///< Parent window of tool button
        QPushButton* zoomInButton = 0;      ///< 放大按钮
        QPushButton* zoomOutButton = 0;     ///< 缩小按钮
        QPushButton* zoomRawButton = 0;     ///< 原始比例按钮
        QPushButton* zoomFitButton = 0;     ///< 适应窗口按钮
        QPushButton* flipHButton = 0;       ///< 水平翻转按钮
        QPushButton* flipVButton = 0;       ///< 垂直翻转按钮
        bool bZoomScrollBarIsShow = false;

        QElapsedTimer m_ThumbnailUpTimer;

        CyMediaRecTimeW* mRetimeItem = nullptr;
        CyMediaDisGrayStretch* mStretchWidget = nullptr;
        CyMedia::StretchType lastUpStretchType = CyMedia::stretch_None;
        CyMediaDisGrayTest* mGrayTestWidget = nullptr;

    private:
        bool guiIsInit = false;
    };

    CyMediaDis::CyMediaDis(QWidget* parent /* = nullptr */)
        : QFrame(parent)
        , d(new CyMediaDis::privateData(this)){
    
        d->initGUI();
        setDrawMode(CyDisDrawItem::Invalid);
        setThemeColor(d->mThemeColor);
        setContentsMargins(0, 0, 0, 0);
        setMouseTracking(true);

        qRegisterMetaType<CyMedia::ImageShowInfo>("CyMedia::ImageShowInfo");
    }

    CyMediaDis::~CyMediaDis() {
        if (d) {
            d->stopImageOpeThread();
            for (int i = 0; i < d->ImageStackMaxNum; ++i) {
                delete[] d->threadPare_ImageDataArray[i].pdata;
            }
            delete[] d->m_rawImageData;

            d->mStretchWidget->setParent(nullptr);
            d->mStretchWidget->close();

            delete d->drawingTool;
            d->drawingTool = nullptr;

            delete d->drawmanager;
            d->drawmanager = nullptr;
        
            delete d->scene;
            d->scene = nullptr;

            delete d->view;
            d->view = nullptr;
        
            delete d;
            d = nullptr;
        }
    }

    bool CyMediaDis::supportsOpenGL(int& mainV, int& subV) {
        // 1. 创建临时 OpenGL 上下文（不显示窗口）
        QOpenGLContext ctx;
        QSurfaceFormat fmt;
        fmt.setRenderableType(QSurfaceFormat::OpenGL);
        ctx.setFormat(fmt);

        // 2. 创建离屏表面（不占用屏幕）
        QOffscreenSurface surface;
        surface.create();
        if (!ctx.create()) {
            mainV = 0;
            subV = 0;
            return false;
        }

        // 3. 绑定上下文并获取版本
        ctx.makeCurrent(&surface);
        auto ver = ctx.format();
        mainV = ver.majorVersion();
        subV = ver.minorVersion();
        ctx.doneCurrent();

        return true;
    }

    bool CyMediaDis::supportsOpenGLForCyMedia() {
        int mainV, subV;
        bool support = supportsOpenGL(mainV, subV);
        if (false == support) {
            return false;
        }

        return !((mainV > 3) || ((mainV == 3) && subV >= 3));
    }

	QString CyMediaDis::pixelFormatStr(CyMedia::ePixType format) {
		switch (format) {
		    case CyMedia::MONO:
			    return QString("MONO");
		    case CyMedia::BAYERGR:
			    return QString("BAYER GRBG");
		    case CyMedia::BAYERBG:
			    return QString("BAYER BGGR");
		    case CyMedia::BAYERGB:
			    return QString("BAYER GBRG");
		    case CyMedia::BAYERRG:
			    return QString("BAYER RGGB");
		    case CyMedia::RGB:
			    return QString("RGB");
		    case CyMedia::RGBA:
			    return QString("RGBA");
		    case CyMedia::MONO10P:
			    return QString("MONO10P");
		    case CyMedia::MONO10P_GVSP:
			    return QString("MONO10P_GVSP");
		    case CyMedia::MONO12P:
			    return QString("MONO12P");
		    case CyMedia::MONO12P_GVSP:
			    return QString("MONO12P_GVSP");
		    case CyMedia::MONO_OVERSIZE:
			    return QString("CMONO_OVERSIZE");
		    default:
			    return QString("UNKNOW");
		}
	}

	CyMedia::eLanguage CyMediaDis::currentLanguage() {
        return d->m_language;
	}

	bool CyMediaDis::setLanguage(CyMedia::eLanguage lang) {
        if (d->m_trans) {
            QCoreApplication::removeTranslator(d->m_trans);
            delete d->m_trans;
            d->m_trans = nullptr;
        }

        d->m_trans = new QTranslator(QCoreApplication::instance());

        QString qmFileName;
        switch (lang) {
            case ENGLISH: {
                qmFileName = QString(":/ttranslations/CyMedia_en_US.qm");
            }break;

            case CHINESE: {
                qmFileName = QString(":/ttranslations/CyMedia_zh_CN.qm");
            }break;
        }

        if (d->m_trans->load(qmFileName)) {
            QCoreApplication::installTranslator(d->m_trans);
            d->mStretchWidget->flushTrans();
            d->mGrayTestWidget->flushTrans();
            d->drawmanager->flushTrans();
            return true;
        }

		delete d->m_trans;
		d->m_trans = nullptr;
        return false;
	}

	void CyMediaDis::setPrintLog(bool flag) {
        d->m_bIsPrintDebug = flag;
    }

    void CyMediaDis::setLogCallback(CyMedia::LogCallback cb, void* pUser/* = nullptr*/) {
        d->m_logCallback = std::move(cb);
        d->m_logCallback_user = pUser;
    }

	void CyMediaDis::setSceneAcceptDrop(bool accept) {
        d->scene->setAcceptDrops(accept);
	}

	void CyMediaDis::setImageStackNum(uint32_t num) {
		if (num <= 1) {
			num = 1;
		}
        else if (num > 10) {

        }
	    QMutexLocker lock(&d->m_dataLock);
		d->ImageStackMaxNum = num;
	}

	bool CyMediaDis::upImageData(CyMedia::ImageShowInfo info, uint8_t* data, bool force /*= false*/) {
        return d->addData(info, data, false, force);
    }

	void CyMediaDis::registerImageCallBack(CyMediaDisImageCallBack func, void* pUser) {
        d->mImageCallBack = func;
        d->mImageCallBackUser = pUser;
	}

	bool CyMediaDis::haveDate(void) {
        return d->bHaveData;
    }

    void CyMediaDis::clearImage(void) {
        return d->clearImage();
    }

	CyMedia::ImageShowInfo& CyMediaDis::imageinfo() {
        return d->m_imageinfo;
	}

	double CyMediaDis::displayFps(void) {
        return d->imageItem->flushFps();
	}

	CyMedia::StretchType CyMediaDis::stretchType() {
        return d->imageItem->stretchType();
    }

    void CyMediaDis::setStretchType(CyMedia::StretchType type) {
        d->imageItem->setStretchType(type);
    }

    void CyMediaDis::setStreaChPara(uint32_t start /*= 0*/, uint32_t end /*= 0*/) {
        d->imageItem->setStreaChPara(start, end, (1 << d->m_rawInfo.bit) - 1 );
    }

    CyMedia::DemosaicMethod CyMediaDis::Demosaic(){
        return d->imageItem->Demosaic();
    }

    void CyMediaDis::setDemosaic(CyMedia::DemosaicMethod method) {
        d->imageItem->setDemosaic(method);
        if (d->upDataIsSlow()) {
            d->addOneGrayData(true);
        }
    }

    QStringList CyMediaDis::ColorMapList() const {
        return d->imageItem->ColorMapList();
    }

    quint32 CyMediaDis::colorMapIndex() const {
        return d->imageItem->colorMapIndex();
    }

    bool CyMediaDis::setColorMap(quint32 index) {
        if (false == d->imageItem->setColorMap(index))
            return false;
        /*if (d->upDataIsSlow()) {
            if (d->m_rawInfo.format >= CyMedia::MONO
                || (d->m_rawInfo.format >= CyMedia::BAYERRG && d->m_rawInfo.format <= CyMedia::BAYERGB && d->imageItem->showBayerSource())) {
                d->addOneGrayData(
                    d->graytestwidget->isVisible(),
                    d->grayStretchWidget->isVisible(),
                    false);
            }
        }*/
        return true;
    }

    bool CyMediaDis::setColorMap(const QString& mapName) {
        if (false == d->imageItem->setColorMap(mapName))
            return false;
        /*if (d->upDataIsSlow()) {
            if (d->m_rawInfo.format >= CyMedia::MONO
                || (d->m_rawInfo.format >= CyMedia::BAYERRG && d->m_rawInfo.format <= CyMedia::BAYERGB && d->imageItem->showBayerSource())) {
                d->addOneGrayData(
                    d->graytestwidget->isVisible(),
                    d->grayStretchWidget->isVisible(),
                    false);
            }
        }*/
        return true;
    }

	void CyMediaDis::zoomAuto() {
        if (false == d->bHaveData)
            return;

        if (d->upDataIsSlow()) {
            d->imageItem->setPos(0, 0);
            d->view->zoomAuto();
        }
        else {
            QTimer::singleShot(0, this, [this]() {
                d->imageItem->setPos(0, 0);
                d->view->zoomAuto();
                });
        }
	}

	void CyMediaDis::setDrawMode(CyDisDrawItem::ItemType mode) {
        d->setDrawMode(mode);
    }

    void CyMediaDis::setThemeColor(QColor color) {
        d->mGrayTestWidget->setThemeColor(color);
        d->mStretchWidget->setThemeColor(color);
        d->drawingTool->setThemeColor(color);
    }

	QUuid CyMediaDis::addItem(CyDisDrawItem::ItemType itemType) {
		return d->drawmanager->addItemByType(itemType);
	}

	QUuid CyMediaDis::addItem(CyDisDrawItem::ItemType itemType, QPainterPath path) {
        return d->drawmanager->addItemByTypeWidthPath(itemType, path);
	}

	CyDisDrawItem::BaseItem* CyMediaDis::getItem(QUuid& id) {
        return d->drawmanager->getItem(id);
    }

    void CyMediaDis::clearItem() {
        d->drawmanager->clearAll();
    }

    bool CyMediaDis::isSingleItemMode(){
        return d->drawingTool->replaceMode();
    }

    void CyMediaDis::setSingleItemMode(bool flag){
        d->drawingTool->setReplaceMode(flag);
    }

    QUuid CyMediaDis::getLaseItem() {
        return d->drawmanager->getLaseItem();
    }

    bool CyMediaDis::isDrawing() {
        if (!d->drawingTool)
            return false;
        return d->drawingTool->isDrawing();
    }

    bool CyMediaDis::toolBarVisible(void) {
        return d->toolWidget->isVisible();
    }

    void CyMediaDis::setToolBarVisible(bool show) {
        d->toolWidget->setVisible(show);
    }

    bool CyMediaDis::zoomScrollBarVisible(void) {
        return d->bZoomScrollBarIsShow;
    }

    void CyMediaDis::setZoomScrollBarVisible(bool show) {
        if (d->bZoomScrollBarIsShow == show) {
            return;
        }
        if (show) {
            /*if (d->view->horizontalScrollBarPolicy() != Qt::ScrollBarAsNeeded){
                d->view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                d->view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            }*/
        }
        else {
            if (d->view->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff) {
                d->view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                d->view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            }
        }
        d->bZoomScrollBarIsShow = show;
    }

	bool CyMediaDis::recTimeVisible() {
        return d->mRetimeItem->isVisible();
	}

	void CyMediaDis::setRecTimeVisible(bool visi) {
        d->mRetimeItem->setVisible(visi);
	}

	void CyMediaDis::upRecTime(uint64_t time) {
        d->mRetimeItem->upRecTime(time);
	}

	void CyMediaDis::upRecTime(uint64_t saved, uint64_t sum) {
        d->mRetimeItem->upRecTime(saved, sum);
	}

	void CyMediaDis::upRecTime_Timed(uint64_t saved, uint64_t sum) {
        d->mRetimeItem->upRecTime_Timed(saved, sum);
	}

	CyMediaDisGrayStretch* CyMediaDis::stretchWidget() {
        return d->mStretchWidget;
    }

    void CyMediaDis::setGrayStretchVisible(bool visible) {
        d->mStretchWidget->setVisible(visible);
    }

    CyMediaDisGrayTest* CyMediaDis::grayTestWidget() {
        return d->mGrayTestWidget;
    }

    void CyMediaDis::setGrayTestVisible(bool visible) {
        d->mGrayTestWidget->setVisible(visible);
    }

    CyMediaDis::privateData::privateData(CyMediaDis* parent /*= nullptr*/)
        : QObject(parent)
        , m_parent(parent) {
    
        pImageDataThread = new QThread(this);
        connect(pImageDataThread, &QThread::started, this, &CyMediaDis::privateData::Thread_ImageData, Qt::DirectConnection);

        connect(this, &privateData::ImageDataDone, this, &privateData::ImageDataDoneReceive);
        connect(this, &privateData::startDataFpsTimer, this, [this]() {
                if (calcolorTimer.isValid()) {
                    calcolorTimer.restart();
                }
                else {
                    calcolorTimer.start();
                }
            });
    }

    CyMediaDis::privateData::~privateData() {
    
    }

    CyMediaDis::privateData::oneFrameBuffer* CyMediaDis::privateData::getBuffer(bool force /*= false*/) {
        int currentOpe = ImageDataStack_currentOpe; // 当前消费者正在用的索引
        int candidateSlot = -1;
        int oldestValidSlot = -1; // 用于 force 覆盖（排除 currentOpe）
        //查找空闲槽位
        for (int i = 0; i < ImageStackMaxNum; ++i) {
            if (!threadPare_ImageDataArray[i].bisUpData && i != currentOpe) {
                candidateSlot = i;
                break;
            }
        }if (candidateSlot != -1) {
            ;// 有可用槽，直接使用
        }
        else if (force) {
            // 无空闲槽，但允许 force → 找一个 bisUpData==true 且 ≠ currentOpe 的最旧帧覆盖
            // 简单策略：从 (currentOpe + 1) % N 开始找第一个可覆盖的
            for (int i = 0; i < ImageStackMaxNum; ++i) {
                int idx = (currentOpe + 1 + i) % ImageStackMaxNum;
                if (idx != currentOpe) {
                    oldestValidSlot = idx;
                    break;
                }
            }
            if (oldestValidSlot == -1) {
                // 极端情况：所有槽要么是 currentOpe，要么空闲（理论上不会发生）
                return nullptr;
            }
            candidateSlot = oldestValidSlot;
        }
        else {
            // 无空闲，且不允许 force → 丢弃
            return nullptr;
        }

        return &threadPare_ImageDataArray[candidateSlot];
    }

    void CyMediaDis::privateData::upImageToBuffer(oneFrameBuffer* buffer, CyMedia::ImageShowInfo& info, uint8_t* data) {
        //内存管理
        if (buffer->pdata == nullptr || buffer->info.length != info.length) {
            delete[] buffer->pdata;
            buffer->pdata = new unsigned char[info.length];
            buffer->info.length = info.length;
        }
        //拷贝数据
        memcpy(&(buffer->info), &info, sizeof(CyMedia::ImageShowInfo));
        memcpy(buffer->pdata, data, info.length);
    }

    bool CyMediaDis::privateData::addData(CyMedia::ImageShowInfo info, uint8_t* data, bool isSource/* = false*/, bool force /*= false*/) {
        if (!data || info.width <= 0 || info.height <= 0 || info.bit <= 0 || info.length <= 0) {
            return false;
        }
        static const size_t MAX_IMAGE_SIZE = 200 * 1024 * 1024; // 200MB
        if (info.length > MAX_IMAGE_SIZE) {
            log_printf("图像过大，拒绝: %u bytes\n", info.length);
            return false;
        }

        //确保NoImage不会被错误覆盖
        if (clearImageTime.isValid() && clearImageTime.elapsed() < 300) {
            log_printf("跳过图像(update)\n\r");
            return false;
        }

        //数据入栈
        QMutexLocker lock(&m_dataLock);
        oneFrameBuffer* t_pBuffer = getBuffer(force);
        if (!t_pBuffer)
            return false;
        upImageToBuffer(t_pBuffer, info,  data);
        //重置状态
        t_pBuffer->bIsSource = isSource;
        t_pBuffer->bUpImage = true;
        t_pBuffer->bIsAddFps = true;

        t_pBuffer->bisUpData = true;
        // 启动线程
        if (false == pImageDataThread->isRunning()) {
            bImageDataThread_flag = true;
            pImageDataThread->start();
            log_printf("启动图像数据处理线程, 数据栈最大缓存:%d\n\r", ImageStackMaxNum);
        }

        return true;
    }

    void CyMediaDis::privateData::addOneGrayData(bool upImage, bool force /*= false*/) {
        if (false == bHaveData) {
            return;
        }
        if (false == pImageDataThread->isRunning()) {
            pImageDataThread->start();
        }
        QMutexLocker lock(&m_dataLock);
        oneFrameBuffer* t_pBuffer = getBuffer(force);
        if (!t_pBuffer)
            return;
    
        upImageToBuffer(t_pBuffer, m_rawInfo, m_rawImageData);
        //重置状态
        t_pBuffer->bIsSource = true;
        t_pBuffer->bUpImage = upImage;
        t_pBuffer->bUpStretch = mStretchWidget->stretchtype() != lastUpStretchType;
        t_pBuffer->bIsAddFps = false;

        t_pBuffer->bisUpData = true;
    }

    void CyMediaDis::privateData::clearImage() {
        if (bHaveData) {
            // 退出数据处理线程
            stopImageOpeThread();
            // 隐藏图像
            imageItem->clearImage();
            m_bFistUpImage = true;
            // 清除图像变换
            view->zoomRaw(true);
            // 隐藏zoom滚动条
            if (view->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff) {
                view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            }
            //关闭绘制模式
            setDrawMode(CyDisDrawItem::Invalid);
            bHaveData = false;
            view->setImageShow(false);
            scene->setTipTextVisible(true);
        }
    }

    bool CyMediaDis::privateData::upDataIsSlow() {
        return (DataFps < 2.1) && bHaveData;
    }

    void CyMediaDis::privateData::stopImageOpeThread() {
        bImageDataThread_flag = false;
        if (pImageDataThread && pImageDataThread->isRunning()) {
            pImageDataThread->quit();
            pImageDataThread->wait(0x3000);
            pImageDataThread->terminate();
        }
    }

    void CyMediaDis::privateData::Thread_ImageData() {
        static opeFrameThreadPara opePara;
        opePara.init();
        bImageDataThread_flag = true;
        QElapsedTimer eTiemr;
        eTiemr.start();
        while (bImageDataThread_flag) {
            //取数据
            int targetIdx = -1;
            {
                QMutexLocker lock(&m_dataLock);
                // 找一个 bisUpData == true 且 ≠ -1 的帧（不关心 currentOpe，因为它是自己设的）
                for (int i = 0; i < ImageStackMaxNum; ++i) {
                    if (threadPare_ImageDataArray[i].bisUpData) {
                        targetIdx = i;
                        ImageDataStack_currentOpe = i; // 标记正在处理
                        break;
                    }
                }
            }
            if (targetIdx == -1) {
                QThread::msleep(10);
                continue;
            }
            //处理
            eTiemr.restart();
            Thread_ImageData_processOne(threadPare_ImageDataArray[targetIdx], opePara);
            log_printf("Thread_ImageData_processOne耗时：%lldms\n", eTiemr.elapsed());
            //回收，清除状态
            QMutexLocker lock(&m_dataLock);
            threadPare_ImageDataArray[targetIdx].bisUpData = false;
        }
        QMutexLocker lock(&m_dataLock);
        ImageDataStack_currentOpe = -1;
        pImageDataThread->quit();
        bImageDataThread_flag = false;
        log_printf("图像处理线程退出\n\r");
    }
    void CyMediaDis::privateData::Thread_ImageData_processOne(oneFrameBuffer& img, opeFrameThreadPara& opePara) {
        QElapsedTimer eTimer;
        eTimer.start();

        // 统计更新帧频
        if (opePara.dataFpsTimer.isValid())
            opePara.dataFpsTimer.start();
        if (nDataFpsCount >= opePara.dataFpsFramePoint || opePara.dataFpsTimer.elapsed() >= opePara.dataFpsTimePointMs) {
            DataFps = 1000.0 * nDataFpsCount / opePara.dataFpsTimer.elapsed();
            nDataFpsCount = 0;
            opePara.dataFpsTimer.restart();
        }

        if (img.bIsAddFps) {
            nDataFpsCount++;
            img.bIsAddFps = false;
        }
        //log_printf("统计帧频 耗时：%lldms\n", eTimer.elapsed());

        eTimer.restart();
        //等待图像更新完毕
        if (false == imageItem->upImageAvailable()) {
            return;
        }

        auto Imagedata = img.pdata;
        if (!Imagedata) {
            return;
        }
        CyMedia::ImageShowInfo Imageinfo;
        memcpy(&Imageinfo, &(img.info), sizeof(CyMedia::ImageShowInfo));
        //检查是否退出
        if (false == bImageDataThread_flag) {
            return;
        }
        //log_printf("等待图像更新完毕 耗时：%lldms\n", eTimer.elapsed());

        eTimer.restart();
        //拷贝原始数据，用以静态图像分析、保存raw等功能
        if (false == img.bIsSource) {
            QMutexLocker lock(&m_dataLock);
            if (memcmp(&m_rawInfo, &Imageinfo, sizeof(CyMedia::ImageShowInfo))) {
                memcpy(&m_rawInfo, &Imageinfo, sizeof(CyMedia::ImageShowInfo));
                delete[] m_rawImageData;
                m_rawImageData = new unsigned char[m_rawInfo.length];
            }
            memcpy(m_rawImageData, Imagedata, m_rawInfo.length);
        }
        // 检查是否退出
        if (false == bImageDataThread_flag) {
            return;
        }
        //log_printf("拷贝原始数据 耗时：%lldms\n", eTimer.elapsed());

        eTimer.restart();
        // 特殊格式处理
        Thread_ImageData_SpecialOpe(Imageinfo, Imagedata, opePara);
        //log_printf("特殊格式处理 耗时：%lldms\n", eTimer.elapsed());

        // 检查是否退出
        if (false == bImageDataThread_flag) {
            return;
        }

        eTimer.restart();
        //更新状态
        if (false == bHaveData) {
            bHaveData = true;
            view->setImageShow(true);
        }
        //log_printf("更新状态 耗时：%lldms\n", eTimer.elapsed());

        //图像处理
        eTimer.restart();
        if (mStretchWidget->isVisible() && img.bUpStretch) {
            mStretchWidget->upImageData(Imageinfo, Imagedata, imageItem->Demosaic());
            lastUpStretchType = mStretchWidget->stretchtype();
        }
        //log_printf("更新灰度拉伸 耗时：%lldms\n", eTimer.elapsed());
        eTimer.restart();
        if (mGrayTestWidget->isVisible()) {
            mGrayTestWidget->upImageData(Imageinfo, Imagedata);
        }
        //log_printf("更新灰度统计 耗时：%lldms\n", eTimer.elapsed());
        //缩略图
        if (false == img.bIsSource) {
            //帧率立即更新
            if (upDataIsSlow()) {
                view->upThumbnaildata(Imageinfo, Imagedata);
            }
            //限制帧率
            else {
                if (false == m_ThumbnailUpTimer.isValid()) {
                    m_ThumbnailUpTimer.start();
                }
                else if (m_ThumbnailUpTimer.elapsed() > 300) {
                    view->upThumbnaildata(Imageinfo, Imagedata);
                    m_ThumbnailUpTimer.restart();
                }
            }
        }
    
        // 显示图像处理
        if (true == img.bUpImage) {
            Thread_ImageData_UpDis(Imageinfo, Imagedata, opePara, img.bIsSource);
        }

        // 图像回调处理
        if (false == img.bIsSource) {
            if (mImageCallBack) {
                mImageCallBack(Imageinfo, Imagedata, mImageCallBackUser);
            }
        }

        // 检查是否退出
        if (false == bImageDataThread_flag) {
            return;
        }

        // 检查是否退出
        if (false == bImageDataThread_flag) {
            return;
        }
    }
    void CyMediaDis::privateData::Thread_ImageData_SpecialOpe(CyMedia::ImageShowInfo& srcInfo, uint8_t* srcData, opeFrameThreadPara& opePara) {
        // 特殊格式处理
        if (srcInfo.format >= CyMedia::BAYERRG
            && srcInfo.format <= CyMedia::BAYERGB
            && imageItem->Demosaic() == CyMedia::BAYERSOUCE) {
            srcInfo.format = CyMedia::MONO;

        }
        switch (srcInfo.format) {
            case CyMedia::BAYERRG:
            case CyMedia::BAYERGR:
            case CyMedia::BAYERGB:
            case CyMedia::BAYERBG: {
                //转RGB
                uint32_t RGBLen = srcInfo.length * 3;
                if (!opePara.pAnalyImage) {
                    opePara.pAnalyImage = new unsigned char[RGBLen];
                    opePara.analyImageLen = RGBLen;
                }
                else if (opePara.analyImageLen != RGBLen) {
                    delete[]opePara.pAnalyImage;
                    opePara.pAnalyImage = new unsigned char[RGBLen];
                    opePara.analyImageLen = RGBLen;
                }
                CyMedia::bayer2RGBConvert(srcInfo, srcData, opePara.pAnalyImage);
                srcInfo.length = opePara.analyImageLen;
                srcInfo.format = CyMedia::RGB;
                srcData = opePara.pAnalyImage;
            }break;

            case CyMedia::MONO10P_GVSP:
            case CyMedia::MONO10P:
            case CyMedia::MONO12P_GVSP:
            case CyMedia::MONO12P: {
                CyMedia::ImageShowInfo tempInfo;
                memcpy(&tempInfo, &srcInfo, sizeof(CyMedia::ImageShowInfo));
                tempInfo.format = CyMedia::MONO;
                if (srcInfo.format == CyMedia::MONO10P || srcInfo.format == CyMedia::MONO10P_GVSP) {
                    tempInfo.bit = 10;
                }
                else {
                    tempInfo.bit = 12;
                }
                tempInfo.upLenth();
                uint32_t convertLen = tempInfo.length;
                if (!opePara.pAnalyImage) {
                    opePara.pAnalyImage = new unsigned char[convertLen];
                    opePara.analyImageLen = convertLen;
                }
                else if (opePara.analyImageLen != convertLen) {
                    delete[]opePara.pAnalyImage;
                    opePara.pAnalyImage = new unsigned char[convertLen];
                    opePara.analyImageLen = convertLen;
                }
                monoUnPack(srcInfo, srcData, opePara.pAnalyImage);
                memcpy(&srcInfo, &tempInfo, sizeof(CyMedia::ImageShowInfo));
                srcData = opePara.pAnalyImage;
            }break;
        }
    }
    void CyMediaDis::privateData::Thread_ImageData_UpDis(CyMedia::ImageShowInfo& src_info, uint8_t* src_data, opeFrameThreadPara& opePara, bool isSource) {
        bool imageInfoChange = false;
        double r = 0, g = 0, b = 0;
        //拉伸信息
        if (false == mStretchWidget->isVisible()) {
            imageItem->setStretchType(CyMedia::stretch_None);
        }
        else {
            imageItem->setStretchType(mStretchWidget->stretchtype());
        }
        auto stretV = mStretchWidget->stretchValue();
        imageItem->setStreaChPara(stretV.start, stretV.end, stretV.max);

        //更新信息
        if (memcmp(&m_imageinfo, &src_info, sizeof(CyMedia::ImageShowInfo)) || true == m_bFistUpImage) {
            if (false == isSource)
                imageInfoChange = true;
            //ImageItem->upImageInfo(Imageinfo);
            memcpy(&m_imageinfo, &src_info, sizeof(CyMedia::ImageShowInfo));
            m_bFistUpImage = false;
        }
        // 鼠标位置颜色
        if (calcolorTimer.isValid()) {
            if (calcolorTimer.elapsed() > 200 || imageItem->flushFps() <= 5.0) {
                CyMedia::calcCoordinateColor(src_info, src_data, posX, posY, &r, &g, &b, imageItem->Demosaic());
                m_parent->emit upPosPix(posX, posY, r, g, b, src_info.isMono() || (src_info.isBayer() && imageItem->Demosaic() == CyMedia::BAYERSOUCE));
                emit startDataFpsTimer();
            }
        }
        else {
            emit startDataFpsTimer();
        }
        emit ImageDataDone(imageInfoChange, src_info);

        //四字节对齐处理
        auto widthRemainder = src_info.width % 4;
        if (widthRemainder) {
            widthRemainder = 4 - widthRemainder;
        }
        auto alignWidth = (src_info.width + widthRemainder);
        auto alignHeight = src_info.height;
        int imageColorCount = 1, picsrclen;
        if (alignWidth != src_info.width || alignHeight != src_info.height) {
            if (src_info.format == CyMedia::RGB) imageColorCount = 3;
            int pixelWideh = 1;
            if (src_info.bit <= 8)
                pixelWideh = 1;
            else if (src_info.bit <= 16)
                pixelWideh = 2;
            else if (src_info.bit <= 32)
                pixelWideh = 4;
            int picsrclen = (alignWidth * alignHeight * pixelWideh * imageColorCount);
            bool bAlooc = false;
            if (!opePara.pAlignImage) {
                bAlooc = true;
            }
            else if (picsrclen != opePara.pAlignImage_info.length) {
                delete[]opePara.pAlignImage;
                bAlooc = true;
            }
            if (bAlooc) {
                opePara.pAlignImage = new unsigned char[picsrclen];
                memset(opePara.pAlignImage, 0x00, picsrclen);
                memcpy(&opePara.pAlignImage_info, &src_info, sizeof(CyMedia::ImageShowInfo));
                opePara.pAlignImage_info.width = alignWidth;
                opePara.pAlignImage_info.height = alignHeight;
                opePara.pAlignImage_info.length = picsrclen;
            }

            CyMedia::copyAlignImage(src_data, opePara.pAlignImage, src_info.width, src_info.height, alignWidth, alignHeight, pixelWideh * imageColorCount);
            imageItem->upImageData(opePara.pAlignImage_info, opePara.pAlignImage);
        }
        else {
            imageItem->upImageData(src_info, src_data);
        }
    }

    void CyMediaDis::privateData::initGUI() {
        if (guiIsInit)
            return;
        guiIsInit = true;

        initScene();
        initToolBar();
        initItem();
        initLayout();
    }

    void CyMediaDis::privateData::log_printf(const char* fmt, ...) {
        if (!m_logCallback || !m_bIsPrintDebug) {
            return;
        }

        // 标准 C 可变参数格式化
        char buffer[1024] = { 0 };
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap); // 格式化到 buffer
        va_end(ap);

        // 转给回调（纯 std::string）
        m_logCallback(QString("CyMediaDis[%1]:%2").arg(__LINE__).arg(QString::fromUtf8(buffer)).toStdString(), m_logCallback_user);
    }

    void CyMediaDis::privateData::log_printf_Sub(const std::string& msg, void* pUser) {
        if (!m_logCallback || !m_bIsPrintDebug) {
            return;
        }

        m_logCallback(msg, pUser);
    }

    void CyMediaDis::privateData::initScene() {
        //画布
        scene = new CyDMediaDisScen(0, 0, 1000, 1000);
        scene->setItemIndexMethod(QGraphicsScene::NoIndex);
        //scene->setBackgroundBrush(Qt::lightGray);
        scene->setSceneRect(QRectF(0, 0, 1000, 1000));
        connect(scene, &CyDMediaDisScen::mousePosChange, this, [this](int x, int y) {
            posX = x;
            posY = y;
            double r = 0, g = 0, b = 0;
            if (upDataIsSlow()) {
                CyMedia::calcCoordinateColor(m_rawInfo, m_rawImageData, x, y, &r, &g, &b, imageItem->Demosaic());
                m_parent->emit upPosPix(x, y, r, g, b, m_rawInfo.isMono() || (m_rawInfo.isBayer() && imageItem->Demosaic() == CyMedia::BAYERSOUCE));
            }
            });
        connect(scene, &CyDMediaDisScen::urlsDrop, this, [this](QList<QUrl> urls) {
                m_parent->emit urlsDrop(urls);
            });
        //视图
        view = new CyMediaDisView();
        QOpenGLWidget* OpenGlwidget = new QOpenGLWidget(m_parent);
        ////指定opengl版本
        //QSurfaceFormat format;
        //format.setVersion(3, 3);
        //format.setOption(QSurfaceFormat::DeprecatedFunctions, false);
        //OpenGlwidget->setFormat(format);
        OpenGlwidget->installEventFilter(this);
        view->setViewport(OpenGlwidget);
        view->viewport()->installEventFilter(this);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
        //view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        view->setCyScene(scene);
        view->setStyleSheet(
            "border:none;");
        view->setMouseTracking(true);
        view->setSceneRect(QRectF(0, 0, m_imageinfo.width, m_imageinfo.height));
        view->setImageShow(bHaveData);
        connect(view, &CyMediaDisView::onMousePress, m_parent, [this]() {
                m_parent->emit PressOnView();
            });
        connect(view, &CyMediaDisView::onMouseDoubleClick, m_parent, [this]() {
                m_parent->emit DoubleClickOnView();
            });
    }

    void CyMediaDis::privateData::initToolBar() {
        toolWidget = new QWidget(m_parent);
        toolWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        static const int toolBarIconSize = 26;
        toolWidget->setStyleSheet(
            "QWidget"
            "{"
            "   border:none;"
            "   background-color:black"
            "}"
            "QToolTip"
            "{"
            "   background-color:white;"
            "   color:black;"
            "}"
        );
        zoomInButton = new QPushButton(toolWidget);
        zoomInButton->setMinimumSize(toolBarIconSize, toolBarIconSize);
        zoomInButton->setMaximumSize(toolBarIconSize, toolBarIconSize);
        zoomInButton->setText("");
        zoomInButton->setToolTip(u8"Zoom In");
        zoomInButton->setStyleSheet(
            "QPushButton"
            "{"
            "image:url(:/CyMediaDis/ICONS/ZommIn.png);"
            "}"
            "QPushButton::hover"
            "{"
            "image:url(:/CyMediaDis/ICONS/ZoomIn-hover.png);"
            "}"
        );
        connect(zoomInButton, &QPushButton::clicked, view, &CyMediaDisView::zoomIn);

        zoomOutButton = new QPushButton(toolWidget);
        zoomOutButton->setMinimumSize(toolBarIconSize, toolBarIconSize);
        zoomOutButton->setMaximumSize(toolBarIconSize, toolBarIconSize);
        zoomOutButton->setText("");
        zoomOutButton->setToolTip(u8"Zoom Out");
        zoomOutButton->setStyleSheet(
            "QPushButton"
            "{"
            "image:url(:/CyMediaDis/ICONS/ZoomOut.png);"
            "}"
            "QPushButton::hover"
            "{"
            "image:url(:/CyMediaDis/ICONS/ZoomOut-hover.png);"
            "}"
        );
        connect(zoomOutButton, &QPushButton::clicked, view, &CyMediaDisView::zoomOut);

        zoomRawButton = new QPushButton(toolWidget);
        zoomRawButton->setMinimumSize(toolBarIconSize, toolBarIconSize);
        zoomRawButton->setMaximumSize(toolBarIconSize, toolBarIconSize);
        zoomRawButton->setText("");
        zoomRawButton->setToolTip(u8"Actual Size");
        zoomRawButton->setStyleSheet(
            "QPushButton"
            "{"
            "image:url(:/CyMediaDis/ICONS/zoonRaw.png);"
            "}"
            "QPushButton::hover"
            "{"
            "image:url(:/CyMediaDis/ICONS/zoonRaw-hover.png);"
            "}"
        );
        connect(zoomRawButton, &QPushButton::clicked, m_parent,
            [this]() {
                view->zoomRaw();
            });

        zoomFitButton = new QPushButton(toolWidget);
        zoomFitButton->setMinimumSize(toolBarIconSize, toolBarIconSize);
        zoomFitButton->setMaximumSize(toolBarIconSize, toolBarIconSize);
        zoomFitButton->setText("");
        zoomFitButton->setToolTip(u8"Fit to window");
        zoomFitButton->setStyleSheet(
            "QPushButton"
            "{"
            "image:url(:/CyMediaDis/ICONS/zoonFit.png);"
            "}"
            "QPushButton::hover"
            "{"
            "image:url(:/CyMediaDis/ICONS/zoonFit-hover.png);"
            "}"
        );
        connect(zoomFitButton, &QPushButton::clicked, view, &CyMediaDisView::zoomAuto);

        flipHButton = new QPushButton(toolWidget);
        flipHButton->setMinimumSize(toolBarIconSize, toolBarIconSize);
        flipHButton->setMaximumSize(toolBarIconSize, toolBarIconSize);
        flipHButton->setText("");
        flipHButton->setToolTip(u8"Flip Horizontal");
        flipHButton->setStyleSheet(
            "QPushButton"
            "{"
            "image:url(:/CyMediaDis/ICONS/FlipH.png);"
            "}"
            "QPushButton::hover"
            "{"
            "image:url(:/CyMediaDis/ICONS/FlipH-hover.png);"
            "}"
        );
        connect(flipHButton, &QPushButton::clicked, view, &CyMediaDisView::hriMirror);

        flipVButton = new QPushButton(toolWidget);
        flipVButton->setMinimumSize(toolBarIconSize, toolBarIconSize);
        flipVButton->setMaximumSize(toolBarIconSize, toolBarIconSize);
        flipVButton->setText("");
        flipVButton->setToolTip(u8"Flip Vertical");
        flipVButton->setStyleSheet(
            "QPushButton"
            "{"
            "image:url(:/CyMediaDis/ICONS/FlipV.png);"
            "}"
            "QPushButton::hover"
            "{"
            "image:url(:/CyMediaDis/ICONS/FlipV-hover.png);"
            "}"
        );
        connect(flipVButton, &QPushButton::clicked, view, &CyMediaDisView::verMirror);

        QHBoxLayout* toolLayout = new QHBoxLayout(toolWidget);
        toolLayout->setContentsMargins(0, 0, 0, 0);
        toolLayout->addWidget(zoomInButton);
        toolLayout->addWidget(zoomOutButton);
        toolLayout->addWidget(zoomRawButton);
        toolLayout->addWidget(zoomFitButton);
        toolLayout->addWidget(flipHButton);
        toolLayout->addWidget(flipVButton);
    }

    void CyMediaDis::privateData::initItem() {
        //图像
        imageItem = new CyDMediaDisBack(view);
        imageItem->setFlag(QGraphicsLineItem::ItemIsMovable, false);
        imageItem->upImageData(m_imageinfo, 0);
        imageItem->setVisible(false);
        scene->setBackDis(imageItem);

        //图形管理
        drawmanager = new CyDisDrawItem::ItemManager(scene, m_parent);
        connect(drawmanager, &CyDisDrawItem::ItemManager::itemAdded, this, &CyMediaDis::privateData::onAddItem);
        connect(drawmanager, &CyDisDrawItem::ItemManager::itemRemoved, this, &CyMediaDis::privateData::onRemoveItem);

        //绘制工具
        drawingTool = new CyDisDrawItem::DrawItemTool(drawmanager, view, m_parent);

        //TipsWidget
        mRetimeItem = new CyMediaRecTimeW(m_parent);
        mRetimeItem->move(10, toolWidget->height());
        mRetimeItem->setVisible(false);


        mStretchWidget = new CyMediaDisGrayStretch(m_parent);
        mStretchWidget->setWindowFlag(Qt::Tool);
        mStretchWidget->resize(600, 300);
        mStretchWidget->setVisible(false);
        connect(mStretchWidget, &CyMediaDisGrayStretch::needImage, this, &CyMediaDis::privateData::onGrayToolNeedImage);
        connect(mStretchWidget, &CyMediaDisGrayStretch::stretchParaChange, this, [this]() {
            if (upDataIsSlow()) {
                QMutexLocker lock(&m_dataLock);
                imageItem->setStretchType(mStretchWidget->stretchtype());
                auto strytchpata = mStretchWidget->stretchValue();
                imageItem->setStreaChPara(strytchpata.start, strytchpata.end, strytchpata.max);
                imageItem->update();
            }
            });

        mGrayTestWidget = new CyMediaDisGrayTest(m_parent);
        mGrayTestWidget->setParentDis(m_parent);
        mGrayTestWidget->setWindowFlag(Qt::Tool);
        mGrayTestWidget->resize(600, 300);
        mGrayTestWidget->setVisible(false);
        connect(mGrayTestWidget, &CyMediaDisGrayTest::needImage, this, &CyMediaDis::privateData::onGrayToolNeedImage);
        connect(mGrayTestWidget, &CyMediaDisGrayTest::testModeChange, this, [this](int drawType) {
            setDrawMode(CyDisDrawItem::ItemType(drawType));
            if (drawType == CyDisDrawItem::Invalid) {
                //drawmanager->removeItem(mGrayTestWidget->getCurrentItem());
                onGrayToolNeedImage();
            }
            });
    }

    void CyMediaDis::privateData::initLayout() {
        QVBoxLayout* Layout = new QVBoxLayout(m_parent);
        Layout->setSpacing(0);
        Layout->setContentsMargins(0, 0, 0, 0);
        Layout->addWidget(toolWidget);
        Layout->addWidget(view);
        toolWidget->setVisible(true);
    }

    void CyMediaDis::privateData::ImageDataDoneReceive(bool InfoChange, CyMedia::ImageShowInfo info) {
        if (InfoChange) {
            scene->setSceneRect(QRect(0, 0, info.width, info.height));
            view->sceneRectUp(QRect(0, 0, info.width, info.height));
        }
        //检查线程是否退出
        if (false == bImageDataThread_flag) {
            log_printf("跳过图像(Receive)\n\r");
            return;
        }

        // 显示图像层
        if (false == imageItem->isVisible()) {
            imageItem->setVisible(true);
            scene->setTipTextVisible(true);
            log_printf("显示图像层\n\r");
        }

        // 显示滚动条
        if (bZoomScrollBarIsShow && view->horizontalScrollBarPolicy() != Qt::ScrollBarAsNeeded) {
            view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }

        scene->update();
        view->update();
        m_parent->update();
    }

    void CyMediaDis::privateData::onGrayToolNeedImage() {
        if (upDataIsSlow()) {
            bool upImage = (sender() == mStretchWidget);
            addOneGrayData(upImage, true);
        }
    }

    void CyMediaDis::privateData::onAddItem(QUuid id) {
        auto item = drawmanager->getItem(id);
        if (!item)
            return;
        if (mGrayTestWidget->isVisible()) {
            mGrayTestWidget->Itemdraw(item);
        }
        m_parent->emit itemDrawed(item);
    }

    void CyMediaDis::privateData::setDrawMode(CyDisDrawItem::ItemType mode) {
        drawingTool->setDrawMode(CyDisDrawItem::ItemType(mode));
        view->setDrawMode(mode != CyDisDrawItem::Invalid);
    }

    void CyMediaDis::privateData::onRemoveItem(QUuid id) {
        if (mGrayTestWidget->isVisible()) {
            mGrayTestWidget->ItemRemoved(id);
        }
    }





	//-----------------------------------------------------------------------------------------------
	//----------------------GetRawImageInfoDoalog
	class CyMediaDis_GetRawInfoDialog::PrivateData {
	public:
		//UI
		QLabel* uiFileNameLable = nullptr;
		QLabel* uiWidthTextLable = nullptr;
		QLabel* uiHeightTextLable = nullptr;
		QLabel* uiBitTextLable = nullptr;
		QLabel* uiChannelLable = nullptr;
		QLabel* uiPixeFormatLable = nullptr;
		QLabel* uiOffsetLable = nullptr;

		QSpinBox* uiWidthBox = nullptr;
		QSpinBox* uiHeightBox = nullptr;
		QSpinBox* uiBitBox = nullptr;
		QSpinBox* uiChannelBox = nullptr;
		QComboBox* uiPixelFormatBox = nullptr;
		QSpinBox* uiOffsetBox = nullptr;

		QPushButton* uiOkBtn = nullptr;
		QPushButton* uiCancleBtn = nullptr;

		CyMedia::eLanguage m_language;
	};
    CyMediaDis_GetRawInfoDialog::CyMediaDis_GetRawInfoDialog(CyMedia::eLanguage language/* = CyMedia::CHINESE*/, QWidget* parent /*= nullptr*/)
        :QDialog(parent)
        , p_data(new CyMediaDis_GetRawInfoDialog::PrivateData) {
        p_data->m_language = language;
        initGUI(true);
    }

    QString CyMediaDis_GetRawInfoDialog::openFileName()
    {
        return p_data->uiFileNameLable->text();
    }

    quint32 CyMediaDis_GetRawInfoDialog::imageWidth()
    {
        return p_data->uiWidthBox->value();
    }

    quint32 CyMediaDis_GetRawInfoDialog::imageHeight()
    {
        return p_data->uiHeightBox->value();
    }

    quint32 CyMediaDis_GetRawInfoDialog::imagenBit()
    {
        return p_data->uiBitBox->value();
    }

    quint32 CyMediaDis_GetRawInfoDialog::imageColorChannels()
    {
        return p_data->uiChannelBox->value();
    }

    CyMedia::ePixType CyMediaDis_GetRawInfoDialog::imagePixelType()
    {
        return CyMedia::ePixType(p_data->uiPixelFormatBox->currentIndex());
    }

    quint32 CyMediaDis_GetRawInfoDialog::imageOffset()
    {
        return p_data->uiOffsetBox->value();
    }

    void CyMediaDis_GetRawInfoDialog::setLanguage(CyMedia::eLanguage language)
    {
        p_data->m_language = language;
        initGUI();
    }

    void CyMediaDis_GetRawInfoDialog::setOpenFileName(QString name)
    {
        p_data->uiFileNameLable->setText(name);
    }

    void CyMediaDis_GetRawInfoDialog::setOpenInfo(quint32 w, quint32 h, quint32 bit, CyMedia::ePixType pixelType, quint32 ch, quint32 offset)
    {
        p_data->uiWidthBox->setValue(w);
        p_data->uiHeightBox->setValue(h);
        p_data->uiBitBox->setValue(bit);
        p_data->uiChannelBox->setValue(ch);
        p_data->uiPixelFormatBox->setCurrentIndex(int(pixelType));
        p_data->uiOffsetBox->setValue(offset);
    }

    void CyMediaDis_GetRawInfoDialog::initGUI(bool first/* = false*/)
    {
        if (first) {
            /*QFont mainFont;
            mainFont.setFamily("Microsoft YaHei UI");
            mainFont.setPointSize(10);
            this->setFont(mainFont);*/

            p_data->uiFileNameLable = new QLabel(this);
            p_data->uiWidthTextLable = new QLabel(this);
            p_data->uiHeightTextLable = new QLabel(this);
            p_data->uiBitTextLable = new QLabel(this);
            p_data->uiChannelLable = new QLabel(this);
            p_data->uiPixeFormatLable = new QLabel(this);
            p_data->uiOffsetLable = new QLabel(this);

            p_data->uiWidthBox = new QSpinBox(this);
            p_data->uiWidthBox->setRange(2, 20000);
            p_data->uiWidthBox->setValue(1000);
            p_data->uiHeightBox = new QSpinBox(this);
            p_data->uiHeightBox->setRange(2, 20000);
            p_data->uiHeightBox->setValue(1000);
            p_data->uiBitBox = new QSpinBox(this);
            p_data->uiBitBox->setRange(1, 32);
            p_data->uiBitBox->setValue(8);
            p_data->uiChannelBox = new QSpinBox(this);
            p_data->uiChannelBox->setRange(1, 4);
            p_data->uiChannelBox->setValue(1);
            p_data->uiPixelFormatBox = new QComboBox(this);
            int pixelTypeI = int(CyMedia::MONO);
            for (; pixelTypeI < int(CyMedia::MONO_OVERSIZE); pixelTypeI++) {
                p_data->uiPixelFormatBox->addItem(CyMediaDis::pixelFormatStr(CyMedia::ePixType(pixelTypeI)));
            }
            /*p_data->uiPixelFormatBox->setStyleSheet(
                "QComboBox{"
                "   font-family:Microsoft YaHei UI;"
                "   font-size:10px;"
                "   font-weight:Regular;"
                "}"
                "QComboBox QAbstractItemView {"
                "   outline:0px solid gray;"
                "   border:2px solid #286477;"
                "   background-color: white;"
                "   selection-background-color:#2aa3c6;"
                "}");*/

            p_data->uiOffsetBox = new QSpinBox(this);
            p_data->uiOffsetBox->setRange(0, 0x7EFFFFFF);
            p_data->uiOffsetBox->setValue(0);

            p_data->uiOkBtn = new QPushButton(this);
            p_data->uiCancleBtn = new QPushButton(this);
            p_data->uiFileNameLable->setText("None");
            p_data->uiFileNameLable->setAlignment(Qt::AlignHCenter);

            QWidget* formWidget = new QWidget(this);
            auto formLayout = new QFormLayout(formWidget);
            quint32 rowNum = 0;
            formLayout->setWidget(rowNum, QFormLayout::LabelRole, p_data->uiWidthTextLable);
            formLayout->setWidget(rowNum, QFormLayout::FieldRole, p_data->uiWidthBox);
            rowNum++;
            formLayout->setWidget(rowNum, QFormLayout::LabelRole, p_data->uiHeightTextLable);
            formLayout->setWidget(rowNum, QFormLayout::FieldRole, p_data->uiHeightBox);
            rowNum++;
            formLayout->setWidget(rowNum, QFormLayout::LabelRole, p_data->uiPixeFormatLable);
            formLayout->setWidget(rowNum, QFormLayout::FieldRole, p_data->uiPixelFormatBox);
            rowNum++;
            formLayout->setWidget(rowNum, QFormLayout::LabelRole, p_data->uiBitTextLable);
            formLayout->setWidget(rowNum, QFormLayout::FieldRole, p_data->uiBitBox);
            rowNum++;
            formLayout->setWidget(rowNum, QFormLayout::LabelRole, p_data->uiChannelLable);
            formLayout->setWidget(rowNum, QFormLayout::FieldRole, p_data->uiChannelBox);
            rowNum++;
            formLayout->setWidget(rowNum, QFormLayout::LabelRole, p_data->uiOffsetLable);
            formLayout->setWidget(rowNum, QFormLayout::FieldRole, p_data->uiOffsetBox);

            QWidget* btnWidget = new QWidget(this);
            QHBoxLayout* btnLayout = new QHBoxLayout(btnWidget);
            btnLayout->addWidget(p_data->uiOkBtn);
            btnLayout->addWidget(p_data->uiCancleBtn);

            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->addWidget(p_data->uiFileNameLable);
            mainLayout->addWidget(formWidget);
            mainLayout->addWidget(btnWidget);

            connect(p_data->uiOkBtn, &QPushButton::clicked, this, &CyMediaDis_GetRawInfoDialog::onOkClicked);
            connect(p_data->uiCancleBtn, &QPushButton::clicked, this, &CyMediaDis_GetRawInfoDialog::onCanCelClicked);
            connect(p_data->uiPixelFormatBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](int index) {
                    switch (CyMedia::ePixType(index)) {
                    case CyMedia::MONO:
                    case CyMedia::BAYERGR:
                    case CyMedia::BAYERBG:
                    case CyMedia::BAYERGB:
                    case CyMedia::BAYERRG:
                    case CyMedia::MONO_OVERSIZE: {
                        p_data->uiChannelBox->setValue(1);
                    }
                                                    break;

                    case CyMedia::RGB: {
                        p_data->uiChannelBox->setValue(3);
                    }
                                          break;

                    case CyMedia::RGBA: {
                        p_data->uiChannelBox->setValue(4);
                    }
                                           break;

                    case CyMedia::MONO10P:
                    case CyMedia::MONO10P_GVSP: {
                        p_data->uiChannelBox->setValue(1);
                        p_data->uiBitBox->setValue(10);
                    }
                                                   break;

                    case CyMedia::MONO12P:
                    case CyMedia::MONO12P_GVSP: {
                        p_data->uiChannelBox->setValue(1);
                        p_data->uiBitBox->setValue(12);
                    }
                                                   break;
                    }
                });
        }

        switch (p_data->m_language)
        {
        case CyMedia::ENGLISH: {
            p_data->uiWidthTextLable->setText(u8"Image width(number of columns)");
            p_data->uiHeightTextLable->setText(u8"Image height(number of line)");
            p_data->uiBitTextLable->setText(u8"Bit width");
            p_data->uiChannelLable->setText(u8"Number of channels");
            p_data->uiPixeFormatLable->setText(u8"Pixel format");
            p_data->uiOffsetLable->setText(u8"Data head");

            p_data->uiOkBtn->setText(u8"confirm");
            p_data->uiCancleBtn->setText(u8"cancle");
            break;
        }
        case CyMedia::CHINESE: {
            p_data->uiWidthTextLable->setText(u8"图像宽度(列数)");
            p_data->uiHeightTextLable->setText(u8"图像高(行数)");
            p_data->uiBitTextLable->setText(u8"位宽");
            p_data->uiChannelLable->setText(u8"通道数");
            p_data->uiPixeFormatLable->setText(u8"像素格式");
            p_data->uiOffsetLable->setText(u8"数据头");

            p_data->uiOkBtn->setText(u8"确认");
            p_data->uiCancleBtn->setText(u8"取消");
            break;
        }
        }
    }

    void CyMediaDis_GetRawInfoDialog::onOkClicked()
    {
        this->accept();
    }

    void CyMediaDis_GetRawInfoDialog::onCanCelClicked()
    {
        this->reject();
    }

}

#include "CyMediaDis.moc"
