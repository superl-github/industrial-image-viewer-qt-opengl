#include "Display.h"

#include "../common/CyMediaLog.h"
#include "../common/CyMediaI18n.h"
#include "Pipeline/ImagePipeline.h"
#include "Render/CyDMediaDisScen.h"
#include "Render/CyMediaDisView.h"
#include "Render/CyMediaDisViewBckDraw.h"
#include "Ui/DisplayToolBar.h"
#include "Ui/ImageShowWindow.h"
#include "Widgets/GrayStretch.h"
#include "Widgets/GrayMeasurement.h"
#include "Dialogs/recordindicatordialog.h"

#include "Annotation/DrawItem.h"
#include "Annotation/DrawItemTool.h"

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

namespace {
    int bitMaxFor(int bit) {
        if (bit <= 0) return 0;
        if (bit >= 31) return 0x7FFFFFFF;
        return (1 << bit) - 1;
    }
}

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
    /// @param sizeChanged 图像宽/高是否改变（首帧、切换分辨率）。
    ///        该参数随信号一起投递，避免接收端用共享标志产生竞态。
    void ImageInfoChange(CyMedia::ImageShowInfo info, bool sizeChanged);
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

public:
    void syncStretchPanelFromState();
    bool setStretchState(CyMedia::StretchType type,
        int start, int end, int max, bool notify);
    int stretchMaxForCurrentImage() const;

private:
    void initScene();
    void initToolBar();
    void initItem();
    void initLayout();

    void ensureFrameContext();
    bool shouldUpdateTools();
    void updateTools(const PipelineFrame& frame);

    void ImageInfoChangeReceiver(CyMedia::ImageShowInfo info, bool sizeChanged);
    void ImageDataDoneReceive(CyMedia::ImageShowInfo info);

    void onLanguageChanged(CyMedia::eLanguage lang);

    void onAddItem(QUuid id);
    void onRemoveItem(QUuid id);
    void onGrayToolNeedImage();

    void onStretchPanelChanged();
    bool updateStretchFromPanel();
    void updateStretchForImageInfo(const ImageShowInfo& info);
    void applyStretch(bool requestRepaint);

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
    /// 尺寸变化后，ImageDataDone 阶段需要补一次缩略图刷新（UI 线程使用）。
    bool m_pendingThumbRefresh = false;
    QElapsedTimer calcolorTimer;
    QElapsedTimer clearImageTime;

    //===== 拉伸：唯一权威状态，仅 UI 线程访问 =====
    CyMedia::StretchType m_stretchType = CyMedia::stretch_None;
    CyMedia::StretchRange m_stretchRange;   // start/end/max
    bool m_pendingStretch = false;          // 首帧前 setStretchPara 暂存
    int m_pendingStart = 0;
    int m_pendingEnd = 0;

    //===== UI / Tool =====
    const QString m_IconPathPrefixStr;
    DisplayToolBar* toolWidget = nullptr;
    bool bZoomScrollBarIsShow = false;

    const int m_upToolsTimeMs = 100;
    QElapsedTimer m_ToolWUpTimer;
    bool m_AutoThumbnail = false;
    QSize m_AutoThumbnailSize = QSize(500, 500);

    // 上传/上下文失败只在状态切换时记录，避免逐帧刷屏。
    std::atomic<bool> m_uploadFailed{ false };
    std::atomic<bool> m_frameCtxErrorLogged{ false };

    RecordIndicatorDialog* m_RetimeItem = nullptr;
    CyMediaDisGrayStretch* m_StretchWidget = nullptr;
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
    mainV = 0;
    subV = 0;

    QOpenGLContext ctx;
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    ctx.setFormat(fmt);

    QOffscreenSurface surface;
    surface.create();
    if (!surface.isValid()) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] OpenGL probe failed: offscreen surface creation failed");
        return false;
    }
    if (!ctx.create()) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] OpenGL probe failed: context creation failed");
        return false;
    }
    if (!ctx.makeCurrent(&surface)) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] OpenGL probe failed: makeCurrent failed");
        return false;
    }

    auto ver = ctx.format();
    mainV = ver.majorVersion();
    subV = ver.minorVersion();
    ctx.doneCurrent();
    CyMediaLog::instance().log_printf_Debug(
        "[Display] OpenGL %d.%d detected", mainV, subV);
    return true;
}

