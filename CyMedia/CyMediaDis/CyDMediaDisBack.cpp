#include "CyDMediaDisBack.h"

#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <chrono>

#include "CyMediaDisView.h"
#include <QVector>

struct VertexData {
    QVector3D position;
    QVector2D texCoord;
};
VertexData verticesArray[4];
GLushort indicesArray[6];

class CyDMediaDisBack::PrivateData {
public:
    void log_printf(CyMedia::LogLevel level, const char* fmt, ...);

    bool loadDataToGLBuffer(uint8_t* data, CyMedia::ImageShowInfo& info, QOpenGLFunctions* f);

public:
    bool mUseOpenGL = true;
    QMutex dataLock;

    bool m_bIsPrintDebug = false;
    CyMedia::LogCallback m_logCallback = nullptr;
    void* m_logCallback_user = nullptr;

    QGraphicsView* m_view = nullptr;

    QMessageBox* vertexErrorMessage = nullptr;
    QMessageBox* fragmentErrorMessage = nullptr;
    QMessageBox* programErrorMessage = nullptr;

    // OpenGL 资源
    QOpenGLShaderProgram* shaderProgram = nullptr;
    QOpenGLVertexArrayObject* pVAO = nullptr;
    QOpenGLBuffer* pVBO = nullptr;
    QOpenGLBuffer* pEBO = nullptr;
    QOpenGLBuffer* texBuffer = nullptr;
    QOpenGLTexture* pTexture = nullptr;
    QOpenGLTexture* pTexture_ColorMap = nullptr;
    QOpenGLContext* m_lastctx = nullptr;

    // 图像数据
    QImage mShowImage;
    QImage mOpeImage;
    CyMedia::ImageShowInfo imageDataInfo;
    std::vector<uint8_t> imageData;
    float maxBitlColor = 256;
    float maxPixelColor = 255;

    // 显示参数
    CyMedia::StretchType eStretchType = CyMedia::stretch_None;
    bool upStretchValue = false;
    bool bShowBayerSource = false;
    CyMedia::DemosaicingMethod mDemosaicMethod = CyMedia::DEMOSAIC_BILINEAR;
    bool bIsOVerSize = false;
    CyMedia::ePixType glslNcolor = CyMedia::MONO;

    // 拉伸参数
    int32_t stretchpara_start;
    int32_t stretchpara_end;
    int32_t stretchpara_max;
    float stretchpara_K;
    float stretchpara_C;

    // 色彩映射
    QString m_ColorMapDirPath;
    QStringList m_ColorMapList;
    quint32 m_ColorMapIndex = 0;
    quint8* m_ColorMapData = nullptr;
    qint32 m_ColorMapFileSize = 768;
    qint32 m_ColorMapData_Width = 0;
    qint32 m_ColorMapData_Height = 0;
    bool bColorMapChange = false;

    //温度计算
    bool useTempeMeasure = false;
    std::vector<double> m_tempeMeasure_poly;
    int m_tempeMeasure_poly_degree = 0;
    int m_tempeMeasure_MaxTemp = 100;
    int m_tempeMeasure_MinTemp = 0;
    double m_tempeMeasure_BackGray = 0;
    double m_tempeMeasure_enteremissivity = 0.0;
    double m_tempeMeasure_AtmosphericTransmittance = 0.0;
    bool m_tempeMeasureParaChange = false;

    // 状态标记
    bool bIsFirstUpData = true;
    bool m_shaderInitalize = false;
    bool bTextureInitialize = false;
    bool bImageDataChange = false;

    // 纹理相关
    GLenum textureInternalFormat = GL_LUMINANCE8;   // 纹理内部存储格式
    GLenum textureFormat = GL_RED;                  // 纹理传入数据的图像格式
    GLenum textureType = GL_UNSIGNED_BYTE;          // 纹理传入数据纹理格式
    float textureWidthMultiplier;
    float textureHeightMultiplier;

    // FPS 计算
    QTimer* DisFpsTimer = nullptr;
    QElapsedTimer DisFpsETimer;
    uint64_t DisFpsCount = 0;
    uint64_t showFpsCount = 0;
    double DisFps = 0.0;
    bool useTrueDataFps = true;

    // 着色器源码
    QString vertexShaderSource;
    QString fragmentShaderSource;
};

