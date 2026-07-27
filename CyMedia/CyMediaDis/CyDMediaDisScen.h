/*
*  @class CyDMediaDisScen
 * @brief QGraphicsScene 派生类，提供鼠标位置信号和文件拖放信号。
 * @details
 *   鼠标移动时发出 mousePosChange，接受 URL 拖放时发出 urlsDrop。
 *   通过 setAcceptDrops 开关拖放功能。
 * @author LLF
 * @date   July 2026
 * @version 1.0
 */
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
