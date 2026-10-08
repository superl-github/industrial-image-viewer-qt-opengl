#include "CyMediaDisViewThumbnail.h"
#include "CyMediaDisView.h"
#include "CyMediaDisViewBckDraw.h"
#include "CyMediaCalc/CyMediaCalc.h"

CyMediaDisViewThumbnail::CyMediaDisViewThumbnail(CyMediaDisView* parentView, QWidget* parent /*= nullptr*/)
: QOpenGLWidget(parent)
, m_parentView(parentView) {

    // 让 FBO 带 alpha 通道
    QSurfaceFormat fmt = format();
    fmt.setAlphaBufferSize(8);
    fmt.setSamples(4);            // 可选：抗锯齿
    setFormat(fmt);

    setAttribute(Qt::WA_TranslucentBackground);// 启用透明背景
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    // 透明方案：保留 parent + WA_AlwaysStackOnTop。
    //
    // 【为什么必须 WA_AlwaysStackOnTop】QOpenGLWidget 作为普通子对象时，
    //   Qt 会把它的 FBO 当不透明子 widget 塞进父的合成流程，FBO 的 alpha
    //   被当不透明处理，透明"物理上"不成立（表现为黑底或糊父背景）——
    //   这是 Qt 渲染机制决定的，不是配置问题。WA_AlwaysStackOnTop 让 Qt
    //   单独合成、最后叠加，alpha 才能与父正确混合。删掉它透明立刻失效。
    //
    // 【代价】缩略图永远置顶，z 序失效。当前是唯一浮层，代价基本为零。
    //
    // 【备选】去掉 parent 做独立顶层窗口也能透明，但需手动同步位置、
    //   父窗口 moveEvent 不触发时还需 eventFilter，维护成本高，故不采用。
    setAttribute(Qt::WA_AlwaysStackOnTop);// 透明生效的必要条件，勿删

    //默认值
    m_boder_pen = QPen(QColor(0x00, 0xEE, 0x00), 3);
}

CyMediaDisViewThumbnail::~CyMediaDisViewThumbnail() {
    if (m_vao) {
        // VAO 必须在创建它的 GL 上下文中析构
        QOpenGLContext* ctx = context();
        if (ctx && ctx->isValid()) {
            makeCurrent();
            delete m_vao;
            doneCurrent();
        }
        else {
            // 上下文已失效，无法安全释放 GL 资源，避免野指针
            delete m_vao;   // QOpenGLVertexArrayObject 内部会判断
        }
        m_vao = nullptr;
    }
}

void CyMediaDisViewThumbnail::setViewRect(const QRectF& rect) {
    mViewRect = rect;
    update();
}

bool CyMediaDisViewThumbnail::isBeingDragged() const {
    return mDragging;
}

void CyMediaDisViewThumbnail::setThumbnailSize(const QSize& size) {
    resize(size);
}

void CyMediaDisViewThumbnail::setSelectColor(QColor color) {
    color.setAlpha(0xFF);
    mSelectRectColor = color;

    color.setAlpha(mSelectRectColor_transparent.alpha());
    mSelectRectColor_transparent = color;
}


bool CyMediaDisViewThumbnail::drawImage() const {
    return m_draw_image;
}


void CyMediaDisViewThumbnail::setDrawImage(bool draw) {
    m_draw_image = draw;
}


QPen CyMediaDisViewThumbnail::borderPen() const {
    return m_boder_pen;
}


void CyMediaDisViewThumbnail::setBorderPen(QPen pen) {
    m_boder_pen = pen;
}

void CyMediaDisViewThumbnail::paintGL() {
    if (!m_parentView) return;
    auto f = context()->extraFunctions();
    if (!f) return;
    QPainter painter(this);

    //绘图
    painter.beginNativePainting();
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CyMediaDisViewBckDraw* drawer = m_parentView->imageDraw();
    if (drawer && drawer->glIsInit() && drawer->haveImage() && m_draw_image) {
        QSizeF imgSize = m_parentView->scene()->sceneRect().size();
        if (imgSize.isEmpty() || imgSize.width() <= 0 || imgSize.height() <= 0) return;
        QTransform transform;
        double scale = qMin(width() / imgSize.width(), height() / imgSize.height());
        double dx = (width() - imgSize.width() * scale) / 2.0;
        double dy = (height() - imgSize.height() * scale) / 2.0;
        transform.translate(dx, dy);
        transform.scale(scale, scale);

        int physWidth = width() * devicePixelRatioF();
        int physHeight = height() * devicePixelRatioF();
        drawer->renderTexture(f, m_vao, rect(), physWidth, physHeight, transform);
    }
    f->glDisable(GL_BLEND);
    painter.endNativePainting();

    //选框
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    QRectF drawRect = getDrawRect();
    painter.setBrush(mSelectRectColor_transparent);
    painter.setPen(mSelectRectColor);
    painter.drawRect(drawRect);

    //边框
    painter.setPen(m_boder_pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect());
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
    if (m_parentView && m_parentView->imageDraw()) {
        if (m_vao) {
            delete m_vao;
            m_vao = nullptr;
        }
        m_vao = m_parentView->imageDraw()->createVAO();
    }
    else {
        ;//记录日志
    }
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
        if (m_pressThumbRect.width() <= 0 || m_pressThumbRect.height() <= 0) return QWidget::mouseMoveEvent(event);;
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
    if (w <= 0 || h <= 0) return mViewRect;
    if (w < MIN_RECT_SIZE || h < MIN_RECT_SIZE) {
        qreal scale = qMax(MIN_RECT_SIZE / w, MIN_RECT_SIZE / h);
        QPointF center = drawRect.center();
        drawRect = QRectF(center.x() - w * scale / 2, center.y() - h * scale / 2,
            w * scale, h * scale);
    }
    return drawRect;
}
