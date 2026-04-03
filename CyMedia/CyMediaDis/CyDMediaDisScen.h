#pragma once

#include <QGraphicsScene>
#include <QUrl>

class CyDMediaDisScen : public QGraphicsScene {
    Q_OBJECT

public:
    CyDMediaDisScen(qreal x, qreal y, qreal width, qreal height, QObject* parent = nullptr);
    ~CyDMediaDisScen();

public:signals:
    void mousePosChange(int x, int y);
    void urlsDrop(QList<QUrl> urls);

public:
    bool acceptDrops();
    void setAcceptDrops(bool accept);

    bool tipTextVisible();
    void setTipTextVisible(bool visi);

    void setBackDis(QGraphicsItem* back);
    QGraphicsItem* BackDis();
    QGraphicsItem* itemAtWithoutBack(const QPointF& pos, const QTransform& deviceTransform);

    QImage backToQImage() const;

public:
    void drawBackground(QPainter* painter, const QRectF& rect)override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event)override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event)override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event)override;

    void dragMoveEvent(QGraphicsSceneDragDropEvent* event)override;
    void dropEvent(QGraphicsSceneDragDropEvent* event)override;

private:
    class PrivateData;
    PrivateData* d = 0;
};
