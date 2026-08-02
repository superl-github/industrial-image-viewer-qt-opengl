#include "CyMediaDisViewThumbnail.h"
#include "CyMediaDisView.h"
#include "CyMediaDisViewBckDraw.h"
#include "CyMediaCalc/CyMediaCalc.h"

CyMediaDisViewThumbnail::CyMediaDisViewThumbnail(CyMediaDisView* parentView, QWidget* parent /*= nullptr*/)
    : QOpenGLWidget(parent)
    , m_parentView(parentView) {
    
}

CyMediaDisViewThumbnail::~CyMediaDisViewThumbnail() {

}

void CyMediaDisViewThumbnail::setScene(QGraphicsScene* scene) {
    m_scene = scene;
}

void CyMediaDisViewThumbnail::setViewRect(const QRectF& rect) {
    mViewRect = rect;
    update();
}

bool CyMediaDisViewThumbnail::isBeingDragged() {
    return mDragging;
}

void CyMediaDisViewThumbnail::setThumbnailSize(const QSize& size) {
    resize(size);
}

void CyMediaDisViewThumbnail::setBackgroundColor(QColor color) {
    mBackGroundColor = color;
}

void CyMediaDisViewThumbnail::setSelectColor(QColor color) {
    color.setAlpha(0xFF);
    mSelectRectColor = color;

    color.setAlpha(mSelectRectColor_transparent.alpha());
    mSelectRectColor_transparent = color;
}

bool CyMediaDisViewThumbnail::drawBorder() {
    return mDrawColor;
}

void CyMediaDisViewThumbnail::setDrawBorder(bool draw) {
    mDrawColor = draw;
}

QColor CyMediaDisViewThumbnail::borderColor() {
    return mBorderColor;
}

void CyMediaDisViewThumbnail::setBorderColor(QColor color) {
    mBorderColor = color;
}

void CyMediaDisViewThumbnail::paintGL() {
    if (!m_parentView) return;
    //绘制背景图像
    CyMediaDisViewBckDraw* drawer = m_parentView->imageDraw();
    if (!drawer || !drawer->glIsInit() || !drawer->haveImage()) {
        QPainter painter(this);
        painter.fillRect(rect(), mBackGroundColor);
    }
    else {
        QSizeF imgSize = m_parentView->scene()->sceneRect().size();
        // 计算世界矩阵
        QTransform transform;
        double scale = qMin(width() / imgSize.width(), height() / imgSize.height());
        double dx = (width() - imgSize.width() * scale) / 2.0;
        double dy = (height() - imgSize.height() * scale) / 2.0;

        // 变换顺序（从右到左执行）
        transform.translate(dx, dy);                 // 4) 平移到缩略图中心
        transform.scale(scale, scale);               // 3) 缩放
        //transform.translate(imgSize.width() / 2.0, imgSize.height() / 2.0); // 2) 平移回图像中心（旋转中心）
        //if (m_parentView->isHriMirror()) transform.rotate(180.0, Qt::YAxis);
        //if (m_parentView->isVerMirror()) transform.rotate(180.0, Qt::XAxis);
        //transform.rotate(m_parentView->rotateValue());// 1) 旋转/镜像
        //transform.translate(-imgSize.width() / 2.0, -imgSize.height() / 2.0); // 0) 平移图像中心至原点
        //采用缩略图一直正向的方案，选框适应旋转/镜像，主图不变
        // 调用渲染器绘制纹理
        int physWidth = width() * devicePixelRatioF();
        int physHeight = height() * devicePixelRatioF();
        auto f = this->context()->extraFunctions();
        if (f) {
            f->glClearColor(0.0, 0.3, 0.3, 1.0);
            f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            drawer->renderTexture(f, m_vao, rect(), physWidth, physHeight, transform);
        }
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // 绘制选择框（现有逻辑）
    QRectF drawRect = getDrawRect();
    painter.setBrush(mSelectRectColor_transparent);
    painter.setPen(mSelectRectColor);
    painter.drawRect(drawRect);

    // 绘制边框
    if (mDrawColor) {
        painter.setPen(QPen(mBorderColor, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect());
    }
}


void CyMediaDisViewThumbnail::resizeGL(int w, int h) {

}

void CyMediaDisViewThumbnail::initializeGL() {
    // ===== 诊断代码 =====
    QOpenGLContext* currentCtx = QOpenGLContext::currentContext();
    if (currentCtx) {
        qDebug() << "[Thumbnail] Current context:" << (void*)currentCtx;
        qDebug() << "[Thumbnail] Share context:" << (void*)currentCtx->shareContext();
        qDebug() << "[Thumbnail] Format:" << currentCtx->format();

        // 检查是否和主窗口共享
        // 你可以在主窗口的 initializeGL 里也打印 context 指针
        // 如果 shareContext() 返回非 null，说明在同一 share group
    }

    m_vao = m_parentView->imageDraw()->createVAO();
}

void CyMediaDisViewThumbnail::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_parentView) {
        // 记录缩略图点击位置
        m_pressThumbPos = event->pos();
        // 记录当前场景矩形在缩略图上的映射（未钳制）
        m_pressThumbRect = mViewRect;
        // 记录视口左上角场景坐标
        m_pressSceneTopLeft = m_parentView->mapToScene(QPoint(0, 0));
        // 记录视口在场景中的大小
        QRect viewRect = m_parentView->viewport()->rect();
        QRectF sceneRect = m_parentView->mapToScene(viewRect).boundingRect();
        m_pressSceneSize = sceneRect.size();

        mDragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(event);
}

void CyMediaDisViewThumbnail::mouseMoveEvent(QMouseEvent* event) {
    if (mDragging && m_parentView && m_parentView->scene()) {
        QPointF deltaThumb = event->pos() - m_pressThumbPos;
        // 计算场景坐标平移量
        double scaleX = m_pressSceneSize.width() / m_pressThumbRect.width();
        double scaleY = m_pressSceneSize.height() / m_pressThumbRect.height();
        QPointF deltaScene(deltaThumb.x() * scaleX, deltaThumb.y() * scaleY);
        QPointF newTopLeft = m_pressSceneTopLeft + deltaScene;
        m_parentView->setViewTopLeft(newTopLeft);
    }
    QWidget::mouseMoveEvent(event);
}

void CyMediaDisViewThumbnail::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mDragging = false;
        setCursor(Qt::ArrowCursor);
    }
    QWidget::mouseReleaseEvent(event);
}

QRectF CyMediaDisViewThumbnail::getDrawRect() const {
    if (mViewRect.isEmpty())
        return mViewRect;

    QRectF drawRect = mViewRect;
    qreal w = drawRect.width();
    qreal h = drawRect.height();
    if (w < MIN_RECT_SIZE || h < MIN_RECT_SIZE) {
        qreal scale = qMax(MIN_RECT_SIZE / w, MIN_RECT_SIZE / h);
        QPointF center = drawRect.center();
        drawRect = QRectF(center.x() - w * scale / 2, center.y() - h * scale / 2,
            w * scale, h * scale);
    }
    return drawRect;
}