bool CyDMediaDisBack::PrivateData::loadDataToGLBuffer(uint8_t* data, CyMedia::ImageShowInfo& info, QOpenGLFunctions* f) {
    texBuffer->bind();
    //映射显存
    void* gpuPtr = texBuffer->map(QOpenGLBuffer::WriteOnly);
    if (bIsFirstUpData) {
        if (gpuPtr) {
            // 将数据复制到GPU内存
            memcpy(gpuPtr, data, info.length);
            texBuffer->unmap();

            // 从PBO上传纹理数据
            f->glTexImage2D(GL_TEXTURE_2D, 0,
                textureInternalFormat,
                info.width * textureWidthMultiplier,
                info.height * textureHeightMultiplier,
                0,
                textureFormat, textureType,
                0);
        }
        else {
            // 如果映射失败，直接上传
            f->glTexImage2D(GL_TEXTURE_2D, 0,
                textureInternalFormat,
                info.width * textureWidthMultiplier,
                info.height * textureHeightMultiplier,
                0,
                textureFormat, textureType,
                data);

        }
        bIsFirstUpData = false;
    }
    else {
        if (gpuPtr) {
            // 将数据复制到GPU内存
            memcpy(gpuPtr, data, info.length);
            texBuffer->unmap();

            // 从PBO上传纹理数据
            f->glTexSubImage2D(GL_TEXTURE_2D, 0,
                0,0,
                info.width * textureWidthMultiplier,
                info.height * textureHeightMultiplier,
                textureFormat, textureType,
                0);
        }
        else {
            // 如果映射失败，直接上传
            f->glTexSubImage2D(GL_TEXTURE_2D, 0,
                0, 0,
                info.width * textureWidthMultiplier,
                info.height * textureHeightMultiplier,
                textureFormat, textureType,
                data);

        }
    }

    texBuffer->release();
    
    return true;
}

void CyDMediaDisBack::PrivateData::log_printf(CyMedia::LogLevel level, const char* fmt, ...) {
    if (!m_logCallback || !m_bIsPrintDebug) {
        return;
    }

    // 标准 C 可变参数格式化
    char buffer[1024] = { 0 };
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap); // 格式化到 buffer
    va_end(ap);

    // 转给回调（纯 std::string）
    m_logCallback(level, QString("CyDMediaDisBack[%1]:%2").arg(__LINE__).arg(QString::fromUtf8(buffer)).toStdString(), m_logCallback_user);
}


CyDMediaDisBack::CyDMediaDisBack(QGraphicsView* view, bool useOpenGL/* = true*/, QGraphicsItem* parent/* = nullptr*/)
    : QGraphicsObject(parent) {
    this->setFlag(QGraphicsItem::ItemIsSelectable, false);
    this->setFlag(QGraphicsItem::ItemIsMovable, false);
    this->setFlag(QGraphicsItem::ItemIsFocusable, false);

    d = new CyDMediaDisBack::PrivateData;
    d->m_view = view;
    d->mUseOpenGL = useOpenGL;

    // 初始化消息框
    d->vertexErrorMessage = new QMessageBox(
        QMessageBox::Warning,
        QString("vertex shader"),
        QString(""),
        QMessageBox::Ok
    );
    d->fragmentErrorMessage = new QMessageBox(
        QMessageBox::Warning,
        QString("fragment shader"),
        QString(""),
        QMessageBox::Ok
    );
    d->programErrorMessage = new QMessageBox(
        QMessageBox::Warning,
        QString("Link Program"),
        QString(""),
        QMessageBox::Ok
    );

    // 初始化默认值
    d->imageDataInfo.bit = 8;
    d->imageDataInfo.format = CyMedia::MONO;

    // 初始化颜色映射
    initColorMap();

    // 设置 FPS 计时器
    d->DisFpsTimer = new QTimer(this);
    d->DisFpsTimer->setTimerType(Qt::PreciseTimer);
    connect(d->DisFpsTimer, &QTimer::timeout, this, [this]() {
        if (d->DisFpsETimer.isValid()) {
            double totalFrames = d->useTrueDataFps ?
                d->DisFpsCount : (d->DisFpsCount + d->showFpsCount);
            d->DisFps = (totalFrames * 1000.0) / d->DisFpsETimer.elapsed();
            d->DisFpsCount = d->showFpsCount = 0;
            d->DisFpsETimer.restart();
        }
        else {
            d->DisFpsETimer.start();
        }
        });
    d->DisFpsTimer->start(1000);
}

CyDMediaDisBack::~CyDMediaDisBack()
{
    if (d) {
        clearGL();
        delete[] d->m_ColorMapData;

        delete d;
    }
}

int CyDMediaDisBack::type() const
{
    return UserType + 1;
}