bool CyMediaDis::supportsOpenGLForCyMedia() {
    int mainV = 0, subV = 0;
    if (!supportsOpenGL(mainV, subV)) return false;
    const bool ok = (mainV > 3) || (mainV == 3 && subV >= 3);
    if (!ok) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] OpenGL %d.%d is below the required 3.3", mainV, subV);
    }
    return ok;
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
    const bool ok = CyMediaI18n::instance()->setLanguage(lang);
    if (ok) {
        CyMediaLog::instance().log_printf_Info(
            "[Display] language set to %d", int(lang));
        d->toolWidget->flushTrans();
    }
    return ok;
}

void CyMediaDis::setLogLevel(CyMedia::LogLevel level) {
    CyMediaLog::instance().setLevel(level);
    CyMediaLog::instance().log_printf_Info(
        "[Display] log level set to %d", int(level));
}

void CyMediaDis::setLogCallback(CyMedia::LogCallback cb, void* pUser /*= nullptr*/) {
    CyMediaLog::instance().setLogCallback(cb, pUser);
    CyMediaLog::instance().log_printf_Info(
        "[Display] log callback registered (user=%p)", pUser);
}

void CyMediaDis::setSceneAcceptDrop(bool accept) {
    d->scene->setAcceptDrops(accept);
    CyMediaLog::instance().log_printf_Debug(
        "[Display] scene drop %s", accept ? "enabled" : "disabled");
}

void CyMediaDis::setImageStackNum(uint32_t num) {
    const uint32_t old = d->pipeline->stackSize();
    d->pipeline->setStackSize(num);
    if (old != d->pipeline->stackSize()) {
        CyMediaLog::instance().log_printf_Info(
            "[Display] image stack size changed: %u -> %u", old, d->pipeline->stackSize());
    }
}

