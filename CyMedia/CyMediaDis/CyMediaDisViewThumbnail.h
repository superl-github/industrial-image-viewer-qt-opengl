/*****************************************************************//**
 * @class CyMediaDisViewThumbnail
 * @brief 缩略图导航小窗口，显示图像总览并支持视口定位。
 * @ingroup Display
 * 
 * @author LLF
 * @date   July 2026
 * @version 1.0
 *********************************************************************/
#pragma once
#include "CyMediaBaseDef.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QWidget>
#include <QGraphicsScene>
#include <QTransform>

class CyMediaDisView;
class QOpenGLVertexArrayObject;
/**
 * @class CyMediaDisViewThumbnail
 * @brief 缩略图导航小窗口，显示图像总览并支持视口定位。
 * @details
 *  本类是一个浮动在 `CyMediaDisView` 之上的 QWidget，用于：
 *  - 显示图像的全景缩略图（自动从原始数据生成并缩放）。
 *  - 在缩略图上绘制一个可拖动的彩色矩形框，代表当前主视口的可视区域。
 *  - 支持鼠标拖拽矩形框，主视口将跟随移动，实现快速定位。
 *
 *  缩略图更新由主视图通过 `upThumbnaildata()` 触发，内部将原始图像数据
 *  转换为 QImage（支持 Bayer/YUV 转换），并缩放到合适尺寸。
 *  窗口大小和位置根据主视图尺寸自动调整，默认位于右下角。
 *
 *  外观可定制：背景色、选择框颜色、边框颜色及是否绘制边框。
 *
 *  @note 缩略图仅在主视图缩放超过视口大小时自动显示，否则隐藏。
 *  @see CyMediaDisView.
 */
class CyMediaDisViewThumbnail : public QOpenGLWidget, public QOpenGLFunctions {
    Q_OBJECT
public:
    CyMediaDisViewThumbnail(CyMediaDisView* parentView, QWidget* parent = nullptr);
    ~CyMediaDisViewThumbnail();

public:
    void setScene(QGraphicsScene* scene);
    void setViewRect(const QRectF& rect);

    bool isBeingDragged();

    void setThumbnailSize(const QSize& size);

    void setBackgroundColor(QColor color);
    void setSelectColor(QColor color);

    bool drawBorder();
    void setDrawBorder(bool draw);
    QColor borderColor();
    void setBorderColor(QColor color);

protected:
    void paintGL() override;          // 替代 paintEvent
    void resizeGL(int w, int h) override;
    void initializeGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QRectF getDrawRect() const;// 辅助计算选择框

private:
    static const int MIN_RECT_SIZE = 30;   // 最小边长（像素）

    CyMediaDisView* m_parentView = nullptr; // 指向主视图
    QGraphicsScene* m_scene = nullptr;
    QOpenGLVertexArrayObject* m_vao = nullptr; //VAO无法共享，每个上下文要有自己的VAO

    QRectF mViewRect;
    QPointF m_pressThumbPos;          // 鼠标按下时在缩略图中的位置
    QPointF m_pressSceneTopLeft;      // 按下时视口左上角场景坐标
    QSizeF  m_pressSceneSize;         // 按下时视口在场景中的大小
    QRectF  m_pressThumbRect;         // 按下时场景矩形在缩略图上的映射（未钳制）

    bool mDragging = false;
    bool mDrawColor = true;

    QColor mBorderColor = QColor(0x00, 0xEE, 0x00);
    QColor mSelectRectColor = QColor(0x2a, 0xa3, 0xc6);
    QColor mSelectRectColor_transparent = QColor(0x2a, 0xa3, 0xc6, 100);
    QColor mBackGroundColor = Qt::lightGray;
};