void CyDMediaDisBack::setPrintLog(bool flag) {
    d->m_bIsPrintDebug = flag;
}

void CyDMediaDisBack::setLogCallback(CyMedia::LogCallback cb, void* pUser /*= nullptr*/) {
    d->m_logCallback = std::move(cb);
    d->m_logCallback_user = pUser;
}

bool CyDMediaDisBack::upImageAvailable()
{
    if (true == d->bImageDataChange && false == isVisible()) {
        d->bImageDataChange = true;
    }
    return (false == d->bImageDataChange);
}

void CyDMediaDisBack::updateTextureFormat()
{
    d->textureWidthMultiplier = 1.0;
    d->textureHeightMultiplier = 1.0;
    // 更新纹理格式
    switch (d->imageDataInfo.format) {
        case CyMedia::MONO_OVERSIZE: {
            if (d->imageDataInfo.bit <= 8) {
                d->textureInternalFormat = GL_RGBA;
                d->textureWidthMultiplier = 0.5;
                d->textureHeightMultiplier = 0.5;
                d->textureFormat = GL_RGBA;
                d->textureType = GL_UNSIGNED_BYTE;
            }
            else if (d->imageDataInfo.bit <= 16) {
                d->textureInternalFormat = GL_RGBA;
                d->textureFormat = GL_RGBA;
                d->textureType = GL_UNSIGNED_SHORT;
            }
            else if (d->imageDataInfo.bit < 32) {
                d->textureInternalFormat = GL_R32F;
                d->textureFormat = GL_RED;
                d->textureType = GL_UNSIGNED_INT;
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
            if (d->imageDataInfo.bit <= 8) {
                d->textureInternalFormat = GL_R8;
                d->textureFormat = GL_RED;
                d->textureType = GL_UNSIGNED_BYTE;
            }
            else if (d->imageDataInfo.bit <= 16) {
                d->textureInternalFormat = GL_R16;
                d->textureFormat = GL_RED;
                d->textureType = GL_UNSIGNED_SHORT;
            }
            else {
                d->textureInternalFormat = GL_R32F;
                d->textureFormat = GL_RED;
                d->textureType = GL_UNSIGNED_INT;
            }
        }break;

        case CyMedia::RGB: {
            if (d->imageDataInfo.bit <= 8) {
                d->textureInternalFormat = GL_RGB;
                d->textureFormat = GL_RGB;
                d->textureType = GL_UNSIGNED_BYTE;
            }
            else if (d->imageDataInfo.bit <= 16) {
                d->textureInternalFormat = GL_RGB;
                d->textureFormat = GL_RGB;
                d->textureType = GL_UNSIGNED_SHORT;
            }
            else {
                d->textureInternalFormat = GL_RGB;
                d->textureFormat = GL_RGB;
                d->textureType = GL_UNSIGNED_INT;
            }
        }break;

        case CyMedia::RGBA: {
            if (d->imageDataInfo.bit <= 8) {
                d->textureInternalFormat = GL_RGBA;
                d->textureFormat = GL_RGBA;
                d->textureType = GL_UNSIGNED_BYTE;
            }
            else if (d->imageDataInfo.bit <= 16) {
                d->textureInternalFormat = GL_RGBA;
                d->textureFormat = GL_RGBA;
                d->textureType = GL_UNSIGNED_SHORT;
            }
            else {
                d->textureInternalFormat = GL_RGBA;
                d->textureFormat = GL_RGBA;
                d->textureType = GL_UNSIGNED_INT;
            }
        }break;
    }
}

bool CyDMediaDisBack::upImageData(CyMedia::ImageShowInfo info, uint8_t* image) {
    if (!image) return false;

    // 快速检查是否有数据正在处理
    if (d->bImageDataChange) {
        // 可以跳过这一帧或返回false
        return false;
    }

    QMutexLocker locker(&d->dataLock);
    // 分配缓冲区
    allocateBuffers(info.length);
    //拷贝数据
    memcpy(d->imageData.data(), image, d->imageData.size());

    // 检查图像信息是否变化
    if (memcmp(&d->imageDataInfo, &info, sizeof(info))) {
        prepareGeometryChange();
        memcpy(&d->imageDataInfo, &info, sizeof(info));
        d->glslNcolor = d->imageDataInfo.format;
        d->maxBitlColor = (1U << d->imageDataInfo.bit);
        d->maxPixelColor = d->maxBitlColor - 1;
        
        // 更新纹理格式
        updateTextureFormat();

        d->bTextureInitialize = false;
        d->bIsFirstUpData = true;
    }

    // 标记数据已改变
    d->bImageDataChange = true;
    update();
    return true;
}

void CyDMediaDisBack::clearImage()
{
    QMutexLocker lock(&d->dataLock);
    // 清除纹理数据
    if (d->texBuffer) {
        d->texBuffer->release();
    }

    // 删除内存
    d->imageData.clear();
    d->imageData.shrink_to_fit();

    update();

    setVisible(false);
    d->bIsFirstUpData = true;
    d->upStretchValue = false;
    d->bImageDataChange = false;
}

CyMedia::StretchType CyDMediaDisBack::stretchType() {
    return d->eStretchType;
}

void CyDMediaDisBack::setStretchType(CyMedia::StretchType type) {
    if (d->eStretchType == type)
        return;
    d->eStretchType = type;
    d->upStretchValue = true;
}

void CyDMediaDisBack::setStreaChPara(uint32_t start /*= 0*/, uint32_t end /*= 0*/, uint32_t max/* = 0*/) {
    if (d->stretchpara_start == start && 
        d->stretchpara_end == end &&
        d->stretchpara_max == max)
        return;

    d->stretchpara_start = start;
    d->stretchpara_end = end;
    d->stretchpara_max = max;

    double ds = static_cast<double>(start) / d->stretchpara_max;
    double de = static_cast<double>(end) / d->stretchpara_max;
    d->stretchpara_K = 1.0 / (de - ds);
    d->stretchpara_C = -(d->stretchpara_K) * ds;

    d->upStretchValue = true;
}

CyMedia::DemosaicingMethod CyDMediaDisBack::Demosaic() {
    return d->mDemosaicMethod;
}

void CyDMediaDisBack::setDemosaic(CyMedia::DemosaicingMethod method) {
    if (d->mDemosaicMethod == method)
        return;
    d->mDemosaicMethod = method;
}

double CyDMediaDisBack::flushFps() const
{
    return d->DisFps;
}

bool CyDMediaDisBack::isTrueDataFps() const
{
    return d->useTrueDataFps;
}

void CyDMediaDisBack::setTrueDataFps(bool flag)
{
    d->useTrueDataFps = flag;
}

QStringList CyDMediaDisBack::ColorMapList() const
{
    return d->m_ColorMapList;
}

quint32 CyDMediaDisBack::colorMapIndex() const
{
    return d->m_ColorMapIndex;
}

bool CyDMediaDisBack::setColorMap(quint32 index)
{
    if (index == d->m_ColorMapIndex)
        return true;
    if (index >= d->m_ColorMapList.size())
        return false;
    //除默认None外读取CM文件
    if (index > 0) {
        QString transCMFileName = QString("%1\\%2.cm").arg(d->m_ColorMapDirPath).arg(d->m_ColorMapList[index]);
        QFile cmFile(transCMFileName);
        if (false == cmFile.open(QIODevice::ReadOnly))
            return false;
        auto readCode = cmFile.readAll();
        cmFile.close();
        memcpy(d->m_ColorMapData, readCode.data(), d->m_ColorMapFileSize);
    }
    d->m_ColorMapIndex = index;
    d->bColorMapChange = true;
    return true;
}

bool CyDMediaDisBack::setColorMap(const QString& mapName)
{
    if (d->m_ColorMapList.contains(mapName))
        return false;
    auto index = d->m_ColorMapList.indexOf(mapName);
    if (index == -1)
        return false;
    return setColorMap(index);
}

bool CyDMediaDisBack::enableTempeMeasure() {
    return d->useTempeMeasure;
}

void CyDMediaDisBack::setUseTempeMeasure(bool use) {
    if (d->useTempeMeasure == use) return;
    d->useTempeMeasure = use;
}

void CyDMediaDisBack::getTempMeasurePara(std::vector<double>& poly, int& maxTempe, int& minTempe, double& backGroundColor, double& enteremissivity, double& AtmosphericTransmittance) {
    poly = d->m_tempeMeasure_poly;
    maxTempe = d->m_tempeMeasure_MaxTemp;
    minTempe = d->m_tempeMeasure_MinTemp;
    backGroundColor = d->m_tempeMeasure_BackGray;

    enteremissivity = d->m_tempeMeasure_enteremissivity;
    AtmosphericTransmittance = d->m_tempeMeasure_AtmosphericTransmittance;
}

void CyDMediaDisBack::setTempMeasurePara(const std::vector<double>& poly, int maxTempe, int minTempe, double backGroundColor/* = 0*/, double enteremissivity/* = 1.0*/, double AtmosphericTransmittance/* = 1.0*/) {
    bool setParaChange = 
        (maxTempe != d->m_tempeMeasure_MaxTemp) || 
        (minTempe != d->m_tempeMeasure_MinTemp) || 
        (backGroundColor != d->m_tempeMeasure_BackGray) ||
        (enteremissivity != d->m_tempeMeasure_enteremissivity) ||
        (AtmosphericTransmittance != d->m_tempeMeasure_AtmosphericTransmittance);
    int comparaSize = std::min(int(poly.size()), 10);
    if (false == setParaChange) {
        setParaChange = !std::equal(poly.begin(), poly.begin() + comparaSize, d->m_tempeMeasure_poly.begin());
    }
    if (setParaChange == false) return;

    d->m_tempeMeasure_MaxTemp = maxTempe;
    d->m_tempeMeasure_MinTemp = minTempe;
    d->m_tempeMeasure_BackGray = backGroundColor;

    d->m_tempeMeasure_enteremissivity = enteremissivity;
    d->m_tempeMeasure_AtmosphericTransmittance = AtmosphericTransmittance;

    d->m_tempeMeasure_poly.assign(10, 0.0);
    std::copy_n(poly.begin(), comparaSize, d->m_tempeMeasure_poly.begin());
    d->m_tempeMeasure_poly_degree = comparaSize - 1;

    d->m_tempeMeasureParaChange = true;
}

QRectF CyDMediaDisBack::boundingRect() const
{
    return QRectF(0, 0, d->imageDataInfo.width, d->imageDataInfo.height);
}

void CyDMediaDisBack::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    //绘制QImage
    if (false == d->mUseOpenGL) {
        if (d->bImageDataChange) {
            QMutexLocker lock(&d->dataLock);
            d->mShowImage = d->mOpeImage.copy();
            d->bImageDataChange = false;
        }
        painter->drawImage(boundingRect(), d->mShowImage);
        return;
    }

    //OPenGL处理
    QOpenGLWidget* glwidget = qobject_cast<QOpenGLWidget*>(widget);
    if (!glwidget) {
        d->showFpsCount++;
        return;
    }

    QOpenGLContext* ctx = glwidget->context();
    if (!ctx) {
        return;
    }

    // 检查上下文是否变化
    if (d->m_lastctx != ctx) {
        clearGL();
        d->m_lastctx = ctx;
    }

    QOpenGLFunctions* f = ctx->functions();
    if (!f) {
        return;
    }
    
    painter->beginNativePainting();
    
    // 初始化着色器
    initglsl(f);

    // 更新纹理
    upTexture(f);
    upColorMapTexture(f);

    // 设置着色器参数
    f->glUseProgram(d->shaderProgram->programId());
    QMatrix4x4 projectionMatrix;
    projectionMatrix.ortho(0.0f, glwidget->width(), glwidget->height(), 0.0f, -1000.0f, 1000.0f);
    setupShaderUniforms(f, projectionMatrix * painter->worldTransform());

    // 绘制
    QOpenGLVertexArrayObject::Binder vaobinder(d->pVAO);
    f->glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

    // 解绑
    f->glBindTexture(GL_TEXTURE_2D, 0);
    vaobinder.release();
    d->shaderProgram->release();
    d->pVBO->release();
    d->pEBO->release();
    painter->endNativePainting();
}

