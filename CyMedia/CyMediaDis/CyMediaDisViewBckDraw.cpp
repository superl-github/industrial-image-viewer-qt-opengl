#include "CyMediaDisViewBckDraw.h"
#include "CyMediaDisLog.h"
#include "CyMediaDisView.h"

#include <QApplication>
#include <QThread>
#include <QElapsedTimer>

CyMediaDisViewBckDraw::CyMediaDisViewBckDraw(QGraphicsView* view)
    : QObject(nullptr) {
    m_view = view;
    // 初始化消息框
    m_message_glslError = new QMessageBox(
        QMessageBox::Warning,
        QString(""),
        QString(""),
        QMessageBox::Ok
    ); 
    m_message_overSize = new QMessageBox{
        QMessageBox::Warning,
        QString("texture error"),
        QString(""),
        QMessageBox::Ok
    };
    connect(this, &CyMediaDisViewBckDraw::textureSizeOver, this, [this](bool flag, QString txt) {
        if (flag) {
            showOverSizeError(txt);
        }
        else {
            hideOverSizeError();
        }
        });
    // 初始化默认值
    oneTexturePara* pTex = nullptr;
    for (int i = 0; i < 2; i++) {
        pTex = &m_textureInfo[i];
        pTex->showInfo.width = 1000;
        pTex->showInfo.height = 1000;
        pTex->showInfo.bit = 8;
        pTex->showInfo.special_pixel = CyMedia::PIXEL_VALUE_INT;
        pTex->showInfo.format = CyMedia::MONO;
        pTex->showInfo.upLenth();
        updateTextureFormat(pTex->showInfo, i);
    }
    m_vertex_current_w = pTex->showInfo.width;
    m_vertex_current_h = pTex->showInfo.height;

    // 初始化颜色映射
    initColorMap(qApp->applicationDirPath() + QString("/colorMap/"));

    // 设置 FPS 计时器
    DisFpsTimer = new QTimer(this);
    DisFpsTimer->setTimerType(Qt::PreciseTimer);
    connect(DisFpsTimer, &QTimer::timeout, this, [this]() {
        if (DisFpsETimer.isValid()) {
            DisFps = (showFpsCount * 1000.0) / DisFpsETimer.elapsed();
            TextureFps = (texrureFpsCount * 1000.0) / DisFpsETimer.elapsed();
            texrureFpsCount = showFpsCount = 0;
            DisFpsETimer.restart();
            //printf("textureFps:%.2f showFps:%.2f\n\n", TextureFps, DisFps);
        }
        else {
            DisFpsETimer.start();
        }
        });
    DisFpsTimer->start(1000);
}

CyMediaDisViewBckDraw::~CyMediaDisViewBckDraw() {
    clearGL();
    delete[] m_ColorMapData;
    m_ColorMapData = nullptr;
}

void CyMediaDisViewBckDraw::renderTexture(QOpenGLExtraFunctions* f, QOpenGLVertexArrayObject* VAO, const QRectF& targetRect, int viewWidth, int viewHeight, const QTransform& transform /*= QTransform()*/) {
    if (!m_gl_init || !m_haveImage.load() || !f || !VAO) return;
    f->initializeOpenGLFunctions();

    GLenum err;
    // 获取当前前台纹理索引
    int idx = m_texture_front_Index.load();
    oneTexturePara* pTex = &m_textureInfo[idx];
    
    if (!m_shader_program) {
        return;
    }

    pTex->isReBiuld = false;
    if (!recompileShader(f, pTex)) {
        return;
    }
    f->glClearColor(0.0, 0.0, 0.0, 1.0);
    f->glClear(GL_COLOR_BUFFER_BIT);

    // 绑定主纹理（主纹理、U/V、ColorMap）
    f->glActiveTexture(GL_TEXTURE0); f->glBindTexture(GL_TEXTURE_2D, pTex->glTexture->textureId());
    err = f->glGetError(); 
    if (err) qWarning() << "Bind Main texture, err=" << err;
    //U 纹理
    if (pTex->useUTex) {
        f->glActiveTexture(GL_TEXTURE1);
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureU->textureId());
    }
    //V纹理
    if (pTex->useVTex) {
        f->glActiveTexture(GL_TEXTURE2);
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureV->textureId());
    }
    //ColorMap纹理
    f->glActiveTexture(GL_TEXTURE3);
    f->glBindTexture(GL_TEXTURE_2D, pTexture_ColorMap->textureId());

    // 更新顶点（如果图像尺寸变化）
    if (pTex->showInfo.width != m_vertex_current_w ||
        pTex->showInfo.height != m_vertex_current_h ||
        pTex->textureWidthMultiplier != m_vertex_mulW ||
        pTex->textureHeightMultiplier != m_vertex_mulH) {
        upVertex(f, pTex->showInfo.width, pTex->showInfo.height,
            pTex->textureWidthMultiplier, pTex->textureHeightMultiplier);
    }

    // 绑定着色器
    f->glUseProgram(m_shader_program->programId());
    f->glViewport(0, 0, viewWidth, viewHeight);

    //设置纹理采样器
    upShaderUniformSampler(f);
    //更新着色器参数
    if (pTex->needUpImageInfo || pTex->isReBiuld) {
        upShaderUniformImageInfo(f, int(pTex->glslNcolor));
        pTex->needUpImageInfo = false;
    }
    if (upStretchValue || pTex->isReBiuld) {
        updateStretchUniforms(f);
        upStretchValue = false;
    }

    // 构建投影矩阵：使用传入的视口尺寸，并结合 transform
    QMatrix4x4 proj;
    proj.setToIdentity();
    proj.ortho(0.0f, viewWidth, viewHeight, 0.0f, -1000.0f, 1000.0f);
    QMatrix4x4 finalMat = proj * transform;
    upShaderUniformOther(f, finalMat);

    //绘制
    QOpenGLVertexArrayObject::Binder vaobinder(VAO);
    err = f->glGetError(); if (err != GL_NO_ERROR) qWarning() << "GL error Bind VAO:" << err;
    f->glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    err = f->glGetError(); if (err != GL_NO_ERROR) qWarning() << "GL error after draw:" << err;

    // 解绑
    f->glBindTexture(GL_TEXTURE_2D, 0);
    m_shader_program->release();
    vaobinder.release();
}

