
#include "CyMediaDisView.h"
#include "../CyMediaCalc/CyMediaCalc.h"
#include "CyMediaDisViewBckDraw.h"
#include "CyMediaDisViewThumbnail.h"

#include <QGraphicsScene.h>
#include <QApplication>
#include <QScrollBar>
#include <QTimer>
#include <QOpenGLWidget>
#include <QGraphicsItem>
#include <QElapsedTimer>
#include <QThread>
#include <QMessageBox>

static const double THUMBNAIL_RATIO = 0.30;     // 占主视图宽高的比例

class CyMediaDisView::MyViewPrivateData {
public:
    CyMediaDisView* pView = nullptr;
    //Zoom
    float scaleFactor = 1.2;            ///< 当前缩放因子
    float xZoomValue = 1.0;             ///< 当前图像宽缩放倍率
    float yZoomValue = 1.0;             ///< 当前图像高缩放倍率
    float maxZoomValue = 256.0;         ///< 最大放大倍数

    //rotate
    double rotateAngle = 0.0;            ///< 当前旋转角度

    //mirror
    bool hIsMirror = false;
    bool vIsMirror = false;

    QPointF posAnchor;                        // 当前鼠标在View中的位置，用来在mouseMove事件中计算偏移
    bool spaceIsPrese = false;
    bool haveTools = false;
    bool drawMode = false;

    bool mThumbnailEnable = true;
public:
    void zoom(double zoomValue);

    void updateThumbnail();
    bool shouldShowThumbnail() const;
    void onViewRectChanged(const QRectF& rect);
    void upThumbanilSize();
    void upThumbanilPosition();
};

void CyMediaDisView::MyViewPrivateData::zoom(double zoomValue) {
    //避免像素点的杂色
    /*if (zoomValue > 10.0)
        zoomValue = int(zoomValue) / 2 * 2;*/
    zoomValue = (int(zoomValue * 1000) / 2 * 2) / 1000.0;
    if (zoomValue > 25.0)
        zoomValue = int(zoomValue);
    if (xZoomValue == zoomValue)
        return;
    QTransform transform(pView->transform());
    transform.scale(zoomValue / xZoomValue, zoomValue / xZoomValue);
    pView->setTransform(transform);
    xZoomValue = zoomValue;
    yZoomValue = zoomValue;
    pView->emit zoomValueChange(xZoomValue);
    updateThumbnail();
    //setupMatrix();
}

void CyMediaDisView::MyViewPrivateData::updateThumbnail() {
    if (!pView->m_Thumbnail || !pView->scene()) {
        return;
    }

    // 检查是否需要显示缩略图
    if (!shouldShowThumbnail()) {
        pView->m_Thumbnail->hide();
        return;
    }

    QRectF sceneRect = pView->scene()->sceneRect();
    if (sceneRect.isEmpty()) return;

    // 获取当前视口在场景中的矩形（世界坐标）
    QPointF topLeft = pView->mapToScene(0, 0);
    QPointF bottomRight = pView->mapToScene(pView->viewport()->width(), pView->viewport()->height());
    //QRectF viewSceneRect(topLeft, bottomRight);
    QRectF viewSceneRect = pView->mapToScene(pView->viewport()->rect()).boundingRect();

    // 获取缩略图实际尺寸
    QSize thumbSize = pView->m_Thumbnail->size();
    // 直接计算实际坐标，避免百分比转换的精度问题
    double x = (viewSceneRect.x() - sceneRect.x()) / sceneRect.width() * thumbSize.width();
    double y = (viewSceneRect.y() - sceneRect.y()) / sceneRect.height() * thumbSize.height();
    double w = viewSceneRect.width() / sceneRect.width() * thumbSize.width();
    double h = viewSceneRect.height() / sceneRect.height() * thumbSize.height();
    // 精确的边界限制
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > thumbSize.width()) {
        w = thumbSize.width() - x;
    }
    if (y + h > thumbSize.height()) {
        h = thumbSize.height() - y;
    }
    // 确保宽度和高度为正数
    w = qMax(w, 0.0);
    h = qMax(h, 0.0);

    QRectF thumbnailViewRect(x, y, w, h);
    pView->m_Thumbnail->setViewRect(thumbnailViewRect);
    upThumbanilPosition();
    pView->m_Thumbnail->show();
    pView->m_Thumbnail->raise();
    pView->m_Thumbnail->update();
}

