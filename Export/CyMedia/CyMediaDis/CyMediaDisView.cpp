
#include "CyMediaDisView.h"
#include "CyDMediaDisScen.h"

#include "../CyMediaCalc/CyMediaCalc.h"

#include <QScrollBar>
#include <QTimer>
#include <QGraphicsItem>
static const int THUMBNAIL_MIN_SIZE = 120;      // 最小边长
static const double THUMBNAIL_RATIO = 0.15;     // 占主视图宽高的比例

class CyThumbnailView : public QWidget {
    Q_OBJECT
public:
    CyThumbnailView(CyMediaDisView* parentView, QWidget* parent = nullptr);
    ~CyThumbnailView();

signals:
    void viewRectChanged(const QRectF& rect);

public:
    void setScene(QGraphicsScene* scene);
    void setViewRect(const QRectF& rect);

    bool isBeingDragged();

    void setThumbnailSize(const QSize& size);

    void upBackImage(CyMedia::ImageShowInfo info, uint8_t* data);
    QImage& backImage();

    void setBackgroundColor(QColor color);
    void setSelectColor(QColor color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    CyMediaDisView* m_parentView = nullptr; // 指向主视图
    QGraphicsScene* m_scene = nullptr;
    QRectF mViewRect;
    bool mDragging = false;
    QPointF mClickOffsetRatio; // 鼠标点击点在小窗内的相对比例 (0~1)
    QImage mBackImage;

    QColor mSelectRectColor = QColor(0x2a, 0xa3, 0xc6);
    QColor mSelectRectColor_transparent = QColor(0x2a, 0xa3, 0xc6, 100);
    QColor mBackGroundColor = QColor(0x2a, 0xa3, 0xc6);
};

class CyMediaDisView::MyViewPrivateData {
public:
    CyMediaDisView* m_view = nullptr;
    bool bShowImage = false;

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

    QPointF posAnchor;				        // 当前鼠标在View中的位置，用来在mouseMove事件中计算偏移
    bool spaceIsPrese = false;
    bool haveTools = false;
    bool drawMode = false;

    CyThumbnailView* mThumbnailView = nullptr;
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
    QTransform transform(m_view->transform());
    transform.scale(zoomValue / xZoomValue, zoomValue / xZoomValue);
    m_view->setTransform(transform);
    xZoomValue = zoomValue;
    yZoomValue = zoomValue;
    m_view->emit zoomValueChange(xZoomValue);
    updateThumbnail();
    //setupMatrix();
}

void CyMediaDisView::MyViewPrivateData::updateThumbnail() {
    if (!mThumbnailView || !m_view->scene()) {
        return;
    }

    // 检查是否需要显示缩略图
    if (!shouldShowThumbnail()) {
        mThumbnailView->hide();
        return;
    }

    QRectF sceneRect = m_view->scene()->sceneRect();
    if (sceneRect.isEmpty()) return;

    // 获取当前视口在场景中的矩形（世界坐标）
    QPointF topLeft = m_view->mapToScene(0, 0);
    QPointF bottomRight = m_view->mapToScene(m_view->viewport()->width(), m_view->viewport()->height());
    QRectF viewSceneRect(topLeft, bottomRight);

    // 获取缩略图实际尺寸
    QSize thumbSize = mThumbnailView->size();
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
    mThumbnailView->setViewRect(thumbnailViewRect);
    upThumbanilPosition();
    mThumbnailView->show();
    mThumbnailView->raise();
    mThumbnailView->update();
}

bool CyMediaDisView::MyViewPrivateData::shouldShowThumbnail() const {
    if (!m_view->scene() || !bShowImage || false == mThumbnailEnable) {
        return false;
    }

    // 检查是否场景放大后超过视图窗口
    return ((m_view->scene()->sceneRect().width() * xZoomValue > m_view->viewport()->width() ||
            m_view->scene()->sceneRect().height() * xZoomValue > m_view->viewport()->height()));
}

void CyMediaDisView::MyViewPrivateData::onViewRectChanged(const QRectF& rect) {
    // 计算场景中的位置
    QRectF sceneRect = m_view->scene()->sceneRect();
    QPointF topLeft = QPointF(rect.x() * sceneRect.width() / 100,
        rect.y() * sceneRect.height() / 100);

    // 转换为视图坐标
    QPointF viewPos = m_view->mapFromScene(topLeft.x(), topLeft.y());

    // 设置视图位置
    m_view->centerOn(viewPos);
}

void CyMediaDisView::MyViewPrivateData::upThumbanilPosition() {
    static int margin = 10;
    mThumbnailView->move(m_view->width() - mThumbnailView->width() - margin,
        m_view->height() - mThumbnailView->height() - margin);
}

void CyMediaDisView::MyViewPrivateData::upThumbanilSize() {
    //预期大小
    float w = m_view->viewport()->width() * THUMBNAIL_RATIO;
    float h = m_view->viewport()->height() * THUMBNAIL_RATIO;
    //图像大小
    auto imageSize = m_view->sceneRect().size();
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
    mThumbnailView->setThumbnailSize(QSize(w + 0.5, h + 0.5));
}

CyMediaDisView::CyMediaDisView(QWidget* parent /*= nullptr*/)
    :QGraphicsView() {
    d = new MyViewPrivateData;
    d->m_view = this;

    setMouseTracking(true);
    setAcceptDrops(true);
    setRenderHints(QPainter::Antialiasing);

    // 初始化缩略图窗口
    d->mThumbnailView = new CyThumbnailView(this, this);
    d->updateThumbnail();
    d->upThumbanilPosition();
    d->mThumbnailView->hide();

    // 连接信号
    connect(d->mThumbnailView, &CyThumbnailView::viewRectChanged, this, [this](const QRectF& rect) {
        d->onViewRectChanged(rect);
        });

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &CyMediaDisView::onScrollValueChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &CyMediaDisView::onScrollValueChanged);
}