void CyMediaDisViewBckDraw::initgl(QOpenGLContext* ctx) {
    QMutexLocker locker(&m_glMutex);
    if (m_gl_init) return;
    if (!ctx) return;
    QOpenGLExtraFunctions* f = ctx->extraFunctions(); if (!f) return;
    f->initializeOpenGLFunctions();
    if (false == initglsl(f)) return;
    initVertex(f, m_textureInfo[0].showInfo);
    initTexture(f);
    // Surface
    m_offscreenSurface = new QOffscreenSurface();
    m_offscreenSurface->setFormat(ctx->format());
    m_offscreenSurface->create();
    if (!m_offscreenSurface->isValid()) {
        qWarning() << "Offscreen surface invalid after creation!";
        m_gl_init = false;
        delete m_offscreenSurface;
        m_offscreenSurface = nullptr;
        return;
    }

    // 获取最大纹理尺寸
    f->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_gl_max_texture_size);

    //禁用驱动优化
    f->glDisable(GL_TEXTURE_RECTANGLE);
    f->glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

    m_ctx_main = ctx;
    m_gl_init = true;
}

bool CyMediaDisViewBckDraw::glIsInit() {
    return m_gl_init;
}

void CyMediaDisViewBckDraw::drawBackground(QPainter* painter, const QRectF& rect) {
    QMutexLocker locker(&m_glMutex);
    GLenum err;
    QSize viewRect = m_view->viewport()->size() * m_view->viewport()->devicePixelRatioF();
    if (false == m_haveImage) {
        painter->save();
        //painter->resetTransform();
        int baseSize = qMin(viewRect.width(), viewRect.height()) / 10;
        int fontSize = qBound(16, baseSize, 48);

        QFont font("Microsoft YaHei", fontSize);
        painter->setFont(font);
        painter->setPen(QColor(200, 200, 200));
        painter->drawText(rect, Qt::AlignCenter, tr("No Image"));
        painter->restore();
        return;
    }

    showFpsCount++;
    if (false == m_gl_init) return;
    // 取得前台纹理
    int idx = m_texture_front_Index.load();
    if (m_hasNewData.load()) {
        // 交换索引：让后台变为前台
        int oldFront = m_texture_front_Index.load();
        int newFront = 1 - oldFront;
        m_texture_front_Index.store(newFront);
        m_hasNewData.store(false);   // 标志重置，表示此新数据已被处理
        idx = newFront;
    }
    oneTexturePara* pTex = &m_textureInfo[idx];

    //开始渲染
    painter->beginNativePainting();
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) { painter->endNativePainting(); return; }
    QOpenGLExtraFunctions* f = ctx->extraFunctions();
    if (!f) { painter->endNativePainting(); return; }

    //检查是否需要重新编译
    pTex->isReBiuld = false;
    if (false == recompileShader(f, pTex)) {
        //结束渲染
        painter->endNativePainting();
        return;
    }
    f->glClearColor(0.0, 0.0, 0.0, 1.0);
    f->glClear(GL_COLOR_BUFFER_BIT);

    //主纹理
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, pTex->glTexture->textureId());
    //U 纹理
    if (pTex->useUTex) {
        f->glActiveTexture(GL_TEXTURE1);
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureU->textureId());
    }
    //V 纹理
    if (pTex->useVTex) {
        f->glActiveTexture(GL_TEXTURE2);
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureV->textureId());
    }
    //Colormap 纹理
    f->glActiveTexture(GL_TEXTURE3);
    f->glBindTexture(GL_TEXTURE_2D, pTexture_ColorMap->textureId());
    if (bColorMapChange) {
        f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            m_ColorMapData_Width,
            m_ColorMapData_Height,
            GL_RED, GL_UNSIGNED_BYTE, m_ColorMapData);
        GLenum glerr = f->glGetError();
        if (glerr != GL_NO_ERROR) {
            qWarning() << "glTexSubImage2D(Colormap) failed:" << glerr;
        }
        f->glFinish();
        bColorMapChange = false;
    }

    //更新顶点
    if (pTex->showInfo.width != m_vertex_current_w || 
        pTex->showInfo.height != m_vertex_current_h ||
        pTex->textureWidthMultiplier != m_vertex_mulW ||
        pTex->textureHeightMultiplier != m_vertex_mulH) {
        upVertex(f, pTex->showInfo.width, pTex->showInfo.height, pTex->textureWidthMultiplier, pTex->textureHeightMultiplier);
    }

    //绑定着色器程序
    f->glUseProgram(m_shader_program->programId());

    //更新着色器参数
    if (pTex->needUpImageInfo || pTex->isReBiuld) {
        upShaderUniformImageInfo(f, int(pTex->glslNcolor));
        pTex->needUpImageInfo = false;
    }
    if (upStretchValue || pTex->isReBiuld) {
        updateStretchUniforms(f);
        upStretchValue = false;
    }
    if (viewRect.width() != m_projectionMat_Width ||
        viewRect.height() != m_projectionMat_Height) {
        m_projectionMat_Width = viewRect.width();
        m_projectionMat_Height = viewRect.height();
        m_projectionMat.setToIdentity();
        m_projectionMat.ortho(0.0f, m_projectionMat_Width, m_projectionMat_Height, 0.0f, -1000.0f, 1000.0f);
    }
    upShaderUniformOther(f, m_projectionMat * painter->worldTransform(), qobject_cast<CyMediaDisView*>(m_view)->zoomValue());
    
    // 绘制
    QOpenGLVertexArrayObject::Binder vaobinder(pVAO);
    f->glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

    // 解绑
    f->glBindTexture(GL_TEXTURE_2D, 0);
    m_shader_program->release();
    vaobinder.release();

    //结束渲染
    painter->endNativePainting();
}

QOpenGLContext* CyMediaDisViewBckDraw::createSharedContext() {
    QMutexLocker locker(&m_glMutex);
    if (!m_gl_init) return nullptr;

    QOpenGLContext* ctx = new QOpenGLContext;
    ctx->setFormat(m_ctx_main->format());   // 沿用主上下文的格式，确保兼容
    ctx->setShareContext(QOpenGLContext::globalShareContext());
    if (!ctx->create()) {
        delete ctx;
        return nullptr;
    }
    // 初始化离屏表面（但无需 makeCurrent 即可使用，为了安全仍做一次）
    if (!ctx->makeCurrent(m_offscreenSurface)) {
        delete ctx;
        return nullptr;
    }
    ctx->doneCurrent();  // 释放，供外部线程使用
    return ctx;
}

