#include "CyMediaDisViewBckDraw.h"
#include "CyMediaDisView.h"

#include <QApplication>
#include <QThread>

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

void CyMediaDisViewBckDraw::initgl(QOpenGLContext* ctx) {
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
    QOpenGLExtraFunctions* f = nullptr;
    if (m_ctx_main) f = m_ctx_main->extraFunctions();
    if (!f) return;

    painter->beginNativePainting();

    oneTexturePara* pTex = &m_textureInfo[m_texture_front_Index.load()];
    if (m_hasNewData.load()) {
        // 交换索引：让后台变为前台
        int oldFront = m_texture_front_Index.load();
        int newFront = 1 - oldFront;
        m_texture_front_Index.store(newFront);
        m_hasNewData.store(false);   // 标志重置，表示此新数据已被处理
        pTex = &m_textureInfo[newFront];
    }

    //绑定纹理
    int idx = m_texture_front_Index.load();
    oneTexturePara* pTextureInfo = &m_textureInfo[idx];

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, pTextureInfo->glTexture->textureId());
    
    //更新着色器参数
    f->glUseProgram(m_shader_program->programId());
    QMatrix4x4 projectionMatrix;
    projectionMatrix.ortho(0.0f, viewRect.width(), viewRect.height(), 0.0f, -1000.0f, 1000.0f);
    setupShaderUniforms(f, projectionMatrix * painter->worldTransform(), int(pTextureInfo->glslNcolor));

    // 绘制
    QOpenGLVertexArrayObject::Binder vaobinder(pVAO);
    f->glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

    // 解绑
    f->glBindTexture(GL_TEXTURE_2D, 0);
    m_shader_program->release();
    vaobinder.release();

    painter->endNativePainting();
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

bool CyMediaDisViewBckDraw::initglsl(QOpenGLExtraFunctions* f) {
    QElapsedTimer timer;
    // 加载源码
    if (vertexShaderSource.isEmpty()) {
        QFile vertexFile(":/CyMediaDis/glslSource/Vertex.c");
        if (vertexFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&vertexFile);
            stream.setCodec("UTF-8");
            vertexShaderSource = stream.readAll();
            vertexFile.close();
        }
    }
#ifdef DEBUG_
    //从文件加载片元着色器
    if (fragmentShaderSource.isEmpty()) {

        QFile fragementFile(QDir::currentPath() + "/SapViewer_Fragement.c");
        if (fragementFile.open(QIODevice::ReadOnly)) {
            auto readCode = fragementFile.readAll();
            fragmentShaderSource = QString(readCode);
            fragementFile.close();
        }

    }
#endif
    if (fragmentShaderSource.isEmpty()) {
        QFile fragmentFile(":/CyMediaDis/glslSource/Fragment.c");
        if (fragmentFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&fragmentFile);
            stream.setCodec("UTF-8");
            fragmentShaderSource = stream.readAll();
            fragmentFile.close();
        }
    }

    //着色器程序初始化
    m_shader_program = new QOpenGLShaderProgram;
    m_shader_program->create();
    f->glUseProgram(m_shader_program->programId());
    if (false == m_shader_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        showGlslError(tr("vertex shader"), m_shader_program->log());
        return false;
    }
    if (false == m_shader_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        showGlslError(tr("fragment shader"), m_shader_program->log());
        return false;
    }
    timer.start();
    if (false == m_shader_program->link()) {
        showGlslError(tr("GL Program Link"), m_shader_program->log());
        return false;
    }
    printf(u8"GL Program Link:%lldms\n\n", timer.elapsed());
    
    // 获取并缓存 uniform 位置
    uniform_texture = m_shader_program->uniformLocation("texture");
    uniform_texture_colormap = m_shader_program->uniformLocation("texture_ColorMap");
    uniform_m_matrix = m_shader_program->uniformLocation("m_matrix");
    uniform_zoomValue = m_shader_program->uniformLocation("zoomValue");
    uniform_colorType = m_shader_program->uniformLocation("colorType");
    uniform_demosacFunc = m_shader_program->uniformLocation("demosacFunc");
    uniform_YUVMethod = m_shader_program->uniformLocation("YUVMethod");
    uniform_nbits = m_shader_program->uniformLocation("nbits");
    uniform_nWidth = m_shader_program->uniformLocation("nWidth");
    uniform_nHeight = m_shader_program->uniformLocation("nHeight");
    uniform_pixrange = m_shader_program->uniformLocation("pixrange");
    uniform_colorMapIndex = m_shader_program->uniformLocation("colorMapIndex");
    uniform_stretchType = m_shader_program->uniformLocation("stretchType");
    uniform_stretchPara = m_shader_program->uniformLocation("stretchPara");

    f->glUniform1i(uniform_texture, 0);
    f->glUniform1i(uniform_texture_colormap, 1);

    return true;
}

