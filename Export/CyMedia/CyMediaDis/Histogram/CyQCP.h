#pragma once
#include "qcustomplot.h"
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <atomic>

class CyHistogram : public QCPBars {
    Q_OBJECT

public:
    explicit CyHistogram(QCPAxis* keyAxis, QCPAxis* valueAxis);

public:
    // 外部调用，线程安全
    void updateHistogramFromThread(const double* values, int size);
    
    // 采样设置
    void setSamplingEnabled(bool enabled);
    bool isSamplingEnabled() const;
    void setSamplingThreshold(int threshold);
    int samplingThreshold() const;

public slots:
    void processPendingSwap();

protected:
    // 绘制相关
    virtual void draw(QCPPainter* painter) override;
    virtual double selectTest(const QPointF& pos, bool onlySelectable, QVariant* details = nullptr) const override;

private:
    void initializeHistogramUnsafe(int binCount);
    void drawSampledBars(QCPPainter* painter, bool sampling = true) const;
    QLineF keyToBarLine(double key, double value) const;

private:
    // 数据
    QMutex m_dataMutex;
    QVector<double> m_keys;          // 固定键值
    QVector<double> m_valuesFront;   // GUI 线程读取
    QVector<double> m_valuesBack;    // 后台写入缓冲区

    std::atomic<bool> m_swapRequested{ false };
    std::atomic<bool> m_updatePending{ false };   // 防止重复投递事件

    // 采样参数
    bool m_samplingEnabled = true;
    int m_samplingThreshold = 500;

    // 记录上次尺寸，用于快速检测变化
    int m_expectedSize = 0;
};


class CyLineChart : public QCPGraph {
    Q_OBJECT

public:
    CyLineChart(QCPAxis* keyAxis, QCPAxis* valueAxis);

public:
    void setGraphData(QVector<QCPGraphData> data, bool alreadySorted = false);

protected:
    virtual double selectTest(const QPointF& pos, bool onlySelectable, QVariant* details =nullptr ) const override;
    virtual void draw(QCPPainter* painter) override;

private:
    QMutex m_Mute;
    std::atomic<bool> m_dataChanged{ false };
};