QOpenGLVertexArrayObject* CyMediaDisViewBckDraw::createVAO() {
    QOpenGLVertexArrayObject* tVAO = new QOpenGLVertexArrayObject(this);
    QOpenGLVertexArrayObject::Binder vaobinder(tVAO);
    pVBO->bind();
    pEBO->bind();
    m_shader_program->enableAttributeArray(0);
    m_shader_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(VerticesAndTextureCoord));
    m_shader_program->enableAttributeArray(1);
    m_shader_program->setAttributeBuffer(1, GL_FLOAT, sizeof(QVector3D), 2, sizeof(VerticesAndTextureCoord));
    vaobinder.release();
    return tVAO;
}

void CyMediaDisViewBckDraw::initColorMap(QString path) {
    //初始化ColorMap内存
    m_ColorMapData = new quint8[m_ColorMapFileSize];
    //添加默认
    m_ColorMapList.clear();
    m_ColorMapList.push_back("None");

    //筛选符合条件文件
    QStringList CMfullPathList;
    QDir colorMapDir(path);
    if (colorMapDir.exists()) {
        m_ColorMapDirPath = path;
        colorMapDir.setFilter(QDir::Files);
        auto cmFileList = colorMapDir.entryInfoList();
        for (auto oneInfo : cmFileList) {
            if (oneInfo.size() == m_ColorMapFileSize && oneInfo.suffix() == "cm") {
                CMfullPathList.push_back(oneInfo.absoluteFilePath());
            }
        }
    }
    //读取文件数据
    if (CMfullPathList.size()) {
        QFile oneCMFile;
        qint32 dataIndex = 0;
        for (auto oneCMPath : CMfullPathList) {
            oneCMFile.setFileName(oneCMPath);
            if (oneCMFile.open(QIODevice::ReadOnly)) {
                dataIndex++;
                m_ColorMapList.push_back(QFileInfo(oneCMPath).baseName());
                oneCMFile.close();
            }
        }
        m_ColorMapData_Width = m_ColorMapFileSize / 3;
        m_ColorMapData_Height = 3;
    }
    m_ColorMapIndex = 0;
}

CyMediaDisViewBckDraw::ShaderVariant CyMediaDisViewBckDraw::determineShaderVariant(const CyMedia::ImageShowInfo& info) {
    if (info.format == CyMedia::MONO_OVERSIZE) {
        return ShaderVariant::MONO_OVERSIZE;
    }
    else if (info.isRGB() || info.isRGBA()) {
        return ShaderVariant::RGB;
    }
    else if (info.isBayer()) {
        return ShaderVariant(mDemosaicMethod + int(ShaderVariant::BAYER_NONE));
    }
    else {
        switch (info.format) {
            case CyMedia::FOURCC_YUY2:
            case CyMedia::FOURCC_YVYU: {
                return ShaderVariant::YUV_PACKED;
            }
            case CyMedia::FOURCC_I422:
            case CyMedia::FOURCC_YV16:
            case CyMedia::FOURCC_I420:
            case CyMedia::FOURCC_YV12: {
                return ShaderVariant::YUV_PLANAR;
            }
            case CyMedia::FOURCC_NV12:
            case CyMedia::FOURCC_NV21: {
                return ShaderVariant::YUV_SEMIPLANAR;
            }
        }
    }

    return ShaderVariant::MONO;
}

void CyMediaDisViewBckDraw::loadCommonShader() {
    if (false == m_commonFragmentSource.isEmpty()) return;
    QFile file(":/CyMediaDis/glslSource/Fragment_Common.c");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        m_commonFragmentSource = stream.readAll();
        file.close();
    }
}

QString CyMediaDisViewBckDraw::loadFragmentShaderSource(ShaderVariant variant) {
    QString fileName;
    switch (variant) {
    case CyMediaDisViewBckDraw::MONO: fileName = "Fragment_Mono.c"; break;
    case CyMediaDisViewBckDraw::MONO_OVERSIZE:fileName = "Fragment_Mono_OverSize.c"; break;
    case CyMediaDisViewBckDraw::RGB:fileName = "Fragment_RGB.c"; break;
    case CyMediaDisViewBckDraw::BAYER_NONE:fileName = "Fragment_Mono.c"; break;
    case CyMediaDisViewBckDraw::BAYER_BILINEAR:fileName = "Fragment_Bayer_Bilinear.c"; break;
    case CyMediaDisViewBckDraw::BAYER_MALVAR:fileName = "Fragment_Bayer_Malvar.c"; break;
    case CyMediaDisViewBckDraw::BAYER_AHD:fileName = "Fragment_Bayer_AHD.c"; break;
    case CyMediaDisViewBckDraw::YUV_PACKED:fileName = "Fragment_YUV_Packed.c"; break;
    case CyMediaDisViewBckDraw::YUV_PLANAR:fileName = "Fragment_YUV_Planar.c"; break;
    case CyMediaDisViewBckDraw::YUV_SEMIPLANAR:fileName = "Fragment_YUV_SemiPlanar.c"; break;
    }

    QFile file(QString(":/CyMediaDis/glslSource/") + fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load shader:" << fileName;
        return QString();
    }
    QTextStream stream(&file);
    return stream.readAll();
}

bool CyMediaDisViewBckDraw::recompileShader(QOpenGLExtraFunctions* f, CyMediaDisViewBckDraw::oneTexturePara* para) {
    if (!para) return false;
    const CyMedia::ImageShowInfo& info = para->showInfo;
    ShaderVariant newVariant = determineShaderVariant(info);
    if (newVariant == m_currentShaderType) {
        return true; // 无需重新编译
    }
    QString src = loadFragmentShaderSource(newVariant);
    if (src.isEmpty()) return false;
    QElapsedTimer timer;
    timer.start();
    QOpenGLShaderProgram* newProgram = new QOpenGLShaderProgram;
    newProgram->create();
    if (!newProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        showGlslError(tr("vertex shader"), newProgram->log());
        delete newProgram;
        return false;
    }
    if (!newProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, m_commonFragmentSource + src)) {
        showGlslError(tr("Fragment shader"), newProgram->log());
        delete newProgram;
        return false;
    }
    if (!newProgram->link()) {
        showGlslError(tr("GL Program link"), newProgram->log());
        delete newProgram;
        return false;
    }
    CyMediaDisLog::instance().log_printf(CyMedia::LogLevel::DEBUG, "GL Program Compilation time %lld ms\n", timer.elapsed());
    // 替换当前程序
    f->glUseProgram(0);
    if (m_shader_program) delete m_shader_program;
    m_shader_program = newProgram;
    //m_shader_program->enableAttributeArray(0);
    //m_shader_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(VerticesAndTextureCoord));
    //m_shader_program->enableAttributeArray(1);
    //m_shader_program->setAttributeBuffer(1, GL_FLOAT, sizeof(QVector3D), 2, sizeof(VerticesAndTextureCoord));

    //重新获取uniform位置
    f->glUseProgram(m_shader_program->programId());
    upUniforLocation(m_shader_program);
    //设置纹理采样器
    upShaderUniformSampler(f);
    //更新状态
    para->isReBiuld = true;
    m_currentShaderType = newVariant;
    return true;
}

