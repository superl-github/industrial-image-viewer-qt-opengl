#include "BaseItem.h"

namespace CyDisDrawItem {
    //====== class CyDisDrawItem::BaseItem ======

    BaseItem::BaseItem(QGraphicsItem* parent /*= nullptr*/)
        : QGraphicsObject(parent)
        , m_id(QUuid::createUuid()) {
        this->setFlags(
            QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsMovable |
            QGraphicsItem::ItemIsFocusable |
            QGraphicsItem::ItemSendsGeometryChanges);
        setAcceptHoverEvents(true);

        //调试
        connect(this, &BaseItem::geometryChanged, this, [this]() {
            CyDisDrawItem::BaseItem* senderItem = qobject_cast<CyDisDrawItem::BaseItem*>(sender());
            auto rect = senderItem->boundingRectInScene();
            printf("geometryChanged:(%d,%d) %d * %d\n", rect.x(), rect.y(), rect.width(), rect.height());
            });
    }

    BaseItem::~BaseItem() {
        removeHandles();
    }

    void BaseItem::pathToMask(const QPainterPath& scenePath, QImage& maskImage) {
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

    QPainterPath BaseItem::shape() const {
        {
            QPainterPath path;
            path.addRect(boundingRect());
            return path;
        };
    }

    QPainterPath BaseItem::pathInScene() const {
        return QPainterPath();
    }

    void BaseItem::setPreviewMode(bool preview) {
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

    void BaseItem::setHandleColor(QColor color) {
        if (m_handles.size() <= 0)
            return;

        for (auto* handle : m_handles) {
            handle->setColor(color);
        }

        if (m_handlesVisible) {
            update();
        }
    }

    void BaseItem::setHandlesVisible(bool visible) {
        if (m_handlesVisible == visible)
            return;
        m_handlesVisible = visible;
        for (auto* handle : m_handles) {
            handle->setVisible(visible && isSelected());
        }
    }

    void BaseItem::setUnSelectedContourColor(QColor color) {
        m_contour_color_unselect = color;
        update();
    }

    void BaseItem::setSelectedContourColor(QColor color) {
        m_contour_color_select = color;
        update();
    }

    void BaseItem::updateHandles() {
        if (m_handles.isEmpty())
            return;

        for (auto* handle : std::as_const(m_handles)) {
            handle->setPos(getHandlePos(handle->position()));
        }
    }

    void BaseItem::removeHandles() {
        for (auto* handle : std::as_const(m_handles)) {
            if (handle->scene()) {
                handle->scene()->removeItem(handle);
            }
            delete handle;
        }
    }

    QVariant BaseItem::itemChange(GraphicsItemChange change, const QVariant& value) {
        if (change == QGraphicsItem::ItemPositionChange) {
            if (scene()) {
                QPoint newPos = value.toPointF().toPoint();
                newPos = constrainToSceneByPos(newPos);
                newPos.setX(int(newPos.x()));
                newPos.setY(int(newPos.y()));
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

    void BaseItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            setCursor(Qt::ClosedHandCursor);
            m_positionChangedDuringDrag = false; // 每次按下重置
        }
        QGraphicsItem::mousePressEvent(event);
    }

    void BaseItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
        QGraphicsObject::mouseMoveEvent(event);
    }

    void BaseItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
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

    void BaseItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
        QMenu menu;
        onContextMenuCreate(menu);
        if (menu.actions().size() <= 0)
            return QGraphicsObject::contextMenuEvent(event);

        QAction* selectedAction = menu.exec(event->screenPos());
        onContexMenu(selectedAction, event);
        event->accept();
    }

    void BaseItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
        setCursor(Qt::ArrowCursor);
        QGraphicsObject::hoverLeaveEvent(event);
    }

    void BaseItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
        setCursor(Qt::ArrowCursor);
        QGraphicsObject::hoverLeaveEvent(event);
    }





    //====== class CyDisDrawItem::HandleItem ======
    HandleItem::HandleItem(HandlePosition pos, BaseItem* parent)
        : QGraphicsItem(parent)
        , m_parent(parent)
        , m_type(pos)
        , m_isResizing(false) {
        setFlag(QGraphicsItem::ItemIsMovable, false); // 由父类控制位置
        setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::SizeFDiagCursor); // 默认，后面会根据方向调整

        // 设置光标
        /*switch (pos) {
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
        }*/
        setCursor(Qt::PointingHandCursor);
    }

    int HandleItem::handleSize() {
        return m_handleSize;
    }

    QPointF HandleItem::posFromRect(const QRectF& rect) {
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

    QRectF HandleItem::boundingRect() const {
        return QRectF(-m_handleSize / 2, -m_handleSize / 2, m_handleSize, m_handleSize);
    }

    void HandleItem::setColor(QColor color) {
        m_color = color;
    }

    void HandleItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        painter->setPen(Qt::black);
        painter->setBrush(m_color);
        painter->drawRect(boundingRect());
        //painter->drawEllipse(boundingRect());
    }

    void HandleItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
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

    void HandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
        if (!m_isResizing) return;
        if (!m_parent || !m_parent->scene()) {
            event->ignore();
            return;
        }

        m_parent->changeByHandle(m_type, event->scenePos(), event->scenePos() - m_dragStartScenePos);

        m_dragStartScenePos = m_parent->getHandlePosInScene(m_type);
        event->accept();
    }

    void HandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
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
}