void CyMediaDisViewBckDraw::initVertex(QOpenGLExtraFunctions* f, const CyMedia::ImageShowInfo& info) {
    //顶点初始化
    m_vertex_current_w = info.width;
    m_vertex_current_h = info.height;

    if (bIsOVerSize) {
        m_verticesArray[0] = { QVector3D(0,0,0.0f),  QVector2D(0.0f, 0.0f) };  // v0
        m_verticesArray[1] = { QVector3D(info.width,0,5.0f),  QVector2D(2.0f, 0.0f) };  // v1
        m_verticesArray[2] = { QVector3D(0,info.height, 5.0f),  QVector2D(0.0f, 2.0f) };  // v2
        m_verticesArray[3] = { QVector3D(info.width,  info.height, 5.0f),QVector2D(2.0f, 2.0f) };  // v3
    }
    else {
        m_verticesArray[0] = { QVector3D(0,0,0.0f),  QVector2D(0.0f, 0.0f) };  // v0
        m_verticesArray[1] = { QVector3D(info.width,0,5.0f),  QVector2D(1.0f, 0.0f) };  // v1
        m_verticesArray[2] = { QVector3D(0,info.height, 5.0f),  QVector2D(0.0f, 1.0f) };  // v2
        m_verticesArray[3] = { QVector3D(info.width,  info.height, 5.0f),QVector2D(1.0f, 1.0f) };  // v3
    }
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
    int attr = -1;
    attr = m_shader_program->attributeLocation("apos");
    m_shader_program->enableAttributeArray(attr);
    m_shader_program->setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(VerticesAndTextureCoord));
    attr = m_shader_program->attributeLocation("atexcoord");
    m_shader_program->enableAttributeArray(attr);
    m_shader_program->setAttributeBuffer(attr, GL_FLOAT, sizeof(QVector3D), 2, sizeof(VerticesAndTextureCoord));
    vaobinder.release();
    //p_data->pVBO->release();
    //p_data->pEBO->release();
}