void CyMediaDisViewBckDraw::upUniforLocation(QOpenGLShaderProgram* program) {
    uniform_texture = program->uniformLocation("texture");
    uniform_textureU = program->uniformLocation("textureU");
    uniform_textureV = program->uniformLocation("textureV");
    uniform_texture_colormap = program->uniformLocation("texture_ColorMap");
    uniform_m_matrix = program->uniformLocation("m_matrix");
    uniform_zoomValue = program->uniformLocation("zoomValue");
    uniform_colorType = program->uniformLocation("colorType");
    uniform_bayerPatter = program->uniformLocation("bayerPatter");
    uniform_demosacFunc = program->uniformLocation("demosacFunc");
    uniform_YUVMethod = program->uniformLocation("YUVMethod");
    uniform_nbits = program->uniformLocation("nbits");
    uniform_nWidth = program->uniformLocation("nWidth");
    uniform_nHeight = program->uniformLocation("nHeight");
    uniform_bitMul = program->uniformLocation("bitMul");
    uniform_colorMapIndex = program->uniformLocation("colorMapIndex");
    uniform_stretchType = program->uniformLocation("stretchType");
    uniform_stretchPara = program->uniformLocation("stretchPara");
}

bool CyMediaDisViewBckDraw::makeOffSurface(QOpenGLContext* ctx) {
    if (!ctx || !ctx->isValid()) {
        qWarning() << "CyMediaDisViewBckDraw::makeOffSurface:Invalid context";
        return false;
    }
    // 确保表面有效，若无效则重建
    if (!m_offscreenSurface || !m_offscreenSurface->isValid()) {
        recreateOffscreenSurface(ctx->format());
    }
    // 如果当前上下文已是目标上下文，则无需切换
    if (QOpenGLContext::currentContext() == ctx) {
        return true;
    }
    // 先解绑当前上下文（避免残留）
    if (QOpenGLContext::currentContext()) {
        QOpenGLContext::currentContext()->doneCurrent();
    }
    // 尝试绑定
    if (!ctx->makeCurrent(m_offscreenSurface)) {
        qWarning() << "makeCurrent failed, recreating surface...";
        recreateOffscreenSurface(ctx->format());
    }
    return true;
}

void CyMediaDisViewBckDraw::recreateOffscreenSurface(const QSurfaceFormat& format) {
    if (m_offscreenSurface) {
        if (QOpenGLContext::currentContext()) {
            QOpenGLContext::currentContext()->doneCurrent();
        }
        delete m_offscreenSurface;
        m_offscreenSurface = nullptr;
    }
    m_offscreenSurface = new QOffscreenSurface();
    m_offscreenSurface->setFormat(format);
    m_offscreenSurface->create();
}

bool CyMediaDisViewBckDraw::initglsl(QOpenGLExtraFunctions* f) {
    QElapsedTimer timer;
    // 加载顶点着色器
    if (vertexShaderSource.isEmpty()) {
        QFile vertexFile(":/CyMediaDis/glslSource/Vertex.c");
        if (vertexFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&vertexFile);
            stream.setCodec("UTF-8");
            vertexShaderSource = stream.readAll();
            vertexFile.close();
        }
    }
    //加载公共片元着色器
    loadCommonShader();
    //编译
    if (false == recompileShader(f, &m_textureInfo[0])) {
        return false;
    }
    upShaderUniformSampler(f);

    return true;
}

void CyMediaDisViewBckDraw::initVertex(QOpenGLExtraFunctions* f, const CyMedia::ImageShowInfo& info) {
    //顶点初始化
    m_vertex_current_w = info.width;
    m_vertex_current_h = info.height;
    m_vertex_mulW = 1.0;
    m_vertex_mulH = 1.0;
    float usetWmul = 1.0 / m_vertex_mulW;
    float usetHmul = 1.0 / m_vertex_mulH;

    m_verticesArray[0] = { QVector3D(0, 0, 0.0f),  QVector2D(0.0f, 0.0f) };  // v0
    m_verticesArray[1] = { QVector3D(m_vertex_current_w, 0,5.0f),  QVector2D(usetWmul, 0.0f) };  // v1
    m_verticesArray[2] = { QVector3D(0,m_vertex_current_h, 5.0f),  QVector2D(0.0f, usetHmul) };  // v2
    m_verticesArray[3] = { QVector3D(m_vertex_current_w, m_vertex_current_h, 5.0f),QVector2D(usetWmul, usetHmul) };  // v3

    // Create VAB, VBO, EBO
    pVAO = new QOpenGLVertexArrayObject;
    pVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    pVBO->setUsagePattern(QOpenGLBuffer::DynamicDraw);
    pEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    pVAO->create();
    pVBO->create();
    pEBO->create();

    QOpenGLVertexArrayObject::Binder vaobinder(pVAO);
    // Binding VBO
    pVBO->bind();
    pVBO->allocate(m_verticesArray, sizeof(VerticesAndTextureCoord) * 4);
    // Binding EBO
    pEBO->bind();
    pEBO->allocate(m_indicesArray, sizeof(GLushort) * 4);
    // set data properties
    m_shader_program->enableAttributeArray(0);
    m_shader_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(VerticesAndTextureCoord));
    m_shader_program->enableAttributeArray(1);
    m_shader_program->setAttributeBuffer(1, GL_FLOAT, sizeof(QVector3D), 2, sizeof(VerticesAndTextureCoord));
    vaobinder.release();
}

