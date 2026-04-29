#include "CyQCP.h"
#include <cmath>
#include <algorithm>

#include <chrono>
#include <QMutex>
#include <QThread>

CyHistogram::CyHistogram(QCPAxis* keyAxis, QCPAxis* valueAxis)
    :QCPBars(keyAxis, valueAxis) {
    setAntialiased(false);
}

void CyHistogram::updateHistogramFromThread(const double* values, int size) {
    if (!values || size <= 0)
        return;
    // 1. 拷贝数据到后台缓冲区
    {
        QMutexLocker locker(&m_dataMutex);
        if (m_keys.size() != size) {
            // 尺寸变化时重新初始化（不频繁）
            initializeHistogramUnsafe(size);
        }
        // 确保后台缓冲区大小正确
        if (m_valuesBack.size() != size) {
            m_valuesBack.resize(size);
        }
        // 快速内存拷贝
        std::memcpy(m_valuesBack.data(), values, static_cast<size_t>(size) * sizeof(double));
        m_swapRequested.store(true, std::memory_order_release);
    }

    // 2. 如果不在 GUI 线程，投递交换事件（且避免重复投递）
    if (QThread::currentThread() != this->thread()) {
        bool expected = false;
        if (m_updatePending.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            QMetaObject::invokeMethod(this, &CyHistogram::processPendingSwap, Qt::QueuedConnection);
        }
    }
    else {
        // 已经在 GUI 线程，直接调用
        processPendingSwap();
    }
}

void CyHistogram::processPendingSwap()
{
    m_updatePending.store(false, std::memory_order_release);

    QMutexLocker locker(&m_dataMutex);
    if (!m_swapRequested.load(std::memory_order_acquire))
        return;

    // 交换缓冲区
    m_valuesFront.swap(m_valuesBack);
    m_swapRequested.store(false, std::memory_order_release);

    auto container = this->data().data();
    if (!container)
        return;

    const int count = m_keys.size();
    if (container->size() != count) {
        // 尺寸变化：重建
        container->clear();
        QVector<QCPBarsData> newData;
        newData.reserve(count);
        for (int i = 0; i < count; ++i) {
            newData.append({ m_keys[i], m_valuesFront[i] });
        }
        container->add(newData, true);
    }
    else {
        // 尺寸一致：原位修改值
        auto it = container->begin();
        for (int i = 0; i < count; ++i, ++it) {
            it->value = m_valuesFront[i];
        }
    }

    if (auto* plot = parentPlot()) {
        plot->replot(QCustomPlot::rpQueuedReplot);
    }
}

void CyHistogram::setSamplingEnabled(bool enabled) {
    m_samplingEnabled = enabled;
}

bool CyHistogram::isSamplingEnabled() const {
    return m_samplingEnabled;
}

void CyHistogram::setSamplingThreshold(int threshold) {
    m_samplingThreshold = qMax(1, threshold);
}

int CyHistogram::samplingThreshold() const {
    return m_samplingThreshold;
}

QLineF CyHistogram::keyToBarLine(double key, double value) const {
    double x = mKeyAxis->coordToPixel(key);
    double yBottom = mValueAxis->coordToPixel(0.0);
    double yTop = mValueAxis->coordToPixel(value);
    return QLineF(x, yBottom, x, yTop);
}

double CyHistogram::selectTest(const QPointF& pos, bool onlySelectable, QVariant* details /*= nullptr*/) const
{
    Q_UNUSED(pos);
    Q_UNUSED(onlySelectable);
    Q_UNUSED(details);
    return -1.0;  // 禁用选择，提升性能
}

void CyHistogram::draw(QCPPainter* painter) {
    if (!mKeyAxis || !mValueAxis) {
        QCPBars::draw(painter);
        return;
    }

    double keyMin = mKeyAxis->range().lower;
    double keyMax = mKeyAxis->range().upper;

    if (qFuzzyCompare(keyMin, keyMax) || qFuzzyIsNull(width())) {
        QCPBars::draw(painter);
        return;
    }

    // 判断是否需要抽样
    int visibleBars = static_cast<int>((keyMax - keyMin) / width());
    if (m_samplingEnabled && visibleBars > m_samplingThreshold) {
        drawSampledBars(painter);
    }
    else {
        drawSampledBars(painter, false);
    }
}

void CyHistogram::initializeHistogramUnsafe(int binCount) {
    if (binCount <= 0 || m_keys.size() == binCount)
        return;

    m_keys.resize(binCount);
    for (int i = 0; i < binCount; ++i) {
        m_keys[i] = static_cast<double>(i);
    }
    m_expectedSize = binCount;
    m_valuesBack.resize(binCount);
    // 如果前台尚未初始化，也分配
    if (m_valuesFront.size() != binCount)
        m_valuesFront.resize(binCount);
}

void CyHistogram::drawSampledBars(QCPPainter* painter, bool sampling/* = true*/) const {
    if (!mKeyAxis || !mValueAxis || data()->isEmpty())
        return;

    const auto* container = data().data();
    double keyMin = mKeyAxis->range().lower;
    double keyMax = mKeyAxis->range().upper;
    if (keyMin >= keyMax)
        return;
    // 如果不启用抽样，或者可见柱数未超阈值 → 逐点画线
    if (!sampling) {
        painter->setPen(mPen);
        for (auto it = container->constBegin(); it != container->constEnd(); ++it) {
            if (it->key >= keyMin && it->key <= keyMax && it->value > 0.0)
                painter->drawLine(keyToBarLine(it->key, it->value));
        }
        return;
    }

    // 计算可见像素范围
    int pixelStart = qRound(mKeyAxis->coordToPixel(keyMin));
    int pixelEnd = qRound(mKeyAxis->coordToPixel(keyMax));
    int visiblePixels = qAbs(pixelEnd - pixelStart);
    if (visiblePixels <= 1)
        return;

    int bucketCount = qMin(visiblePixels, m_samplingThreshold);
    struct Bucket {
        double peakVal = -std::numeric_limits<double>::max();
        double repKey = 0.0;
        bool valid = false;
    };
    QVector<Bucket> buckets(bucketCount);

    double bucketWidth = (keyMax - keyMin) / bucketCount;
    const int dataSize = container->size();
    // 遍历所有数据，寻找每个桶内的最大值
    for (int i = 0; i < dataSize; ++i) {
        auto point = container->at(i);
        double k = point->key;
        double v = point->value;
        if (k < keyMin || k > keyMax || v <= 0.0)
            continue;

        int idx = static_cast<int>((k - keyMin) / bucketWidth);
        idx = qBound(0, idx, bucketCount - 1);
        if (!buckets[idx].valid || v > buckets[idx].peakVal) {
            buckets[idx].peakVal = v;
            buckets[idx].repKey = k;
            buckets[idx].valid = true;
        }
    }

    painter->setPen(mPen);
    for (const auto& bucket : buckets) {
        if (bucket.valid) {
            painter->drawLine(keyToBarLine(bucket.repKey, bucket.peakVal));
        }
    }
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
