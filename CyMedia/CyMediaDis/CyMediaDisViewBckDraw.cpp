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
    // 初始化默认值
    oneTexturePara* ptextureInfo = nullptr;
    for (int i = 0; i < 2; i++) {
        ptextureInfo = &m_textureInfo[i];
        ptextureInfo->showInfo.width = 1000;
        ptextureInfo->showInfo.height = 1000;
        ptextureInfo->showInfo.bit = 8;
        ptextureInfo->showInfo.special_pixel = CyMedia::PIXEL_VALUE_INT;
        ptextureInfo->showInfo.format = CyMedia::MONO;
        ptextureInfo->showInfo.upLenth();
        updateTextureFormat(ptextureInfo->showInfo, i);
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
            printf("textureFps:%.2f showFps:%.2f\n\n", TextureFps, DisFps);
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
    QOpenGLFunctions* f = ctx->functions(); if (!f) return;
    f->initializeOpenGLFunctions();
    if (false == initglsl(f)) return;
    initVertex(f, m_textureInfo[0].showInfo);
    initTexture(f);
    // Surface
    m_offscreenSurface = new QOffscreenSurface();
    m_offscreenSurface->setFormat(ctx->format());
    m_offscreenSurface->create();

    m_ctx_main = ctx;
    m_gl_init = true;
}

bool CyMediaDisViewBckDraw::glIsInit() {
    return m_gl_init;
}

void CyMediaDisViewBckDraw::drawBackground(QPainter* painter, const QRectF& rect) {
    QSize viewRect = m_view->viewport()->size() * m_view->viewport()->devicePixelRatioF();

    showFpsCount++;
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

    if (false == m_gl_init) return;
    QOpenGLFunctions* f = nullptr;
    if (m_ctx_main) f = m_ctx_main->functions();
    if (!f) return;

    painter->beginNativePainting();

    if (m_hasNewData.load()) {
        // 交换索引：让后台变为前台
        int oldFront = m_texture_front_Index.load();
        int newFront = 1 - oldFront;
        m_texture_front_Index.store(newFront);
        m_hasNewData.store(false);   // 标志重置，表示此新数据已被处理
    }

    //绑定纹理
    int idx = m_texture_front_Index.load();
    oneTexturePara* pTextureInfo = &m_textureInfo[idx];

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, pTextureInfo->glTexture->textureId());

    //更新顶点
    if (pTextureInfo->imag_sizeChange) {
        upVertex(f, pTextureInfo->showInfo);
        pTextureInfo->imag_sizeChange = false;
    }

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

    pVBO->release();
    pEBO->release();

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

bool CyMediaDisViewBckDraw::initglsl(QOpenGLFunctions* f) {
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
        showGlslErro(tr("vertex shader"), m_shader_program->log());
        return false;
    }
    if (false == m_shader_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        showGlslErro(tr("fragment shader"), m_shader_program->log());
        return false;
    }
    timer.start();
    if (false == m_shader_program->link()) {
        showGlslErro(tr("GL Program Link"), m_shader_program->log());
        return false;
    }
    printf(u8"GL Program Link:%lldms\n\n", timer.elapsed());
    f->glUniform1i(f->glGetUniformLocation(m_shader_program->programId(), "texture"), 0);
    f->glUniform1i(f->glGetUniformLocation(m_shader_program->programId(), "texture_ColorMap"), 1);

    //禁用驱动优化
    f->glDisable(GL_TEXTURE_RECTANGLE);
    f->glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

    return true;
}

void CyMediaDisViewBckDraw::initVertex(QOpenGLFunctions* f, const CyMedia::ImageShowInfo& info) {
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

void CyMediaDisViewBckDraw::initTexture(QOpenGLFunctions* f) {
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
        f->glGenerateMipmap(GL_TEXTURE_2D);
    }

    //image
    for (int i = 0; i < 2; i++) {
        oneTexturePara* pTextureInfo = &m_textureInfo[i];
        // Create texture and bind it
        pTextureInfo->glTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        pTextureInfo->glTexture->create();
        f->glActiveTexture(GL_TEXTURE0);
        f->glBindTexture(GL_TEXTURE_2D, pTextureInfo->glTexture->textureId());
        // Texture filtering method
        pTextureInfo->glTexture->setMagnificationFilter(QOpenGLTexture::Nearest); // GL_NEAREST：临近插值，容易看出像素点
        pTextureInfo->glTexture->setMinificationFilter(QOpenGLTexture::Linear);  // GL_LINEAR：线性插值，平滑模糊
        // Texture wrapping
        pTextureInfo->glTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
        // alloca
        f->glTexImage2D(GL_TEXTURE_2D, 0, pTextureInfo->textureInternalFormat,
            pTextureInfo->showInfo.width * pTextureInfo->textureWidthMultiplier, pTextureInfo->showInfo.height * pTextureInfo->textureHeightMultiplier,
            0, pTextureInfo->textureFormat, pTextureInfo->textureType, nullptr);
        // Mipmap
        f->glGenerateMipmap(GL_TEXTURE_2D);
    }
}

