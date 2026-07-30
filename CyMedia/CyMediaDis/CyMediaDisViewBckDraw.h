/*****************************************************************//**
 * @file CyMediaDisViewBckDraw.h
 * @brief OpenGL 图像渲染引擎，负责纹理管理、着色器编译及背景绘制。
 * @details
 *  本类作为 `CyMediaDisView` 的后端，执行实际的 OpenGL 渲染工作。
 *  主要职责：
 *  - 初始化 OpenGL 上下文、着色器（支持运行时切换 shader 变体）、VBO/VAO。
 *  - 维护双缓冲纹理（前台/后台），支持异步纹理上传（通过同步对象确保数据一致性）。
 *  - 解析多种图像格式（MONO, RGB, RGBA, Bayer, YUV 系列），并转换为纹理数据。
 *  - 提供颜色映射（ColorMap）和像素值拉伸（Stretch）功能，增强图像对比度。
 *  - 支持 Bayer 去马赛克（Bilinear/Malvar/AHD）和 YUV 转换标准（BT.601/BT.709等）。
 *  - 提供 FPS 统计（渲染帧率和纹理上传帧率）。
 *
 *  设计为可在任意线程（主线程或子线程）中调用 `upBackGround()` 更新纹理，
 *  需配合 `createSharedContext()` 创建共享 OpenGL 上下文以确保线程安全。
 *  
 *  @warning 采用全局共享上下文方案，需要在初始化QApplication前，QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
 *  @see CyMediaDisView
 *  @author LLF
 *  @date   July 2026
 *  @version 1.0
 *********************************************************************/
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
#include <QOpenGLExtraFunctions>
#include <QElapsedTimer>
#include <QTimer>
#include <QDir>
#include <QMutex>

class CyMediaDisViewBckDraw : public QObject {
    Q_OBJECT
public:
    CyMediaDisViewBckDraw(QGraphicsView* view);
    ~CyMediaDisViewBckDraw();

    /**
    * @brief （缩略图专用）
    */
    void renderTexture(QOpenGLExtraFunctions* f, QOpenGLVertexArrayObject* VAO,
        const QRectF& targetRect, int viewWidth, int viewHeight,
        const QTransform& transform = QTransform());

signals:
    void textureSizeOver(bool flag, QString txt);

public:
    // 更新图像相关 QGraphicsView调用
    void initgl(QOpenGLContext* ctx);
    bool glIsInit();
    void drawBackground(QPainter* painter, const QRectF& rect);

