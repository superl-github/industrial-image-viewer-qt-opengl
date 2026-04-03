#pragma once
#include "qcustomplot.h"
#include <QMutex>
#include <QMutexLocker>

class CyHistogram : public QCPBars {
    Q_OBJECT

public:
    explicit CyHistogram(QCPAxis* keyAxis, QCPAxis* valueAxis);

public:
    void updateHistogramFromThread(const double* values, int size);

    Q_INVOKABLE void processPendingSwap();

    void setSamplingEnabled(bool enabled);
    bool isSamplingEnabled() const;

    void setSamplingThreshold(int threshold);
    int samplingThreshold() const;

private:
    //draw
    QLineF keyToBarLine(double key, double value) const; 
    virtual double selectTest(const QPointF& pos, bool onlySelectable, QVariant* details = nullptr) const override;
    virtual void draw(QCPPainter* painter) override;
    void drawSampledBars(QCPPainter* painter, int interval) const;

    //update
    void initializeHistogramUnsafe(int binCount);
    void markDataChanged();
    void onUpdateTimeout();

private:
    QMutex m_dataMutex;
    //draw
    bool mSamplingEnabled = true;
    int mSamplingThreshold = 500;

    //update
    QVector<double> m_keys;          // 固定横轴（初始化后不变）
    QVector<double> m_valuesFront;   // GUI线程当前使用（用于抽样/渲染）
    QVector<double> m_valuesBack;    // GUI线程待交换缓冲区

    std::atomic<bool> m_swapRequested{ false };
    int m_expectedSize = 0;

    QTimer* m_updateTimer;
    std::atomic<bool> m_dataChanged{ false };
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