void CyMediaDisViewBckDraw::upVertex(QOpenGLExtraFunctions* f, int width, int height, float mulW, float mulH) {
    if (!pVBO) return;
    CyMediaDisLog::instance().log_printf(CyMedia::LogLevel::DEBUG, "%s: width:%d height:%d\n", __FUNCTION__, width, height);
    float usetWmul = 1.0 / mulW;
    float usetHmul = 1.0 / mulH;
    m_verticesArray[0] = { QVector3D(0, 0, 0.0f),  QVector2D(0.0f, 0.0f) };  // v0
    m_verticesArray[1] = { QVector3D(width, 0,5.0f),  QVector2D(usetWmul, 0.0f) };  // v1
    m_verticesArray[2] = { QVector3D(0,height, 5.0f),  QVector2D(0.0f, usetHmul) };  // v2
    m_verticesArray[3] = { QVector3D(width, height, 5.0f),QVector2D(usetWmul, usetHmul) };  // v3

    m_vertex_current_w = width;
    m_vertex_current_h = height;
    m_vertex_mulW = mulW;
    m_vertex_mulH = mulH;

    QOpenGLVertexArrayObject::Binder vaobinder(pVAO);
    pVBO->bind();
    pVBO->write(0, m_verticesArray, sizeof(VerticesAndTextureCoord) * 4);
    pVBO->release();
}

void CyMediaDisViewBckDraw::initTexture(QOpenGLExtraFunctions* f) {
    //image
    for (int i = 0; i < 2; i++) {
        oneTexturePara* pTex = &m_textureInfo[i];
        initTextureOne(f, pTex);
    }
    //ColorMap
    pTexture_ColorMap = new QOpenGLTexture(QOpenGLTexture::Target2D);
    pTexture_ColorMap->create();
    f->glActiveTexture(GL_TEXTURE3);
    f->glBindTexture(GL_TEXTURE_2D, pTexture_ColorMap->textureId());
    // Texture filtering method
    pTexture_ColorMap->setMagnificationFilter(QOpenGLTexture::Nearest); // GL_NEAREST：临近插值，容易看出像素点
    pTexture_ColorMap->setMinificationFilter(QOpenGLTexture::Linear);  // GL_LINEAR：线性插值，平滑模糊
    // Texture wrapping
    pTexture_ColorMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    if (m_ColorMapData) {
        f->glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_ColorMapData_Width, m_ColorMapData_Height,
            0, GL_RED, GL_UNSIGNED_BYTE, m_ColorMapData);
        GLenum glerr = f->glGetError();
        if (glerr != GL_NO_ERROR) {
            qWarning() << "glTexImage2D(Colormap) failed:" << glerr;
        }
    }
}

void CyMediaDisViewBckDraw::initTextureOne(QOpenGLExtraFunctions* f, oneTexturePara* pTex, bool allocaMem /*= false*/) {
    //主纹理
    if (!pTex->glTexture) {
        pTex->glTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        pTex->glTexture->create();
        f->glActiveTexture(GL_TEXTURE0);
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTexture->textureId());
        // 设置过滤/包裹
        pTex->glTexture->setMagnificationFilter(QOpenGLTexture::Nearest); // GL_NEAREST：临近插值，容易看出像素点
        pTex->glTexture->setMinificationFilter(QOpenGLTexture::Linear);  // GL_LINEAR：线性插值，平滑模糊
        // 设置环绕模式
        pTex->glTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    }
    // 分配内存
    if (allocaMem) {
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTexture->textureId());
        f->glTexImage2D(GL_TEXTURE_2D, 0, pTex->textureInternalFormat,
            pTex->showInfo.width * pTex->textureWidthMultiplier, pTex->showInfo.height * pTex->textureHeightMultiplier,
            0, pTex->textureFormat, pTex->textureType, nullptr);
    }
    //U 纹理
    if (pTex->useUTex) {
        if (!pTex->glTextureU) {
            pTex->glTextureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
            pTex->glTextureU->create();
            f->glActiveTexture(GL_TEXTURE1);
            f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureU->textureId());
            // 设置过滤/包裹
            pTex->glTextureU->setMagnificationFilter(QOpenGLTexture::Nearest);
            pTex->glTextureU->setMinificationFilter(QOpenGLTexture::Linear);
            // 设置环绕模式
            pTex->glTextureU->setWrapMode(QOpenGLTexture::ClampToEdge);
        }
        // 分配内存
        if (allocaMem) {
            f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureU->textureId());
            f->glTexImage2D(GL_TEXTURE_2D, 0, pTex->uvInternalFormat,
                pTex->uTex_width, pTex->uTex_height, 0, pTex->uvFormat, pTex->uvType, nullptr);
        }
    }
    //不使用则删除？
    else {
        if (pTex->glTextureU) {
            pTex->glTextureU->destroy();
            delete pTex->glTextureU;
            pTex->glTextureU = nullptr;
        }
    }

    //V 纹理
    if (pTex->useVTex) {
        if (!pTex->glTextureV) {
            pTex->glTextureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
            pTex->glTextureV->create();
            f->glActiveTexture(GL_TEXTURE2);
            f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureV->textureId());
            // 设置过滤/包裹
            pTex->glTextureV->setMagnificationFilter(QOpenGLTexture::Nearest);
            pTex->glTextureV->setMinificationFilter(QOpenGLTexture::Linear);
            // 设置环绕模式
            pTex->glTextureV->setWrapMode(QOpenGLTexture::ClampToEdge);
        }
        // 分配内存
        if (allocaMem) {
            f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureV->textureId());
            f->glTexImage2D(GL_TEXTURE_2D, 0, pTex->uvInternalFormat,
                pTex->vTex_width, pTex->vTex_height, 0, pTex->uvFormat, pTex->uvType, nullptr);
        }
    }
    //不使用则删除？
    else {
        if (pTex->glTextureV) {
            pTex->glTextureV->destroy();
            delete pTex->glTextureV;
            pTex->glTextureV = nullptr;
        }
    }
}