bool CyMediaDisView::MyViewPrivateData::shouldShowThumbnail() const {
    if (!pView->scene() || !pView->m_backDraw->haveImage() || false == mThumbnailEnable) {
        return false;
    }

    // 检查是否场景放大后超过视图窗口
    return (pView->scene()->sceneRect().width() * xZoomValue  > pView->viewport()->width() ||
            pView->scene()->sceneRect().height() * xZoomValue > pView->viewport()->height());
}

void CyMediaDisView::MyViewPrivateData::onViewRectChanged(const QRectF& rect) {
    // 计算场景中的位置
    QRectF sceneRect = pView->scene()->sceneRect();
    QPointF topLeft = QPointF(rect.x() * sceneRect.width() / 100,
        rect.y() * sceneRect.height() / 100);

    // 转换为视图坐标
    QPointF viewPos = pView->mapFromScene(topLeft.x(), topLeft.y());

    // 设置视图位置
    pView->centerOn(viewPos);
}

void CyMediaDisView::MyViewPrivateData::upThumbanilPosition() {
    static int margin = 10;
    pView->m_Thumbnail->move(pView->width() - pView->m_Thumbnail->width() - margin,
        pView->height() - pView->m_Thumbnail->height() - margin);
}

void CyMediaDisView::MyViewPrivateData::upThumbanilSize() {
    //预期大小
    float w = pView->viewport()->width() * THUMBNAIL_RATIO;
    float h = pView->viewport()->height() * THUMBNAIL_RATIO;
    //图像大小
    auto imageSize = pView->sceneRect().size();
    //放大倍数
    float wRadio = w * 1.0 / imageSize.width();
    float hRadio = h * 1.0 / imageSize.height();
    if (wRadio < hRadio) {
        if (w < 100.0) {
            w = 100.0;
            wRadio = w * 1.0 / imageSize.width();
        }
        h = imageSize.height() * wRadio;
    }
    else {
        if (h < 100.0) {
            h = 100.0;
            hRadio = h * 1.0 / imageSize.height();
        }
        w = imageSize.width() * hRadio;
    }
    QSize setSize(w + 0.5, h + 0.5);
    if (setSize != pView->m_Thumbnail->size()) pView->m_Thumbnail->setThumbnailSize(setSize);
}

CyMediaDisView::CyMediaDisView(QWidget* parent /*= nullptr*/)
    :QGraphicsView() {
    d = new MyViewPrivateData;
    d->pView = this;

    //setAttribute(Qt::WA_TranslucentBackground);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    setAcceptDrops(true);
    setRenderHints(QPainter::Antialiasing);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &CyMediaDisView::onScrollValueChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &CyMediaDisView::onScrollValueChanged);

    m_backDraw = new CyMediaDisViewBckDraw(this);

    // 初始化缩略图窗口
    //m_Thumbnail = new CyMediaDisViewThumbnail(this, this);
    ////m_Thumbnail->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    //m_Thumbnail->setAttribute(Qt::WA_TranslucentBackground);
    //d->updateThumbnail();
    //d->upThumbanilPosition();
    //m_Thumbnail->hide();

    //指定opengl版本
    QOpenGLWidget* OpenGlwidget = new QOpenGLWidget();
    OpenGlwidget->setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    OpenGlwidget->setMouseTracking(true);
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    OpenGlwidget->setFormat(format);
    setViewport(OpenGlwidget);
}

CyMediaDisView::~CyMediaDisView() {
    if (d) {
        m_backDraw->clearBackGround();
        delete[] m_backDraw;
        delete d;
    }
}

void CyMediaDisView::setCyScene(QGraphicsScene* scene) {
    this->setScene(scene);
    if (m_Thumbnail) m_Thumbnail->setScene(scene);
    scene->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
}

void CyMediaDisView::sceneRectUp(const QRectF& rect) {
    setSceneRect(rect);
    d->upThumbanilSize();
    d->updateThumbnail();
}

void CyMediaDisView::setViewFromThumbnailPos(double xRatio, double yRatio) {
    if (!scene()) return;
    QRectF sceneRect = scene()->sceneRect();
    if (sceneRect.isEmpty()) return;
    double targetX = sceneRect.x() + xRatio * sceneRect.width();
    double targetY = sceneRect.y() + yRatio * sceneRect.height();
    setViewTopLeft(QPointF(targetX, targetY));
}

float CyMediaDisView::zoomValue(void) {
    return d->xZoomValue;
}

void CyMediaDisView::zoomIn(void) {
    if (!this->scene() || !m_backDraw->haveImage())
        return;

    float value = d->xZoomValue * d->scaleFactor;
    value = (value > d->maxZoomValue) ? d->maxZoomValue : value;
    d->zoom(value);
}

