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
    bool sharaContext(QOpenGLContext* ctx);
    //多线程更新背景图像，需要在工作线程创建QOpenGLContext并调用sharaContext
    bool upBackGround(CyMedia::ImageShowInfo info, uint8_t* data, QOpenGLContext* ctx, bool upThumbnaildata = false);
    void clearBackGround();
    int backTextureIndex();

    // 拉伸设置
    CyMedia::StretchType stretchType();
    void setStretchType(CyMedia::StretchType type);
    void setStreaChPara(uint32_t start = 0, uint32_t end = 0, uint32_t max = 0);

    // Bayer重建
    CyMedia::DemosaicingMethod Demosaic();
    void setDemosaic(CyMedia::DemosaicingMethod method);

    // YUV转换
    CyMedia::YUVTransMethod yuvMethod();
    void setYUVMethod(CyMedia::YUVTransMethod method);

    // FPS 相关
    double flushFps() const;
    bool isTrueDataFps() const;
    void setTrueDataFps(bool flag);

    // 颜色映射
    QStringList ColorMapList() const;
    qint32 colorMapIndex() const;
    bool setColorMap(qint32 index);
    bool setColorMap(const QString& mapName);

    //缩略图
    bool thumbnailEnable();
    void setThumbnailEnable(bool enable);
    bool thumbnailVisible();
    void upThumbnaildata(CyMedia::ImageShowInfo info, uint8_t* data);
    QImage& ThumbnailImage();
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