bool CyMediaDis::upImageData(const CyMedia::ImageShowInfo& info, void* data, bool force /*= false*/) {
    // 清空后的短暂窗口内拒绝新帧，避免"空图"状态被立刻覆盖。
    if (d->clearImageTime.isValid() && d->clearImageTime.elapsed() < 300) {
        CyMediaLog::instance().log_printf_Debug(
            "[Display] frame rejected during clear cooldown (%lld/300 ms)",
            d->clearImageTime.elapsed());
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

    const ImageShowInfo last = d->pipeline->imageInfo();
    CyMediaLog::instance().log_printf_Info(
        "[Display] clear image (last=%dx%d bit=%d format=%d)",
        last.width, last.height, int(last.bit), int(last.format));
    d->pipeline->clear();
    d->view->clearBackGround();
    d->m_lastInfo = ImageShowInfo();   // 复位后下一帧会被判定为“尺寸变化”，从而重新套用视图模式
    d->view->zoomRaw(true);
    if (d->view->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff) {
        d->view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        d->view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    d->setDrawMode(CyDisDrawItem::Invalid);
    d->clearImageTime.start();
}


void CyMediaDis::setCacheIamge(bool enable) {
    d->pipeline->cacheIamge(enable);
}

bool CyMediaDis::isCacheIamgeEnablee() {
    return d->pipeline->isCacheIamgeEnablee();
}

CyMedia::ImageShowInfo CyMediaDis::imageInfoCopy() const {
    return d->pipeline->imageInfo();
}

const uint8_t* CyMediaDis::latestCachedImage(CyMedia::ImageShowInfo& info) {
    return d->pipeline->latestCachedImage(info);
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
    // 外部手动设置优先：关闭内置面板的自动拉伸
    d->m_StretchWidget->setAutoStretch(false);
    if (type == CyMedia::stretch_None) {
        d->setStretchState(type,
            d->m_stretchRange.start, d->m_stretchRange.end, d->m_stretchRange.max, true);
        return;
    }
    const int max = d->stretchMaxForCurrentImage();
    if (max <= 0) {
        d->setStretchState(type, 0, 0, 0, true);   // 记下类型，首帧后补量程
        return;
    }

    // 类型改变导致量程变化（例如 Lab=100）时，重置到全量程
    if (d->m_stretchRange.max != max
        || d->m_stretchRange.end <= d->m_stretchRange.start) {
        d->setStretchState(type, 0, max, max, true);
        return;
    }
}

void CyMediaDis::setStretchPara(uint32_t start /*= 0*/, uint32_t end /*= 0*/) {
    d->m_StretchWidget->setAutoStretch(false);
    const int max = d->stretchMaxForCurrentImage();
    if (max <= 0) {
        // 首帧前调用：暂存，等 ImageInfoChangeReceiver 补量程
        d->m_pendingStretch = true;
        d->m_pendingStart = int(start);
        d->m_pendingEnd = int(end);
        if (d->m_stretchType == CyMedia::stretch_None)
            d->m_stretchType = CyMedia::stretch_Gray;
        d->syncOptions();
        CyMediaLog::instance().log_printf_Debug(
            "[Display] setStretchPara deferred: no image info yet (start=%u end=%u)",
            start, end);
        return;
    }

    auto type = d->m_stretchType;
    if (type == CyMedia::stretch_None) type = CyMedia::stretch_Gray;
    d->setStretchState(type, int(start), int(end), max, true);
}

CyMedia::StretchRange CyMediaDis::stretchRange() {
    return d ? d->m_stretchRange : CyMedia::StretchRange{};
}

bool CyMediaDis::isStretchEnabled() {
    return d && d->m_stretchType != CyMedia::stretch_None
        && d->m_stretchRange.max > 0
        && d->m_stretchRange.end > d->m_stretchRange.start;
}

CyMedia::DemosaicingMethod CyMediaDis::Demosaic() {
    return d->view->imageDraw()->Demosaic();
}

void CyMediaDis::setDemosaic(CyMedia::DemosaicingMethod method) {
    if (d->view->imageDraw()->Demosaic() == method) return;
    CyMediaLog::instance().log_printf_Debug(
        "[Display] demosaic method: %d -> %d",
        int(d->view->imageDraw()->Demosaic()), int(method));
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
    CyMediaLog::instance().log_printf_Debug(
        "[Display] YUV method: %d -> %d",
        int(d->view->imageDraw()->yuvMethod()), int(method));
    d->view->imageDraw()->setYUVTMethod(method);
    d->syncOptions();
    if (d->pipeline->isDataSlow() && hasImage()) {
        d->pipeline->requestReprocess(false);
        QMetaObject::invokeMethod(d->view->viewport(), "update", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->view->thumnailWidget(), "update", Qt::QueuedConnection);
    }
}

const QString CyMediaDis::colorMapPath() const {
    return d->view->imageDraw()->colorMapPath();
}

const QString CyMediaDis::colorMapIconPath(int index) const {
    return d->view->imageDraw()->colorMapIconPath(index);
}

const QString CyMediaDis::colorMapIconPath(const QString& name) const {
    return d->view->imageDraw()->colorMapIconPath(name);
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
    if (!ok) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] setColorMap(index=%u) failed (%d map(s) available)",
            index, d->view->imageDraw()->ColorMapList().size());
        return false;
    }
    if (d->pipeline->isDataSlow()) {
        QMetaObject::invokeMethod(d->view->viewport(), "update", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->view->thumnailWidget(), "update", Qt::QueuedConnection);
    }
    return true;
}

bool CyMediaDis::setColorMap(const QString& mapName) {
    if (d->view->imageDraw()->colorMapName() == mapName) return true;
    bool ok = d->view->imageDraw()->setColorMap(mapName);
    if (!ok) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] setColorMap(name=\"%s\") failed",
            mapName.toUtf8().constData());
        return false;
    }
    if (d->pipeline->isDataSlow()) {
        QMetaObject::invokeMethod(d->view->viewport(), "update", Qt::QueuedConnection);
        QMetaObject::invokeMethod(d->view->thumnailWidget(), "update", Qt::QueuedConnection);
    }
    return true;
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
    if (!d->pipeline->hasImage()) return;

    d->view->zoomRaw(reset);
}

bool CyMediaDis::isMirrorHorizon() {
    return  d->view->isHoriMirror();
}

void CyMediaDis::mirrorHorizon(bool mirror) {
    if (d->view->isHoriMirror() != mirror) d->view->horiMirror();
}

bool CyMediaDis::isMirrorVertical() {
    return d->view->isVerMirror();
}

void CyMediaDis::mirrorVertical(bool mirror) {
    if (d->view->isVerMirror() != mirror) d->view->verMirror();
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
    const QUuid id = d->drawmanager->addItemByType(itemType);
    if (id.isNull()) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] addItem failed: unsupported type %d", int(itemType));
    }
    return id;
}

