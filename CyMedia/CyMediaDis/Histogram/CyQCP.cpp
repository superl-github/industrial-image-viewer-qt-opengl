#include "CyQCP.h"
#include <cmath>
#include <algorithm>

#include <chrono>
#include <QMutex>
#include <QThread>

CyHistogram::CyHistogram(QCPAxis* keyAxis, QCPAxis* valueAxis)
    :QCPBars(keyAxis, valueAxis) {
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, &QTimer::timeout, this, &CyHistogram::onUpdateTimeout);
    setAntialiased(false);
}

void CyHistogram::updateHistogramFromThread(const double* values, int size) {
    if (!values || !m_swapRequested.is_lock_free())
        return;
    {
        QMutexLocker locker(&m_dataMutex);
        // 按需初始化/调整缓冲区
        if (static_cast<int>(m_keys.size()) != size) {
            initializeHistogramUnsafe(size);
        }
        else if (static_cast<int>(m_valuesBack.size()) != size) {
            m_valuesBack.resize(size);
        }

        // 安全复制数据
        std::memcpy(m_valuesBack.data(), values, size * sizeof(double));
        m_swapRequested.store(true, std::memory_order_release);
    }

    if (QThread::currentThread() != thread()) {
        auto linkRe = QMetaObject::invokeMethod(this, "processPendingSwap", Qt::QueuedConnection);
        if (false == linkRe) {
            printf("invokeMethod error\n");
        }
    }
    else {
        // 同线程直接调用（如测试场景）
        processPendingSwap();
    }
}

void CyHistogram::processPendingSwap() {
    QMutexLocker locker(&m_dataMutex);

    if (!m_swapRequested.load(std::memory_order_acquire)) return;

    m_valuesFront.swap(m_valuesBack);
    m_swapRequested.store(false, std::memory_order_release);

    markDataChanged();
}

void CyHistogram::setSamplingEnabled(bool enabled) {
    mSamplingEnabled = enabled;
}

bool CyHistogram::isSamplingEnabled() const {
    return mSamplingEnabled;
}

void CyHistogram::setSamplingThreshold(int threshold) {
    mSamplingThreshold = qMax(1, threshold);
}

int CyHistogram::samplingThreshold() const {
    return mSamplingThreshold;
}

QLineF CyHistogram::keyToBarLine(double key, double value) const {
    double x = mKeyAxis->coordToPixel(key);
    double yBottom = mValueAxis->coordToPixel(0);
    double yTop = mValueAxis->coordToPixel(value);
    return QLineF(x, yBottom, x, yTop);
}

double CyHistogram::selectTest(const QPointF& pos, bool onlySelectable, QVariant* details /*= nullptr*/) const
{
    //QCPBars::selectTest(pos, onlySelectable, details);
    return -1;
}

void CyHistogram::draw(QCPPainter* painter) {
    // 检查轴是否有效
    if (!mKeyAxis || !mValueAxis) {
        QCPBars::draw(painter);
        return;
    }

    // 检查当前可见区域
    double keyMin = mKeyAxis->range().lower;
    double keyMax = mKeyAxis->range().upper;

    // 如果可见范围无效，直接绘制所有柱体
    if (qFuzzyCompare(keyMin, keyMax)) {
        QCPBars::draw(painter);
        return;
    }

    // 计算可见区域内的柱体数量
    double keyRange = keyMax - keyMin;
    double barWidth = width(); // 柱体宽度（在键轴单位中）

    // 如果柱体宽度为0，直接绘制
    if (qFuzzyIsNull(barWidth)) {
        QCPBars::draw(painter);
        return;
    }

    int visibleBars = static_cast<int>(keyRange / barWidth);

    drawSampledBars(painter, 1);
    return;

    // 如果柱体数量超过阈值，进行抽样
    if (mSamplingEnabled && visibleBars > mSamplingThreshold) {
        // 计算抽样间隔
        int interval = qMax(1, data()->size() / mSamplingThreshold);

        // 绘制抽样后的柱体
        drawSampledBars(painter, interval);
    }
    else {
        // 不需要抽样，直接绘制
        QCPBars::draw(painter);
    }
}

void CyHistogram::drawSampledBars(QCPPainter* painter, int interval) const {
    if (!mKeyAxis || !mValueAxis || data()->isEmpty()) return;

    double keyMin = mKeyAxis->range().lower;
    double keyMax = mKeyAxis->range().upper;
    if (keyMin >= keyMax) return;

    int pixelStart = qRound(mKeyAxis->coordToPixel(keyMin));
    int pixelEnd = qRound(mKeyAxis->coordToPixel(keyMax));
    int visiblePixels = qAbs(pixelEnd - pixelStart);
    if (visiblePixels <= 1) return;

    int bucketCount = qMin(visiblePixels, mSamplingThreshold);

    struct Bucket {
        double peakVal = -1e300;
        double repKey = 0;
        bool valid = false;
    };
    QVector<Bucket> buckets(bucketCount);
    double bucketWidth = (keyMax - keyMin) / bucketCount;

    for (int i = 0; i < data()->size(); ++i) {
        double k = data()->at(i)->key;
        double v = data()->at(i)->value;
        if (k < keyMin || k > keyMax || v <= 0) continue;

        int idx = qBound(0, static_cast<int>((k - keyMin) / bucketWidth), bucketCount - 1);
        if (!buckets[idx].valid || v > buckets[idx].peakVal) {
            buckets[idx].peakVal = v;
            buckets[idx].repKey = k;
            buckets[idx].valid = true;
        }
    }

    painter->setPen(mPen);

    for (const auto& bucket : buckets) {
        if (!bucket.valid) continue;
        painter->drawLine(keyToBarLine(bucket.repKey, bucket.peakVal));
    }
}