void CyMediaDisViewBckDraw::initTexture(QOpenGLExtraFunctions* f) {
    //ColorMap
    pTexture_ColorMap = new QOpenGLTexture(QOpenGLTexture::Target2D);
    pTexture_ColorMap->create();
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, pTexture_ColorMap->textureId());
    // Texture filtering method
    pTexture_ColorMap->setMagnificationFilter(QOpenGLTexture::Nearest); // GL_NEAREST：临近插值，容易看出像素点
    pTexture_ColorMap->setMinificationFilter(QOpenGLTexture::Linear);  // GL_LINEAR：线性插值，平滑模糊
    // Texture wrapping
    pTexture_ColorMap->setWrapMode(QOpenGLTexture::ClampToEdge);
    if (m_ColorMapData) {
        f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_ColorMapData_Width, m_ColorMapData_Height,
            0, GL_RGB, GL_UNSIGNED_BYTE, m_ColorMapData);
    }

    //image
    for (int i = 0; i < 2; i++) {
        oneTexturePara* pTex = &m_textureInfo[i];
        // Create texture and bind it
        pTex->glTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        pTex->glTexture->create();
        f->glActiveTexture(GL_TEXTURE0);
        f->glBindTexture(GL_TEXTURE_2D, pTex->glTexture->textureId());
        // Texture filtering method
        pTex->glTexture->setMagnificationFilter(QOpenGLTexture::Nearest); // GL_NEAREST：临近插值，容易看出像素点
        pTex->glTexture->setMinificationFilter(QOpenGLTexture::Linear);  // GL_LINEAR：线性插值，平滑模糊
        // Texture wrapping
        pTex->glTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
        // alloca
        f->glTexImage2D(GL_TEXTURE_2D, 0, pTex->textureInternalFormat,
            pTex->showInfo.width * pTex->textureWidthMultiplier, pTex->showInfo.height * pTex->textureHeightMultiplier,
            0, pTex->textureFormat, pTex->textureType, nullptr);
    }
}

void CyMediaDisViewBckDraw::clearGL() {
    if (m_gl_init) {
        m_gl_init = false;

        m_ctx_main->makeCurrent(m_offscreenSurface);
        QOpenGLExtraFunctions* f = m_ctx_main->extraFunctions();
        delete m_shader_program; m_shader_program = nullptr;
        delete pVAO; pVAO = nullptr;
        delete pVBO; pVBO = nullptr;
        delete pEBO; pEBO = nullptr;
        for (int i = 0; i < 2; i++) {
            delete m_textureInfo[i].glTexture; m_textureInfo[i].glTexture = nullptr;
            if (m_textureInfo[i].syncFence) f->glDeleteSync(m_textureInfo[i].syncFence); m_textureInfo[i].syncFence = nullptr;
        }
        delete pTexture_ColorMap; pTexture_ColorMap = nullptr;
        m_ctx_main->doneCurrent();

        delete m_offscreenSurface;m_offscreenSurface = nullptr;
        m_ctx_main = nullptr;
    }
}

void CyMediaDisViewBckDraw::upVertex(QOpenGLExtraFunctions* f, const CyMedia::ImageShowInfo& info) {
    if (!pVBO) return;
    printf("CyMediaDisViewBckDraw::upVertex: width:%d height:%d\n", info.width, info.height);
    if (bIsOVerSize) {
        m_verticesArray[0] = { QVector3D(0,0,0.0f),  QVector2D(0.0f, 0.0f) };  // v0
        m_verticesArray[1] = { QVector3D(info.width,0,5.0f),  QVector2D(2.0f, 0.0f) };  // v1
        m_verticesArray[2] = { QVector3D(0,info.width, 5.0f),  QVector2D(0.0f, 2.0f) };  // v2
        m_verticesArray[3] = { QVector3D(info.width,  info.height, 5.0f),QVector2D(2.0f, 2.0f) };  // v3
    }
    else {
        m_verticesArray[0] = { QVector3D(0,0,0.0f),  QVector2D(0.0f, 0.0f) };  // v0
        m_verticesArray[1] = { QVector3D(info.width,0,5.0f),  QVector2D(1.0f, 0.0f) };  // v1
        m_verticesArray[2] = { QVector3D(0,info.height, 5.0f),  QVector2D(0.0f, 1.0f) };  // v2
        m_verticesArray[3] = { QVector3D(info.width,  info.height, 5.0f),QVector2D(1.0f, 1.0f) };  // v3
    }
    m_vertex_current_w = info.width;
    m_vertex_current_h = info.height;
    QOpenGLVertexArrayObject::Binder vaobinder(pVAO);
    pVBO->bind();
    pVBO->write(0, m_verticesArray, sizeof(VerticesAndTextureCoord) * 4);
    pVBO->release();
}

