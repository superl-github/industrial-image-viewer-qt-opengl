#pragma once
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QUuid>
#include <QAction>
#include <QPainter>

#include <vector>

namespace CyDisDrawItem {
    enum ItemType {
        Invalid = 0,
        Point,
        Rectangle,
        Line,
        Ellipse,
        //Text, Image...
    };

    enum HandlePosition {
        TopLeft = 0, Top, TopRight,
        Left, Center, Right,
        BottomLeft, Bottom, BottomRight,
    };

    QImage pathToMask(const QPainterPath& scenePath, const QSize& imageSize);
    void pathToMask(const QPainterPath& scenePath, const QSize& imageSize, std::vector<uint8_t>& outMask);

    QPixmap drawItemIcon(ItemType type, int size = 24, QColor color = Qt::black);
};