#pragma once
#include "CyMediaBaseDef.h"

#include <QWidget>
#include <QGraphicsScene>

class CyMediaDisView;
class CyMediaDisViewThumbnail : public QWidget {
    Q_OBJECT
public:
    CyMediaDisViewThumbnail(CyMediaDisView* parentView, QWidget* parent = nullptr);
    ~CyMediaDisViewThumbnail();

signals:
    void viewRectChanged(const QRectF& rect);

public:
    void setScene(QGraphicsScene* scene);
    void setViewRect(const QRectF& rect);

    bool isBeingDragged();

    void setThumbnailSize(const QSize& size);

    void upBackImage(CyMedia::ImageShowInfo info, uint8_t* data);
    QImage& backImage();

    void setBackgroundColor(QColor color);
    void setSelectColor(QColor color);

    bool drawBorder();
    void setDrawBorder(bool draw);
    QColor borderColor();
    void setBorderColor(QColor color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    QRectF getDrawRect() const;

private:
    static const int MIN_RECT_SIZE = 30;   // 最小边长（像素）
    CyMediaDisView* m_parentView = nullptr; // 指向主视图
    QGraphicsScene* m_scene = nullptr;
    QRectF mViewRect;
    bool mDragging = false;
    QPointF mClickOffsetRatio; // 鼠标点击点在小窗内的相对比例 (0~1)
    QImage mBackImage;
    bool mDrawColor = true;

    QColor mBorderColor = QColor(0x00, 0xEE, 0x00);
    QColor mSelectRectColor = QColor(0x2a, 0xa3, 0xc6);
    QColor mSelectRectColor_transparent = QColor(0x2a, 0xa3, 0xc6, 100);
    QColor mBackGroundColor = Qt::lightGray;
};
