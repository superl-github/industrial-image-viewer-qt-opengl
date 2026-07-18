#pragma once
#include "CyMediaBaseDef.h"

#include <QObject>
#include <QGraphicsView>
#include <QMessageBox>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QElapsedTimer>
#include <QTimer>
#include <QDir>

class CyMediaDisViewBckDraw : public QObject {
    Q_OBJECT
public:
    CyMediaDisViewBckDraw(QGraphicsView* view);
    ~CyMediaDisViewBckDraw();

public:
    // 更新图像相关
    void initgl(QOpenGLContext* ctx);
    bool glIsInit();
    void drawBackground(QPainter* painter, const QRectF& rect);

    bool shareContext(QOpenGLContext* ctx);
    /**
     * @brief upBackGround  更新背景图像
     * @details 多线程更新前置条件: 
     * @details 1、在工作线程创建QOpenGLContext
     * @details 2、调用sharaContext
     * 
     * @param[in] info  更新图像信息
     * @param[in] data  更新图像数据
     * @param[in] ctx   多线程：传入外部QOpenGLContext
     * @param[in] ctx   主线程：传入mainContext()
     * @ref  
     * @return bool     更新结果
    ***/
    bool upBackGround(CyMedia::ImageShowInfo info, uint8_t* data, QOpenGLContext* ctx);
    void clearBackGround();
    int backTextureIndex();
    bool haveImage();
    QOpenGLContext* mainContext();

    // 拉伸设置
    CyMedia::StretchType stretchType();
    void setStretchType(CyMedia::StretchType type);
    void setStreaChPara(uint32_t start = 0, uint32_t end = 0, uint32_t max = 0);

    // Bayer重建
    CyMedia::DemosaicingMethod Demosaic();
    void setDemosaic(CyMedia::DemosaicingMethod method);

    // FPS 相关
    double flushFps() const;
    bool isTrueDataFps() const;
    void setTrueDataFps(bool flag);

    // 颜色映射
    QStringList ColorMapList() const;
    qint32 colorMapIndex() const;
    bool setColorMap(qint32 index);
    bool setColorMap(const QString& mapName);

private:
    struct VerticesAndTextureCoord {
        QVector3D position;
        QVector2D texCoord;
    };
    struct oneTexturePara {
        QOpenGLTexture* glTexture = nullptr;

        CyMedia::ImageShowInfo showInfo; //图像信息
        
        bool imag_sizeChange = false;  //图像尺寸变化
        bool imag_infoChange = false;  //图像信息变化

        float maxBitlColor = 256;        //图像位宽最大值

        float textureWidthMultiplier = 1.0;//更新纹理时图像宽度乘数
        float textureHeightMultiplier = 1.0;//更新纹理时图像高度乘数

        GLenum textureInternalFormat = GL_R8;   // 纹理内部存储格式
        GLenum textureFormat = GL_RED;          // 纹理传入数据的图像格式
        GLenum textureType = GL_UNSIGNED_BYTE;  // 纹理传入数据纹理格式
        CyMedia::ePixType glslNcolor;           // 传入着色器的图像格式
    };

    QGraphicsView* m_view = nullptr;

    //QpenGL
    QMessageBox* m_message_glslError = nullptr;
    VerticesAndTextureCoord m_verticesArray[4];
    GLushort m_indicesArray[6] = { 0, 1, 2, 3, 2, 1 };

    bool mUseOpenGL = true;
    QOpenGLShaderProgram* m_shader_program = nullptr;
    QOpenGLVertexArrayObject* pVAO = nullptr;
    QOpenGLBuffer* pVBO = nullptr;
    QOpenGLBuffer* pEBO = nullptr;
    QOpenGLTexture* pTexture_ColorMap = nullptr;

    QOpenGLContext* m_ctx_main = nullptr;
    QOffscreenSurface* m_offscreenSurface = nullptr;

    // 图像信息
    oneTexturePara m_textureInfo[2];

    // bayer处理
    CyMedia::DemosaicingMethod mDemosaicMethod = CyMedia::DEMOSAIC_BILINEAR;
    bool bIsOVerSize = false;

    //拉伸参数
    CyMedia::StretchType eStretchType = CyMedia::stretch_None;
    bool upStretchValue = false;
    int32_t stretchpara_start;
    int32_t stretchpara_end;
    int32_t stretchpara_max;
    float stretchpara_K;
    float stretchpara_C;

    // 色彩映射
    QString m_ColorMapDirPath;
    QStringList m_ColorMapList;
    qint32 m_ColorMapIndex = 0;
    quint8* m_ColorMapData = nullptr;
    qint32 m_ColorMapFileSize = 768;
    qint32 m_ColorMapData_Width = 0;
    qint32 m_ColorMapData_Height = 0;
    bool bColorMapChange = false;

    // 状态标记
    std::atomic<bool> m_haveImage = { false };
    std::atomic<bool> m_hasNewData{ false };
    std::atomic<int> m_texture_front_Index{ 0 };// 双缓冲索引
    bool m_gl_init = false;
    int m_vertex_current_w = 1000;
    int m_vertex_current_h = 1000;

    // FPS 计算
    QTimer* DisFpsTimer = nullptr;
    QElapsedTimer DisFpsETimer;
    uint64_t texrureFpsCount = 0;
    uint64_t showFpsCount = 0;
    double DisFps = 0.0;
    double TextureFps = 0.0;
    bool useTrueDataFps = true;

    // 着色器源码
    QString vertexShaderSource;
    QString fragmentShaderSource;

private:
    void initColorMap(QString path);

    bool initglsl(QOpenGLFunctions* f);
    void initVertex(QOpenGLFunctions* f, const CyMedia::ImageShowInfo& info);
    void initTexture(QOpenGLFunctions* f);
    void clearGL();

    bool makeOffSurface(QOpenGLContext* ctx);
    void upVertex(QOpenGLFunctions* f, const CyMedia::ImageShowInfo& info);
    void updateTextureFormat(const CyMedia::ImageShowInfo& info, int idx);

    void setupShaderUniforms(QOpenGLFunctions* f, QMatrix4x4 mat, int colortype);
    void updateStretchUniforms(QOpenGLFunctions* f);

    void showGlslErro(QString tiltle, QString  txt);


};