CyMediaDisView::~CyMediaDisView() {

}

void CyMediaDisView::setCyScene(QGraphicsScene* scene) {
    this->setScene(scene);
    d->mThumbnailView->setScene(scene);
}

void CyMediaDisView::setViewFromThumbnailPos(double xRatio, double yRatio) {
    if (!scene()) return;

    QRectF sceneRect = scene()->sceneRect();
    if (sceneRect.isEmpty()) return;

    // 计算目标视口左上角在场景中的理想位置
    double targetX = sceneRect.x() + xRatio * sceneRect.width();
    double targetY = sceneRect.y() + yRatio * sceneRect.height();

    // 将该点映射到视图坐标（考虑 transform）
    QPointF viewPoint = mapFromScene(QPointF(targetX, targetY));

    // 但我们真正要设置的是滚动条
    // 更简单：直接移动视图，使该点位于视口左上角附近
    // 但更好的方式：计算滚动条应设的值

    // 获取当前 transform 的缩放（假设无旋转/剪切）
    QTransform t = transform();
    double sx = t.m11(); // x 缩放
    double sy = t.m22(); // y 缩放

    // 滚动条值 = 场景坐标 * 缩放
    int hVal = qRound(targetX * sx);
    int vVal = qRound(targetY * sy);

    // 设置滚动条（自动 clamp）
    horizontalScrollBar()->setValue(hVal);
    verticalScrollBar()->setValue(vVal);
}

float CyMediaDisView::zoomValue(void) {
    return d->xZoomValue;
}

void CyMediaDisView::zoomIn(void) {
    if (!this->scene() || !d->bShowImage)
        return;

    float value = d->xZoomValue * d->scaleFactor;
    value = (value > d->maxZoomValue) ? d->maxZoomValue : value;
    d->zoom(value);
}

void CyMediaDisView::zoomOut(void) {
    if (!this->scene() || !d->bShowImage)
        return;

    float value = d->xZoomValue / d->scaleFactor;
    if ((value * this->scene()->width()) <= 200 ||
        (value * this->scene()->height()) <= 100) {
        return;
    }
    d->zoom(value);
}

void CyMediaDisView::zoomAuto(void) {
    if (!this->scene() || !d->bShowImage)
        return;

    QRect fRect = this->rect();

    qreal sw = qreal(fRect.width() * 0.99) / this->scene()->width();
    qreal sh = qreal(fRect.height() * 0.99) / this->scene()->height();
    qreal s = qMin(sh, sw);
    d->zoom(s);
}

void CyMediaDisView::zoomRaw(bool Reset /*= true*/) {
    if (!this->scene())
        return;

    QTransform transform(this->transform());
    double value = 1.0 / d->xZoomValue;
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
    if (!this->scene() || !d->bShowImage)
        return;

    QTransform transform(this->transform());
    d->rotateAngle += angle;
    if (d->rotateAngle > 360.0) {
        d->rotateAngle -= 360.0;
    }
    transform.rotate(d->rotateAngle);
    setTransform(transform);
}