// 设置着色器uniform
void CyDMediaDisBack::setupShaderUniforms(QOpenGLFunctions* f, QMatrix4x4 mat)
{
    auto program = d->shaderProgram->programId();

    // 绑定纹理
    f->glUniform1i(f->glGetUniformLocation(program, "texture"), 0);
    f->glUniform1i(f->glGetUniformLocation(program, "texture_ColorMap"), 1);

    // 设置矩阵
    d->shaderProgram->setUniformValue("m_matrix", mat);
    d->shaderProgram->setUniformValue("zoomValue", qobject_cast<CyMediaDisView*>(d->m_view)->zoomValue());

    // 设置图像参数
    if (d->bShowBayerSource && d->glslNcolor >= CyMedia::BAYERRG && d->glslNcolor <= CyMedia::BAYERGB) {
        d->shaderProgram->setUniformValue("colorType", int(CyMedia::MONO));
    }
    else {
        d->shaderProgram->setUniformValue("colorType", int(d->glslNcolor));
    }
    d->shaderProgram->setUniformValue("nbits", d->imageDataInfo.bit);
    d->shaderProgram->setUniformValue("nWidth", d->imageDataInfo.width);
    d->shaderProgram->setUniformValue("nHeight", d->imageDataInfo.height);
    d->shaderProgram->setUniformValue("pixrange", d->maxBitlColor);

    // 温度计算
    d->shaderProgram->setUniformValue("useTempeMeasure", d->useTempeMeasure ? 1 : 0);
    if (d->useTempeMeasure && d->m_tempeMeasureParaChange) {
        d->m_tempeMeasureParaChange = false;
        d->shaderProgram->setUniformValue("tempeMeasure_MaxTempe", float(d->m_tempeMeasure_MaxTemp));
        d->shaderProgram->setUniformValue("tempeMeasure_MinTempe", float(d->m_tempeMeasure_MinTemp));
        d->shaderProgram->setUniformValue("tempeMeasure_background", float(d->m_tempeMeasure_BackGray));

        d->shaderProgram->setUniformValue("tempeMeasure_enteremissivity", float(d->m_tempeMeasure_enteremissivity));
        d->shaderProgram->setUniformValue("tempeMeasure_AtmosphericTransmittance", float(d->m_tempeMeasure_AtmosphericTransmittance));
        GLint loc = f->glGetUniformLocation(program, "tempeMeasure_Poly");
        if (loc != -1) {
            std::vector<float> temp(d->m_tempeMeasure_poly.begin(), d->m_tempeMeasure_poly.end());
            f->glUniform1fv(loc, temp.size(), temp.data());
        }
        d->shaderProgram->setUniformValue("tempeMeasure_Poly_degree", d->m_tempeMeasure_poly_degree);
    }

    // 设置颜色类型
    CyMedia::ePixType colorType = d->bShowBayerSource ?
        CyMedia::MONO : d->imageDataInfo.format;
    d->shaderProgram->setUniformValue("colorType", int(colorType));

    // 设置颜色映射
    d->shaderProgram->setUniformValue("colorMapIndex", d->m_ColorMapIndex);

    // 设置拉伸参数
    if (d->upStretchValue) {
        updateStretchUniforms(f);
        d->upStretchValue = false;
    }
}