bool CyMediaDisViewBckDraw::upTexture(QOpenGLExtraFunctions* f, int backIdx, CyMedia::ImageShowInfo info, uint8_t* data) {
    oneTexturePara* pTex = &m_textureInfo[backIdx];
    GLenum glerr = GL_NO_ERROR;
    int newWidth = info.width;
    int newHeight = info.height;
    // 信息变化 更新纹理参数
    bool imag_infoChange = memcmp(&pTex->showInfo, &info, sizeof(CyMedia::ImageShowInfo)) != 0 || m_fisrt_up_image;
    //信息未变，且已超限
    if (false == imag_infoChange && true == m_bIsOVerSize) return false;
    //重新分配尺寸
    if (imag_infoChange) {
        //copy新的信息
        memcpy(&pTex->showInfo, &info, sizeof(CyMedia::ImageShowInfo));
        m_bIsOVerSize = newWidth > m_gl_max_texture_size || newHeight > m_gl_max_texture_size;
        bool tOverSize = m_bIsOVerSize;
        //如果超限且是灰度图可按RGBA传输
        if (m_bIsOVerSize) {
            if (info.isMono()) {
                if (newWidth / 2 > m_gl_max_texture_size || newHeight / 2 > m_gl_max_texture_size) {
                    m_bIsOVerSize = false;
                    pTex->showInfo.format = CyMedia::MONO_OVERSIZE;
                    pTex->glslNcolor = CyMedia::MONO_OVERSIZE;
                }
            }
        }
        //否则不处理
        if (m_bIsOVerSize) {
            QString errTxt = QString("currentSize(%1*%2) maxSize(%3*%4)").arg(newWidth).arg(newHeight).arg(m_gl_max_texture_size).arg(m_gl_max_texture_size);
            if (false == m_message_overSize->isVisible() || errTxt != m_message_overSize->text()) {
                emit textureSizeOver(true, errTxt);
            }
            return false;
        }
        //隐藏超限警告
        if (m_message_overSize->isVisible()) {
            emit textureSizeOver(false, "");
        }

        //更新格式数据
        updateTextureFormat(pTex->showInfo, backIdx, tOverSize);
        if (pTex->needUpImageInfo == false) {
            pTex->needUpImageInfo = true;
        }

        //重新初始化纹理
        initTextureOne(f, pTex, true);
        m_fisrt_up_image = true;
    }
    //和上一帧前台纹理信息不同也需要更新
    if (pTex->needUpImageInfo == false) {
        pTex->needUpImageInfo = (memcmp(&pTex->showInfo, &m_textureInfo[1 - backIdx].showInfo, sizeof(CyMedia::ImageShowInfo)) != 0);
    }
    //更新纹理
    //主纹理(Y 或 Packed)
    f->glBindTexture(GL_TEXTURE_2D, pTex->glTexture->textureId());
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
        info.width * pTex->textureWidthMultiplier,
        info.height * pTex->textureHeightMultiplier,
        pTex->textureFormat, pTex->textureType, data);
    glerr = f->glGetError();
    if (glerr != GL_NO_ERROR) {
        qWarning() << "glTexSubImage2D(main) failed:" << glerr;
        return false;
    }
    //U 纹理 (U/UV)
    if (pTex->useUTex) {
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureU->textureId());
        f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            pTex->uTex_width,
            pTex->uTex_height,
            pTex->uvFormat, pTex->uvType, data + pTex->uTexDataOffset);
        glerr = f->glGetError();
        if (glerr != GL_NO_ERROR) {
            qWarning() << "glTexSubImage2D(U) failed:" << glerr;
            return false;
        }
    }
    //V 纹理
    if (pTex->useVTex) {
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTextureV->textureId());
        f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            pTex->vTex_width,
            pTex->vTex_height,
            pTex->uvFormat, pTex->uvType, data + pTex->vTexDataOffset);
        glerr = f->glGetError();
        if (glerr != GL_NO_ERROR) {
            qWarning() << "glTexSubImage2D(v) failed:" << glerr;
            return false;
        }
    }
    return true;
}

void CyMediaDisViewBckDraw::clearGL() {
    if (m_gl_init) {
        m_gl_init = false;
        m_ctx_main->makeCurrent(m_offscreenSurface);
        QOpenGLExtraFunctions* f = m_ctx_main->extraFunctions(); f->initializeOpenGLFunctions();
        delete m_shader_program; m_shader_program = nullptr;
        delete pVAO; pVAO = nullptr;
        delete pVBO; pVBO = nullptr;
        delete pEBO; pEBO = nullptr;
        for (int i = 0; i < 2; i++) {
            if (m_textureInfo[i].glTexture) { delete m_textureInfo[i].glTexture; m_textureInfo[i].glTexture = nullptr; }
            //if (m_textureInfo[i].glTextureU) { m_textureInfo[i].glTextureU->destroy(); delete m_textureInfo[i].glTextureU; m_textureInfo[i].glTextureU = nullptr; }
            //if (m_textureInfo[i].glTextureV) { m_textureInfo[i].glTextureV->destroy(); delete m_textureInfo[i].glTextureV; m_textureInfo[i].glTextureV = nullptr; }
            if (m_textureInfo[i].syncFence) f->glDeleteSync(m_textureInfo[i].syncFence); m_textureInfo[i].syncFence = nullptr;
        }
        delete pTexture_ColorMap; pTexture_ColorMap = nullptr;
        m_ctx_main->doneCurrent();

        delete m_offscreenSurface;m_offscreenSurface = nullptr;
        m_ctx_main = nullptr;
    }
}

