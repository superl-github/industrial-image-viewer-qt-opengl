// HandleItem.cpp
#include "CyMediaDisHandleItem.h"
#include <QPainter>
#include <QCursor>
#include <QGraphicsScene>
#include <QDebug>

#include "CyMediaDisBaseItem.h"

static const int HANDLE_SIZE = 8;

CyMediaDisHandleItem::CyMediaDisHandleItem(HandlePosition pos, CyMediaDisBaseItem* parent)
    : QGraphicsItem(parent)
    , m_parent(parent)
    , m_type(pos)
    , m_isResizing(false) {
    setFlag(QGraphicsItem::ItemIsMovable, false); // 由父类控制位置
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setCursor(Qt::SizeFDiagCursor); // 默认，后面会根据方向调整

    // 设置光标
    switch (pos) {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Center:
            setCursor(Qt::SizeAllCursor);
            break;
    }
}

int CyMediaDisHandleItem::handleSize() {
    return HANDLE_SIZE;
}

QPointF CyMediaDisHandleItem::posFromRect(const QRectF& rect) {
    switch (m_type) {
        case TopLeft:      return rect.topLeft();
        case Top:          return QPointF(rect.center().x(), rect.top());
        case TopRight:     return rect.topRight();
        case Left:         return QPointF(rect.left(), rect.center().y());
        case Center:       return rect.center();
        case Right:        return QPointF(rect.right(), rect.center().y());
        case BottomLeft:   return rect.bottomLeft();
        case Bottom:       return QPointF(rect.center().x(), rect.bottom());
        case BottomRight:  return rect.bottomRight();
        default:           return rect.center();
    }
}

QRectF CyMediaDisHandleItem::boundingRect() const {
    return QRectF(-HANDLE_SIZE / 2, -HANDLE_SIZE / 2, HANDLE_SIZE, HANDLE_SIZE);
}

void CyMediaDisHandleItem::setColor(QColor color) {
    m_color = color;
}

void CyMediaDisHandleItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(Qt::black);
    painter->setBrush(m_color);
    painter->drawRect(boundingRect());
    //painter->drawEllipse(boundingRect());
}

void CyMediaDisHandleItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_parent && !m_parent->isSelected()) {
            m_parent->setSelected(true);
        }

        m_isResizing = true;
        m_dragStartScenePos = event->scenePos();
        grabMouse();
        event->accept();
    }
}

void CyMediaDisHandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (!m_isResizing) return;
    if (!m_parent || !m_parent->scene()) {
        event->ignore();
        return;
    }

    m_parent->changeByHandle(m_type, event->scenePos(), event->scenePos() - m_dragStartScenePos);

    m_dragStartScenePos = m_parent->getHandlePosInScene(m_type);
    event->accept();
}

void CyMediaDisHandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_isResizing) {
        m_isResizing = false;
        ungrabMouse();
        // 最终 emit 信号
        if (m_parent) {
            m_parent->emit geometryChanged();
        }
        event->accept();
    }
    
}