void CyHistogram::initializeHistogramUnsafe(int binCount) {
    if (binCount <= 0) return;
    if (m_keys.size() == binCount)
        return;
    m_keys.resize(binCount);
    for (int i = 0; i < binCount; ++i)
        m_keys[i] = static_cast<double>(i); // 显式类型转换更安全
    m_expectedSize = binCount;
    m_valuesBack.resize(binCount);
}

void CyHistogram::markDataChanged() {
    m_dataChanged.store(true);

    if (m_updateTimer->isActive()) m_updateTimer->stop();
    m_updateTimer->start(50);
}

void CyHistogram::onUpdateTimeout() {
    if (!m_dataChanged.load()) return;

    if (auto container = data()) {
        container->clear();
        for (int i = 0; i < m_keys.size(); ++i)
            container->add({ m_keys[i], m_valuesFront[i] });
    }
    m_dataChanged.store(false);
    parentPlot()->replot(QCustomPlot::rpQueuedReplot);
}














//class CyLineChart------------------------------------------------------
CyLineChart::CyLineChart(QCPAxis* keyAxis, QCPAxis* valueAxis)
    :QCPGraph(keyAxis, valueAxis) {

}

void CyLineChart::setGraphData(QVector<QCPGraphData> data, bool alreadySorted /*= false*/) {
    if (true == m_dataChanged.load())
        return;
    QMutexLocker lock(&m_Mute);
    mDataContainer->clear();
    mDataContainer->add(data, alreadySorted);
    m_dataChanged.store(true);
}

double CyLineChart::selectTest(const QPointF& pos, bool onlySelectable, QVariant* details /*=nullptr */) const {
    //QCPBars::selectTest(pos, onlySelectable, details);
    return -1;
}

void CyLineChart::draw(QCPPainter* painter)
{
    if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
    if (mKeyAxis.data()->range().size() <= 0 || mDataContainer->isEmpty()) return;
    if (mLineStyle == lsNone && mScatterStyle.isNone()) return;

    QVector<QPointF> lines, scatters; // line and (if necessary) scatter pixel coordinates will be stored here while iterating over segments

    // loop over and draw segments of unselected/selected data:
    QList<QCPDataRange> selectedSegments, unselectedSegments, allSegments;
    getDataSegments(selectedSegments, unselectedSegments);
    allSegments << unselectedSegments << selectedSegments;
    for (int i = 0; i < allSegments.size(); ++i)
    {
        bool isSelectedSegment = i >= unselectedSegments.size();
        // get line pixel points appropriate to line style:
        QCPDataRange lineDataRange = isSelectedSegment ? allSegments.at(i) : allSegments.at(i).adjusted(-1, 1); // unselected segments extend lines to bordering selected data point (safe to exceed total data bounds in first/last segment, getLines takes care)
        getLines(&lines, lineDataRange);

        // check data validity if flag set:
#ifdef QCUSTOMPLOT_CHECK_DATA
        QCPGraphDataContainer::const_iterator it;
        for (it = mDataContainer->constBegin(); it != mDataContainer->constEnd(); ++it)
        {
            if (QCP::isInvalidData(it->key, it->value))
                qDebug() << Q_FUNC_INFO << "Data point at" << it->key << "invalid." << "Plottable name:" << name();
        }
#endif
        // draw fill of graph:
        if (isSelectedSegment && mSelectionDecorator)
            mSelectionDecorator->applyBrush(painter);
        else
            painter->setBrush(mBrush);
        painter->setPen(Qt::NoPen);
        drawFill(painter, &lines);

        // draw line:
        if (mLineStyle != lsNone)
        {
            if (isSelectedSegment && mSelectionDecorator)
                mSelectionDecorator->applyPen(painter);
            else
                painter->setPen(mPen);
            painter->setBrush(Qt::NoBrush);
            if (mLineStyle == lsImpulse)
                drawImpulsePlot(painter, lines);
            else
                drawLinePlot(painter, lines); // also step plots can be drawn as a line plot
        }

        // draw scatters:
        QCPScatterStyle finalScatterStyle = mScatterStyle;
        if (isSelectedSegment && mSelectionDecorator)
            finalScatterStyle = mSelectionDecorator->getFinalScatterStyle(mScatterStyle);
        if (!finalScatterStyle.isNone())
        {
            getScatters(&scatters, allSegments.at(i));
            drawScatterPlot(painter, scatters, finalScatterStyle);
        }
    }

    // draw other selection decoration that isn't just line/scatter pens and brushes:
    if (mSelectionDecorator)
        mSelectionDecorator->drawDecoration(painter, selection());

    m_dataChanged.store(false);
}
