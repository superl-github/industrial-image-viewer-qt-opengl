//**********
// Author       : llf
// Project      : CyDisplay
// Date / Time  : 29 January 2026
// Department   : host computer
// ClassModeule : Used to display camera images
//**********

#pragma once
#include "../CyMediaBaseDef.h"

#include <QCoreApplication>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QPainter>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLWidget>
#include <QMutex>
#include <QTimer>
#include <QThread>
#include <QElapsedTimer>

class CyDMediaDisBack : public QGraphicsObject {
    Q_OBJECT

public:
    CyDMediaDisBack(QGraphicsView* view, bool useOpenGL = true, QGraphicsItem* parent = nullptr);
    ~CyDMediaDisBack();

public:
    uint32_t id_;

    virtual int type() const override;

    void setPrintLog(bool flag);
    void setLogCallback(CyMedia::LogCallback cb, void* pUser = nullptr);

    // 图像操作
    bool upImageAvailable();
    bool upImageData(CyMedia::ImageShowInfo info, uint8_t* data);
    void clearImage();

    // 拉伸设置
    CyMedia::StretchType stretchType();
    void setStretchType(CyMedia::StretchType type);
    void setStreaChPara(uint32_t start = 0, uint32_t end = 0, uint32_t max = 0);

    // Bayer
    CyMedia::DemosaicingMethod Demosaic();
    void setDemosaic(CyMedia::DemosaicingMethod method);

    // FPS 相关
    double flushFps() const;
    bool isTrueDataFps() const;
    void setTrueDataFps(bool flag);

    // 颜色映射
    QStringList ColorMapList() const;
    quint32 colorMapIndex() const;
    bool setColorMap(quint32 index);
    bool setColorMap(const QString& mapName);

    //温度计算
    bool enableTempeMeasure();
    void setUseTempeMeasure(bool use);
    void getTempMeasurePara(std::vector<double>& poly, int& maxTempe, int& minTempe, double& backGroundColor, double& enteremissivity, double& AtmosphericTransmittance);
    void setTempMeasurePara(const std::vector<double>& poly, int maxTempe, int minTempe, double backGroundColor = 0, double enteremissivity = 1.0, double AtmosphericTransmittance = 1.0);

private:
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    // OpenGL 相关
    void clearGL();
    void initColorMap();
    void setupShaderUniforms(QOpenGLFunctions* f, QMatrix4x4 mat);
    void updateStretchUniforms(QOpenGLFunctions* f);
    void initglsl(QOpenGLFunctions* f);
    void initTexture(QOpenGLFunctions* f);
    void upTexture(QOpenGLFunctions* f);
    void upColorMapTexture(QOpenGLFunctions* f);

    // 内部辅助函数
    void updateTextureFormat();
    void allocateBuffers(size_t size);

private:
    class PrivateData;
    PrivateData* d;
};