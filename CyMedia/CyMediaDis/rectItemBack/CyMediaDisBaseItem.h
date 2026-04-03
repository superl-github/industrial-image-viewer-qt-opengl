// CyMediaDisBaseItem.h
#pragma once
#include "CyMediaDisHandleItem.h"

#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsScene>
#include <QRectF>
#include <QPointF>
#include <QPainterPath>
#include <QCursor>
#include <QUuid>
#include <QMenu>

class CyMediaDisBaseItem : public QGraphicsObject {
    Q_OBJECT

public:
    enum class ItemType {
        Invalid = 0,
        Rectangle,
        Line,
        Ellipse, 
        //Text, Image...
    };

public:
    explicit CyMediaDisBaseItem(QGraphicsItem* parent = nullptr);
    virtual ~CyMediaDisBaseItem();

public:
    static QImage pathToMask(const QPainterPath& scenePath, const QSize& imageSize);
    void pathToMask(const QPainterPath& scenePath, QImage& maskImage);
    static void pathToMask(const QPainterPath& scenePath, const QSize& imageSize, std::vector<uint8_t>& outMask);

    QUuid id() const { return m_id; }
    void setid(QUuid& id) { m_id = id; }

    virtual ItemType itemType() const = 0;

    virtual QRectF boundingRect() const = 0;
    virtual QRectF boundingRectInScene() const = 0;

    virtual QPainterPath shape() const override;
    virtual QPainterPath pathInScene() const = 0;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) = 0;

    virtual void setBoundingRectInScene(const QPointF p1, const QPointF p2, bool needSignals = true) = 0;

    virtual void setPreviewMode(bool preview);
    
    virtual void setHandleColor(QColor color);
    virtual void setHandlesVisible(bool visible);

    void setUnSelectedContourColor(QColor color);
    void setSelectedContourColor(QColor color);

public:signals:
    void geometryChanged(); // 适用于有 rect 的 item
    void selectedChanged();

protected:
    QUuid m_id;

    QColor m_contour_color_unselect = Qt::gray;
    QColor m_contour_color_select = Qt::yellow;
    QColor m_handleColor = Qt::white;

    bool m_trackingMove = false;
    bool m_positionChangedDuringDrag = false;

    bool m_isPreviewMode = false;

    QList<CyMediaDisHandleItem*> m_handles;
    bool m_handlesVisible = true;

    virtual QPointF getHandlePos(CyMediaDisHandleItem::HandlePosition type) = 0;
    virtual QPointF getHandlePosInScene(CyMediaDisHandleItem::HandlePosition type) = 0;
    void updateHandles();
    void removeHandles();

    virtual QPointF constrainToSceneByPos(const QPointF& pos) const = 0;

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
    friend class CyMediaDisHandleItem;
    virtual bool changeByHandle(CyMediaDisHandleItem::HandlePosition handletype, QPointF mousePos, QPointF delta) = 0;

};