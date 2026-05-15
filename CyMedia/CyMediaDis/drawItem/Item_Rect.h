#pragma once
#include "BaseItem.h"
#include <QVector>
#include <QDialog>

namespace CyDisDrawItem {
    class Item_Rect : public BaseItem {
        Q_OBJECT

    public:
        explicit Item_Rect(QGraphicsItem* parent = nullptr);

    public:
        //属性
        ItemType itemType() const override { return ItemType::Rectangle; }
        QRectF boundingRect() const override;
        QRect boundingRectInScene() const override;
        QPainterPath shape() const override;
        QPainterPath pathInScene() const;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
        
        //更新形状
        void setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals = true) override;
        void setPainterPathInScene(QPainterPath path, bool needSignals = true) override;

    private:
        //绘制
        bool onDrawMouseEvent(QEvent::Type type, const QPointF& scenePos) override;
        bool isDrawFinished() const override;

        QPoint constrainToSceneByPos(const QPoint& pos) const override;
        bool changeByHandle(CyDisDrawItem::HandlePosition handletype, int id, QPointF mousePos, QPointF delta)override;

        void createHandles();
        QPoint getHandlePos(CyDisDrawItem::HandlePosition type, int id = 0) override;
        QPoint getHandlePosInScene(CyDisDrawItem::HandlePosition type, int id = 0)override;
		//右键菜单
        void onContexMenu(ContextMenuType type, QGraphicsSceneContextMenuEvent* event) override;

        QRect constrainToSceneByPos(const QRect& r) const;

    private:
        QRect m_localRect;
        QSize m_MinSize{ 1, 1 };

        // 绘制过程状态
        bool m_drawFinished = false;
        QPointF m_drawStartPos;
    };
}