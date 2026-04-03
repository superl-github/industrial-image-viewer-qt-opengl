// HandleItem.h
#pragma once
#include <QGraphicsItem>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>

class CyMediaDisBaseItem;
class CyMediaDisHandleItem : public QGraphicsItem {
public:
    enum HandlePosition {
        TopLeft = 0, Top, TopRight,
        Left, Center, Right,
        BottomLeft, Bottom, BottomRight,
    };
public:
    explicit CyMediaDisHandleItem(HandlePosition pos, CyMediaDisBaseItem* parent);

public:
    static int handleSize ();

    HandlePosition position() const { return m_type; }

    QPointF posFromRect(const QRectF& rect);
    QRectF boundingRect() const override;
    void setColor(QColor color);

private:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    HandlePosition m_type;
    CyMediaDisBaseItem* m_parent;
    QColor m_color = Qt::white;
    
    bool m_isResizing = false;
    QPointF m_dragStartScenePos; // 用于 Center 手柄计算偏移
};