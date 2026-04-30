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
        //属性
        ItemType itemType() const override { return ItemType::Line; }
        QRectF boundingRect() const override;
        QRect boundingRectInScene() const override;
        QPainterPath shape() const override;
        QPainterPath pathInScene() const override;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
        
        //更新形状
        void setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals = true) override;
        QLine lineInScene() const;
        void setLineInScene(const QLine& line, bool needSignals = true);
        void setPainterPathInScene(QPainterPath path, bool needSignals = true) override;

    private:
        //绘制
        bool onDrawMouseEvent(QEvent::Type type, const QPointF& scenePos) override;
        bool isDrawFinished() const override;

        virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
        QPoint constrainToSceneByPos(const QPoint& pos) const override;
        bool changeByHandle(CyDisDrawItem::HandlePosition handletype, int id, QPointF mousePos, QPointF delta)override;

        void createHandles();
        QPoint getHandlePos(CyDisDrawItem::HandlePosition type, int id = 0)override;
        QPoint getHandlePosInScene(CyDisDrawItem::HandlePosition type, int id = 0)override;
        void onContextMenuCreate(QMenu& menu) override;
        void onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) override;

    private:
        QPoint m_offsetP1;
        QPoint m_offsetP2;
        CyMediaDisLineItem_Menu_geo* m_Menu = nullptr;

        bool m_drawFinished = false;
        QPointF m_drawStartPos;
    };
};