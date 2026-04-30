#include "BaseItem.h"
#include <QVector>
#include <QDialog>

namespace CyDisDrawItem {
    class Item_Point : public BaseItem {
        Q_OBJECT

    public:
        explicit Item_Point(QGraphicsItem* parent = nullptr);

    public:
        //属性
        ItemType itemType() const override { return ItemType::Point; }
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
        void onContextMenuCreate(QMenu& menu) override;
        void onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) override;

        QRect constrainToSceneByPos(const QRect& r) const;

    private:
        QRect m_localRect;
        static const int32_t mOuterframeLenth = 3;

        bool m_drawFinished = false;
    };
}