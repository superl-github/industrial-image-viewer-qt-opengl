/*****************************************************************//**
 * @file   CyMediaDisView.h
 * @brief  高性能图像显示控件（基于 QGraphicsView + OpenGL）
 * @details
 *  本控件集成了完整的图像显示、交互与辅助功能，适用于需要实时显示
 *  高分辨率或特殊格式图像（如 Bayer、YUV、16位灰度等）的应用程序。
 *
 *  ## 核心功能
 *  1. **图像渲染**：通过 `CyMediaDisViewBckDraw` 实现 OpenGL 加速渲染，
 *     支持单色、RGB/RGBA、Bayer、多种 YUV 格式（Packed/Planar/Semi-Planar），
 *     自动处理 8/16/32 位深度，并提供颜色映射（ColorMap）和拉伸（Stretch）功能。
 *  2. **交互操作**：
 *      - 缩放：Ctrl+滚轮 以鼠标为中心缩放，最大倍数可配置。
 *      - 平移：左键拖拽视口。
 *      - 旋转/镜像：提供编程接口设置旋转角度（0~360°）及水平/垂直翻转。
 *  3. **缩略图导航**：内置 `CyMediaDisViewThumbnail` 小窗口，实时显示图像缩略图，
 *     并用矩形框标识当前视口位置，支持通过拖拽矩形框快速定位视图。
 *  4. **多线程纹理上传**：渲染后端支持在子线程中更新 OpenGL 纹理，
 *     避免阻塞 UI，适用于实时数据流（如相机采集）。
 *  5. **可配置项**：可启用/禁用缩略图，设置缩放范围，切换 Bayer 插值算法，
 *     YUV 转换标准，颜色映射列表等。
 *
 *  ## 典型用法
 *  ```cpp
 *  // 1. 创建控件并设置场景
 *  CyMediaDisView* view = new CyMediaDisView(parent);
 *  QGraphicsScene* scene = new QGraphicsScene(this);
 *  view->setCyScene(scene);
 *
 *  // 2. 获取渲染后端，上传图像（可在主线程或子线程）
 *  CyMediaDisViewBckDraw* drawer = view->imageDraw();
 *  drawer->initgl(glContext);            // 需要先初始化 OpenGL
 *  CyMedia::ImageShowInfo info{...};
 *  drawer->upBackGround(info, data, drawer->mainContext());
 *
 *  // 3. 控制视图
 *  view->zoomIn();
 *  view->rotateView(90.0);
 *  view->setThumbnailEnable(true);
 *  ```
 *  ## 依赖
 *  - Qt 5.14.2
 *  - OpenGL 3.3 Core Profile
 *
 *  @note 采用全局共享上下文方案，需要在初始化QApplication前，QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
 *  @see CyMediaDisViewBckDraw, CyMediaDisViewThumbnail
 * 
 *  @author LLF
 *  @date   July 2026
 *  @version 1.0
 *********************************************************************/
#pragma once
#include "../CyMediaBaseDef.h"

#include <QGraphicsView>
#include <QWheelEvent>

class CyMediaDisViewBckDraw;
class CyMediaDisViewThumbnail;
class CyMediaDisView : public QGraphicsView {
    Q_OBJECT

public:
    CyMediaDisView(QWidget* parent = nullptr);
    ~CyMediaDisView();

public: signals:
    void onMousePress();
    void onMouseDoubleClick();

    void zoomValueChange(double value);

public:
    void setCyScene(QGraphicsScene* scene);

    void sceneRectUp(const QRectF& rect);

    //视图操作
    void setViewFromThumbnailPos(double xRatio, double yRatio);

    float zoomValue(void);
    void zoomIn(void);
    void zoomOut(void);
    void zoomAuto(void);
    void zoomRaw(bool Reset = true);

    bool isHriMirror();
    bool isVerMirror();
    void hriMirror(void);
    void verMirror(void);

    double rotateValue(void);
    void rotateView(double angle);

    //图像操作
    CyMediaDisViewBckDraw* imageDraw() const;
    void clearBackGround();

    //缩略图
    QWidget* thumnailWidget();
    bool thumbnailEnable();
    void setThumbnailEnable(bool enable);
    bool thumbnailVisible();
    void setThumbnailSelectColor(QColor color);
    void setThumbnailBackgroundColor(QColor color);
    QColor thumbnailBorderColor();
    void setThumbnailBorderColor(QColor color);
    bool ThumbnailDrawBorder();
    void setThumbnailDrawBorder(bool draw);

    //测量工具
    bool drawMode();
    void setDrawMode(bool draw);

private:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void showEvent(QShowEvent* e)override;
    void closeEvent(QCloseEvent* e)override;

    void wheelEvent(QWheelEvent* event) override;

    void mousePressEvent(QMouseEvent* event)override;
    void mouseMoveEvent(QMouseEvent* event)override;
    void mouseReleaseEvent(QMouseEvent* event)override;
    void mouseDoubleClickEvent(QMouseEvent* event)override;
    void keyPressEvent(QKeyEvent* event)override;
    void keyReleaseEvent(QKeyEvent* event)override;

    void resizeEvent(QResizeEvent* event) override;

private:
    void onScrollValueChanged();

private:
    class MyViewPrivateData;
    MyViewPrivateData* d = nullptr;

    CyMediaDisViewBckDraw* m_backDraw;
    CyMediaDisViewThumbnail* m_Thumbnail = nullptr;
};
