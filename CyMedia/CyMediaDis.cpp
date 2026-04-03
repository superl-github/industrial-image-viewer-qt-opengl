#include "CyMediaDis.h"

#include "CyMediaDis/CyMediaDisView.h"
#include "CyMediaDis/CyDMediaDisScen.h"
#include "CyMediaDis/CyDMediaDisBack.h"
#include "CyMediaCalc/CyMediaCalc.h"
#include "CyMediaDis/drawItem/CyDisDrawItem.h"
#include "CyMediaDis/drawItem/ItemDrawTool.h"

#include <QElapsedTimer>

#include <queue>

#define debug_msg(fmt, ...) printf("[CyMediaDis(%d)  " fmt, __LINE__, ##__VA_ARGS__)
#define ImageStackMaxSize 3

class CyMediaDis::privateData : public QObject {
    Q_OBJECT

public:
    typedef struct oneFrameBuffer {
        CyMedia::ImageShowInfo info;                        // 图像信息
        unsigned char* pdata = nullptr;                     // 图像数据
        bool            bisUpData = false;                  // 是否是新数据需要更新
        bool            bIsSource = false;                  // 是否是模块存储的原始数据
        bool            bUpImage = true;                    // 是否更新显示图像
        bool            bIsAddFps = true;                   // 是否统计帧频
    }oneFrameBuffer_;