void CyMediaDisViewBckDraw::updateTextureFormat(const CyMedia::ImageShowInfo& info, int idx, bool oversize/* = false*/) {
    oneTexturePara* pTex = &m_textureInfo[idx];

    pTex->glslNcolor = info.format;
    pTex->bitMul = 1.0;
    if (info.bit > 8 && info.bit < 16) {
        pTex->bitMul = 65535.0f / (1 << info.bit);
    }
    else if (info.bit > 16 && info.bit <= 31) {
        pTex->bitMul = INT_MAX * 1.0f / (1 << info.bit);
    }
    pTex->textureWidthMultiplier = 1.0;
    pTex->textureHeightMultiplier = 1.0;
    pTex->useUTex = false;
    pTex->useVTex = false;

    //MONO/BAYER
    if (info.isMono() || info.isBayer()) {
        pTex->textureFormat = GL_RED;
        if (info.bit <= 8) {
            pTex->textureInternalFormat = GL_R8;
            pTex->textureType = GL_UNSIGNED_BYTE;
        }
        else if (info.bit <= 16) {
            pTex->textureInternalFormat = GL_R16;
            pTex->textureType = GL_UNSIGNED_SHORT;
        }
        else {
            pTex->textureInternalFormat = GL_R32F;
            pTex->textureType = GL_INT;
        }
        if (oversize) {
            pTex->textureInternalFormat = GL_RGBA;
            pTex->textureFormat = GL_RGBA;
            if (info.bit <= 8) pTex->textureType = GL_UNSIGNED_BYTE;
            else if (info.bit <= 16) pTex->textureType = GL_UNSIGNED_SHORT;
            else if (info.bit < 32) pTex->textureType = GL_INT;

            pTex->textureWidthMultiplier = 0.5;
            pTex->textureHeightMultiplier = 0.5;
            pTex->glslNcolor = CyMedia::MONO_OVERSIZE;
        }
    }
    // RGB
    else if (info.isRGB()) {
        pTex->textureInternalFormat = GL_RGB;
        pTex->textureFormat = GL_RGB; if (info.format == CyMedia::BGR) pTex->textureFormat = GL_BGR;

        if (info.bit <= 8) pTex->textureType = GL_UNSIGNED_BYTE;
        else if (info.bit <= 16) pTex->textureType = GL_UNSIGNED_SHORT;
        else pTex->textureType = GL_INT;
    }
    // RGBA
    else if (info.isRGBA()) {
        pTex->textureInternalFormat = GL_RGBA;
        pTex->textureFormat = GL_RGBA;

        if (info.bit <= 8) pTex->textureType = GL_UNSIGNED_BYTE;
        else if (info.bit <= 16) pTex->textureType = GL_UNSIGNED_SHORT;
        else pTex->textureType = GL_INT;
    }
    // yuv
    else if (info.isYUV()) {
        if (info.isYUV_Packed()) {
            // Packed: YUY2 / YVYU，单纹理，宽度双倍
            pTex->textureWidthMultiplier = 2.0f;
            pTex->textureInternalFormat = GL_R8;
            pTex->textureFormat = GL_RED;
            pTex->textureType = GL_UNSIGNED_BYTE;
            return;
        }
        else if (info.isYUV_Planar()) {
            // 三平面：Y, U, V 均为单通道
            pTex->textureWidthMultiplier = 1.0f;
            pTex->textureHeightMultiplier = 1.0f;
            pTex->textureInternalFormat = GL_R8;
            pTex->textureFormat = GL_RED;
            pTex->textureType = GL_UNSIGNED_BYTE;

            // U/V 平面格式相同
            pTex->useUTex = true;
            pTex->useVTex = true;

            int ySize = info.width * info.height;
            int uvSize = (info.width / 2) * (info.height / ((info.format == CyMedia::FOURCC_I420 || info.format == CyMedia::FOURCC_YV12) ? 2 : 1));
            pTex->uTexDataOffset = ySize;
            pTex->vTexDataOffset = ySize + uvSize;

            pTex->uTex_width = pTex->vTex_width = pTex->showInfo.width / 2;
            pTex->uTex_height = pTex->vTex_height = pTex->showInfo.height;
            if (pTex->showInfo.format == CyMedia::FOURCC_I420 || pTex->showInfo.format == CyMedia::FOURCC_YV12)
                pTex->uTex_height = pTex->vTex_height = pTex->showInfo.height / 2;

            pTex->uvInternalFormat = GL_R8;
            pTex->uvFormat = GL_RED;
            pTex->uvType = pTex->textureType;
            return;
        }
        else if (info.isYUV_SemiPlanar()) {
            // NV12/NV21：Y 平面单通道，UV 平面双通道 RG
            pTex->textureWidthMultiplier = 1.0f;
            pTex->textureHeightMultiplier = 1.0f;
            pTex->textureInternalFormat = GL_R8;
            pTex->textureFormat = GL_RED;
            pTex->textureType = GL_UNSIGNED_BYTE;

            //只是用U纹理装UV平面
            pTex->useUTex = true;
            pTex->useVTex = false;

            pTex->uTexDataOffset = info.width * info.height;

            pTex->uTex_width = pTex->showInfo.width / 2;
            pTex->uTex_height = pTex->showInfo.height;

            pTex->uvInternalFormat = GL_RG8;
            pTex->uvFormat = GL_RG;
            pTex->uvType = pTex->textureType;
            return;
        }
    }
}

void CyMediaDisViewBckDraw::upShaderUniformSampler(QOpenGLExtraFunctions* f) {
    f->glUniform1i(uniform_texture, 0);
    f->glUniform1i(uniform_textureU, 1);
    f->glUniform1i(uniform_textureV, 2);
    f->glUniform1i(uniform_texture_colormap, 3);
}

void CyMediaDisViewBckDraw::upShaderUniformImageInfo(QOpenGLExtraFunctions* f, int colorType) {
    int fontIdx = m_texture_front_Index.load();
    const auto& showInfo = m_textureInfo[fontIdx].showInfo;
    //设置图像信息
    f->glUniform1i(uniform_nWidth, showInfo.width);
    f->glUniform1i(uniform_nHeight, showInfo.height);
    f->glUniform1i(uniform_nbits, showInfo.bit);
    f->glUniform1i(uniform_colorType, colorType);
    f->glUniform1i(uniform_bayerPatter, colorType - int(CyMedia::BAYERRG));
    f->glUniform1f(uniform_bitMul, m_textureInfo[fontIdx].bitMul);
}

void CyMediaDisViewBckDraw::upShaderUniformOther(QOpenGLExtraFunctions* f, QMatrix4x4 mat, float zoom /*= 1.0f*/) {
    // 设置矩阵
    f->glUniformMatrix4fv(uniform_m_matrix, 1, GL_FALSE, mat.constData());
    f->glUniform1f(uniform_zoomValue, zoom);
    // 设置颜色格式转换方法
    f->glUniform1i(uniform_demosacFunc, int(mDemosaicMethod));
    f->glUniform1i(uniform_YUVMethod, int(m_yuv_trans_method));
    // 设置颜色映射
    f->glUniform1i(uniform_colorMapIndex, m_ColorMapIndex);
}

void CyMediaDisViewBckDraw::updateStretchUniforms(QOpenGLExtraFunctions* f) {
    auto program = m_shader_program->programId();

    f->glUniform1i(f->glGetUniformLocation(program, "stretchType"), int32_t(eStretchType));
    f->glUniform2f(f->glGetUniformLocation(program, "stretchPara"),
        stretchpara_K, stretchpara_C);
}

void CyMediaDisViewBckDraw::showGlslError(QString tiltle, QString txt) {
    if (m_message_glslError->isVisible()) return;
    m_message_glslError->setWindowTitle(tiltle);
    m_message_glslError->setText(txt);
    m_message_glslError->show();
}

