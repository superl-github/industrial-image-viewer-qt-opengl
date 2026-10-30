#include "Display.h"

#include "common/CyMediaLog.h"
#include "common/CyMediaI18n.h"
#include "View/Pipeline/ImagePipeline.h"
#include "View/Render/CyDMediaDisScen.h"
#include "View/Render/CyMediaDisView.h"
#include "View/Render/CyMediaDisViewBckDraw.h"
#include "View/Ui/DisplayToolBar.h"
#include "View/Ui/ImageShowWindow.h"
#include "View/Widgets/CyMediaDisGrayStretch.h"
#include "View/Widgets/CyMediaDisGrayTest.h"
#include "View/Widgets/CyMediaRecTimeW.h"

#include "View/Annotation/DrawItem.h"
#include "View/Annotation/DrawItemTool.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QThread>
#include <QTimer>
#include <QTranslator>
#include <QVBoxLayout>

#include <atomic>

namespace CyMedia {

/**
    * @brief CyMediaDis 的 UI 适配器与控制器。
    *
    * @details
    *  - 只负责"UI 组合 + 渲染 + 交互"；
    *  - 帧缓存、处理线程、格式转换、像素采样全部委托给 `ImagePipeline`；
    *  - 作为 `ImagePipeline::Sink`，在工作线程接收处理结果并上传 OpenGL 纹理。
    */
class CyMediaDis::privateData : public QObject, public ImagePipeline::Sink {
    Q_OBJECT

public:
    explicit privateData(CyMediaDis* parent = nullptr);
    ~privateData() override;

signals:
    void ImageInfoChange(CyMedia::ImageShowInfo info);
    void ImageDataDone(CyMedia::ImageShowInfo info);

public:
    //==================== ImagePipeline::Sink ====================
    bool isReady() const override;
    void onFrame(const PipelineFrame& frame) override;
    void onNeedImage() override;

    void initGUI();
    void syncOptions();

    CyDisDrawItem::ItemType drawMode();
    void setDrawMode(CyDisDrawItem::ItemType mode);

private:
    void initScene();
    void initToolBar();
    void initItem();
    void initLayout();

    void ensureFrameContext();
    bool shouldUpdateTools();
    void updateTools(const PipelineFrame& frame);

    void ImageInfoChangeReceiver(CyMedia::ImageShowInfo info);
    void ImageDataDoneReceive(CyMedia::ImageShowInfo info);

    void onLanguageChanged(CyMedia::eLanguage lang);

    void onAddItem(QUuid id);
    void onRemoveItem(QUuid id);
    void onGrayToolNeedImage();

private:
    bool guiIsInit = false;

public:
    //===== UI / Item =====
    QColor mThemeColor = QColor(0x2a, 0xa3, 0xc6);
    CyMediaDis* m_parent = nullptr;
    CyMediaDisView* view = nullptr;
    CyDMediaDisScen* scene = nullptr;
    CyDisDrawItem::ItemManager* drawmanager = nullptr;
    CyDisDrawItem::DrawItemTool* drawingTool = nullptr;

    //===== 图像管线 =====
    ImagePipeline* pipeline = nullptr;
    ImageShowInfo m_lastInfo;              ///< 最近一次显示/处理的信息
    QOpenGLContext* m_frameCtx = nullptr;  ///< 工作线程共享上下文
    QThread* m_frameCtxThread = nullptr;   ///< 上下文所属线程
    std::atomic<int32_t> posX{ 0 };
    std::atomic<int32_t> posY{ 0 };
    std::atomic<bool> m_bFirstUpImage{ true };
    QElapsedTimer calcolorTimer;
    QElapsedTimer clearImageTime;

    //===== UI / Tool =====
    const QString m_IconPathPrefixStr;
    DisplayToolBar* toolWidget = nullptr;
    bool bZoomScrollBarIsShow = false;

    QElapsedTimer m_ToolWUpTimer;
    bool m_AutoThumbnail = false;
    QSize m_AutoThumbnailSize = QSize(500, 500);