// 更新拉伸uniform
void CyDMediaDisBack::updateStretchUniforms(QOpenGLFunctions* f) {
    auto program = d->shaderProgram->programId();

    f->glUniform1i(f->glGetUniformLocation(program, "stretchType"), int32_t(d->eStretchType));
    f->glUniform2f(f->glGetUniformLocation(program, "stretchPara"),
        d->stretchpara_K, d->stretchpara_C);
}

void CyDMediaDisBack::initglsl(QOpenGLFunctions* f)
{
    if (false == d->m_shaderInitalize) {
        //加载GLSL
        if (d->vertexShaderSource.isEmpty()) {
            QFile vertexFile(":/CyMediaDis/glslSource/Vertex.c");
            if (vertexFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream stream(&vertexFile);
                stream.setCodec("UTF-8");
                d->vertexShaderSource = stream.readAll();
                vertexFile.close();
            }
        }
#ifdef DEBUG_
        //从文件加载片元着色器
        if (d->fragmentShaderSource.isEmpty()) {

            QFile fragementFile(QDir::currentPath() + "/SapViewer_Fragement.c");
            if (fragementFile.open(QIODevice::ReadOnly)) {
                auto readCode = fragementFile.readAll();
                d->fragmentShaderSource = QString(readCode);
                fragementFile.close();
            }

        }
#endif
        if (d->fragmentShaderSource.isEmpty()) {
            QFile fragmentFile(":/CyMediaDis/glslSource/Fragment.c");
            if (fragmentFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream stream(&fragmentFile);
                stream.setCodec("UTF-8");
                d->fragmentShaderSource = stream.readAll();
                fragmentFile.close();
            }
        }


        //着色器程序初始化
        d->shaderProgram = new QOpenGLShaderProgram;
        d->shaderProgram->create();
        f->glUseProgram(d->shaderProgram->programId());
        if (false == d->shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, d->vertexShaderSource)) {
            d->vertexErrorMessage->setText(d->shaderProgram->log());
            d->log_printf(CyMedia::LogLevel::ERR, "%s\r\n", d->shaderProgram->log().toLatin1().data());
            d->vertexErrorMessage->setVisible(true);
            return;
        }

        if (false == d->shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, d->fragmentShaderSource)) {
            d->fragmentErrorMessage->setText(d->shaderProgram->log());
            d->fragmentErrorMessage->setVisible(true);
            return;
        }

        if (false == d->shaderProgram->link()) {
            d->programErrorMessage->setText(d->shaderProgram->log());
            d->programErrorMessage->setVisible(true);
            return;
        }
        f->glUniform1i(f->glGetUniformLocation(d->shaderProgram->programId(), "texture"), 0);
        f->glUniform1i(f->glGetUniformLocation(d->shaderProgram->programId(), "texture_ColorMap"), 1);

        //禁用驱动优化
        f->glDisable(GL_TEXTURE_RECTANGLE);
        f->glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

        d->m_shaderInitalize = true;
    }
}