QUuid CyMediaDis::addItem(CyDisDrawItem::ItemType itemType, const QPainterPath& path) {
    const QUuid id = d->drawmanager->addItemByTypeWidthPath(itemType, path);
    if (id.isNull()) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] addItem(path) failed: unsupported type %d or empty path", int(itemType));
    }
    return id;
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
    if (item) {
        item->setSelected(true);
    }
    else {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] select item failed: item %s not found",
            id.toString().toUtf8().constData());
    }
}

void CyMediaDis::clearItem() {
    d->drawmanager->clearAll();
}

bool CyMediaDis::isSingleItemMode() {
    return d->drawingTool->replaceMode();
}

void CyMediaDis::setSingleItemMode(bool flag) {
    CyMediaLog::instance().log_printf_Debug(
        "[Display] single item mode %s", flag ? "enabled" : "disabled");
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
    if (d->m_AutoThumbnail) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] setThumbnailEnable ignored: auto-thumbnail mode is enabled");
        return;
    }
    CyMediaLog::instance().log_printf_Debug(
        "[Display] thumbnail %s", enable ? "enabled" : "disabled");
    d->view->setThumbnailEnable(enable);
    d->syncOptions();
}

bool CyMediaDis::thumbnailAutoEnable() {
    return d->m_AutoThumbnail;
}

void CyMediaDis::setThumbnailAutoEnable(bool enable) {
    d->m_AutoThumbnail = enable;
    CyMediaLog::instance().log_printf_Debug(
        "[Display] auto-thumbnail %s (threshold=%dx%d)",
        enable ? "enabled" : "disabled",
        d->m_AutoThumbnailSize.width(), d->m_AutoThumbnailSize.height());
    d->syncOptions();
    if (d->pipeline->isDataSlow()) {
        d->pipeline->requestReprocess(true, false);
    }
}

QSize CyMediaDis::thumbnailAutoEnableSize() {
    return d->m_AutoThumbnailSize;
}

void CyMediaDis::setThumbnailAutoEnableSize(const QSize size) {
    CyMediaLog::instance().log_printf_Debug(
        "[Display] auto-thumbnail threshold: %dx%d -> %dx%d",
        d->m_AutoThumbnailSize.width(), d->m_AutoThumbnailSize.height(),
        size.width(), size.height());
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
        d->m_RetimeItem->move(20, d->toolWidget->height());
    }
    d->m_RetimeItem->setVisible(visi);
}

bool CyMediaDis::isRecTimeLEDVisbible() {
    return d->m_RetimeItem->isLedVisible();
}

void CyMediaDis::setRecTimeLEDVisible(bool visi) {
    d->m_RetimeItem->setLedVisible(visi);
}

void CyMediaDis::upRecTimeMsg(QString msg, bool blink /*= false*/) {
    d->m_RetimeItem->updateText(msg, blink);
}

void CyMediaDis::setRecTimeFont(const QFont& font) {
    d->m_RetimeItem->setTextFont(font);
}

QWidget* CyMediaDis::stretchWidget() {
    return d->m_StretchWidget;
}

void CyMediaDis::setGrayStretchVisible(bool visible) {
    if (visible) {
        // 先同步再 show：避免 showEvent 的 needImage 读到旧面板值
        d->syncStretchPanelFromState();
    }
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

    CyMediaLog::instance().log_printf_Info(
        "[Display] pipeline created (stack=%u)", pipeline->stackSize());
}

CyMediaDis::privateData::~privateData() {
    CyMediaLog::instance().log_printf_Debug(
        "[Display] destroying private data");
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
    CyMediaLog::instance().log_printf_Debug(
        "[Display] initializing GUI");

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
        if (pipeline->samplePixel(x, y, &r, &g, &b, &gray)) m_parent->emit upPosPix(x, y, r, g, b, gray);
        else m_parent->emit upPos(x, y);
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

    m_RetimeItem = new RecordIndicatorDialog(QString(), m_parent);
    m_RetimeItem->setBackgroundColor(QColor(0, 0, 0, 80));//透黑
    m_RetimeItem->setVisible(false);

    m_StretchWidget = new CyMediaDisGrayStretch(m_parent);
    m_StretchWidget->setWindowFlag(Qt::Tool);
    m_StretchWidget->resize(800, 400);
    m_StretchWidget->setVisible(false);
    connect(m_StretchWidget, &CyMediaDisGrayStretch::stretchParaChange,
        this, &CyMediaDis::privateData::onStretchPanelChanged);
    connect(m_StretchWidget, &CyMediaDisGrayStretch::stretchTypeChanged,
        this, &CyMediaDis::privateData::onStretchPanelChanged);
    connect(m_StretchWidget, &CyMediaDisGrayStretch::needImage,
        this, &CyMediaDis::privateData::onGrayToolNeedImage);

    mGrayTestWidget = new CyMediaDisGrayTest(m_parent);
    mGrayTestWidget->setParentDis(m_parent);
    mGrayTestWidget->setWindowFlag(Qt::Tool);
    mGrayTestWidget->resize(800, 400);
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

    opt.stretch = m_stretchType;
    opt.stretchRange = m_stretchRange;
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
    if (!m_frameCtx) {
        // 逐帧重试，但只在首次失败时记录，避免刷屏。
        if (!m_frameCtxErrorLogged.exchange(true)) {
            CyMediaLog::instance().log_printf_Error(
                "[Display] create shared OpenGL context failed (thread=%p), texture upload disabled",
                (void*)QThread::currentThreadId());
        }
        return;
    }
    m_frameCtxErrorLogged.store(false);
    CyMediaLog::instance().log_printf_Debug(
        "[Display] frame upload context created (thread=%p)",
        (void*)QThread::currentThreadId());
}