void CyMediaDisViewBckDraw::updateTextureFormat(const CyMedia::ImageShowInfo& info, int idx) {
    oneTexturePara* pTextureInfo = &m_textureInfo[idx];

    pTextureInfo->glslNcolor = info.format;
    pTextureInfo->maxBitlColor = (1U << info.bit);
    pTextureInfo->textureWidthMultiplier = 1.0;
    pTextureInfo->textureHeightMultiplier = 1.0;
    // 更新纹理格式
    if (info.isYUV()) {
        // YUV 每个平面为单通道，按位深设置
        if (info.bit <= 8) {
            pTextureInfo->textureInternalFormat = GL_R8;
            pTextureInfo->textureFormat = GL_RED;
            pTextureInfo->textureType = GL_UNSIGNED_BYTE;
        }
        else if (info.bit <= 16) {
            pTextureInfo->textureInternalFormat = GL_R16;
            pTextureInfo->textureFormat = GL_RED;
            pTextureInfo->textureType = GL_UNSIGNED_SHORT;
        }
        else {
            pTextureInfo->textureInternalFormat = GL_R32F;
            pTextureInfo->textureFormat = GL_RED;
            pTextureInfo->textureType = GL_UNSIGNED_INT;
        }
        return;
    }
    switch (info.format) {
        case CyMedia::MONO_OVERSIZE: {
            if (info.bit <= 8) {
                pTextureInfo->textureInternalFormat = GL_RGBA;
                pTextureInfo->textureWidthMultiplier = 0.5;
                pTextureInfo->textureHeightMultiplier = 0.5;
                pTextureInfo->textureFormat = GL_RGBA;
                pTextureInfo->textureType = GL_UNSIGNED_BYTE;
            }
            else if (info.bit <= 16) {
                pTextureInfo->textureInternalFormat = GL_RGBA;
                pTextureInfo->textureFormat = GL_RGBA;
                pTextureInfo->textureType = GL_UNSIGNED_SHORT;
            }
            else if (info.bit < 32) {
                pTextureInfo->textureInternalFormat = GL_R32F;
                pTextureInfo->textureFormat = GL_RED;
                pTextureInfo->textureType = GL_UNSIGNED_INT;
            }
        }break;

        case CyMedia::MONO:
        case CyMedia::BAYERRG:
        case CyMedia::BAYERGR:
        case CyMedia::BAYERBG:
        case CyMedia::BAYERGB:
        case CyMedia::MONO10P:
        case CyMedia::MONO10P_GVSP:
        case CyMedia::MONO12P:
        case CyMedia::MONO12P_GVSP: {
            if (info.bit <= 8) {
                pTextureInfo->textureInternalFormat = GL_R8;
                pTextureInfo->textureFormat = GL_RED;
                pTextureInfo->textureType = GL_UNSIGNED_BYTE;
            }
            else if (info.bit <= 16) {
                pTextureInfo->textureInternalFormat = GL_R16;
                pTextureInfo->textureFormat = GL_RED;
                pTextureInfo->textureType = GL_UNSIGNED_SHORT;
            }
            else {
                pTextureInfo->textureInternalFormat = GL_R32F;
                pTextureInfo->textureFormat = GL_RED;
                pTextureInfo->textureType = GL_UNSIGNED_INT;
            }
        }break;

        case CyMedia::RGB: {
            if (info.bit <= 8) {
                pTextureInfo->textureInternalFormat = GL_RGB;
                pTextureInfo->textureFormat = GL_RGB;
                pTextureInfo->textureType = GL_UNSIGNED_BYTE;
            }
            else if (info.bit <= 16) {
                pTextureInfo->textureInternalFormat = GL_RGB;
                pTextureInfo->textureFormat = GL_RGB;
                pTextureInfo->textureType = GL_UNSIGNED_SHORT;
            }
            else {
                pTextureInfo->textureInternalFormat = GL_RGB;
                pTextureInfo->textureFormat = GL_RGB;
                pTextureInfo->textureType = GL_UNSIGNED_INT;
            }
        }break;

        case CyMedia::RGBA: {
            if (info.bit <= 8) {
                pTextureInfo->textureInternalFormat = GL_RGBA;
                pTextureInfo->textureFormat = GL_RGBA;
                pTextureInfo->textureType = GL_UNSIGNED_BYTE;
            }
            else if (info.bit <= 16) {
                pTextureInfo->textureInternalFormat = GL_RGBA;
                pTextureInfo->textureFormat = GL_RGBA;
                pTextureInfo->textureType = GL_UNSIGNED_SHORT;
            }
            else {
                pTextureInfo->textureInternalFormat = GL_RGBA;
                pTextureInfo->textureFormat = GL_RGBA;
                pTextureInfo->textureType = GL_UNSIGNED_INT;
            }
        }break;
    }
}

