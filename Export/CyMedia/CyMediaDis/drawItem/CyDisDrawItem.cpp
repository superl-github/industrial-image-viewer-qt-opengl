#include "CyDisDrawItem.h"

#include <QDebug>

namespace CyDisDrawItem {
    //===== 工具函数 =====
    QImage pathToMask(const QPainterPath& scenePath, const QSize& imageSize) {
        QImage mask(imageSize, QImage::Format_Grayscale8);
        mask.fill(Qt::black); // 0 = background

        QPainter painter(&mask);
        painter.setRenderHint(QPainter::Antialiasing, false); // 关闭抗锯齿，得到硬边缘
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white); // 255 = foreground

        // 将 scene 坐标映射到图像像素坐标（假设 scene 原点 = 图像原点，1:1）
        // 如果有缩放/偏移，需传入 QTransform
        painter.drawPath(scenePath);

        return mask;
    }

    void pathToMask(const QPainterPath& scenePath, const QSize& imageSize, std::vector<uint8_t>& outMask) {
        const int width = imageSize.width();
        const int height = imageSize.height();
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

        outMask.assign(pixelCount, 0);

        if (width <= 0 || height <= 0 || scenePath.isEmpty()) {
            return;
        }

        QImage maskImg(
            reinterpret_cast<uchar*>(outMask.data()), // 外部数据指针
            width,
            height,
            width, // bytesPerLine = width (no padding)
            QImage::Format_Grayscale8
        );

        QPainter painter(&maskImg);
        painter.setRenderHint(QPainter::Antialiasing, false); // 关闭抗锯齿 → 硬边缘
        //painter.setPen(Qt::NoPen);
        QPen pen(Qt::white, 1); // 白色，宽度为1像素
        painter.setPen(pen);
        painter.setBrush(Qt::white);
        painter.drawPath(scenePath);
        painter.end();
    }




    // 绘制箭头图标
    QPixmap drawAarrowIcon(int size, QColor color) {
        // 创建透明背景的Pixmap
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 设置画笔（1px宽度，符合系统默认）
        QPen pen(color);
        pen.setWidth(1);
        painter.setPen(pen);

        // 计算箭头坐标
        int height = 10;
        int halfHeight = height / 2.0;
        int angle = 45;

        int positionAngle = 90 - angle;
        float angle_radian = positionAngle * 3.1415926 / 180.0;
        float width = height * std::cos(angle_radian);
        float height_angle = height * std::sin(angle_radian);

        int x = (size - width) / 2.0;
        int y = (size - height - halfHeight) / 2.0;
        QPointF leftTop(x, y);
        QPointF leftBottom(x, y + height);
        QPointF rightTip(x + width, y + height_angle);

        // 绘制箭头三角形
        QPolygonF polygon;
        polygon << leftTop << leftBottom << rightTip;
        painter.setBrush(color);
        painter.drawPolygon(polygon);

        //计算尾巴
        QPointF BottomCenter = QPointF((leftBottom.x() + rightTip.x()) / 2.0, (leftBottom.y() + rightTip.y()) / 2.0);
        QPointF Bottomvector = QPointF(abs(rightTip.y() - leftBottom.y()), rightTip.x() - leftBottom.x());
        float vectorLenth = sqrt(pow(Bottomvector.x(), 2) + pow(Bottomvector.y(), 2));
        Bottomvector = QPointF(Bottomvector.x() / vectorLenth, Bottomvector.y() / vectorLenth);
        float Taillength = (halfHeight + (leftBottom.y() - BottomCenter.y())) / Bottomvector.y();
        
        QPointF endPos = QPointF(BottomCenter.x() + Taillength * Bottomvector.x(), BottomCenter.y() + Taillength * Bottomvector.y());

        pen.setWidth(vectorLenth / 3.0);
        painter.setPen(pen);
        painter.drawLine(BottomCenter, endPos);

        return pixmap;
    }

    // 绘制直线图标
    QPixmap drawLineIcon(int size, QColor color) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(color, 2));
        painter.drawLine(2, size * (2.0/3.0), size - 2, 16 * (1.0/3.0));

        return pixmap;
    }

    // 绘制点图标
    QPixmap drawPointIcon(int size, QColor color) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(color, 2));
        painter.setBrush(color);
        int rectwidth = size / 5.0;
        int halfRectWidth = rectwidth / 2.0;
        int spaceWidth = (size - rectwidth) / 2.0;
        
        //左右
        painter.drawLine(0, spaceWidth + halfRectWidth, spaceWidth, spaceWidth + halfRectWidth);
        painter.drawLine(spaceWidth + rectwidth, spaceWidth + halfRectWidth, size, spaceWidth + halfRectWidth);
        //上下
        painter.drawLine(spaceWidth + halfRectWidth, 0, spaceWidth + halfRectWidth, spaceWidth);
        painter.drawLine(spaceWidth + halfRectWidth, spaceWidth + rectwidth, spaceWidth + halfRectWidth, size);
        //点
        painter.drawRect(spaceWidth, spaceWidth, rectwidth, rectwidth);

        return pixmap;
    }

    // 绘制椭圆图标
    QPixmap drawEllipseIcon(int size, QColor color) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(color, 2));
        painter.drawEllipse(2, 4, size - 4, size - 8);

        return pixmap;
    }

    // 绘制矩形图标
    QPixmap drawRectangleIcon(int size, QColor color) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(color, 2));
        painter.drawRect(2, 2, size - 4, size - 4);

        return pixmap;
    }
    // 绘制量角器图标
    QPixmap createProtractorIcon(int size, QColor color) {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(0, 0, 0), 2));
        painter.drawArc(5, 5, 22, 22, 0, 180 * 16); // 180度扇形
        painter.drawLine(16, 5, 16, 12); // 量角器中心线

        return pixmap;
    }
    QPixmap drawItemIcon(ItemType type, int size /*= 24*/, QColor color/* = Qt::black*/) {
        if (size < 24)
            size = 24;
        switch (type) {
            case CyDisDrawItem::Invalid:
                return drawAarrowIcon(size, color);
            case CyDisDrawItem::Point:
                return drawPointIcon(size, color);
            case CyDisDrawItem::Rectangle:
                return drawRectangleIcon(size, color);
            case CyDisDrawItem::Line:
                return drawLineIcon(size, color);
            case CyDisDrawItem::Ellipse:
                return drawEllipseIcon(size, color);
        }

        return QPixmap();
    }

};