void CyDMediaDisBack::initTexture(QOpenGLFunctions* f)
{
    if (false == d->bTextureInitialize) {
        //顶点初始化
        if (d->bIsOVerSize) {
            verticesArray[0] = { QVector3D(0,0,0.0f),  QVector2D(0.0f, 0.0f) };  // v0
            verticesArray[1] = { QVector3D(d->imageDataInfo.width,0,5.0f),  QVector2D(2.0f, 0.0f) };  // v1
            verticesArray[2] = { QVector3D(0,d->imageDataInfo.width, 5.0f),  QVector2D(0.0f, 2.0f) };  // v2
            verticesArray[3] = { QVector3D(d->imageDataInfo.width,  d->imageDataInfo.height, 5.0f),QVector2D(2.0f, 2.0f) };  // v3
        }
        else {
            verticesArray[0] = { QVector3D(0,0,0.0f),  QVector2D(0.0f, 0.0f) };  // v0
            verticesArray[1] = { QVector3D(d->imageDataInfo.width,0,5.0f),  QVector2D(1.0f, 0.0f) };  // v1
            verticesArray[2] = { QVector3D(0,d->imageDataInfo.height, 5.0f),  QVector2D(0.0f, 1.0f) };  // v2
            verticesArray[3] = { QVector3D(d->imageDataInfo.width,  d->imageDataInfo.height, 5.0f),QVector2D(1.0f, 1.0f) };  // v3
        }
        GLushort t_indices[6] = {
            0, 1, 2,
            3, 2, 1
        };
        memcpy(indicesArray, t_indices, sizeof(t_indices));

        // Create VAB, VBO, EBO
        d->pVAO = new QOpenGLVertexArrayObject;
        d->pVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        d->pEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        d->pVAO->create();
        d->pVBO->create();
        d->pEBO->create();

        QOpenGLVertexArrayObject::Binder vaobinder(d->pVAO);
        // Binding VBO
        d->pVBO->bind();
        d->pVBO->allocate(verticesArray, sizeof(VertexData) * 4);
        // Binding EBO
        d->pEBO->bind();
        d->pEBO->allocate(indicesArray, sizeof(GLushort) * 4);
        // set data properties
        int attr = -1;
        attr = d->shaderProgram->attributeLocation("apos");
        d->shaderProgram->enableAttributeArray(attr);
        d->shaderProgram->setAttributeBuffer(attr, GL_FLOAT, 0, 3, sizeof(VertexData));
        attr = d->shaderProgram->attributeLocation("atexcoord");
        d->shaderProgram->enableAttributeArray(attr);
        d->shaderProgram->setAttributeBuffer(attr, GL_FLOAT, sizeof(QVector3D), 2, sizeof(VertexData));
        vaobinder.release();
        //p_data->pVBO->release();
        //p_data->pEBO->release();
        
        // Create texture and bind it
        d->pTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        d->pTexture->create();
        f->glActiveTexture(GL_TEXTURE0);// 纹理0默认激活
        f->glBindTexture(GL_TEXTURE_2D, d->pTexture->textureId());
        // Texture filtering method
        d->pTexture->setMagnificationFilter(QOpenGLTexture::Nearest); // GL_NEAREST：临近插值，容易看出像素点
        d->pTexture->setMinificationFilter(QOpenGLTexture::Linear);  // GL_LINEAR：线性插值，平滑模糊
        // Texture wrapping
        d->pTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
        // Create PBO
        d->texBuffer = new QOpenGLBuffer(QOpenGLBuffer::PixelUnpackBuffer);
        d->texBuffer->create();

        //ColorMap纹理初始化
        if (d->pTexture_ColorMap) {
            d->pTexture_ColorMap->release();
            delete d->pTexture_ColorMap;
            d->pTexture_ColorMap = 0;
        }

        d->pTexture_ColorMap = new QOpenGLTexture(QOpenGLTexture::Target2D);
        d->pTexture_ColorMap->create();
        f->glActiveTexture(GL_TEXTURE1);
        f->glBindTexture(GL_TEXTURE_2D, d->pTexture_ColorMap->textureId());
        // Texture filtering method
        d->pTexture_ColorMap->setMagnificationFilter(QOpenGLTexture::Nearest); // GL_NEAREST：临近插值，容易看出像素点
        d->pTexture_ColorMap->setMinificationFilter(QOpenGLTexture::Linear);  // GL_LINEAR：线性插值，平滑模糊
        // Texture wrapping
        d->pTexture_ColorMap->setWrapMode(QOpenGLTexture::ClampToEdge);
        if (d->m_ColorMapData) {
            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, d->m_ColorMapData_Width, d->m_ColorMapData_Height,
                0, GL_RGB, GL_UNSIGNED_BYTE, d->m_ColorMapData);
            f->glGenerateMipmap(GL_TEXTURE_2D);
        }

        d->bTextureInitialize = true;
    }
}