void CyMediaDisViewBckDraw::clearGL() {
    if (m_gl_init) {
        m_gl_init = false;

        m_ctx_main->makeCurrent(m_offscreenSurface);
        delete m_shader_program;
        delete pVAO;
        delete pVBO;
        delete pEBO;
        delete m_textureInfo[0].glTexture;
        delete m_textureInfo[1].glTexture;
        delete pTexture_ColorMap;

        m_shader_program = nullptr;
        pVAO = nullptr;
        pVBO = nullptr;
        pEBO = nullptr;
        m_textureInfo[0].glTexture = nullptr;
        m_textureInfo[1].glTexture = nullptr;
        pTexture_ColorMap = nullptr;
        m_ctx_main->doneCurrent();

        delete m_offscreenSurface;
        m_offscreenSurface = nullptr;

        m_ctx_main = nullptr;
    }
}

void CyMediaDisViewBckDraw::upVertex(QOpenGLFunctions* f, const CyMedia::ImageShowInfo& info) {
    if (!pVBO) return;

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

void CyMediaDisViewBckDraw::setupShaderUniforms(QOpenGLFunctions* f, QMatrix4x4 mat, int colortype) {
    auto program = m_shader_program->programId();
    // 设置纹理
    f->glUniform1i(f->glGetUniformLocation(program, "texture"), 0);
    f->glUniform1i(f->glGetUniformLocation(program, "texture_ColorMap"), 1);

    // 设置矩阵
    m_shader_program->setUniformValue("m_matrix", mat);
    m_shader_program->setUniformValue("zoomValue", qobject_cast<CyMediaDisView*>(m_view)->zoomValue());

    // 设置颜色类型
    m_shader_program->setUniformValue("colorType", colortype);
    //马赛克方法
    m_shader_program->setUniformValue("demosacFunc", int(mDemosaicMethod));

    int fontIdx = m_texture_front_Index.load();
    m_shader_program->setUniformValue("nbits", m_textureInfo[fontIdx].showInfo.bit);
    m_shader_program->setUniformValue("nWidth", m_textureInfo[fontIdx].showInfo.width);
    m_shader_program->setUniformValue("nHeight", m_textureInfo[fontIdx].showInfo.height);
    m_shader_program->setUniformValue("pixrange", m_textureInfo[fontIdx].maxBitlColor);

    // 设置颜色映射
    m_shader_program->setUniformValue("colorMapIndex", m_ColorMapIndex);

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

void CyMediaDisViewBckDraw::updateStretchUniforms(QOpenGLFunctions* f) {
    auto program = m_shader_program->programId();

    f->glUniform1i(f->glGetUniformLocation(program, "stretchType"), int32_t(eStretchType));
    f->glUniform2f(f->glGetUniformLocation(program, "stretchPara"),
        stretchpara_K, stretchpara_C);
}

void CyMediaDisViewBckDraw::showGlslErro(QString tiltle, QString txt) {
    if (m_message_glslError->isVisible()) return;
    m_message_glslError->setWindowTitle(tiltle);
    m_message_glslError->setText(txt);
    m_message_glslError->show();
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
    QOpenGLFunctions* f = ctx->functions(); if (!f) return false;
    f->initializeOpenGLFunctions();

    //当前后台(空闲)纹理
    int backIdx = 1 - m_texture_front_Index.load();
    oneTexturePara* pTextureInfo = &m_textureInfo[backIdx];

    int newWidth = info.width;
    int newHeight = info.height;

    f->glBindTexture(GL_TEXTURE_2D, pTextureInfo->glTexture->textureId());
    // 尺寸变化 更新顶点
    pTextureInfo->imag_sizeChange = (newWidth != pTextureInfo->showInfo.width || newHeight != pTextureInfo->showInfo.height);
    // 信息变化 更新纹理参数
    if (pTextureInfo->imag_sizeChange) pTextureInfo->imag_infoChange = true;
    else pTextureInfo->imag_infoChange = memcmp(&pTextureInfo->showInfo, &info, sizeof(CyMedia::ImageShowInfo));
    //信息变化重新分配尺寸
    if (pTextureInfo->imag_infoChange) {
        memcpy(&pTextureInfo->showInfo, &info, sizeof(CyMedia::ImageShowInfo));
        updateTextureFormat(pTextureInfo->showInfo, backIdx);
        // alloca
        f->glTexImage2D(GL_TEXTURE_2D, 0, pTextureInfo->textureInternalFormat,
            pTextureInfo->showInfo.width * pTextureInfo->textureWidthMultiplier, pTextureInfo->showInfo.height * pTextureInfo->textureHeightMultiplier,
            0, pTextureInfo->textureFormat, pTextureInfo->textureType, nullptr);
        // Mipmap
        f->glGenerateMipmap(GL_TEXTURE_2D);
    }
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
        pTextureInfo->showInfo.width * pTextureInfo->textureWidthMultiplier, pTextureInfo->showInfo.height * pTextureInfo->textureHeightMultiplier,
        pTextureInfo->textureFormat, pTextureInfo->textureType, data);
    f->glFlush();

    f->glBindTexture(GL_TEXTURE_2D, 0);
    ctx->doneCurrent();
    //标记当前显示图像状态
    m_haveImage = true;
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