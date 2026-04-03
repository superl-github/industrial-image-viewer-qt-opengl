// CyMediaDisBaseItem.cpp
#include "CyMediaDisBaseItem.h"
#include <QPainter>
#include <QGraphicsScene>

CyMediaDisBaseItem::CyMediaDisBaseItem(QGraphicsItem* parent /*= nullptr*/) 
    : QGraphicsObject(parent)
    , m_id(QUuid::createUuid()){
    this->setFlags(
        QGraphicsItem::ItemIsSelectable |
        QGraphicsItem::ItemIsMovable |
        QGraphicsItem::ItemIsFocusable |
        QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    
    //调试
    connect(this, &CyMediaDisBaseItem::geometryChanged, this, [this]() {
        CyMediaDisBaseItem* senderItem = qobject_cast<CyMediaDisBaseItem*>(sender());
        auto rect = senderItem->boundingRectInScene();
        printf("geometryChanged:(%.0f,%.0f) %.0f * %.0f\n", rect.x(), rect.y(), rect.width(), rect.height());
        });
}

CyMediaDisBaseItem::~CyMediaDisBaseItem() {
    removeHandles();
}

QImage CyMediaDisBaseItem::pathToMask(const QPainterPath& scenePath, const QSize& imageSize) {
    QImage mask(imageSize, QImage::Format_Grayscale8);
    mask.fill(Qt::black); // 0 = background

    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing, false); // 关闭抗锯齿，得到硬边缘
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white); // 255 = foreground

    // 将 scene 坐标映射到图像像素坐标（假设 scene 原点 = 图像原点，1:1）
    // 如果有缩放/偏移，需传入 QTransform
    painter.drawPath(scenePath);

    return mask;
}

void CyMediaDisBaseItem::pathToMask(const QPainterPath& scenePath, const QSize& imageSize, std::vector<uint8_t>& outMask) {
    const int width = imageSize.width();
    const int height = imageSize.height();
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

    outMask.assign(pixelCount, 0);

    if (width <= 0 || height <= 0 || scenePath.isEmpty()) {
        return;
    }

    QImage maskImg(
        reinterpret_cast<uchar*>(outMask.data()), // 外部数据指针
        width,
        height,
        width, // bytesPerLine = width (no padding)
        QImage::Format_Grayscale8
    );

    QPainter painter(&maskImg);
    painter.setRenderHint(QPainter::Antialiasing, false); // 关闭抗锯齿 → 硬边缘
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawPath(scenePath);
    painter.end();
}

void CyMediaDisBaseItem::pathToMask(const QPainterPath& scenePath, QImage& maskImage) {
    if (maskImage.isNull() || maskImage.format() != QImage::Format_Grayscale8) {
        return;
    }

    // 清空为黑色背景
    maskImage.fill(Qt::black);

    if (scenePath.isEmpty()) return;

    QPainter painter(&maskImage);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawPath(scenePath);
}

QPainterPath CyMediaDisBaseItem::shape() const {
    {
        QPainterPath path;
        path.addRect(boundingRect());
        return path;
    };
}

QPainterPath CyMediaDisBaseItem::pathInScene() const {
    return QPainterPath();
}

void CyMediaDisBaseItem::setPreviewMode(bool preview) {
    if (m_isPreviewMode == preview) return;

    m_isPreviewMode = preview;

    if (preview) {
        setFlag(ItemIsSelectable, false);
        setFlag(ItemIsMovable, false);
        setHandlesVisible(false);
    }
    else {
        // 正常模式：恢复交互
        setFlag(ItemIsSelectable, true);
        setFlag(ItemIsMovable, true);
        setHandlesVisible(isSelected());
    }
}

void CyMediaDisBaseItem::setHandleColor(QColor color) {
    if (m_handles.size() <= 0)
        return;

    for (auto* handle : m_handles) {
        handle->setColor(color);
    }

    if (m_handlesVisible) {
        update();
    }
}

void CyMediaDisBaseItem::setHandlesVisible(bool visible) {
    if (m_handlesVisible == visible)
        return;
    m_handlesVisible = visible;
    for (auto* handle : m_handles) {
        handle->setVisible(visible && isSelected());
    }
}

void CyMediaDisBaseItem::setUnSelectedContourColor(QColor color) {
    m_contour_color_unselect = color;
    update();
}

void CyMediaDisBaseItem::setSelectedContourColor(QColor color) {
    m_contour_color_select = color;
    update();
}

void CyMediaDisBaseItem::updateHandles() {
    if (m_handles.isEmpty())
        return;

    for (auto* handle : std::as_const(m_handles)) {
        handle->setPos(getHandlePos(handle->position()));
    }
}

void CyMediaDisBaseItem::removeHandles() {
    for (auto* handle : std::as_const(m_handles)) {
        if (handle->scene()) {
            handle->scene()->removeItem(handle);
        }
        delete handle;
    }
}

QVariant CyMediaDisBaseItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemPositionChange) {
        if (scene()) {
            QPointF newPos = value.toPointF();
            newPos = constrainToSceneByPos(newPos);
            return newPos; // 返回修正后的 scene 位置
        }
    }
    else if (change == QGraphicsItem::ItemSelectedHasChanged) {
        setHandlesVisible(value.toBool());
        emit selectedChanged();
    }

    if (change == ItemPositionHasChanged) {
        // 只要位置变了（且可移动），就标记
        m_positionChangedDuringDrag = true;
        if (m_trackingMove) {
            emit geometryChanged();
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void CyMediaDisBaseItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
        m_positionChangedDuringDrag = false; // 每次按下重置
    }
    QGraphicsItem::mousePressEvent(event);
}

void CyMediaDisBaseItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsObject::mouseMoveEvent(event);
}

void CyMediaDisBaseItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsObject::mouseReleaseEvent(event);

    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ArrowCursor);
        if (m_positionChangedDuringDrag) {
            m_positionChangedDuringDrag = false;
            if (false == m_trackingMove) {
                emit geometryChanged();
            }
        }
    }
}

void CyMediaDisBaseItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    onContextMenuCreate(menu);
    if (menu.actions().size() <= 0)
        return QGraphicsObject::contextMenuEvent(event);
    
    QAction* selectedAction = menu.exec(event->screenPos());
    onContexMenu(selectedAction, event);
    event->accept();
}

void CyMediaDisBaseItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    setCursor(Qt::ArrowCursor);
    QGraphicsObject::hoverLeaveEvent(event);
}

void CyMediaDisBaseItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    setCursor(Qt::ArrowCursor);
    QGraphicsObject::hoverLeaveEvent(event);
}
