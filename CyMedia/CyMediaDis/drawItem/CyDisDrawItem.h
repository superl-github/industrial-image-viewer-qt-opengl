/**
 * @file CyDisDrawItem.h
 * @brief 绘图模块的全局枚举、类型定义及工具函数。
 *
 * 本文件定义了整个绘图子系统的基础设施：
 * - ItemType：所有可绘制图形类型的枚举（包括 Point、Rectangle、Line、Ellipse、Polygon）。
 * - HandlePosition：图形编辑手柄的位置枚举（用于缩放/平移）。
 * - pathToMask：将 QPainterPath 转换为二值掩码图像的工具函数。
 * - drawItemIcon：根据 ItemType 生成对应的工具栏图标。
 *
 * 该文件被所有图形项类、工厂类及工具类包含，应保持轻量，避免引入具体类的依赖。
 */
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