bool CyMediaDis::privateData::shouldUpdateTools() {
    if (pipeline->isDataSlow()) return true;
    if (!m_ToolWUpTimer.isValid()) {
        m_ToolWUpTimer.start();
        return true;
    }
    if (m_ToolWUpTimer.elapsed() > m_upToolsTimeMs) {
        m_ToolWUpTimer.restart();
        return true;
    }
    return false;
}

void CyMediaDis::privateData::updateTools(const PipelineFrame& frame) {
    ImageShowInfo info = frame.info;
    if (frame.updateStretch && frame.stretchPanelVisible) {
        m_StretchWidget->upImageData(info, frame.data, frame.colorOpe);
    }
    mGrayTestWidget->upImage(info, frame.data, frame.colorOpe);
}

void CyMediaDis::privateData::onFrame(const PipelineFrame& frame) {
    if (!view) return;

    // 1) 图像尺寸/格式变化通知（在工作线程发出，Qt 自动排队到 UI 线程）。
    //    sizeChanged 单独投递：接收端要据此在纹理上传排队的那次 update()
    //    之前确定“适应窗口 / 1:1”，否则第一帧会先按旧比例绘制一次。
    const bool sizeChanged = m_lastInfo.width != frame.info.width
        || m_lastInfo.height != frame.info.height;
    bool infoChanged = sizeChanged
        || m_lastInfo.bit != frame.info.bit
        || m_lastInfo.format != frame.info.format
        || m_lastInfo.length != frame.info.length
        || m_lastInfo.special_pixel != frame.info.special_pixel;
    if (infoChanged) {
        m_lastInfo = frame.info;
        CyMediaLog::instance().log_printf_Info(
            "[Display] image info changed: %dx%d bit=%d format=%d len=%u",
            frame.info.width, frame.info.height,
            int(frame.info.bit), int(frame.info.format), frame.info.length);
        emit ImageInfoChange(frame.info, sizeChanged);
    }

    // 2) 鼠标像素颜色（节流）。
    if (calcolorTimer.isValid()) {
        if (calcolorTimer.elapsed() > 200 || view->imageDraw()->flushFps() <= 5.0) {
            double r = 0, g = 0, b = 0;
            bool gray = false;
            if (pipeline->samplePixel(posX, posY, &r, &g, &b, &gray)) 
                m_parent->emit upPosPix(posX, posY, r, g, b, gray);
            else
                m_parent->emit upPos(posX, posY);
            calcolorTimer.restart();
        }
    }
    else {
        calcolorTimer.start();
    }

    // 3) 辅助面板（帧率低时每帧，高帧率时节流到 (1000/m_upToolsTimeMs) Hz）。
    if (shouldUpdateTools()) {
        updateTools(frame);
    }

    // 4) 上传纹理并请求刷新。
    if (frame.updateDisplay) {
        //等待尺寸调整完毕
        if (sizeChanged) {
            QElapsedTimer t;
            t.start();
            while (pipeline->isRunning() && t.elapsed() < 500) {
                if (view->sceneRect().width() == frame.info.width
                    && view->sceneRect().height() == frame.info.height) {
                    break;
                }
                QThread::msleep(1);
            }
            if (!pipeline->isRunning()) return;   // 管线正在停止，丢弃本帧
        }
        ensureFrameContext();
        if (!m_frameCtx) return;

        const bool uploaded = view->imageDraw()->upBackGround(frame.info, frame.data, m_frameCtx);
        if (uploaded) {
            if (m_uploadFailed.exchange(false)) {
                CyMediaLog::instance().log_printf_Info(
                    "[Display] texture upload recovered");
            }
        }
        else if (!m_uploadFailed.exchange(true)) {
            CyMediaLog::instance().log_printf_Warning(
                "[Display] texture upload failed: %dx%d bit=%d format=%d "
                "(further failures suppressed until recovery)",
                frame.info.width, frame.info.height,
                int(frame.info.bit), int(frame.info.format));
        }
        emit ImageDataDone(frame.info);
    }
}