void CyDMediaDisBack::upTexture(QOpenGLFunctions* f)
{
    // 初始化纹理对象
    if (!d->bTextureInitialize) {
        initTexture(f);
        d->texBuffer->bind();
        d->texBuffer->allocate(d->imageData.size());
    }
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, d->pTexture->textureId());

    if (d->bImageDataChange) {
        QElapsedTimer elapTimer;
        elapTimer.start();
        QMutexLocker lock(&d->dataLock);
        if (d->imageData.size()) {
            d->loadDataToGLBuffer(d->imageData.data(), d->imageDataInfo, f);
            d->DisFpsCount++;
        }
        //d->log_printf("更新纹理%lld字节用时%lldms\n", d->imageData.size(), elapTimer.elapsed());
        d->bImageDataChange = false;
    }
    else {
        d->showFpsCount++;
    }
}

void CyDMediaDisBack::upColorMapTexture(QOpenGLFunctions* f)
{
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, d->pTexture_ColorMap->textureId());
    if (true == d->bColorMapChange) {
        f->glTexImage2D(
            GL_TEXTURE_2D, 0, GL_LUMINANCE8, 
            d->m_ColorMapData_Width, 
            d->m_ColorMapData_Height, 
            0, 
            GL_LUMINANCE, GL_UNSIGNED_BYTE, 
            d->m_ColorMapData);
        f->glGenerateMipmap(GL_TEXTURE_2D);
        d->bColorMapChange = false;
    }
}

