#pragma once
#include "BaseItem.h"
#include <QList>
namespace CyDisDrawItem {
    class Item_Polygon : public BaseItem {
        Q_OBJECT
    public:
        explicit Item_Polygon(QGraphicsItem* parent = nullptr);
    public:
        //属性
        ItemType itemType() const override { return ItemType::Polygon; }
        QRectF boundingRect() const override;
        QRect boundingRectInScene() const override;
        QPainterPath shape() const override;
        QPainterPath pathInScene() const;

        //更新形状
        void setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals = true) override;
        void setPainterPathInScene(QPainterPath path, bool needSignals = true) override;

    private:
        // 绘制
        bool onDrawMouseEvent(QEvent::Type type, const QPointF& scenePos) override;
        bool isDrawFinished() const override;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
        QPoint constrainToSceneByPos(const QPoint& pos) const override;
        void createHandles();
        virtual void updateHandles() override;
        bool changeByHandle(CyDisDrawItem::HandlePosition handletype, int id, QPointF mousePos, QPointF delta)override;
        QPoint getHandlePos(CyDisDrawItem::HandlePosition type, int id = 0) override;
        QPoint getHandlePosInScene(CyDisDrawItem::HandlePosition type, int id = 0)override;
		//右键菜单
        void onContexMenu(ContextMenuType type, QGraphicsSceneContextMenuEvent* event) override;
        QRect constrainToSceneByPos(const QRect& r) const;
    private:
        QList<QPointF> m_points;          // 本地坐标顶点列表
        QPointF m_tempPoint;               // 绘制过程中的临时预览点
        bool m_drawFinished = false;       // 绘制是否完成
        QSize m_MinSize{ 10, 10 };        // 最小尺寸约束
    };
}