void CyMediaDis::privateData::ImageInfoChangeReceiver(CyMedia::ImageShowInfo info, bool sizeChanged) {
    // 首帧 / 分辨率变化：**先**把视图设成目标模式，再更新场景矩形。
    //
    // 防止尺寸改变后的第一帧图像先按照原来的缩放显示一次再跳转到新的缩放：
    //  - 本槽先于 ImageDataDone 执行，也先于 upBackGround() 内部排队的那次
    //    viewport()->update()；
    //  - 本函数里 setSceneRect() 也可能触发一次重绘；（可以考虑设置QGraphicsView::NoViewportUpdate再恢复）
    // 所以只要在函数入口就定好缩放，之后任何一次绘制都已经是目标比例。
    // applyViewMode() 不依赖 haveImage()，首帧纹理尚未上屏时也能用。
    scene->setSceneRect(QRect(0, 0, info.width, info.height));
    view->sceneRectUp(QRect(0, 0, info.width, info.height));
    updateStretchForImageInfo(info);
    if (sizeChanged) {
        const bool fit = info.width > view->viewport()->width()
            || info.height > view->viewport()->height();
        view->applyViewMode(fit, QSize(info.width, info.height));
        // 此时还没有图像，缩略图可见性判断会失败，留到 ImageDataDone 补一次。
        m_pendingThumbRefresh = true;
    }
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
    if (m_pendingThumbRefresh) {
        // 缩放已在 ImageInfoChange 阶段应用（那时还没有图像），
        // 这里图像已上屏，按“已有图像”补一次缩略图刷新。
        m_pendingThumbRefresh = false;
        view->refreshThumbnail();
    }
    else if (view->thumbnailVisible()) {
        view->thumnailWidget()->update();
    }
    view->viewport()->update();
    syncOptions();
}

void CyMediaDis::privateData::onLanguageChanged(CyMedia::eLanguage lang) {
    CyMediaLog::instance().log_printf_Debug(
        "[Display] language changed to %d, refreshing panels", int(lang));
    if (m_StretchWidget) m_StretchWidget->flushTrans();
    if (mGrayTestWidget) mGrayTestWidget->flushTrans();
    if (drawmanager)     drawmanager->flushTrans();
}

void CyMediaDis::privateData::onGrayToolNeedImage() {
    if (!pipeline->isDataSlow()) return;

    syncOptions();
    if (pipeline && pipeline->isDataSlow()) {
        pipeline->requestReprocess(true, true, true);
    }
}

void CyMediaDis::privateData::onAddItem(QUuid id) {
    auto* item = drawmanager->getItem(id);
    if (!item) {
        CyMediaLog::instance().log_printf_Warning(
            "[Display] itemAdded(%s) but lookup failed",
            id.toString().toUtf8().constData());
        return;
    }
    CyMediaLog::instance().log_printf_Debug(
        "[Display] item added: id=%s type=%d",
        id.toString().toUtf8().constData(), int(item->itemType()));
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
    CyMediaLog::instance().log_printf_Debug(
        "[Display] item removed: id=%s", id.toString().toUtf8().constData());
    if (mGrayTestWidget->isVisible()) {
        mGrayTestWidget->ItemRemoved(id);
    }
    m_parent->emit itemRemoved(id);
}

void CyMediaDis::privateData::onStretchPanelChanged() {
    if (!m_StretchWidget) return;

    bool changed = false;
    // 面板已隐藏（如 closeEvent）时只更新 options，不反向改状态
    if (m_StretchWidget->isVisible()) {
        changed = updateStretchFromPanel();
    }

    syncOptions();

    if (changed && pipeline && pipeline->isDataSlow()) {
        pipeline->requestReprocess(true, true, true);
    }
}

