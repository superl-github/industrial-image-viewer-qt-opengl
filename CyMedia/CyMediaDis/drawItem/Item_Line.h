#pragma once
#include "BaseItem.h"

namespace CyDisDrawItem {
    class CyMediaDisLineItem_Menu_geo;
    class Item_Line : public BaseItem {
        Q_OBJECT

    public:
        explicit Item_Line(QGraphicsItem* parent = nullptr);
        ~Item_Line();

    public:
        ItemType itemType() const override { return ItemType::Line; }
        QRectF boundingRect() const override;
        QRect boundingRectInScene() const override;
        QPainterPath shape() const override;
        QPainterPath pathInScene() const override;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
        void setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals = true) override;

        QLine lineInScene() const;
        void setLineInScene(const QLine& line, bool needSignals = true);
        void setPainterPathInScene(QPainterPath path, bool needSignals = true) override;

    private:
        virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
        QPoint constrainToSceneByPos(const QPoint& pos) const override;
        bool changeByHandle(CyDisDrawItem::HandlePosition handletype, QPointF mousePos, QPointF delta)override;

        void createHandles();
        QPoint getHandlePos(CyDisDrawItem::HandlePosition type)override;
        QPoint getHandlePosInScene(CyDisDrawItem::HandlePosition type)override;
        void onContextMenuCreate(QMenu& menu) override;
        void onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) override;

    private:
        QPoint m_offsetP1;
        QPoint m_offsetP2;
        CyMediaDisLineItem_Menu_geo* m_Menu = nullptr;
    };
};