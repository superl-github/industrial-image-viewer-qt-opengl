#pragma once
#include "CyDisDrawItem.h"

namespace CyDisDrawItem {
    //====== class CyDisDrawItem::BaseItem ======
    class HandleItem;
    class BaseItem : public QGraphicsObject {
        Q_OBJECT
    public:
        explicit BaseItem(QGraphicsItem* parent = nullptr);
        virtual ~BaseItem();

    public:
        void pathToMask(const QPainterPath& scenePath, QImage& maskImage);

        QUuid id() const { return m_id; }
        void setid(QUuid& id) { m_id = id; }

        virtual ItemType itemType() const = 0;

        virtual QRectF boundingRect() const = 0;
        virtual QRect boundingRectInScene() const = 0;

        virtual QPainterPath shape() const override;
        virtual QPainterPath pathInScene() const = 0;

        virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) = 0;

        virtual void setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals = true) = 0;

        virtual void setPreviewMode(bool preview);

        virtual void setHandleColor(QColor color);
        virtual void setHandlesVisible(bool visible);

        void setUnSelectedContourColor(QColor color);
        void setSelectedContourColor(QColor color);

    public:signals:
        void removeClicked();

        void geometryChanged(); // 适用于有 rect 的 item
        void selectedChanged();
        
    protected:
        QUuid m_id;

        QColor m_contour_color_unselect = Qt::gray;
        QColor m_contour_color_select = QColor(0x2a, 0xa3, 0xc6);
        QColor m_handleColor = Qt::white;

        bool m_trackingMove = false;
        bool m_positionChangedDuringDrag = false;

        bool m_isPreviewMode = false;

        QList<HandleItem*> m_handles;
        bool m_handlesVisible = true;

        virtual QPoint getHandlePos(HandlePosition type) = 0;
        virtual QPoint getHandlePosInScene(HandlePosition type) = 0;
        void updateHandles();
        void removeHandles();

        virtual QPoint constrainToSceneByPos(const QPoint& pos) const = 0;

        virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
        virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
        virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event)override;
        virtual void onContextMenuCreate(QMenu& menu) {};
        virtual void onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) {};

        virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    private:
        friend class HandleItem;
        virtual bool changeByHandle(HandlePosition handletype, QPointF mousePos, QPointF delta) = 0;
    };



    //====== class CyDisDrawItem::HandleItem ======
    class HandleItem : public QGraphicsItem {
    public:
        explicit HandleItem(HandlePosition pos, BaseItem* parent);

    public:
        static int handleSize();

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
        static const int m_handleSize = 8;
        HandlePosition m_type;
        BaseItem* m_parent;
        QColor m_color = Qt::white;

        bool m_isResizing = false;
        QPointF m_dragStartScenePos; // 用于 Center 手柄计算偏移

    };
}