    CyMediaRecTimeW* m_RetimeItem = nullptr;
    CyMediaDisGrayStretch* m_StretchWidget = nullptr;
    CyMedia::StretchType lastUpStretchType = CyMedia::stretch_None;
    CyMediaDisGrayTest* mGrayTestWidget = nullptr;
};

QWidget* CyMediaDis::imgShow(CyMedia::ImageShowInfo info, void* data, const QString& title /*= {}*/, QWidget* parent /*= nullptr*/) {
    auto* w = new CyMedia::ImageShowWindow();
    w->upImageData(info, data);
    w->setWindowTitle(title);
    return w;
}

CyMediaDis::CyMediaDis(QWidget* parent /* = nullptr */)
    : QFrame(parent)
    , d(new CyMediaDis::privateData(this)) {
    d->initGUI();
    setDrawMode(CyDisDrawItem::Invalid);
    setThemeColor(d->mThemeColor);
    setContentsMargins(0, 0, 0, 0);
    setMouseTracking(true);

    qRegisterMetaType<CyMedia::ImageShowInfo>("CyMedia::ImageShowInfo");
}

CyMediaDis::~CyMediaDis() {
    if (d) {
        delete d;
        d = nullptr;
    }
}

bool CyMediaDis::supportsOpenGL(int& mainV, int& subV) {
    QOpenGLContext ctx;
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    ctx.setFormat(fmt);

    QOffscreenSurface surface;
    surface.create();
    if (!ctx.create()) {
        mainV = 0;
        subV = 0;
        return false;
    }

    ctx.makeCurrent(&surface);
    auto ver = ctx.format();
    mainV = ver.majorVersion();
    subV = ver.minorVersion();
    ctx.doneCurrent();
    return true;
}

bool CyMediaDis::supportsOpenGLForCyMedia() {
    int mainV, subV;
    if (!supportsOpenGL(mainV, subV)) return false;
    return (mainV > 3) || (mainV == 3 && subV >= 3);
}

QString CyMediaDis::pixelFormatStr(CyMedia::ePixType format) {
    switch (format) {
        case CyMedia::MONO: return QString("MONO");
        case CyMedia::MONO10P: return QString("MONO10P");
        case CyMedia::MONO12P: return QString("MONO12P");
        case CyMedia::MONO12P_GVSP: return QString("MONO12P_GVSP");
        case CyMedia::MONO_OVERSIZE: return QString("CMONO_OVERSIZE");
        case CyMedia::RGB: return QString("RGB");
        case CyMedia::RGBA: return QString("RGBA");
        case CyMedia::BGR: return QString("BGR");
        case CyMedia::BAYERGR: return QString("BAYER GRBG");
        case CyMedia::BAYERBG: return QString("BAYER BGGR");
        case CyMedia::BAYERGB: return QString("BAYER GBRG");
        case CyMedia::BAYERRG: return QString("BAYER RGGB");
        case CyMedia::FOURCC_YUY2: return QString("YUYV422");
        case CyMedia::FOURCC_YVYU: return QString("YVYU422");
        case CyMedia::FOURCC_I422: return QString("YUV422P");
        case CyMedia::FOURCC_YV16: return QString("YVU422P");
        case CyMedia::FOURCC_I420: return QString("YUV420P");
        case CyMedia::FOURCC_YV12: return QString("YVU420P");
        case CyMedia::FOURCC_NV12: return QString("YUV420SP");
        case CyMedia::FOURCC_NV21: return QString("YVU420SP");
        case CyMedia::MJPG: return QString("JPG");
        default: return QString("UNKNOW");
    }
}

CyMedia::eLanguage CyMediaDis::currentLanguage() const {
    return CyMediaI18n::instance()->language();
}

bool CyMediaDis::setLanguage(CyMedia::eLanguage lang) {
    return CyMediaI18n::instance()->setLanguage(lang);
}

void CyMediaDis::setLogLevel(CyMedia::LogLevel level) {
    return CyMediaLog::instance().setLevel(level);
}

void CyMediaDis::setLogCallback(CyMedia::LogCallback cb, void* pUser /*= nullptr*/) {
    CyMediaLog::instance().setLogCallback(cb, pUser);
}

void CyMediaDis::setSceneAcceptDrop(bool accept) {
    d->scene->setAcceptDrops(accept);
}

void CyMediaDis::setImageStackNum(uint32_t num) {
    d->pipeline->setStackSize(num);
}

bool CyMediaDis::upImageData(const CyMedia::ImageShowInfo& info, void* data, bool force /*= false*/) {
    // 清空后的短暂窗口内拒绝新帧，避免"空图"状态被立刻覆盖。
    if (d->clearImageTime.isValid() && d->clearImageTime.elapsed() < 300) {
        return false;
    }
    return d->pipeline->submit(info, data, force);
}

bool CyMediaDis::isDirectMode() {
    return d->pipeline->direct();
}

void CyMediaDis::setDirectUpImage(bool flag) {
    d->pipeline->setDirect(flag);
}

void CyMediaDis::registerImageCallBack(CyMedia::ImageFrameCallback func, void* pUser) {
    d->pipeline->setImageCallback(func, pUser);
}

bool CyMediaDis::hasImage(void) const {
    return d->pipeline->hasImage();
}

void CyMediaDis::clearImage(void) {
    if (!d->pipeline->hasImage()) return;

    d->pipeline->clear();
    d->view->clearBackGround();
    d->m_bFirstUpImage = true;
    d->m_lastInfo = ImageShowInfo();
    d->view->zoomRaw(true);
    if (d->view->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff) {
        d->view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        d->view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    d->setDrawMode(CyDisDrawItem::Invalid);
    d->clearImageTime.start();
}


CyMedia::ImageShowInfo CyMediaDis::imageInfoCopy() const {
    return d->pipeline->imageInfo();
}

double CyMediaDis::displayFps(void) const {
    return d->view->imageDraw()->flushFps();
}


double CyMediaDis::dataFps() const {
    return d->pipeline->dataFps();
}


bool CyMediaDis::isDataSlow() const {
    return d->pipeline->isDataSlow();
}

CyMedia::StretchType CyMediaDis::stretchType() {
    return d->view->imageDraw()->stretchType();
}

void CyMediaDis::setStretchType(CyMedia::StretchType type) {
    d->view->imageDraw()->setStretchType(type);
    d->syncOptions();
    QMetaObject::invokeMethod(d->view->viewport(), "update", Qt::QueuedConnection);
}

void CyMediaDis::setStretchPara(uint32_t start /*= 0*/, uint32_t end /*= 0*/) {
    const int bit = d->pipeline->imageInfo().bit;
    const uint32_t max = (bit <= 0) ? 0u
        : (bit >= 31 ? 0x7FFFFFFFu : ((1u << bit) - 1u));
    d->view->imageDraw()->setStreaChPara(start, end, max);
    d->syncOptions();
}

CyMedia::DemosaicingMethod CyMediaDis::Demosaic() {
    return d->view->imageDraw()->Demosaic();
}

void CyMediaDis::setDemosaic(CyMedia::DemosaicingMethod method) {
    if (d->view->imageDraw()->Demosaic() == method) return;
    d->view->imageDraw()->setDemosaic(method);
    d->syncOptions();
    if (d->pipeline->isDataSlow() && hasImage()) {
        d->pipeline->requestReprocess(false);
        QMetaObject::invokeMethod(d->view->viewport(), "update", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->view->thumnailWidget(), "update", Qt::QueuedConnection);
    }
}

CyMedia::YUVTransMethod CyMediaDis::YUVMethod() {
    return d->view->imageDraw()->yuvMethod();
}

void CyMediaDis::setYUVMethod(CyMedia::YUVTransMethod method) {
    if (d->view->imageDraw()->yuvMethod() == method) return;
    d->view->imageDraw()->setYUVTMethod(method);
    d->syncOptions();
    if (d->pipeline->isDataSlow() && hasImage()) {
        d->pipeline->requestReprocess(false);
        QMetaObject::invokeMethod(d->view->viewport(), "update", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->view->thumnailWidget(), "update", Qt::QueuedConnection);
    }
}

QStringList CyMediaDis::ColorMapList() const {
    return d->view->imageDraw()->ColorMapList();
}

quint32 CyMediaDis::colorMapIndex() const {
    return d->view->imageDraw()->colorMapIndex();
}

bool CyMediaDis::setColorMap(quint32 index) {
    if (d->view->imageDraw()->colorMapIndex() == index) return true;
    bool ok = d->view->imageDraw()->setColorMap(index);
    if (ok && d->pipeline->isDataSlow()) {
        QMetaObject::invokeMethod(d->view->viewport(), "update", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->view->thumnailWidget(), "update", Qt::QueuedConnection);
    }
    return ok;
}

bool CyMediaDis::setColorMap(const QString& mapName) {
    if (d->view->imageDraw()->colorMapName() == mapName) return true;
    bool ok = d->view->imageDraw()->setColorMap(mapName);
    if (ok && d->pipeline->isDataSlow()) {
        QMetaObject::invokeMethod(d->view->viewport(), "update", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->view->thumnailWidget(), "update", Qt::QueuedConnection);
    }
    return ok;
}

void CyMediaDis::zoomIn() {
    d->view->zoomIn();
}

void CyMediaDis::zoomOut() {
    d->view->zoomOut();
}

void CyMediaDis::zoomAuto() {
    if (!d->pipeline->hasImage()) return;
    if (d->pipeline->isDataSlow()) {
        d->view->zoomAuto();
    }
    else {
        QTimer::singleShot(0, d->view, &CyMediaDisView::zoomAuto);
    }
}

void CyMediaDis::zoomRaw(bool reset) {
    d->view->zoomRaw(reset);
}

CyDisDrawItem::ItemType CyMediaDis::drawMode() {
    return d->drawMode();
}

void CyMediaDis::setDrawMode(CyDisDrawItem::ItemType mode) {
    d->setDrawMode(mode);
}

void CyMediaDis::setThemeColor(const QColor color) {
    d->mGrayTestWidget->setThemeColor(color);
    d->m_StretchWidget->setThemeColor(color);
    d->drawingTool->setThemeColor(color);
}

QUuid CyMediaDis::addItem(CyDisDrawItem::ItemType itemType) {
    return d->drawmanager->addItemByType(itemType);
}

QUuid CyMediaDis::addItem(CyDisDrawItem::ItemType itemType, const QPainterPath& path) {
    return d->drawmanager->addItemByTypeWidthPath(itemType, path);
}

void CyMediaDis::removeItem(QUuid id) {
    d->drawmanager->removeItem(id, true);
}

int CyMediaDis::itemCount() {
    return d->drawmanager->itemCount();
}

QList<CyDisDrawItem::BaseItem*> CyMediaDis::items() const {
    return d->drawmanager->items();
}

CyDisDrawItem::BaseItem* CyMediaDis::getItem(const QUuid& id) {
    return d->drawmanager->getItem(id);
}

void CyMediaDis::setItemSelected(QUuid id) {
    auto* item = getItem(id);
    if (item) item->setSelected(true);
}

void CyMediaDis::clearItem() {
    d->drawmanager->clearAll();
}

bool CyMediaDis::isSingleItemMode() {
    return d->drawingTool->replaceMode();
}

void CyMediaDis::setSingleItemMode(bool flag) {
    d->drawingTool->setReplaceMode(flag);
}

QUuid CyMediaDis::getLastItem() {
    return d->drawmanager->getLastItem();
}

bool CyMediaDis::isDrawing() {
    return d->drawingTool && d->drawingTool->isDrawing();
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
    if (d->bZoomScrollBarIsShow == show) return;
    if (!show) {
        if (d->view->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff) {
            d->view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            d->view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
    d->bZoomScrollBarIsShow = show;
}

bool CyMediaDis::thumbnailEnable() {
    return d->view->thumbnailEnable();
}

void CyMediaDis::setThumbnailEnable(bool enable) {
    if (d->m_AutoThumbnail) return;
    d->view->setThumbnailEnable(enable);
    d->syncOptions();
}

bool CyMediaDis::thumbnailAutoEnable() {
    return d->m_AutoThumbnail;
}

void CyMediaDis::setThumbnailAutoEnable(bool enable) {
    d->m_AutoThumbnail = enable;
    d->syncOptions();
    if (d->pipeline->isDataSlow()) {
        d->pipeline->requestReprocess(true, false);
    }
}

QSize CyMediaDis::thumbnailAutoEnableSize() {
    return d->m_AutoThumbnailSize;
}

void CyMediaDis::setThumbnailAutoEnableSize(const QSize size) {
    d->m_AutoThumbnailSize = size;
    d->syncOptions();
    if (d->pipeline->isDataSlow()) {
        d->pipeline->requestReprocess(true, false);
    }
}

bool CyMediaDis::recTimeVisible() {
    return d->m_RetimeItem->isVisible();
}

void CyMediaDis::setRecTimeVisible(bool visi) {
    if (visi) {
        d->m_RetimeItem->raise();
        d->m_RetimeItem->move(10, d->toolWidget->height());
    }
    d->m_RetimeItem->setVisible(visi);
}

void CyMediaDis::upRecTime(uint64_t time) {
    d->m_RetimeItem->upRecTime(time);
}

void CyMediaDis::upRecTime(float saved, float sum) {
    d->m_RetimeItem->upRecTime(saved, sum);
}

void CyMediaDis::upRecTime_Timed(uint64_t saved, uint64_t sum) {
    d->m_RetimeItem->upRecTime_Timed(saved, sum);
}

QWidget* CyMediaDis::stretchWidget() {
    return d->m_StretchWidget;
}

void CyMediaDis::setGrayStretchVisible(bool visible) {
    d->m_StretchWidget->setVisible(visible);
    d->syncOptions();
}

QWidget* CyMediaDis::grayTestWidget() {
    return d->mGrayTestWidget;
}

void CyMediaDis::setGrayTestVisible(bool visible) {
    d->mGrayTestWidget->setVisible(visible);
}

//========================================================================================
// privateData
//========================================================================================

CyMediaDis::privateData::privateData(CyMediaDis* parent /*= nullptr*/)
    : QObject(parent)
    , m_parent(parent)
    , m_IconPathPrefixStr(":/resources/ICONS/") {
    pipeline = new ImagePipeline(this);
    CyMedia::CyMediaI18n::instance()->setTranslatorPath(":/resources/translate/");
    CyMedia::CyMediaI18n::instance()->setLanguage(CyMedia::CHINESE);

    connect(this, &privateData::ImageInfoChange, 
        this, &privateData::ImageInfoChangeReceiver);
    connect(this, &privateData::ImageDataDone, 
        this, &privateData::ImageDataDoneReceive);

    // 订阅应用级语言变更
    connect(CyMediaI18n::instance(), &CyMediaI18n::languageChanged,
        this, &privateData::onLanguageChanged);
}

CyMediaDis::privateData::~privateData() {
    if (pipeline) {
        pipeline->stop();
        delete pipeline;
        pipeline = nullptr;
    }
    if (m_frameCtx) {
        delete m_frameCtx;
        m_frameCtx = nullptr;
    }

    // scene/view/draw 子系统由本类显式持有。
    delete drawingTool;
    drawingTool = nullptr;
    delete drawmanager;
    drawmanager = nullptr;
    delete scene;
    scene = nullptr;
    delete view;
    view = nullptr;
}

void CyMediaDis::privateData::initGUI() {
    if (guiIsInit) return;
    guiIsInit = true;

    initScene();
    initToolBar();
    initItem();
    initLayout();
    syncOptions();
}

void CyMediaDis::privateData::initScene() {
    scene = new CyDMediaDisScen(0, 0, 1000, 1000);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);

    connect(scene, &CyDMediaDisScen::mousePosChange, 
        this, [this](int x, int y) {
        posX = x;
        posY = y;
        if (!pipeline->isDataSlow()) return;

        static QElapsedTimer moveTimer;
        if (moveTimer.isValid() && moveTimer.elapsed() < 200) return;
        moveTimer.restart();

        double r = 0, g = 0, b = 0;
        bool gray = false;
        if (!pipeline->samplePixel(x, y, &r, &g, &b, &gray)) return;
        m_parent->emit upPosPix(x, y, r, g, b, gray);
        });

    connect(scene, &CyDMediaDisScen::urlsDrop, 
        this, [this](QList<QUrl> urls) {
        m_parent->emit urlsDrop(urls);
        });

    view = new CyMediaDisView(m_parent);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    view->setCyScene(scene);

    connect(view, &CyMediaDisView::onMousePress, 
        m_parent, [this]() {
        m_parent->emit pressOnView();
        });
    connect(view, &CyMediaDisView::onMouseDoubleClick, 
        m_parent, [this]() {
        m_parent->emit DoubleClickOnView();
        });
    connect(view, &CyMediaDisView::zoomValueChange,
        m_parent, &CyMediaDis::zoomValueChange);
}

void CyMediaDis::privateData::initToolBar() {
    toolWidget = new DisplayToolBar(m_IconPathPrefixStr, m_parent);
    connect(toolWidget, &DisplayToolBar::zoomInRequested, view, &CyMediaDisView::zoomIn);
    connect(toolWidget, &DisplayToolBar::zoomOutRequested, view, &CyMediaDisView::zoomOut);
    connect(toolWidget, &DisplayToolBar::zoomRawRequested, view, [this]() { view->zoomRaw(); });
    connect(toolWidget, &DisplayToolBar::zoomFitRequested, view, &CyMediaDisView::zoomAuto);
    connect(toolWidget, &DisplayToolBar::flipHorizontalRequested, view, &CyMediaDisView::horiMirror);
    connect(toolWidget, &DisplayToolBar::flipVerticalRequested, view, &CyMediaDisView::verMirror);
}

void CyMediaDis::privateData::initItem() {
    drawmanager = new CyDisDrawItem::ItemManager(scene, m_parent);
    connect(drawmanager, &CyDisDrawItem::ItemManager::itemAdded,
        this, &CyMediaDis::privateData::onAddItem);
    connect(drawmanager, &CyDisDrawItem::ItemManager::itemRemoved,
        this, &CyMediaDis::privateData::onRemoveItem);
    connect(drawmanager, &CyDisDrawItem::ItemManager::itemSelectionChanged,
        this, [this](QUuid id, bool selected) {
            if (selected) m_parent->emit itemSelected(id);
        });

    drawingTool = new CyDisDrawItem::DrawItemTool(drawmanager, view, m_parent);

    m_RetimeItem = new CyMediaRecTimeW(m_parent);
    m_RetimeItem->move(10, toolWidget->height());
    m_RetimeItem->setVisible(false);

    m_StretchWidget = new CyMediaDisGrayStretch(m_parent);
    m_StretchWidget->setWindowFlag(Qt::Tool);
    m_StretchWidget->resize(600, 300);
    m_StretchWidget->setVisible(false);
    connect(m_StretchWidget, &CyMediaDisGrayStretch::needImage,
        this, &CyMediaDis::privateData::onGrayToolNeedImage);
    connect(m_StretchWidget, &CyMediaDisGrayStretch::stretchParaChange, this, [this]() {
        if (!pipeline->isDataSlow()) return;
        view->imageDraw()->setStretchType(m_StretchWidget->stretchtype());
        auto p = m_StretchWidget->stretchValue();
        view->imageDraw()->setStreaChPara(p.start, p.end, p.max);
        view->viewport()->update();
        if (view->thumnailWidget()->isVisible()) view->thumnailWidget()->update();
        syncOptions();
        });

    mGrayTestWidget = new CyMediaDisGrayTest(m_parent);
    mGrayTestWidget->setParentDis(m_parent);
    mGrayTestWidget->setWindowFlag(Qt::Tool);
    mGrayTestWidget->resize(600, 300);
    mGrayTestWidget->setVisible(false);
    connect(mGrayTestWidget, &CyMediaDisGrayTest::needImage,
        this, &CyMediaDis::privateData::onGrayToolNeedImage);
    connect(mGrayTestWidget, &CyMediaDisGrayTest::testModeChange, this,
        [this](int drawType, CyMediaDisGrayTest::testModeChangeType emitType) {
            setDrawMode(CyDisDrawItem::ItemType(drawType));
            if (drawType == CyDisDrawItem::ItemType::Invalid) {
                drawmanager->removeItem(mGrayTestWidget->getCurrentItem());
            }
            switch (emitType) {
                case CyMediaDisGrayTest::ModeChange_normal: onGrayToolNeedImage(); break;
                case CyMediaDisGrayTest::ModeChange_hide:
                    drawmanager->removeItem(mGrayTestWidget->getCurrentItem());
                    break;
                case CyMediaDisGrayTest::QPainterPath_show: {
                    auto path = mGrayTestWidget->oldItemPpath();
                    if (path.elementCount()) {
                        drawmanager->addItemByTypeWidthPath(
                            CyDisDrawItem::ItemType(drawType), path);
                    }
                }break;
            }
        });

    syncOptions();
}

void CyMediaDis::privateData::initLayout() {
    auto* layout = new QVBoxLayout(m_parent);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(toolWidget);
    layout->addWidget(view);
    view->setVisible(true);
    toolWidget->setVisible(true);
}

void CyMediaDis::privateData::syncOptions() {
    if (!pipeline || !view || !m_StretchWidget) return;

    ImagePipeline::Options opt;
    opt.demosaic = view->imageDraw()->Demosaic();
    opt.yuv = view->imageDraw()->yuvMethod();
    opt.stretch = m_StretchWidget->stretchtype();
    const auto range = m_StretchWidget->stretchValue();
    opt.stretchRange = { range.start, range.end, range.max };
    opt.stretchPanelVisible = m_StretchWidget->isVisible();
    opt.thumbnailVisible = view->thumbnailVisible();
    opt.forceCpuBayer = false; // AHD 由 ImagePipeline 内部强制 CPU
    opt.forceCpuYuv = false;
    pipeline->setOptions(opt);
}

bool CyMediaDis::privateData::isReady() const {
    return view && view->glIsInit();
}

void CyMediaDis::privateData::onNeedImage() {
    m_parent->emit needUpImage();
}

void CyMediaDis::privateData::ensureFrameContext() {
    QThread* cur = QThread::currentThread();
    if (m_frameCtx && m_frameCtxThread == cur) return;

    if (m_frameCtx) {
        delete m_frameCtx;
        m_frameCtx = nullptr;
    }
    m_frameCtx = view->imageDraw()->createSharedContext();
    m_frameCtxThread = cur;
}

bool CyMediaDis::privateData::shouldUpdateTools() {
    if (pipeline->isDataSlow()) return true;
    if (!m_ToolWUpTimer.isValid()) {
        m_ToolWUpTimer.start();
        return true;
    }
    if (m_ToolWUpTimer.elapsed() > 500) {
        m_ToolWUpTimer.restart();
        return true;
    }
    return false;
}

void CyMediaDis::privateData::updateTools(const PipelineFrame& frame) {
    // 历史行为：拉伸/测试面板的数据更新在工作线程完成；面板只做计算并发信号。
    // 面板接口接收非 const info，这里用副本隔离，避免修改管线帧。
    ImageShowInfo info = frame.info;
    if (frame.updateStretch && frame.stretchEnabled) {
        m_StretchWidget->upImageData(info, frame.data, frame.colorOpe);
        lastUpStretchType = frame.colorOpe.stretchType;
    }
    mGrayTestWidget->upImage(info, frame.data, frame.colorOpe);
}

void CyMediaDis::privateData::onFrame(const PipelineFrame& frame) {
    if (!view) return;

    // 1) 图像尺寸/格式变化通知（在工作线程发出，Qt 自动排队到 UI 线程）。
    bool infoChanged = m_bFirstUpImage
        || m_lastInfo.width != frame.info.width
        || m_lastInfo.height != frame.info.height
        || m_lastInfo.bit != frame.info.bit
        || m_lastInfo.format != frame.info.format
        || m_lastInfo.length != frame.info.length
        || m_lastInfo.special_pixel != frame.info.special_pixel;
    if (infoChanged) {
        m_lastInfo = frame.info;
        m_bFirstUpImage = true;
        emit ImageInfoChange(frame.info);
    }

    // 2) 鼠标像素颜色（节流）。
    if (calcolorTimer.isValid()) {
        if (calcolorTimer.elapsed() > 200 || view->imageDraw()->flushFps() <= 5.0) {
            double r = 0, g = 0, b = 0;
            bool gray = false;
            if (pipeline->samplePixel(posX, posY, &r, &g, &b, &gray)) {
                m_parent->emit upPosPix(posX, posY, r, g, b, gray);
            }
            calcolorTimer.restart();
        }
    }
    else {
        calcolorTimer.start();
    }

    // 3) 辅助面板（帧率低时每帧，高帧率时节流到 2Hz）。
    if (shouldUpdateTools()) {
        updateTools(frame);
    }

    // 4) 上传纹理并请求刷新。
    if (frame.updateDisplay) {
        ensureFrameContext();
        if (!m_frameCtx) return;

        if (frame.stretchEnabled) {
            view->imageDraw()->setStretchType(frame.colorOpe.stretchType);
            view->imageDraw()->setStreaChPara(
                frame.stretch.start, frame.stretch.end, frame.stretch.max);
        }
        else {
            view->imageDraw()->setStretchType(CyMedia::stretch_None);
        }

        view->imageDraw()->upBackGround(frame.info, frame.data, m_frameCtx);
        emit ImageDataDone(frame.info);
    }
}

void CyMediaDis::privateData::ImageInfoChangeReceiver(CyMedia::ImageShowInfo info) {
    scene->setSceneRect(QRect(0, 0, info.width, info.height));
    view->sceneRectUp(QRect(0, 0, info.width, info.height));
    m_parent->emit imageSizeChanged(info.width, info.height, info.bit);
}

void CyMediaDis::privateData::ImageDataDoneReceive(CyMedia::ImageShowInfo info) {
    if (bZoomScrollBarIsShow && view->horizontalScrollBarPolicy() != Qt::ScrollBarAsNeeded) {
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    if (m_AutoThumbnail) {
        view->setThumbnailEnable(info.width >= m_AutoThumbnailSize.width()
            || info.height >= m_AutoThumbnailSize.height());
    }
    if (view->thumbnailVisible()) view->thumnailWidget()->update();
    view->viewport()->update();

    if (m_bFirstUpImage) {
        m_bFirstUpImage = false;
        m_parent->zoomAuto();
    }
    syncOptions();
}

void CyMediaDis::privateData::onLanguageChanged(CyMedia::eLanguage lang) {
    if (m_StretchWidget) m_StretchWidget->flushTrans();
    if (mGrayTestWidget) mGrayTestWidget->flushTrans();
    if (drawmanager)     drawmanager->flushTrans();
}

void CyMediaDis::privateData::onGrayToolNeedImage() {
    if (!pipeline->isDataSlow()) return;

    bool upImage = false;
    if (sender() == m_StretchWidget) {
        upImage = (m_StretchWidget->stretchtype() != lastUpStretchType)
            || m_StretchWidget->isAutoStretch();
    }
    syncOptions();
    pipeline->requestReprocess(upImage, true, true);
}

void CyMediaDis::privateData::onAddItem(QUuid id) {
    auto* item = drawmanager->getItem(id);
    if (!item) return;
    if (mGrayTestWidget->isVisible()) {
        mGrayTestWidget->Itemdraw(item);
    }
    m_parent->emit itemDrawed(id);
}

CyDisDrawItem::ItemType CyMediaDis::privateData::drawMode() {
    return drawingTool->draMode();
}

void CyMediaDis::privateData::setDrawMode(CyDisDrawItem::ItemType mode) {
    drawingTool->setDrawMode(mode);
    view->setDrawMode(mode != CyDisDrawItem::Invalid);
    m_parent->emit drawModeChange(mode);
}

void CyMediaDis::privateData::onRemoveItem(QUuid id) {
    if (mGrayTestWidget->isVisible()) {
        mGrayTestWidget->ItemRemoved(id);
    }
    m_parent->emit itemRemoved(id);
}

}

#include "Display.moc"