void CyDMediaDisBack::allocateBuffers(size_t size)
{
    if (d->imageData.size() != size)
        d->imageData.resize(size);
}

// 清理 OpenGL 资源
void CyDMediaDisBack::clearGL()
{
    delete d->shaderProgram;
    delete d->pVBO;
    delete d->pEBO;
    delete d->texBuffer;
    delete d->pTexture;
    delete d->pTexture_ColorMap;

    d->shaderProgram = nullptr;
    d->pVBO = nullptr;
    d->pEBO = nullptr;
    d->texBuffer = nullptr;
    d->pTexture = nullptr;
    d->pTexture_ColorMap = nullptr;

    d->m_shaderInitalize = false;
    d->bTextureInitialize = false;
}

void CyDMediaDisBack::initColorMap()
{
    //初始化ColorMap内存
    if (!d->m_ColorMapData) {
        d->m_ColorMapData = new quint8[d->m_ColorMapFileSize];
    }
    //添加默认
    d->m_ColorMapList.clear();
    d->m_ColorMapList.push_back("None");

    //筛选符合条件文件
    QStringList CMfullPathList;
    QString colorMapDirName = qApp->applicationDirPath() + QString("/colorMap/");
    QDir colorMapDir(colorMapDirName);
    if (colorMapDir.exists()) {
        d->m_ColorMapDirPath = colorMapDirName;
        colorMapDir.setFilter(QDir::Files);
        auto cmFileList = colorMapDir.entryInfoList();
        for (auto oneInfo : cmFileList) {
            if (oneInfo.size() == d->m_ColorMapFileSize && oneInfo.suffix() == "cm") {
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
                d->m_ColorMapList.push_back(QFileInfo(oneCMPath).baseName());
                oneCMFile.close();
            }
        }
        d->m_ColorMapData_Width = d->m_ColorMapFileSize / 3;
        d->m_ColorMapData_Height = 3;
    }
    d->m_ColorMapIndex = 0;
}