void CyMediaDisViewBckDraw::showOverSizeError(QString Text) {
    m_message_overSize->setText(Text);
    m_message_overSize->show();
}

void CyMediaDisViewBckDraw::hideOverSizeError() {
    m_message_overSize->hide();
}

bool CyMediaDisViewBckDraw::upBackGround(CyMedia::ImageShowInfo info, uint8_t* data, QOpenGLContext* ctx) {
    Q_ASSERT_X(ctx != nullptr, "upBackGround", "Context must not be null!");
    if (!data || !m_gl_init) return false;
    if (false == makeOffSurface(ctx)) return false;
    QOpenGLExtraFunctions* f = ctx->extraFunctions(); if (!f) return false;
    f->initializeOpenGLFunctions();

    //当前后台(空闲)纹理
    int backIdx = 1 - m_texture_front_Index.load();
    oneTexturePara* pTex = &m_textureInfo[backIdx];

    //更新纹理
    if (false == upTexture(f, backIdx, info, data)) return false;
    
    //同步
    if (pTex->syncFence) f->glDeleteSync(pTex->syncFence);
    pTex->syncFence = f->glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    const GLuint64 timeoutNs = 5 * 1000 * 1000; // 5,000,000 ns
    const GLuint64 waitStepNs = 100 * 1000;     // 每次等待 0.1ms
    GLuint64 elapsed = 0;
    GLenum result = GL_UNSIGNALED;
    while (elapsed < timeoutNs) {
        result = f->glClientWaitSync(pTex->syncFence, GL_SYNC_FLUSH_COMMANDS_BIT, waitStepNs);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
            break; // 更新完毕
        }
        elapsed += waitStepNs + 100;
        QThread::usleep(100);
    }

    if (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED) {
        CyMediaDisLog::instance().log_printf(CyMedia::LogLevel::WAR, "glClientWaitSync timeout after %lld ns, fallback to glFinish()", elapsed);
        // 回退：强制完成所有命令，确保纹理上传完毕
        f->glFinish();
    }

    f->glDeleteSync(pTex->syncFence);
    pTex->syncFence = nullptr;

    f->glBindTexture(GL_TEXTURE_2D, 0);
    //f->glFlush();
    //f->glFinish();
    ctx->doneCurrent();
    //标记当前显示图像状态
    m_haveImage = true;
    m_fisrt_up_image = false;
    //交换前后台纹理索引
    if (!m_hasNewData.exchange(true)) {
        QMetaObject::invokeMethod(m_view->viewport(), "update", Qt::QueuedConnection);
    }
    texrureFpsCount++;
    return true;
}

void CyMediaDisViewBckDraw::clearBackGround() {
    m_haveImage = false;
    m_hasNewData.store(false);
    if (QThread::currentThread() == qApp->thread()) {
        if (m_view->viewport()) m_view->viewport()->update();
    }
}

int CyMediaDisViewBckDraw::backTextureIndex() {
    return 1 - m_texture_front_Index.load();
}

bool CyMediaDisViewBckDraw::haveImage() {
    return m_haveImage;
}

QOpenGLContext* CyMediaDisViewBckDraw::mainContext() {
    if (false == m_gl_init) return 0;
    return m_ctx_main;
}

CyMedia::StretchType CyMediaDisViewBckDraw::stretchType() {
    return eStretchType;
}

void CyMediaDisViewBckDraw::setStretchType(CyMedia::StretchType type) {
    if (eStretchType == type)
        return;
    eStretchType = type;
    upStretchValue = true;
}

void CyMediaDisViewBckDraw::setStreaChPara(uint32_t start /*= 0*/, uint32_t end /*= 0*/, uint32_t max /*= 0*/) {
    if (stretchpara_start == start &&
        stretchpara_end == end &&
        stretchpara_max == max)
        return;

    stretchpara_start = start;
    stretchpara_end = end;
    stretchpara_max = max;

    double ds = static_cast<double>(start) / stretchpara_max;
    double de = static_cast<double>(end) / stretchpara_max;
    stretchpara_K = 1.0 / (de - ds);
    stretchpara_C = -(stretchpara_K)*ds;

    upStretchValue = true;
}

CyMedia::DemosaicingMethod CyMediaDisViewBckDraw::Demosaic() {
    return mDemosaicMethod;
}

void CyMediaDisViewBckDraw::setDemosaic(CyMedia::DemosaicingMethod method) {
    mDemosaicMethod = method;
}

CyMedia::YUVTransMethod CyMediaDisViewBckDraw::yuvMethod() {
    return m_yuv_trans_method;
}

void CyMediaDisViewBckDraw::setYUVTMethod(CyMedia::YUVTransMethod method) {
    m_yuv_trans_method = method;
}

double CyMediaDisViewBckDraw::flushFps() const {
    if (useTrueDataFps) return TextureFps;
    else return DisFps;
}

bool CyMediaDisViewBckDraw::isTrueDataFps() const {
    return useTrueDataFps;
}

void CyMediaDisViewBckDraw::setTrueDataFps(bool flag) {
    useTrueDataFps = flag;
}

QStringList CyMediaDisViewBckDraw::ColorMapList() const {
    return m_ColorMapList;
}

qint32 CyMediaDisViewBckDraw::colorMapIndex() const {
    return m_ColorMapIndex;
}

QString CyMediaDisViewBckDraw::colorMapName() const {
    return m_ColorMapName;
}

bool CyMediaDisViewBckDraw::setColorMap(qint32 index) {
    if (index == m_ColorMapIndex)
        return true;
    if (index >= m_ColorMapList.size())
        return false;
    //除默认None外读取CM文件
    if (index > 0) {
        QString transCMFileName = QString("%1/%2.cm").arg(m_ColorMapDirPath).arg(m_ColorMapList[index]);
        QFile cmFile(transCMFileName);
        if (false == cmFile.open(QIODevice::ReadOnly))
            return false;
        auto readCode = cmFile.readAll();
        cmFile.close();
        memcpy(m_ColorMapData, readCode.data(), m_ColorMapFileSize);
    }
    m_ColorMapIndex = index;
    m_ColorMapName = m_ColorMapList[index];
    bColorMapChange = true;
    return true;
}

bool CyMediaDisViewBckDraw::setColorMap(const QString& mapName) {
    if (m_ColorMapList.contains(mapName))
        return false;
    auto index = m_ColorMapList.indexOf(mapName);
    if (index == -1)
        return false;
    return setColorMap(index);
}