void CyMediaDisViewBckDraw::setupShaderUniforms(QOpenGLExtraFunctions* f, QMatrix4x4 mat, int colortype) {
    auto program = m_shader_program->programId();
    // 设置纹理
    f->glUniform1i(uniform_texture, 0);
    f->glUniform1i(uniform_texture_colormap, 1);
    // 设置矩阵
    f->glUniformMatrix4fv(uniform_m_matrix, 1, GL_FALSE, mat.constData());
    f->glUniform1f(uniform_zoomValue, qobject_cast<CyMediaDisView*>(m_view)->zoomValue());
    // 设置颜色类型和格式转换方法
    f->glUniform1i(uniform_colorType, colortype);
    f->glUniform1i(uniform_demosacFunc, int(mDemosaicMethod));
    f->glUniform1i(uniform_YUVMethod, int(m_yuv_trans_method));
    //设置图像信息
    int fontIdx = m_texture_front_Index.load();
    const auto& showInfo = m_textureInfo[fontIdx].showInfo;
    if (m_textureInfo[fontIdx].imag_infoChange) {
        f->glUniform1i(uniform_nbits, showInfo.bit);
        f->glUniform1i(uniform_nWidth, showInfo.width);
        f->glUniform1i(uniform_nHeight, showInfo.height);
        f->glUniform1f(uniform_pixrange, m_textureInfo[fontIdx].maxBitlColor);
    }
    // 设置颜色映射
    f->glUniform1i(uniform_colorMapIndex, m_ColorMapIndex);
    // 设置拉伸参数
    if (upStretchValue) {
        updateStretchUniforms(f);
        upStretchValue = false;
    }
}

