#pragma once

#include "../../CyMediaBaseDef.h"

#include <vector>

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QUuid>
#include <QAction>
#include <QPainter>

namespace CyDisDrawItem {
    enum ItemType {
        Invalid = 0,
        Point,
        Rectangle,
        Line,
        Ellipse,
        Polygon,
        //Text, Image...
    };

    enum HandlePosition {
        TopLeft = 0, Top, TopRight,
        Left, Center, Right,
        BottomLeft, Bottom, BottomRight,
        Free,
    };

    QImage CYMEDIA_LIB pathToMask(const QPainterPath& scenePath, const QSize& imageSize);
    bool CYMEDIA_LIB pathToMask(const QPainterPath& scenePath, const QSize& imageSize, std::vector<uint8_t>& outMask);

    QPixmap CYMEDIA_LIB drawItemIcon(ItemType type, int size = 24, QColor color = Qt::black);
};