void CyMediaDisView::setImageShow(bool show) {
    d->bShowImage = show;
}

bool CyMediaDisView::thumbnailEnable() {
    return d->mThumbnailEnable;
}

void CyMediaDisView::setThumbnailEnable(bool enable) {
    if (d->mThumbnailEnable == enable)
        return;
    d->mThumbnailEnable = enable;
    if (false == enable) {
        d->mThumbnailView->hide();
    }
}

void CyMediaDisView::upThumbnaildata(CyMedia::ImageShowInfo info, uint8_t* data) {
    d->mThumbnailView->upBackImage(info, data);
}

QImage& CyMediaDisView::ThumbnailImage() {
    return d->mThumbnailView->backImage();
}

void CyMediaDisView::setthumbnailSelectColor(QColor color) {
    d->mThumbnailView->setSelectColor(color);
}

void CyMediaDisView::setthumbnailBackgroundColor(QColor color) {
    d->mThumbnailView->setBackgroundColor(color);
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
        if (item == ((CyDMediaDisScen*)scene)->BackDis())
            continue;
        // 设置为不可选择
        item->setEnabled(draw);

        // 如果当前项被选中，取消选中状态
        if (item->isSelected()) {
            item->setSelected(draw);
        }
    }
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

    if (d->mThumbnailView) {
        d->upThumbanilSize();
        d->updateThumbnail();
    }
}

void CyMediaDisView::onScrollValueChanged() {
    if (d->mThumbnailView) {
        d->updateThumbnail();
    }
}






//**********************class CyThumbnailView**********************
CyThumbnailView::CyThumbnailView(CyMediaDisView* parentView, QWidget* parent /*= nullptr*/)
    : QWidget(parent)
    , m_parentView(parentView) {
}

CyThumbnailView::~CyThumbnailView() {

}

void CyThumbnailView::setScene(QGraphicsScene* scene) {
    m_scene = scene;
}

void CyThumbnailView::setViewRect(const QRectF& rect) {
    mViewRect = rect;
    update();
}

bool CyThumbnailView::isBeingDragged() {
    return mDragging;
}

void CyThumbnailView::setThumbnailSize(const QSize& size) {
    resize(size);
}

void CyThumbnailView::upBackImage(CyMedia::ImageShowInfo info, uint8_t* data) {
    QImage image;
    switch (info.format) {
        case CyMedia::MONO: {
            // 单色图像
            if (info.bit == 8) {
                image = QImage(data, info.width, info.height, QImage::Format_Grayscale8);
            }
            else if (info.bit <= 16) {
                uint16_t* pImage = (uint16_t*)data;
                float maxPixel = (1 << info.bit) - 1;
                float maxPixel_16 = (1 << 16) - 1;
                QImage temp(info.width, info.height, QImage::Format_Grayscale16);
                for (int y = 0; y < info.height; y++) {
                    for (int x = 0; x < info.width; x++) {
                        temp.bits()[y * info.width + x] = (pImage[y * info.width + x] * maxPixel_16) / maxPixel;
                    }
                }
            }
            else if (info.bit < 32) {
                uint32_t* pImage = (uint32_t*)data;
                float maxPixel = (1 << info.bit) - 1;
                QImage temp(info.width, info.height, QImage::Format_Grayscale8);
                for (int y = 0; y < info.height; y++) {
                    for (int x = 0; x < info.width; x++) {
                        temp.bits()[y * info.width + x] = (pImage[y * info.width + x] * 255) / maxPixel;
                    }
                }
            }
        }break;

        case CyMedia::RGB: {
            // RGB图像
            if (info.bit == 8) {
                image = QImage(data, info.width, info.height, QImage::Format_RGB888);
            }
            else if (info.bit == 16) {
                // 处理16位RGB图像
                // 假设16位RGB是565格式
                QImage temp(info.width, info.height, QImage::Format_RGB16);
                memcpy(temp.bits(), data, info.width * info.height * 2);
                image = temp;
            }
        }break;

        case CyMedia::BAYERRG:
        case CyMedia::BAYERGR:
        case CyMedia::BAYERGB:
        case CyMedia::BAYERBG: {
            // Bayer图像，需要转换为RGB
            // 使用CyMedia::bayer2RGBConvert函数
            uint32_t rgbLen = info.width * info.height * 3;
            uint8_t* rgbData = new uint8_t[rgbLen];
            CyMedia::bayer2RGBConvert(info, data, rgbData);
            image = QImage(rgbData, info.width, info.height, QImage::Format_RGB888);
            delete[] rgbData;
        }break;

        default:
            // 其他格式，尝试用默认方式
            image = QImage(data, info.width, info.height, QImage::Format_RGB888);
            break;
    }

    // 如果图像创建成功，设置为缩略图
    if (!image.isNull()) {
        mBackImage = image;
        // 缩放到合适的大小，保持宽高比
        int thumbSize = THUMBNAIL_MIN_SIZE; // 120
        if (image.width() > image.height()) {
            int newHeight = thumbSize * image.height() / image.width();
            mBackImage = mBackImage.scaled(thumbSize, newHeight, Qt::KeepAspectRatio);
        }
        else {
            int newWidth = thumbSize * image.width() / image.height();
            mBackImage = mBackImage.scaled(newWidth, thumbSize, Qt::KeepAspectRatio);
        }
        //update(); // 重绘缩略图
    }
}