bool CyMediaDisViewBckDraw::makeOffSurface(QOpenGLContext* ctx) {
    if (!ctx->makeCurrent(m_offscreenSurface)) {
        qWarning("无法激活工作线程 OpenGL 上下文");
        return false;
    }
    return true;
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

bool CyMediaDisViewBckDraw::shareContext(QOpenGLContext* ctx) {
    if (false == m_gl_init) return false;
    if (!ctx) return false;
    ctx->setFormat(m_ctx_main->format());
    ctx->setShareContext(m_ctx_main);
    if (!ctx->create()) {
        qWarning("无法创建共享上下文");
        return false;
    }
    return true;
}

bool CyMediaDisViewBckDraw::upBackGround(CyMedia::ImageShowInfo info, uint8_t* data, QOpenGLContext* ctx) {
    Q_ASSERT_X(ctx != nullptr, "upBackGround", "Context must not be null!");
    if (!data) return false;

    if (!data || !m_gl_init || !ctx) return false;
    if (false == makeOffSurface(ctx)) return false;
    QOpenGLExtraFunctions* f = ctx->extraFunctions(); if (!f) return false;
    f->initializeOpenGLFunctions();

    //当前后台(空闲)纹理
    int backIdx = 1 - m_texture_front_Index.load();
    oneTexturePara* pTex = &m_textureInfo[backIdx];

    int newWidth = info.width;
    int newHeight = info.height;

    f->glBindTexture(GL_TEXTURE_2D, pTex->glTexture->textureId());
    // 尺寸变化 更新顶点
    pTex->imag_sizeChange = (newWidth != pTex->showInfo.width || newHeight != pTex->showInfo.height) || m_fisrt_up_image;
    // 信息变化 更新纹理参数
    if (pTex->imag_sizeChange) pTex->imag_infoChange = true;
    else {
        pTex->imag_infoChange = memcmp(&pTex->showInfo, &info, sizeof(CyMedia::ImageShowInfo)) != 0 || m_fisrt_up_image;
    }
    //尺寸变化更新顶点
    if (pTex->imag_sizeChange) {
        upVertex(f, info);
    }

    //信息变化重新分配尺寸
    if (pTex->imag_infoChange) {
        //纹理尺寸超限
        bIsOVerSize = newWidth > m_gl_max_texture_size || newHeight > m_gl_max_texture_size;
        bool tSizeOver = bIsOVerSize;
        if (bIsOVerSize) {
            //灰度图像按照RGBA传输
            if (info.isMono()) {
                if (newWidth / 2 > m_gl_max_texture_size || newHeight / 2 > m_gl_max_texture_size) {
                    tSizeOver = false;
                }
            }
        }
        if (tSizeOver) {
            QString errTxt = QString("currentSize(%1*%2) maxSize(%3*%4)").arg(newWidth).arg(newHeight).arg(m_gl_max_texture_size).arg(m_gl_max_texture_size);
            if (false == m_message_overSize->isVisible() || errTxt != m_message_overSize->text()) {
                emit textureSizeOver(true, errTxt);
            }
            return false;
        }
        if (m_message_overSize->isVisible()) {
            emit textureSizeOver(false, "");
        }

        memcpy(&pTex->showInfo, &info, sizeof(CyMedia::ImageShowInfo));
        updateTextureFormat(pTex->showInfo, backIdx);
        // alloca
        f->glTexImage2D(GL_TEXTURE_2D, 0, pTex->textureInternalFormat,
            pTex->showInfo.width * pTex->textureWidthMultiplier, pTex->showInfo.height * pTex->textureHeightMultiplier,
            0, pTex->textureFormat, pTex->textureType, nullptr);
        m_fisrt_up_image = true;
    }

    f->glBindTexture(GL_TEXTURE_2D, pTex->glTexture->textureId());
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
        pTex->showInfo.width * pTex->textureWidthMultiplier, pTex->showInfo.height * pTex->textureHeightMultiplier,
        pTex->textureFormat, pTex->textureType, data);
    if (pTex->syncFence) {
        f->glDeleteSync(pTex->syncFence);
        pTex->syncFence = nullptr;
    }
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
        qWarning("glClientWaitSync timeout after %lld ns, fallback to glFinish()", elapsed);
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
    if (mDemosaicMethod == method)
        return;
    mDemosaicMethod = method;
}

CyMedia::YUVTransMethod CyMediaDisViewBckDraw::yuvMethod() {
    return m_yuv_trans_method;
}

void CyMediaDisViewBckDraw::setYUVTMethod(CyMedia::YUVTransMethod method) {
    if (m_yuv_trans_method == method)
        return;
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

bool CyMediaDisViewBckDraw::setColorMap(qint32 index) {
    if (index == m_ColorMapIndex)
        return true;
    if (index >= m_ColorMapList.size())
        return false;
    //除默认None外读取CM文件
    if (index > 0) {
        QString transCMFileName = QString("%1\\%2.cm").arg(m_ColorMapDirPath).arg(m_ColorMapList[index]);
        QFile cmFile(transCMFileName);
        if (false == cmFile.open(QIODevice::ReadOnly))
            return false;
        auto readCode = cmFile.readAll();
        cmFile.close();
        memcpy(m_ColorMapData, readCode.data(), m_ColorMapFileSize);
    }
    m_ColorMapIndex = index;
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