    typedef struct opeFrameThreadPara {
        QElapsedTimer dataFpsTimer;
        qint64 dataFpsTimerElapsed = 0;

        uint8_t* pAnalyImage = 0;
        uint32_t analyImageLen = 0;

        uint8_t* pAlignImage = 0;
        CyMedia::ImageShowInfo pAlignImage_info;

        bool openglOpeBayer = false;

        void init() {
            dataFpsTimer.invalidate();
            dataFpsTimerElapsed = 0;
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

    oneFrameBuffer* getBuffer(bool force = false);
    void upImageToBuffer(oneFrameBuffer* buffer, CyMedia::ImageShowInfo& info, uint8_t* data);
    bool addData(CyMedia::ImageShowInfo info, uint8_t* data, bool isSource = false, bool force = false);
    void addOneGrayData(bool upImage, bool force = false);
    void clearImage();
    bool upDataIsSlow();

    void stopImageOpeThread();

private:
    void Thread_ImageData();
    void Thread_ImageData_processOne(oneFrameBuffer& img, opeFrameThreadPara& opePara);
    void Thread_ImageData_SpecialOpe(
        CyMedia::ImageShowInfo& srcInfo, uint8_t* srcData, opeFrameThreadPara& opePara);
    void Thread_ImageData_UpDis(CyMedia::ImageShowInfo& info, uint8_t* data, opeFrameThreadPara& opePara, bool isSource);
    void ImageDataDoneReceive(bool InfoChange, CyMedia::ImageShowInfo info);

    void onDrawItem(CyDisDrawItem::BaseItem* item);
    void onGrayToolNeedImage();

private:
    void initScene();
    void initToolBar();
    void initItem();
    void initLayout();

public:
    QMutex  m_dataLock;
    bool m_bIsPrintDebug = false;

    //=====BaseUI/Item=====
    QColor mThemeColor = QColor(0x2a, 0xa3, 0xc6);
    CyMediaDis* m_parent = nullptr;
    CyMediaDisView* view = nullptr;
    CyDMediaDisScen* scene = nullptr;
    CyDMediaDisBack* imageItem = nullptr;
    CyDisDrawItem::ItemManager* drawmanager = nullptr;
    CyDisDrawItem::ItemDrawTool* drawingTool = nullptr;
    QThread* pImageDataThread = 0;        ///< 图像处理线程句柄
    bool     bImageDataThread_flag = true;   ///< 图像处理线程循环标志
    std::queue<oneFrameBuffer*>  threadPare_ImageDataStack;      ///< 图像线程所使用的数据栈
    oneFrameBuffer threadPare_ImageDataArray[ImageStackMaxSize];
    quint32 ImageStackMaxNum = ImageStackMaxSize;
    quint32 ImageDataStack_currentOpe = 0;

    //=====Image=====
    CyMedia::ImageShowInfo m_imageinfo;
    int32_t posX = 0;
    int32_t posY = 0;

    bool bHaveData = false;              ///< 当前是否有图像数据
    bool m_bFistUpImage = true;          ///< 是否首次更新图像

    bool m_bShowBayerSource = false;

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
    CyMediaDisGrayTest* mGrayTestWidget = nullptr;

private:
    bool guiIsInit = false;
};

CyMediaDis::CyMediaDis(QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , d(new CyMediaDis::privateData(this)){
    
    d->initGUI();
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

bool CyMediaDis::upImageData(CyMedia::ImageShowInfo info, uint8_t* data, bool force /*= false*/) {
    return d->addData(info, data, false, force);
}

bool CyMediaDis::haveDate(void) {
    return d->bHaveData;
}

void CyMediaDis::clearImage(void) {
    return d->clearImage();
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

void CyMediaDis::setThemeColor(QColor color) {
    d->mGrayTestWidget->setThemeColor(color);
    d->mStretchWidget->setThemeColor(color);
    d->drawingTool->setThemeColor(color);
}

CyDisDrawItem::BaseItem* CyMediaDis::getItem(QUuid& id) {
    return d->drawmanager->getItem(id);
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

CyMediaRecTimeW* CyMediaDis::rectimeItem() {
    return d->mRetimeItem;
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
            if (idx != currentOpe && threadPare_ImageDataArray[idx].bisUpData) {
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
        if (m_bIsPrintDebug) debug_msg("图像过大，拒绝: %u bytes\n", info.length);
        return false;
    }

    //确保NoImage不会被错误覆盖
    if (clearImageTime.isValid() && clearImageTime.elapsed() < 300) {
        if (m_bIsPrintDebug) {
            debug_msg("跳过图像(update)\n\r");
        }
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
        if (m_bIsPrintDebug) {
            debug_msg("启动图像数据处理线程, 数据栈最大缓存:%d\n\r", ImageStackMaxNum);
        }
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
    t_pBuffer->bIsSource = false;
    t_pBuffer->bUpImage = upImage;
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
        if (view->drawMode()) {
            view->setDrawMode(false);
        }
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
        Thread_ImageData_processOne(threadPare_ImageDataArray[targetIdx], opePara);
        //回收，清除状态
        QMutexLocker lock(&m_dataLock);
        threadPare_ImageDataArray[targetIdx].bisUpData = false;
    }
    QMutexLocker lock(&m_dataLock);
    ImageDataStack_currentOpe = -1;
    pImageDataThread->quit();
    bImageDataThread_flag = false;
    if (m_bIsPrintDebug) {
        debug_msg("图像处理线程退出\n\r");
    }
}
void CyMediaDis::privateData::Thread_ImageData_processOne(oneFrameBuffer& img, opeFrameThreadPara& opePara) {
    // 统计更新帧频
    if (opePara.dataFpsTimer.isValid())
        opePara.dataFpsTimer.start();
    if (nDataFpsCount >= 30 || opePara.dataFpsTimer.elapsed() >= 3000) {
        opePara.dataFpsTimerElapsed = opePara.dataFpsTimer.elapsed();
        DataFps = 1000.0 * nDataFpsCount / opePara.dataFpsTimerElapsed;
        //printf("DataFps:%llf  nDataFpsCount:%lld\n", DataFps, nDataFpsCount);
        nDataFpsCount = 0;
        opePara.dataFpsTimer.restart();
    }

    if (img.bIsAddFps) {
        nDataFpsCount++;
        img.bIsAddFps = false;
    }

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
    // 特殊格式处理
    Thread_ImageData_SpecialOpe(Imageinfo, Imagedata, opePara);

    // 检查是否退出
    if (false == bImageDataThread_flag) {
        return;
    }
    //更新状态
    if (false == bHaveData) {
        bHaveData = true;
        view->setImageShow(true);
    }
    //图像处理
    if (mStretchWidget->isVisible()) {
        mStretchWidget->upImageData(Imageinfo, Imagedata, imageItem->Demosaic());
    }
    if (mGrayTestWidget->isVisible()) {
        mGrayTestWidget->upImageData(Imageinfo, Imagedata);
    }
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
        && m_bShowBayerSource) {
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
            m_parent->emit upPosPix(posX, posY, r, g, b);
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
            m_parent->emit upPosPix(x, y, r, g, b);
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

    //绘制工具
    drawingTool = new CyDisDrawItem::ItemDrawTool(drawmanager, view, m_parent);
    drawingTool->setDrawMode(CyDisDrawItem::ItemType::Invalid);
    drawingTool->setDrawMode(CyDisDrawItem::ItemType::Ellipse);
    connect(drawingTool, &CyDisDrawItem::ItemDrawTool::drawItem, this, &CyMediaDis::privateData::onDrawItem);

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
    mGrayTestWidget->setWindowFlag(Qt::Tool);
    mGrayTestWidget->resize(600, 300);
    mGrayTestWidget->setVisible(false);
    connect(mGrayTestWidget, &CyMediaDisGrayTest::needImage, this, &CyMediaDis::privateData::onGrayToolNeedImage);
    connect(mGrayTestWidget, &CyMediaDisGrayTest::testModeChange, this, [this](int drawType) {
        drawingTool->setDrawMode(CyDisDrawItem::ItemType(drawType));
        view->setDrawMode(drawType != CyDisDrawItem::Invalid);
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
        view->setSceneRect(QRect(0, 0, info.width, info.height));
    }
    //检查线程是否退出
    if (false == bImageDataThread_flag) {
        if (m_bIsPrintDebug) {
            debug_msg("跳过图像(Receive)\n\r");
        }
        return;
    }

    // 显示图像层
    if (false == imageItem->isVisible()) {
        imageItem->setVisible(true);
        scene->setTipTextVisible(true);
        if (m_bIsPrintDebug) {
            debug_msg("显示图像层\n\r");
        }
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
        addOneGrayData(true);
    }
}

void CyMediaDis::privateData::onDrawItem(CyDisDrawItem::BaseItem* item) {
    if (mGrayTestWidget->isVisible()) {
        mGrayTestWidget->Itemdraw(item);
    }
}

#include "CyMediaDis.moc"