bool CyMediaDis::privateData::updateStretchFromPanel() {
    if (!m_StretchWidget) return false;
    const auto pv = m_StretchWidget->stretchValue();
    if (pv.max <= 0 || pv.end <= pv.start) return false;
    return setStretchState(m_StretchWidget->stretchtype(),
        pv.start, pv.end, pv.max, true);
}

void CyMediaDis::privateData::updateStretchForImageInfo(const ImageShowInfo& info) {
    int newMax = bitMaxFor(info.bit);
    if (m_stretchType == CyMedia::stretch_Lab) newMax = 100;
    if (newMax <= 0) return;
    if (m_stretchRange.max == newMax && !m_pendingStretch) return;

    int start = m_stretchRange.start;
    int end = m_stretchRange.end;

    if (m_pendingStretch) {
        start = m_pendingStart;
        end = m_pendingEnd;
        m_pendingStretch = false;
    }
    else if (m_stretchRange.max <= 0) {
        start = 0;
        end = newMax;
    }

    // 策略：绝对窗口只做 clamp；越界部分收回新量程。
    // 如果想“位深变化时保持画面观感”，这里改成按比例缩放即可。
    setStretchState(m_stretchType, start, end, newMax, true);
}

void CyMediaDis::privateData::syncStretchPanelFromState() {
    if (!m_StretchWidget || m_stretchRange.max <= 0) return;

    CyMedia::StretchType type = m_stretchType;
    int start = m_stretchRange.start;
    int end = m_stretchRange.end;
    const int max = m_stretchRange.max;

    // 面板没有 None 选项：None 时显示 Gray + 全量程作为中性状态
    if (type == CyMedia::stretch_None) {
        type = CyMedia::stretch_Gray;
        start = 0;
        end = max;
    }

    const auto pv = m_StretchWidget->stretchValue();
    if (m_StretchWidget->stretchtype() == type
        && pv.start == start && pv.end == end && pv.max == max) {
        return;   // 一致，不恢复
    }

    // 不一致：以权威状态恢复面板（setStretchValue 内部 blockSignals）
    m_StretchWidget->setStretchValue(type, start, end, max);
}

void CyMediaDis::privateData::applyStretch(bool requestRepaint) {
    if (!view || !view->imageDraw()) return;

    const bool enabled = (m_stretchType != CyMedia::stretch_None)
        && m_stretchRange.max > 0
        && m_stretchRange.end > m_stretchRange.start;

    if (enabled) {
        view->imageDraw()->setStretchType(m_stretchType);
        view->imageDraw()->setStreaChPara(
            m_stretchRange.start, m_stretchRange.end, m_stretchRange.max);
    }
    else {
        view->imageDraw()->setStretchType(CyMedia::stretch_None);
    }

    syncOptions();

    if (requestRepaint) {
        view->viewport()->update();
        if (view->thumnailWidget()->isVisible())
            view->thumnailWidget()->update();
    }
}

bool CyMediaDis::privateData::setStretchState(CyMedia::StretchType type, int start, int end, int max, bool notify) {
    if (type == CyMedia::stretch_None) {
        if (m_stretchType == CyMedia::stretch_None) return false;
        m_stretchType = type;
        applyStretch(true);
        syncStretchPanelFromState();
        if (notify)
            emit m_parent->stretchChanged(int(type),
                m_stretchRange.start, m_stretchRange.end, m_stretchRange.max);
        return true;
    }

    if (max <= 0) {
        if (m_stretchType == type) return false;
        m_stretchType = type;
        applyStretch(true);
        if (notify)
            emit m_parent->stretchChanged(int(type), start, end, max);
        return true;
    }

    start = qBound(0, start, max - 1);
    end = qBound(start + 1, end, max);

    if (m_stretchType == type
        && m_stretchRange.start == start
        && m_stretchRange.end == end
        && m_stretchRange.max == max) {
        return false;
    }

    m_stretchType = type;
    m_stretchRange = { start, end, max };
    m_pendingStretch = false;

    applyStretch(true);
    syncStretchPanelFromState();

    if (notify)
        emit m_parent->stretchChanged(int(type), start, end, max);
    return true;
}

int CyMediaDis::privateData::stretchMaxForCurrentImage() const {
    if (m_stretchType == CyMedia::stretch_Lab) return 100;
    return bitMaxFor(pipeline->imageInfo().bit);
}

}

#include "Display.moc"
