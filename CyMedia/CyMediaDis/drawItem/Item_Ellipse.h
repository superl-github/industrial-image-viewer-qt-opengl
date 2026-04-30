#pragma once
#include "BaseItem.h"
#include <QVector>

namespace CyDisDrawItem {
    class Item_Ellipse : public BaseItem {
        Q_OBJECT

    public:
        explicit Item_Ellipse(QGraphicsItem* parent = nullptr);

    public:
        //属性
        ItemType itemType() const override { return ItemType::Ellipse; }
        QRectF boundingRect() const override;
        QRect boundingRectInScene() const override;
        QPainterPath shape() const override;
        QPainterPath pathInScene() const;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
        
        //更新形状
        void setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals = true) override;
        void setPainterPathInScene(QPainterPath path, bool needSignals = true) override;

    private:
        // 绘制过程事件接口
        bool onDrawMouseEvent(QEvent::Type type, const QPointF& scenePos) override;
        bool isDrawFinished() const override;

        QPoint constrainToSceneByPos(const QPoint& pos) const override;
        bool changeByHandle(CyDisDrawItem::HandlePosition handletype, int id, QPointF mousePos, QPointF delta) override;
        void createHandles();
        QPoint getHandlePos(CyDisDrawItem::HandlePosition type, int id = 0) override;
        QPoint getHandlePosInScene(CyDisDrawItem::HandlePosition type, int id = 0) override;
        void onContextMenuCreate(QMenu& menu) override;
        void onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) override;

        QRect constrainToSceneByPos(const QRect& r) const;

    private:
        QRect m_localRect;
        QSize m_MinSize{ 10, 10 };

        // 绘制过程状态
        bool m_drawFinished = false;
        QPointF m_drawStartPos;

        QPointF projectToEllipse(const QPointF& rectCorner, const QRectF& rect);
        static bool isCornerHandle(CyDisDrawItem::HandlePosition type);
    };
}