    QOpenGLContext* createSharedContext();
    QOpenGLVertexArrayObject* createVAO();
    /**
     * @brief upBackGround  更新背景图像
     * @details 支持子线程上传纹理
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

    //YUV转换
    CyMedia::YUVTransMethod yuvMethod();
    void setYUVTMethod(CyMedia::YUVTransMethod method);

    // FPS 相关
    double flushFps() const;
    bool isTrueDataFps() const;
    void setTrueDataFps(bool flag);

    // 颜色映射
    QStringList ColorMapList() const;
    qint32 colorMapIndex() const;
    QString colorMapName() const;
    bool setColorMap(qint32 index);
    bool setColorMap(const QString& mapName);

private:
    enum ShaderVariant {
        None = 0,
        MONO,
        MONO_OVERSIZE,
        RGB,
        BAYER_NONE,
        BAYER_BILINEAR,
        BAYER_MALVAR,
        BAYER_AHD,
        YUV_PACKED,
        YUV_PLANAR,
        YUV_SEMIPLANAR
    };

    struct VerticesAndTextureCoord {
        QVector3D position;
        QVector2D texCoord;
    };
    struct oneTexturePara {
        //纹理对象和同步
        QOpenGLTexture* glTexture = nullptr;// 若为 YUV，用为 Y 纹理或 Packed 纹理
        QOpenGLTexture* glTextureU = nullptr; // U 平面 或 UV 平面（SP 格式时使用）
        QOpenGLTexture* glTextureV = nullptr; // V 平面（仅平面格式）
        GLsync syncFence = nullptr;// 同步对象可共用，或每个纹理单独使用（可选）
        //图像信息及OpenGL对应信息
        CyMedia::ImageShowInfo showInfo;//图像信息
        float bitMul = 1.0;
        CyMedia::ePixType glslNcolor;   // 传入着色器的图像格式
        GLenum textureInternalFormat = GL_R8;   // 主纹理内部存储格式
        GLenum textureFormat = GL_RED;          // 主纹理传入数据的图像格式
        GLenum textureType = GL_UNSIGNED_BYTE;  // 主纹理传入数据纹理格式


        int uTex_width = 0;                 //U 平面宽
        int uTex_height = 0;                //U 平面高
        int vTex_width = 0;                 //V 平面宽
        int vTex_height = 0;                //V 平面宽
        GLenum uvInternalFormat = GL_R8;    //UV 纹理内部存储格式
        GLenum uvFormat = GL_RED;           //UV 纹理传入数据的图像格式
        GLenum uvType = GL_UNSIGNED_BYTE;   //UV 纹理传入数据纹理格式

        //数据处理信息
        bool useUTex = false;
        bool useVTex = false;
        float textureWidthMultiplier = 1.0;//更新纹理时图像宽度乘数
        float textureHeightMultiplier = 1.0;//更新纹理时图像高度乘数
        uint32_t uTexDataOffset = 0;
        uint32_t vTexDataOffset = 0;

        // 标志
        bool isReBiuld = false;
        bool needUpImageInfo = false;
    };

    QGraphicsView* m_view = nullptr;

    //QpenGL
    mutable QMutex m_glMutex;
    QMessageBox* m_message_glslError = nullptr;
    QMessageBox* m_message_overSize = nullptr;
    VerticesAndTextureCoord m_verticesArray[4];
    GLushort m_indicesArray[6] = { 0, 1, 2, 3, 2, 1 };

    bool mUseOpenGL = true;
    ShaderVariant m_currentShaderType = ShaderVariant::None;
    QString m_commonFragmentSource;
    QOpenGLShaderProgram* m_shader_program = nullptr;
    QOpenGLVertexArrayObject* pVAO = nullptr;
    QOpenGLBuffer* pVBO = nullptr;
    QOpenGLBuffer* pEBO = nullptr;
    QOpenGLTexture* pTexture_ColorMap = nullptr;

    QOpenGLContext* m_ctx_main = nullptr;
    QOffscreenSurface* m_offscreenSurface = nullptr;
    int m_gl_max_texture_size = 16384;
    bool m_bIsOVerSize = false;

    // 图像信息
    oneTexturePara m_textureInfo[2];

    // bayer处理
    bool m_fisrt_up_image = true;
    CyMedia::DemosaicingMethod mDemosaicMethod = CyMedia::DEMOSAIC_BILINEAR;
    
    // YUV处理
    CyMedia::YUVTransMethod m_yuv_trans_method = CyMedia::YUVTRANS_NORMAL;

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
    QString m_ColorMapName;
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
    int m_vertex_mulW = 1.0;
    int m_vertex_mulH = 1.0;

    // FPS 计算
    QTimer* DisFpsTimer = nullptr;
    QElapsedTimer DisFpsETimer;
    uint64_t texrureFpsCount = 0;
    uint64_t showFpsCount = 0;
    double DisFps = 0.0;
    double TextureFps = 0.0;
    bool useTrueDataFps = false;

    // 着色器源码
    QString vertexShaderSource;
    QString fragmentShaderSource;

    // Uniform 相关
    GLint uniform_m_matrix = -1;
    GLint uniform_zoomValue = -1;
    GLint uniform_demosacFunc = -1;
    GLint uniform_YUVMethod = -1;
    GLint uniform_colorMapIndex = -1;
    GLint uniform_stretchType = -1;
    GLint uniform_stretchPara = -1;
    //图像信息变化时才需要改变的参数
    GLint uniform_nWidth = -1;
    GLint uniform_nHeight = -1;
    GLint uniform_nbits = -1;
    GLint uniform_colorType = -1;
    GLint uniform_bayerPatter = -1;
    GLint uniform_bitMul = -1;
    // 纹理采样器(初始化或环境变化才要设置)
    GLint uniform_texture = -1;
    GLint uniform_textureU = -1;
    GLint uniform_textureV = -1;
    GLint uniform_texture_colormap = -1;

    QMatrix4x4 m_projectionMat;
    int m_projectionMat_Width = 0;
    int m_projectionMat_Height = 0;

private:
    void initColorMap(QString path);

    ShaderVariant determineShaderVariant(const CyMedia::ImageShowInfo& info);
    void loadCommonShader();
    QString loadFragmentShaderSource(ShaderVariant variant);
    bool recompileShader(QOpenGLExtraFunctions* f, CyMediaDisViewBckDraw::oneTexturePara* para);
    void upUniforLocation(QOpenGLShaderProgram* program);

    //共享上下文相关
    bool makeOffSurface(QOpenGLContext* ctx);
    void recreateOffscreenSurface(const QSurfaceFormat& format);


    bool initglsl(QOpenGLExtraFunctions* f);
    void initVertex(QOpenGLExtraFunctions* f, const CyMedia::ImageShowInfo& info);
    void upVertex(QOpenGLExtraFunctions* f, int width, int height, float mulW, float mulH);
    void initTexture(QOpenGLExtraFunctions* f);
    void initTextureOne(QOpenGLExtraFunctions* f, oneTexturePara* pTex, bool allocaMem = false);
    bool upTexture(QOpenGLExtraFunctions* f, int backIdx, CyMedia::ImageShowInfo info, uint8_t* data);
    void clearGL();

    void updateTextureFormat(const CyMedia::ImageShowInfo& info, int idx, bool oversize = false);
    void upShaderUniformSampler(QOpenGLExtraFunctions* f);
    void upShaderUniformImageInfo(QOpenGLExtraFunctions* f, int colorType);
    void upShaderUniformOther(QOpenGLExtraFunctions* f, QMatrix4x4 mat, float zoom = 1.0f);
    void updateStretchUniforms(QOpenGLExtraFunctions* f);

    void showGlslError(QString tiltle, QString  txt);
    void showOverSizeError(QString Text);
    void hideOverSizeError();
};