QImage& CyThumbnailView::backImage() {
    return mBackImage;
}

void CyThumbnailView::setBackgroundColor(QColor color) {
    mBackGroundColor = color;
}

void CyThumbnailView::setSelectColor(QColor color) {
    color.setAlpha(0xFF);
    mSelectRectColor = color;

    color.setAlpha(mSelectRectColor_transparent.alpha());
    mSelectRectColor_transparent = color;
}

void CyThumbnailView::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    // 绘制场景缩略图
    if (m_scene) {
        //int thumbWidth = width();
        //int thumbHeight = height();
        //// 创建缩略图
        //QPixmap thumbnail(thumbWidth, thumbHeight);
        //thumbnail.fill(Qt::white);
        //QPainter painter(&thumbnail);
        //painter.setRenderHint(QPainter::Antialiasing);
        //painter.setRenderHint(QPainter::SmoothPixmapTransform);
        //// 绘制场景到缩略图
        //m_scene->render(&painter, QRect(0, 0, thumbWidth, thumbHeight), m_scene->sceneRect());
        //painter.end();
        //// 绘制缩略图
        //QPainter painter2(this);
        //painter2.drawPixmap(0, 0, thumbnail);
        //// 绘制视图矩形
        //painter2.setPen(Qt::red);
        //painter2.drawRect(mViewRect.toRect());

        // 绘制背景
        QPainter painter(this);
        if (!mBackImage.isNull()) {
            painter.drawImage(rect(), mBackImage);
        }
        else {
            painter.setBrush(mBackGroundColor);
            painter.setPen(Qt::transparent);
            painter.drawRect(rect());
        }

        // 绘制视图矩形
        painter.setBrush(mSelectRectColor_transparent);
        painter.setPen(mSelectRectColor);
        auto rectI = QRect(mViewRect.x(), mViewRect.y(), mViewRect.width(), mViewRect.height());
        painter.drawRect(rectI);
    }
}

void CyThumbnailView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_parentView) {
        mDragging = true;
        setCursor(Qt::ClosedHandCursor);
        // 计算点击点在小窗内的局部比例（0=左上, 1=右下）
        double localX = (event->x() - mViewRect.x()) / mViewRect.width();
        double localY = (event->y() - mViewRect.y()) / mViewRect.height();
        mClickOffsetRatio = QPointF(
            qBound(0.0, localX, 1.0),
            qBound(0.0, localY, 1.0)
        );
    }
    QWidget::mousePressEvent(event);
}

void CyThumbnailView::mouseMoveEvent(QMouseEvent* event) {
    if (mDragging && m_parentView && m_parentView->scene()) {
        // 根据鼠标位置和点击偏移，反推小窗左上角应在的位置
        double desiredCenterX = event->x() - mClickOffsetRatio.x() * mViewRect.width();
        double desiredCenterY = event->y() - mClickOffsetRatio.y() * mViewRect.height();

        // 转换为在整个缩略图中的比例 [0.0 ~ 1.0]
        double xRatio = qBound(0.0, desiredCenterX / width(), 1.0);
        double yRatio = qBound(0.0, desiredCenterY / height(), 1.0);

        // 通知主视图更新视口
        m_parentView->setViewFromThumbnailPos(xRatio, yRatio);

        // 注意：updateThumbnail() 会在主视图滚动后被调用（通过 onScrollValueChanged）
        // 所以这里不需要手动 update()
    }
    QWidget::mouseMoveEvent(event);
}

void CyThumbnailView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mDragging = false;
        setCursor(Qt::ArrowCursor);
    }
    QWidget::mouseReleaseEvent(event);
}

#include "CyMediaDisView.moc"
