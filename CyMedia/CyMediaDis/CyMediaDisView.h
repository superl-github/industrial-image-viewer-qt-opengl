#pragma once
#include "../CyMediaBaseDef.h"

#include <QGraphicsView>
#include <QWheelEvent>

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

    void setImageShow(bool show);

    //缩略图
    bool thumbnailEnable();
    void setThumbnailEnable(bool enable);
    void upThumbnaildata(CyMedia::ImageShowInfo info, uint8_t* data);
    QImage& ThumbnailImage();
    void setthumbnailSelectColor(QColor color);
    void setthumbnailBackgroundColor(QColor color);

    //测量工具
    bool drawMode();
    void setDrawMode(bool draw);

private:
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
};