void CyMediaDisView::zoomOut(void) {
    if (!this->scene() || !m_backDraw->haveImage())
        return;

    float value = d->xZoomValue / d->scaleFactor;
    if ((value * this->scene()->width()) <= 200 ||
        (value * this->scene()->height()) <= 100) {
        return;
    }
    d->zoom(value);
}

void CyMediaDisView::zoomAuto(void) {
    if (!this->scene() || !m_backDraw->haveImage())
        return;

    QSize fRect = this->size();
    QSizeF sceneRect = this->scene()->sceneRect().size();
    qreal s = qMin(
        fRect.width() * 0.999 / sceneRect.width(),
        fRect.height() * 0.999 / sceneRect.height());
    d->zoom(s);
}

void CyMediaDisView::zoomRaw(bool Reset /*= true*/) {
    if (!this->scene())
        return;

    QTransform transform(this->transform());
    double value = 1.0;
    d->zoom(value);
    if (Reset) {
        resetTransform();
        d->hIsMirror = false;
        d->vIsMirror = false;
        d->rotateAngle = 0.0;
        rotateView(0.0);
    }
    else {
        setTransform(transform);
    }
}

bool CyMediaDisView::isHriMirror() {
    return d->hIsMirror;
}

bool CyMediaDisView::isVerMirror() {
    return d->vIsMirror;
}

void CyMediaDisView::hriMirror(void) {
    QTransform transform(this->transform());
    transform.rotate(180.0, Qt::YAxis);
    setTransform(transform);
    
    d->hIsMirror = !d->hIsMirror;
}

void CyMediaDisView::verMirror(void) {
    QTransform transform(this->transform());
    transform.rotate(180.0, Qt::XAxis);
    setTransform(transform);

    d->vIsMirror = !d->vIsMirror;
}

double CyMediaDisView::rotateValue(void) {
    return d->rotateAngle;
}

void CyMediaDisView::rotateView(double angle) {
    if (!this->scene() || !m_backDraw->haveImage())
        return;

    QTransform transform(this->transform());
    d->rotateAngle += angle;
    if (d->rotateAngle > 360.0) {
        d->rotateAngle -= 360.0;
    }
    transform.rotate(d->rotateAngle);
    setTransform(transform);
}

CyMediaDisViewBckDraw* CyMediaDisView::imageDraw() const {
    return m_backDraw;
}

void CyMediaDisView::clearBackGround() {
    m_backDraw->clearBackGround();
    if (QThread::currentThread() == qApp->thread()) {
        m_Thumbnail->setVisible(false);
    }
}

void CyMediaDisView::setViewTopLeft(const QPointF& topLeft) {
    if (!scene()) return;
    QPointF oldTopLeft = mapToScene(QPoint(0, 0));
    QPointF delta = topLeft - oldTopLeft;
    QPointF newCenter = mapToScene(viewport()->rect().center()) + delta;
    centerOn(newCenter);
}

QWidget* CyMediaDisView::thumnailWidget() {
    return m_Thumbnail;
}

bool CyMediaDisView::thumbnailEnable() {
    return d->mThumbnailEnable;
}

void CyMediaDisView::setThumbnailEnable(bool enable) {
    if (d->mThumbnailEnable == enable)
        return;
    d->mThumbnailEnable = enable;
    if (false == enable) {
        m_Thumbnail->hide();
    }
}

bool CyMediaDisView::thumbnailVisible() {
    return m_Thumbnail->isVisible();
}

void CyMediaDisView::setThumbnailSelectColor(QColor color) {
    m_Thumbnail->setSelectColor(color);
}

void CyMediaDisView::setThumbnailBackgroundColor(QColor color) {
    m_Thumbnail->setBackgroundColor(color);
}

QColor CyMediaDisView::thumbnailBorderColor() {
    return m_Thumbnail->borderColor();
}

void CyMediaDisView::setThumbnailBorderColor(QColor color) {
    m_Thumbnail->setBorderColor(color);
}

bool CyMediaDisView::ThumbnailDrawBorder() {
    return m_Thumbnail->drawBorder();
}

void CyMediaDisView::setThumbnailDrawBorder(bool draw) {
    m_Thumbnail->setDrawBorder(draw);
}

bool CyMediaDisView::drawMode() {
    return d->drawMode;
}

void CyMediaDisView::setDrawMode(bool draw) {
    d->drawMode = draw;
    auto scene = this->scene();
    if (!scene) {
        return;
    }

    // 获取场景中所有项
    QList<QGraphicsItem*> allItems = scene->items();
    for (QGraphicsItem* item : allItems) {
        // 设置为不可选择
        item->setEnabled(draw);

        // 如果当前项被选中，取消选中状态
        if (item->isSelected()) {
            item->setSelected(draw);
        }
    }
}

void CyMediaDisView::drawBackground(QPainter* painter, const QRectF& rect) {
    m_backDraw->drawBackground(painter, rect);
}

void CyMediaDisView::showEvent(QShowEvent* e) {
    QGraphicsView::showEvent(e);
    QOpenGLWidget* glWidget = qobject_cast<QOpenGLWidget*>(viewport());
    if (glWidget && false == m_backDraw->glIsInit()) {
        glWidget->makeCurrent();
        m_backDraw->initgl(glWidget->context());
        glWidget->doneCurrent();
    }
    // 创建缩略图（如果尚未创建）
    if (!m_Thumbnail) {
        m_Thumbnail = new CyMediaDisViewThumbnail(this, this);
        m_Thumbnail->setScene(this->scene());
        d->upThumbanilSize();
        d->upThumbanilPosition();
        m_Thumbnail->hide(); // 初始隐藏
    }
    d->updateThumbnail();
}

void CyMediaDisView::closeEvent(QCloseEvent* e) {
    QGraphicsView::closeEvent(e);
}

void CyMediaDisView::wheelEvent(QWheelEvent* event) {
    // ========================
    // Ctrl + 滚轮 → 缩放(中心缩放)
    // ========================
    if (event->modifiers() & Qt::ControlModifier) {
        // 缩放前鼠标相对于View的位置
        QPointF cusorpoint = event->position();
        QPointF scenePos = mapToScene(QPoint(cusorpoint.x(), cusorpoint.y()));
        // 缩放前View的宽高
        qreal viewWidth = viewport()->width();
        qreal viewHeight = viewport()->height();
        // 缩放前鼠标当前位置相当于view大小的横纵比例
        qreal hScale = cusorpoint.x() / viewWidth;
        qreal vScale = cusorpoint.y() / viewHeight;

        //缩放
        event->angleDelta().y() > 0 ? zoomIn() : zoomOut();

        // 将scene坐标转换为放大缩小后的坐标
        QPointF viewPoint = matrix().map(scenePos);
        horizontalScrollBar()->setValue(int(viewPoint.x()) - viewWidth * hScale);
        verticalScrollBar()->setValue(int(viewPoint.y()) - viewHeight * vScale);

        event->accept();
        return;
    }
    // ========================
    // Shift + 滚轮 → 横向滑动
    // ========================
    if (event->modifiers() & Qt::ShiftModifier) {
        // 滚轮垂直方向 → 转为水平滚动
        int delta = event->angleDelta().y();
        if (delta != 0) {
            // 调整步长
            int scrollStep = delta / 2;
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - scrollStep);
            event->accept();
            return;
        }
    }

    QGraphicsView::wheelEvent(event);
}

void CyMediaDisView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && this->scene()) {
        d->posAnchor = event->pos();
    }
    QGraphicsView::mousePressEvent(event);
}

void CyMediaDisView::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() == Qt::LeftButton) {
        if (false == d->drawMode) {
            QPointF offsetPos = event->pos() - d->posAnchor;
            //setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + (d->posAnchor.x() - event->x()));
            verticalScrollBar()->setValue(verticalScrollBar()->value() + (d->posAnchor.y() - event->y()));
            d->posAnchor = event->pos();
            return;
        }
    }
    QGraphicsView::mouseMoveEvent(event);
}

void CyMediaDisView::mouseReleaseEvent(QMouseEvent* event)
{
    return QGraphicsView::mouseReleaseEvent(event);
}

void CyMediaDisView::mouseDoubleClickEvent(QMouseEvent* event)
{
    return QGraphicsView::mouseDoubleClickEvent(event);
}

void CyMediaDisView::keyPressEvent(QKeyEvent* event)
{
    return QGraphicsView::keyPressEvent(event);
}

void CyMediaDisView::keyReleaseEvent(QKeyEvent* event)
{
    return QGraphicsView::keyReleaseEvent(event);
}

void CyMediaDisView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    // 延迟 1ms 刷新，等待 DWM 完成窗口位置计算
    QTimer::singleShot(1, this, [this]() {
        if (viewport()) viewport()->update();
        });
    if (m_Thumbnail) {
        d->upThumbanilSize();
        d->updateThumbnail();
    }
}

void CyMediaDisView::onScrollValueChanged() {
    if (m_Thumbnail) {
        d->updateThumbnail();
    }
}

