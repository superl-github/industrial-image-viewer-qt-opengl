#include "CyHistogramView.h"

#include <QBrush>
#include <QEvent>
#include <QFontMetrics>
#include <QLineF>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QStringList>
#include <QThread>
#include <QVector>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
    const int kResizeThreshold = 5;     ///< 选区边缘的抓取像素范围
    const int kMinCellWidth = 18;
    const int kMinCellHeight = 14;

    /// 暗色主题下把过暗的颜色提亮，保证读数可读。
    QColor ensureContrast(QColor c, bool dark) {
        if (!dark) return c;
        const double luma = 0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue();
        if (luma < 110.0) {
            const double k = 1.9;
            c.setRed(qMin(255, int(c.red() * k) + 40));
            c.setGreen(qMin(255, int(c.green() * k) + 40));
            c.setBlue(qMin(255, int(c.blue() * k) + 40));
        }
        return c;
    }
}

struct CyHistogramView::PrivateData {
    struct Series {
        std::vector<double> front;  ///< GUI 线程读取
        std::vector<double> back;   ///< 工作线程写入
        int base = 0;
        int step = 1;
        int backBase = 0;
        int backStep = 1;
        bool dirty = false;
        bool clearRequested = false;
        bool visible = false;
        double frontMax = 0.0;
        QColor color;
        int style = Style_Inherit;
        QString name;
    };

    Series series[Ch_Count];

    mutable QMutex dataMutex;
    std::atomic<bool> updatePending{ false };
    std::atomic<bool> dataDirty{ false };

    //外观
    Orientation orientation = Horizontal;
    DisplayStyle style = Style_Area;
    int areaAlpha = -1;
    bool readoutVisible = true;
    bool gridVisible = true;
    bool levelLabelsVisible = true;
    bool countLabelsVisible = true;
    bool stepLine = true;
    QColor themeColor = QColor(0x2a, 0xa3, 0xc6);
    QColor selectionColor;
    QString xTitle;
    QString yTitle;

    //坐标轴/格子
    QSize cellSize = QSize(58, 34);
    int xPerCell = 1;
    int yPerCell = 1;
    int xMinRange = 0;
    int xMaxRange = 255;
    double yMaxRange = 1e12;
    double xOrigin = 0.0;
    bool autoX = true;
    bool autoY = true;
    bool xAdjustable = true;
    double yHintMax = 0.0;
    double dataMaxY = 0.0;

    //选区
    bool selEnabled = false;
    double selStart = 0.0;
    double selEnd = 255.0;
    double selMinWidth = 1.0;

    //交互
    enum DragMode {
        DragNone = 0,
        DragPan,
        DragSelBody,
        DragSelLeft,
        DragSelRight,
    };
    int dragMode = DragNone;
    QPoint pressPos;
    double pressValue = 0.0;
    double pressOrigin = 0.0;
    double pressSelStart = 0.0;
    double pressSelEnd = 0.0;

    //鼠标
    QPoint mousePos;
    bool mouseInside = false;
    double readoutX = 0.0;

    //布局
    QRect plot;
    int marginLeft = 46;
    int marginRight = 10;
    int marginTop = 8;
    int marginBottom = 22;

    //字体缓存：每帧都构造 QFont/QFontMetrics 会触发字体匹配，实测占整帧绘制成本的 20%+
    QFont labelFont;
    QFontMetrics labelMetrics = QFontMetrics(QFont());
};

CyHistogramView::CyHistogramView(QWidget* parent /*= nullptr*/)
    : QWidget(parent)
    , d(new PrivateData) {
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::ArrowCursor);

    d->series[Ch_Gray].color = QColor(0x3a, 0x41, 0x4b);
    d->series[Ch_Red].color = QColor(0xe0, 0x4f, 0x5f);
    d->series[Ch_Green].color = QColor(0x2e, 0xa0, 0x43);
    d->series[Ch_Blue].color = QColor(0x33, 0x72, 0xd6);

    d->series[Ch_Gray].name = tr("Gray");
    d->series[Ch_Red].name = tr("Red");
    d->series[Ch_Green].name = tr("Green");
    d->series[Ch_Blue].name = tr("Blue");

    //默认只显示灰度通道，三通道模式由上层显式打开
    d->series[Ch_Gray].visible = true;

    d->selectionColor = d->themeColor;
    d->xTitle = tr("Grayscale");
    d->yTitle = tr("Quantity");

    updateLayout();
}

CyHistogramView::~CyHistogramView() {
    delete d;
    d = nullptr;
}

//========================================================================================
// 数据
//========================================================================================
void CyHistogramView::setSeriesData(int channel, const double* values, int count, int binBase, int binStep) {
    if (channel < 0 || channel >= Ch_Count)
        return;

    {
        QMutexLocker locker(&d->dataMutex);
        PrivateData::Series& s = d->series[channel];
        if (count <= 0 || !values) {
            //清空请求也走同一条排队通道，保证 front 只在 GUI 线程被修改
            s.back.clear();
            s.clearRequested = true;
        }
        else {
            if (binStep <= 0) binStep = 1;
            if (static_cast<int>(s.back.size()) != count) {
                s.back.resize(static_cast<size_t>(count));
            }
            std::memcpy(s.back.data(), values, sizeof(double) * static_cast<size_t>(count));
            s.backBase = binBase;
            s.backStep = binStep;
            s.clearRequested = false;
        }
        s.dirty = true;
        d->dataDirty.store(true, std::memory_order_release);
    }

    if (QThread::currentThread() != this->thread()) {
        bool expected = false;
        if (d->updatePending.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            QMetaObject::invokeMethod(this, [this]() { onDataReady(); }, Qt::QueuedConnection);
        }
    }
    else {
        onDataReady();
    }
}

void CyHistogramView::clearSeries(int channel) {
    if (channel < 0 || channel >= Ch_Count)
        return;
    {
        QMutexLocker locker(&d->dataMutex);
        d->series[channel].back.clear();
        d->series[channel].front.clear();
        d->series[channel].frontMax = 0.0;
        d->series[channel].dirty = false;
        d->series[channel].clearRequested = false;
    }
    update();
}

void CyHistogramView::clearAllSeries() {
    {
        QMutexLocker locker(&d->dataMutex);
        for (int i = 0; i < Ch_Count; ++i) {
            d->series[i].back.clear();
            d->series[i].front.clear();
            d->series[i].frontMax = 0.0;
            d->series[i].dirty = false;
            d->series[i].clearRequested = false;
        }
        d->dataMaxY = 0.0;
    }
    update();
}

void CyHistogramView::onDataReady() {
    d->updatePending.store(false, std::memory_order_release);

    bool hasNewData = false;
    {
        QMutexLocker locker(&d->dataMutex);
        for (int i = 0; i < Ch_Count; ++i) {
            PrivateData::Series& s = d->series[i];
            if (!s.dirty)
                continue;
            s.dirty = false;
            hasNewData = true;

            if (s.clearRequested) {
                s.front.clear();
                s.frontMax = 0.0;
                s.clearRequested = false;
                continue;
            }

            s.front.swap(s.back);
            s.base = s.backBase;
            s.step = s.backStep;

            s.frontMax = 0.0;
            for (double v : s.front) {
                if (v > s.frontMax) s.frontMax = v;
            }
        }

        double maxY = 0.0;
        for (int i = 0; i < Ch_Count; ++i) {
            if (d->series[i].visible) {
                maxY = qMax(maxY, d->series[i].frontMax);
            }
        }
        d->dataMaxY = maxY;
    }
    d->dataDirty.store(false, std::memory_order_release);

    if (!hasNewData)
        return;

    applyAutoX(false);
    applyAutoY(false);
    update();
}

void CyHistogramView::setSeriesVisible(int channel, bool visible) {
    if (channel < 0 || channel >= Ch_Count || d->series[channel].visible == visible)
        return;
    d->series[channel].visible = visible;

    double maxY = 0.0;
    for (int i = 0; i < Ch_Count; ++i) {
        if (d->series[i].visible) maxY = qMax(maxY, d->series[i].frontMax);
    }
    d->dataMaxY = maxY;

    applyAutoY(false);
    update();
}

bool CyHistogramView::isSeriesVisible(int channel) const {
    if (channel < 0 || channel >= Ch_Count) return false;
    return d->series[channel].visible;
}

void CyHistogramView::setSeriesColor(int channel, const QColor& color) {
    if (channel < 0 || channel >= Ch_Count) return;
    d->series[channel].color = color;
    update();
}

QColor CyHistogramView::seriesColor(int channel) const {
    if (channel < 0 || channel >= Ch_Count) return QColor();
    return d->series[channel].color;
}

void CyHistogramView::setSeriesStyle(int channel, int style) {
    if (channel < 0 || channel >= Ch_Count) return;
    if (style < Style_Inherit || style > Style_Area) style = Style_Inherit;
    d->series[channel].style = style;
    update();
}

int CyHistogramView::seriesStyle(int channel) const {
    if (channel < 0 || channel >= Ch_Count) return Style_Inherit;
    return d->series[channel].style;
}

void CyHistogramView::setChannelName(int channel, const QString& name) {
    if (channel < 0 || channel >= Ch_Count) return;
    d->series[channel].name = name;
    update();
}

QString CyHistogramView::channelName(int channel) const {
    if (channel < 0 || channel >= Ch_Count) return QString();
    return d->series[channel].name;
}

//========================================================================================
// 外观
//========================================================================================
void CyHistogramView::setDisplayStyle(DisplayStyle style) {
    if (style < Style_Bars || style > Style_Area) return;
    if (d->style == style) return;
    d->style = style;
    update();
}

CyHistogramView::DisplayStyle CyHistogramView::displayStyle() const {
    return d->style;
}

void CyHistogramView::setAreaAlpha(int alpha) {
    d->areaAlpha = (alpha < 0) ? -1 : qBound(0, alpha, 255);
    update();
}

int CyHistogramView::areaAlpha() const {
    return d->areaAlpha;
}

void CyHistogramView::setReadoutVisible(bool visible) {
    if (d->readoutVisible == visible) return;
    d->readoutVisible = visible;
    update();
}

bool CyHistogramView::readoutVisible() const {
    return d->readoutVisible;
}

void CyHistogramView::setGridVisible(bool visible) {
    if (d->gridVisible == visible) return;
    d->gridVisible = visible;
    update();
}

bool CyHistogramView::gridVisible() const {
    return d->gridVisible;
}

void CyHistogramView::setAxisLabelsVisible(bool levelLabels, bool countLabels) {
    if (d->levelLabelsVisible == levelLabels && d->countLabelsVisible == countLabels)
        return;
    d->levelLabelsVisible = levelLabels;
    d->countLabelsVisible = countLabels;
    updateLayout();
    applyAutoX(false);
    applyAutoY(false);
    clampX();
    update();
}

bool CyHistogramView::levelLabelsVisible() const {
    return d->levelLabelsVisible;
}

bool CyHistogramView::countLabelsVisible() const {
    return d->countLabelsVisible;
}

void CyHistogramView::setStepLine(bool enabled) {
    if (d->stepLine == enabled) return;
    d->stepLine = enabled;
    update();
}

bool CyHistogramView::stepLine() const {
    return d->stepLine;
}

void CyHistogramView::setThemeColor(const QColor& color) {
    d->themeColor = color;
    d->selectionColor = color;
    update();
}

QColor CyHistogramView::themeColor() const {
    return d->themeColor;
}

void CyHistogramView::setAxisTitles(const QString& xTitle, const QString& yTitle) {
    d->xTitle = xTitle;
    d->yTitle = yTitle;
    update();
}

//========================================================================================
// 渲染方向（竖向渲染）
//========================================================================================
void CyHistogramView::setOrientation(Orientation orientation) {
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;

    //方向变了：边距、自动缩放、选区命中判定都依赖轴向，统一刷新一次
    updateLayout();
    applyAutoX(true);
    applyAutoY(true);
    clampX();
    update();
}

CyHistogramView::Orientation CyHistogramView::orientation() const {
    return d->orientation;
}

int CyHistogramView::levelCellPixels() const {
    //灰度轴（数值轴）：横向占宽度，竖向占高度
    const int px = (d->orientation == Vertical) ? d->cellSize.height() : d->cellSize.width();
    return qMax(kMinCellHeight, px);
}

int CyHistogramView::countCellPixels() const {
    //计数轴（幅度轴）：横向占高度，竖向占宽度
    const int px = (d->orientation == Vertical) ? d->cellSize.width() : d->cellSize.height();
    return qMax(kMinCellHeight, px);
}

int CyHistogramView::levelAxisPixels() const {
    return (d->orientation == Vertical) ? d->plot.height() : d->plot.width();
}

int CyHistogramView::countAxisPixels() const {
    return (d->orientation == Vertical) ? d->plot.width() : d->plot.height();
}

int CyHistogramView::countAxisPixelLength() const {
    return qMax(1, countAxisPixels());
}

double CyHistogramView::levelCoordOf(int pixelX, int pixelY) const {
    Q_UNUSED(pixelX);
    return (d->orientation == Vertical) ? double(pixelY) : double(pixelX);
}

double CyHistogramView::levelCoordOf(const QPoint& pos) const {
    return levelCoordOf(pos.x(), pos.y());
}

QPointF CyHistogramView::pixelOfValue(double level, double count) const {
    return QPointF(pixelOfValueX(level), pixelOfValueY(count));
}

//========================================================================================
// 坐标轴 / 格子
//========================================================================================
void CyHistogramView::setCellPixelSize(const QSize& size) {
    d->cellSize = QSize(qMax(kMinCellWidth, size.width()), qMax(kMinCellHeight, size.height()));
    updateLayout();
    applyAutoX(false);
    applyAutoY(false);
    update();
}

QSize CyHistogramView::cellPixelSize() const {
    return d->cellSize;
}

int CyHistogramView::xPerCell() const {
    return d->xPerCell;
}

void CyHistogramView::setXPerCell(int levels, bool keepAnchor /*= true*/) {
    if (!d->xAdjustable)
        return;
    levels = qBound(1, levels, maxXPerCell());
    if (levels == d->xPerCell)
        return;

    const QPoint anchorPt = (d->mouseInside && d->plot.contains(d->mousePos))
        ? d->mousePos : d->plot.center();
    const int anchorPx = int(levelCoordOf(anchorPt));
    const double anchorValue = xValueAt(anchorPx);
    const double frac = (levelAxisPixels() > 0)
        ? ((d->orientation == Vertical)
            ? (double(d->plot.bottom()) - double(anchorPx)) / double(d->plot.height())
            : (double(anchorPx) - double(d->plot.left())) / double(d->plot.width()))
        : 0.0;

    d->xPerCell = levels;
    if (keepAnchor && levelAxisPixels() > 0) {
        d->xOrigin = anchorValue - frac * visibleSpanX();
    }
    clampX();

    if (d->autoX) {
        d->autoX = false;
        emit autoXChanged(false);
    }
    update();
}

int CyHistogramView::yPerCell() const {
    return d->yPerCell;
}

void CyHistogramView::setYPerCell(int counts, bool keepAnchor /*= true*/) {
    Q_UNUSED(keepAnchor);
    counts = qBound(1, counts, maxYPerCell());
    if (counts == d->yPerCell)
        return;
    d->yPerCell = counts;
    if (d->autoY) {
        d->autoY = false;
        emit autoYChanged(false);
    }
    update();
}

void CyHistogramView::setXMinRange(int minLevels) {
    d->xMinRange = minLevels;
    if (d->xMaxRange < d->xMinRange)
        d->xMaxRange = d->xMinRange;
    applyAutoX(false);
    clampX();
    if (d->selEnabled) setSelection(d->selStart, d->selEnd);
    update();
}

int CyHistogramView::xMinRange() const {
    return d->xMinRange;
}

void CyHistogramView::setXMaxRange(int maxLevels) {
    d->xMaxRange = qMax(d->xMinRange, maxLevels);
    applyAutoX(false);
    clampX();
    if (d->selEnabled) setSelection(d->selStart, d->selEnd);
    update();
}

int CyHistogramView::xMaxRange() const {
    return d->xMaxRange;
}

void CyHistogramView::setYMaxRange(double maxCount) {
    d->yMaxRange = qBound(1.0, maxCount, 1e9);
    applyAutoY(false);
    update();
}

double CyHistogramView::yMaxRange() const {
    return d->yMaxRange;
}

bool CyHistogramView::autoX() const {
    return d->autoX;
}

void CyHistogramView::setAutoX(bool on) {
    if (!on && !d->xAdjustable)
        return;
    if (d->autoX == on) return;
    d->autoX = on;
    if (on) {
        applyAutoX(true);
        emit autoXChanged(true);
    }
    update();
}

bool CyHistogramView::autoY() const {
    return d->autoY;
}

void CyHistogramView::setAutoY(bool on) {
    if (d->autoY == on) return;
    d->autoY = on;
    if (on) {
        applyAutoY(true);
        emit autoYChanged(true);
    }
    update();
}

void CyHistogramView::setXAdjustable(bool adjustable) {
    if (d->xAdjustable == adjustable)
        return;
    d->xAdjustable = adjustable;
    if (!adjustable) {
        //关闭后 X 轴固定为全量程
        d->autoX = true;
        applyAutoX(true);
        emit autoXChanged(true);
        clampX();
    }
    update();
}

bool CyHistogramView::xAdjustable() const {
    return d->xAdjustable;
}

void CyHistogramView::setYHintMax(double value) {
    if (value <= 0.0) return;
    const bool changed = std::fabs(value - d->yHintMax) > qMax(1.0, d->yHintMax * 0.02);
    d->yHintMax = value;
    if (changed && d->autoY) {
        applyAutoY(false);
        update();
    }
}

double CyHistogramView::yHintMax() const {
    return d->yHintMax;
}

void CyHistogramView::fitX() {
    d->autoX = true;
    applyAutoX(true);
    emit autoXChanged(true);
    update();
}

void CyHistogramView::zoomX(double factor) {
    if (!d->xAdjustable) return;
    if (factor <= 0.0 || levelAxisPixels() <= 0) return;
    const QPoint anchorPt = (d->mouseInside && d->plot.contains(d->mousePos))
        ? d->mousePos : d->plot.center();
    const int anchorPx = int(levelCoordOf(anchorPt));
    const double anchorValue = xValueAt(anchorPx);
    const double frac = (d->orientation == Vertical)
        ? (double(d->plot.bottom()) - double(anchorPx)) / double(d->plot.height())
        : (double(anchorPx) - double(d->plot.left())) / double(d->plot.width());

    const int newStep = qBound(1, int(std::lround(double(d->xPerCell) * factor)), maxXPerCell());
    if (newStep == d->xPerCell) return;

    d->xPerCell = newStep;
    d->xOrigin = anchorValue - frac * visibleSpanX();
    clampX();

    if (d->autoX) {
        d->autoX = false;
        emit autoXChanged(false);
    }
    update();
}

void CyHistogramView::zoomY(double factor) {
    if (factor <= 0.0) return;
    const int newStep = qBound(1, int(std::lround(double(d->yPerCell) * factor)), maxYPerCell());
    if (newStep == d->yPerCell) return;
    d->yPerCell = newStep;
    if (d->autoY) {
        d->autoY = false;
        emit autoYChanged(false);
    }
    update();
}

void CyHistogramView::resetView() {
    bool xChanged = !d->autoX;
    bool yChanged = !d->autoY;
    d->autoX = true;
    d->autoY = true;
    applyAutoX(true);
    applyAutoY(true);
    if (xChanged) emit autoXChanged(true);
    if (yChanged) emit autoYChanged(true);
    update();
}

//========================================================================================
// 选区
//========================================================================================
void CyHistogramView::setSelectionEnabled(bool enabled) {
    if (d->selEnabled == enabled) return;
    d->selEnabled = enabled;
    if (!enabled) {
        d->dragMode = PrivateData::DragNone;
    }
    setCursor(Qt::ArrowCursor);
    update();
}

bool CyHistogramView::isSelectionEnabled() const {
    return d->selEnabled;
}

void CyHistogramView::setSelection(double start, double end) {
    if (end < start) std::swap(start, end);
    const double lo = double(d->xMinRange);
    const double hi = double(d->xMaxRange);
    start = qBound(lo, start, hi);
    end = qBound(lo, end, hi);
    if (end - start < d->selMinWidth) {
        end = qMin(hi, start + d->selMinWidth);
        if (end - start < d->selMinWidth) {
            start = qMax(lo, end - d->selMinWidth);
        }
    }
    if (qFuzzyCompare(start + 1.0, d->selStart + 1.0) && qFuzzyCompare(end + 1.0, d->selEnd + 1.0))
        return;
    d->selStart = start;
    d->selEnd = end;
    update();
}

QPointF CyHistogramView::selection() const {
    return QPointF(d->selStart, d->selEnd);
}

void CyHistogramView::setSelectionMinWidth(double width) {
    d->selMinWidth = qMax(1.0, width);
}

void CyHistogramView::setSelectionColor(const QColor& color) {
    d->selectionColor = color;
    update();
}

//========================================================================================
// 查询
//========================================================================================
double CyHistogramView::xValueAt(int pixelX) const {
    //横向：pixelX 为横坐标；竖向：pixelX 语义为"沿灰度轴的像素坐标"（即控件的 y）
    const double span = visibleSpanX();
    const int axisPx = levelAxisPixels();
    if (axisPx <= 0 || span <= 0.0)
        return d->xOrigin;
    const double pos = (d->orientation == Vertical)
        ? (double(d->plot.bottom()) - double(pixelX))
        : (double(pixelX) - double(d->plot.left()));
    return d->xOrigin + pos / double(axisPx) * span;
}

double CyHistogramView::yValueAt(int pixelY) const {
    //横向：pixelY 为纵坐标（自下向上）；竖向：pixelY 语义为"沿计数轴的像素坐标"（即控件的 x）
    const double span = visibleSpanY();
    const int axisPx = countAxisPixels();
    if (axisPx <= 0 || span <= 0.0)
        return 0.0;
    const double pos = (d->orientation == Vertical)
        ? (double(pixelY) - double(d->plot.left()))
        : (double(d->plot.bottom()) - double(pixelY));
    return pos / double(axisPx) * span;
}

double CyHistogramView::seriesValueAt(int channel, double x) const {
    if (channel < 0 || channel >= Ch_Count)
        return 0.0;
    const PrivateData::Series& s = d->series[channel];
    if (s.front.empty() || s.step <= 0)
        return 0.0;
    const double idx = (x - double(s.base)) / double(s.step);
    const int i = int(std::llround(idx));
    if (i < 0 || i >= int(s.front.size()))
        return 0.0;
    return s.front[size_t(i)];
}

QRect CyHistogramView::plotRect() const {
    return d->plot;
}

//========================================================================================
// 事件
//========================================================================================
void CyHistogramView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateLayout();
    applyAutoX(false);
    applyAutoY(false);
    clampX();
}

void CyHistogramView::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::PaletteChange
        || event->type() == QEvent::StyleChange) {
        updateLayout();
        applyAutoY(false);
        clampX();
        update();
    }
}

QSize CyHistogramView::sizeHint() const {
    return QSize(420, 200);
}

QSize CyHistogramView::minimumSizeHint() const {
    return QSize(160, 90);
}

int CyHistogramView::hitTestSelection(const QPoint& pos) const {
    if (!d->selEnabled || !d->plot.contains(pos))
        return 0;

    const double start = pixelOfValueX(d->selStart);
    const double end = pixelOfValueX(d->selEnd);
    const double cur = levelCoordOf(pos);
    if (std::fabs(cur - start) <= kResizeThreshold)
        return 1;
    if (std::fabs(cur - end) <= kResizeThreshold)
        return 2;
    if (cur > qMin(start, end) && cur < qMax(start, end))
        return 3;
    return 0;
}

void CyHistogramView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        d->pressPos = event->pos();
        d->pressValue = xValueAt(int(levelCoordOf(event->pos())));
        d->pressOrigin = d->xOrigin;
        d->pressSelStart = d->selStart;
        d->pressSelEnd = d->selEnd;

        const int hit = (event->button() == Qt::LeftButton) ? hitTestSelection(event->pos()) : 0;
        switch (hit) {
            case 1: d->dragMode = PrivateData::DragSelLeft; break;
            case 2: d->dragMode = PrivateData::DragSelRight; break;
            case 3: d->dragMode = PrivateData::DragSelBody; break;
            default: d->dragMode = PrivateData::DragPan; break;
        }
        if (d->dragMode == PrivateData::DragPan) {
            setCursor(Qt::ClosedHandCursor);
        }
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void CyHistogramView::mouseMoveEvent(QMouseEvent* event) {
    d->mousePos = event->pos();
    d->mouseInside = d->plot.contains(event->pos());
    if (d->mouseInside) {
        d->readoutX = xValueAt(int(levelCoordOf(event->pos())));
    }

    switch (d->dragMode) {
        case PrivateData::DragPan: {
            const double span = visibleSpanX();
            if (d->xAdjustable && levelAxisPixels() > 0 && span > 0.0) {
                const double deltaPx = levelCoordOf(event->pos()) - levelCoordOf(d->pressPos);
                const double dx = deltaPx / double(levelAxisPixels()) * span;
                d->xOrigin = d->pressOrigin - dx;
                clampX();
                if (d->autoX) {
                    d->autoX = false;
                    emit autoXChanged(false);
                }
            }
        } break;

        case PrivateData::DragSelLeft: {
            const double cur = xValueAt(int(levelCoordOf(event->pos())));
            const double newStart = qMax(double(d->xMinRange), qMin(cur, d->selEnd - d->selMinWidth));
            if (!qFuzzyCompare(newStart + 1.0, d->selStart + 1.0)) {
                d->selStart = newStart;
                emit selectionChanged(std::floor(d->selStart + 0.5), std::floor(d->selEnd + 0.5));
            }
        } break;

        case PrivateData::DragSelRight: {
            const double cur = xValueAt(int(levelCoordOf(event->pos())));
            const double newEnd = qMin(double(d->xMaxRange), qMax(cur, d->selStart + d->selMinWidth));
            if (!qFuzzyCompare(newEnd + 1.0, d->selEnd + 1.0)) {
                d->selEnd = newEnd;
                emit selectionChanged(std::floor(d->selStart + 0.5), std::floor(d->selEnd + 0.5));
            }
        } break;

        case PrivateData::DragSelBody: {
            const double width = d->pressSelEnd - d->pressSelStart;
            const double cur = xValueAt(int(levelCoordOf(event->pos())));
            double newStart = d->pressSelStart + (cur - d->pressValue);
            if (newStart < double(d->xMinRange)) newStart = double(d->xMinRange);
            if (newStart + width > double(d->xMaxRange)) newStart = double(d->xMaxRange) - width;
            const double newEnd = newStart + width;
            if (!qFuzzyCompare(newStart + 1.0, d->selStart + 1.0)) {
                d->selStart = newStart;
                d->selEnd = newEnd;
                emit selectionChanged(std::floor(d->selStart + 0.5), std::floor(d->selEnd + 0.5));
            }
        } break;

        default: {
            const int hit = hitTestSelection(event->pos());
            const bool vertical = (d->orientation == Vertical);
            switch (hit) {
                case 1:
                case 2: setCursor(vertical ? Qt::SizeVerCursor : Qt::SizeHorCursor); break;
                case 3: setCursor(Qt::SizeAllCursor); break;
                default: setCursor(d->plot.contains(event->pos()) ? Qt::CrossCursor : Qt::ArrowCursor); break;
            }
        } break;
    }

    update();
    event->accept();
}

void CyHistogramView::mouseReleaseEvent(QMouseEvent* event) {
    if (d->dragMode != PrivateData::DragNone) {
        d->dragMode = PrivateData::DragNone;
        setCursor(d->plot.contains(event->pos()) ? Qt::CrossCursor : Qt::ArrowCursor);
        update();
    }
    event->accept();
}

void CyHistogramView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && d->plot.contains(event->pos())) {
        resetView();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CyHistogramView::wheelEvent(QWheelEvent* event) {
    const double steps = double(event->angleDelta().y()) / 120.0;
    if (qFuzzyIsNull(steps)) {
        event->ignore();
        return;
    }

    const Qt::KeyboardModifiers mods = event->modifiers();
    if (mods & Qt::ControlModifier) {
        zoomY(std::pow(1.25, -steps));
    }
    else if ((mods & Qt::ShiftModifier) && d->xAdjustable) {
        const double span = visibleSpanX();
        if (d->plot.width() > 0 && span > 0.0) {
            d->xOrigin -= steps * span * 0.15;
            clampX();
            if (d->autoX) {
                d->autoX = false;
                emit autoXChanged(false);
            }
            update();
        }
    }
    else {
        zoomX(std::pow(0.8, steps));
    }
    event->accept();
}

void CyHistogramView::leaveEvent(QEvent* event) {
    d->mouseInside = false;
    setCursor(Qt::ArrowCursor);
    update();
    QWidget::leaveEvent(event);
}

//========================================================================================
// 布局与绘制
//========================================================================================
bool CyHistogramView::isDarkTheme() const {
    return palette().color(QPalette::Window).lightness() < 128;
}

QFont CyHistogramView::axisFont() const {
    return d->labelFont;
}

const QFontMetrics& CyHistogramView::axisMetrics() const {
    return d->labelMetrics;
}

void CyHistogramView::updateLayout() {
    QFont f = font();
    if (f.pointSizeF() > 0.0) {
        f.setPointSizeF(qMax(6.5, f.pointSizeF() - 1.5));
    }
    d->labelFont = f;
    d->labelMetrics = QFontMetrics(f);

    const QFontMetrics& fm = d->labelMetrics;
    const bool vertical = (d->orientation == Vertical);
    //横向时左侧放计数刻度、底部放灰度刻度；竖向时左侧放灰度刻度、底部放计数刻度。
    //刻度文字被关闭时不预留对应边距，避免嵌在窄面板里出现整块空白。
    const int levelLabelWidth = fm.horizontalAdvance(QStringLiteral("000000"));
    const int countLabelWidth = fm.horizontalAdvance(QStringLiteral("000M"));

    const bool useLeftLabels = vertical ? d->levelLabelsVisible : d->countLabelsVisible;
    const bool useBottomLabels = vertical ? d->countLabelsVisible : d->levelLabelsVisible;

    d->marginLeft = useLeftLabels
        ? qMax(34, qMax(levelLabelWidth, countLabelWidth) + 12) : 4;
    d->marginRight = (d->orientation == Vertical) ? 12 : 10;
    d->marginTop = 8;
    d->marginBottom = useBottomLabels ? qMax(18, fm.height() + 8) : 6;

    const int w = qMax(0, width() - d->marginLeft - d->marginRight);
    const int h = qMax(0, height() - d->marginTop - d->marginBottom);
    d->plot = QRect(d->marginLeft, d->marginTop, w, h);
}

double CyHistogramView::visibleSpanX() const {
    //竖向 + 固定 X 轴（灰度拉伸面板）时，灰度轴必须精确铺满 [xMinRange, xMaxRange]。
    //否则"好看的格子刻度"会把量程向上取整（例如 16bit 的 65535 被拉到 75000），
    //选区上沿就永远到不了绘图区顶部，和右侧灰度映射条的把手对不齐。
    if (d->orientation == Vertical && !d->xAdjustable) {
        return double(qMax(1, d->xMaxRange - d->xMinRange));
    }

    const int axisPx = levelAxisPixels();
    const int cellPx = levelCellPixels();
    if (axisPx <= 0 || cellPx <= 0) return 0.0;
    const double cells = double(axisPx) / double(cellPx);
    return cells * double(d->xPerCell);
}

double CyHistogramView::visibleSpanY() const {
    const int axisPx = countAxisPixels();
    const int cellPx = countCellPixels();
    if (axisPx <= 0 || cellPx <= 0) return 0.0;
    const double cells = double(axisPx) / double(cellPx);
    return cells * double(d->yPerCell);
}

int CyHistogramView::maxXPerCell() const {
    //上限是"整条 X 轴范围放进一个格子"，而不是刚好铺满屏幕：
    //自动/手动缩放需要能取到 1/2/2.5/5/10 这类整数刻度，不能被铺满值截断。
    return qMax(1, d->xMaxRange - d->xMinRange + 1);
}

int CyHistogramView::maxYPerCell() const {
    //注意：默认 yMaxRange 很大，必须先夹到 int 安全范围再转换，否则溢出成负数会把上限压成 1。
    const double bound = std::ceil(d->yMaxRange);
    if (bound >= 2000000000.0) return 2000000000;
    return qMax(1, int(bound));
}

void CyHistogramView::clampX() {
    d->xPerCell = qBound(1, d->xPerCell, maxXPerCell());

    //竖向固定 X 轴的精确铺满模式：起点恒等于 xMinRange，保证像素↔灰度值线性对应。
    if (d->orientation == Vertical && !d->xAdjustable) {
        d->xOrigin = double(d->xMinRange);
        return;
    }

    const int total = d->xMaxRange - d->xMinRange + 1;
    const double span = visibleSpanX();
    if (span <= 0.0) {
        d->xOrigin = d->xMinRange;
        return;
    }
    if (span >= double(total)) {
        d->xOrigin = double(d->xMinRange);
    }
    else {
        d->xOrigin = qBound(double(d->xMinRange), d->xOrigin,
            double(d->xMinRange) + double(total) - span);
    }
}

void CyHistogramView::applyAutoX(bool force) {
    if (!d->autoX && !force) return;

    const int total = qMax(1, d->xMaxRange - d->xMinRange + 1);
    const int axisPx = levelAxisPixels();
    const int cellPx = levelCellPixels();
    const double cells = (axisPx > 0 && cellPx > 0)
        ? double(axisPx) / double(cellPx) : 1.0;
    int step = niceCellStep(double(total) / qMax(1.0, cells));
    step = qBound(1, step, maxXPerCell());

    d->xPerCell = step;
    d->xOrigin = double(d->xMinRange);
    clampX();
}

void CyHistogramView::applyAutoY(bool force) {
    if (!d->autoY && !force) return;

    double peak = (d->yHintMax > 0.0) ? d->yHintMax : d->dataMaxY;
    if (peak <= 0.0) peak = 1.0;
    peak *= 1.06; // 顶部留一点余量

    const int axisPx = countAxisPixels();
    const int cellPx = countCellPixels();
    const double cells = (axisPx > 0 && cellPx > 0)
        ? double(axisPx) / double(cellPx) : 1.0;
    int step = niceCellStep(peak / qMax(1.0, cells));
    step = qBound(1, step, maxYPerCell());
    d->yPerCell = step;
}

double CyHistogramView::pixelOfValueX(double value) const {
    const double span = visibleSpanX();
    if (levelAxisPixels() <= 0 || span <= 0.0)
        return (d->orientation == Vertical) ? double(d->plot.bottom()) : double(d->plot.left());
    const double frac = (value - d->xOrigin) / span;
    if (d->orientation == Vertical) {
        //灰度轴竖向：0 在最底，数值向上增长
        return double(d->plot.bottom()) - frac * double(d->plot.height());
    }
    return double(d->plot.left()) + frac * double(d->plot.width());
}

double CyHistogramView::pixelOfValueY(double value) const {
    const double span = visibleSpanY();
    if (countAxisPixels() <= 0 || span <= 0.0)
        return (d->orientation == Vertical) ? double(d->plot.left()) : double(d->plot.bottom());
    const double frac = value / span;
    if (d->orientation == Vertical) {
        //计数轴横向：自左向右增长
        return double(d->plot.left()) + frac * double(d->plot.width());
    }
    return double(d->plot.bottom()) - frac * double(d->plot.height());
}

QString CyHistogramView::formatNumber(double value) {
    if (!std::isfinite(value))
        return QStringLiteral("-");

    const double a = std::fabs(value);
    if (a >= 1e9) return QString::number(value / 1e9, 'f', 1) + QLatin1Char('G');
    if (a >= 1e6) return QString::number(value / 1e6, 'f', 1) + QLatin1Char('M');
    if (a >= 1e4) return QString::number(value / 1e3, 'f', 0) + QLatin1Char('K');
    if (a >= 1e3) return QString::number(value / 1e3, 'f', 1) + QLatin1Char('K');
    if (a >= 1.0) return QString::number(std::llround(value));
    return QString::number(value, 'f', 0);
}

QString CyHistogramView::formatCount(double value) {
    //读数需要精确值，不做 K/M 缩写
    if (!std::isfinite(value))
        return QStringLiteral("-");
    return QString::number(std::llround(value));
}

int CyHistogramView::niceCellStep(double raw) {
    if (!(raw > 1.0)) return 1;

    // 1/1.2/1.5/2/2.5/3/4/5/6/8/10 × 10ⁿ：既贴近"整数刻度"习惯，又不会过度放大范围
    static const double kMantissa[] = { 1.0, 1.2, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 8.0, 10.0 };
    const double p = std::pow(10.0, std::floor(std::log10(raw)));
    for (double m : kMantissa) {
        const double s = m * p;
        if (raw <= s + 1e-9) {
            return qMax(1, int(std::llround(s)));
        }
    }
    return qMax(1, int(std::llround(10.0 * p)));
}

void CyHistogramView::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    const bool dark = isDarkTheme();

    const QColor bgColor = dark ? QColor(0x26, 0x29, 0x2e) : QColor(0xfb, 0xfb, 0xfc);
    const QColor plotColor = dark ? QColor(0x1f, 0x22, 0x26) : QColor(0xff, 0xff, 0xff);

    painter.fillRect(rect(), bgColor);
    if (d->plot.width() < 8 || d->plot.height() < 8) {
        return;
    }
    painter.fillRect(d->plot, plotColor);

    drawGrid(painter);
    drawSeries(painter);
    drawSelection(painter);
    drawReadout(painter);

    painter.setClipping(false);
    const QColor borderColor = dark ? QColor(0x44, 0x49, 0x50) : QColor(0xd2, 0xd7, 0xde);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(d->plot.adjusted(0, 0, -1, -1));
}

void CyHistogramView::drawGrid(QPainter& painter) {
    const bool dark = isDarkTheme();
    const QColor gridColor = dark ? QColor(0x33, 0x38, 0x3e) : QColor(0xeb, 0xee, 0xf2);
    const QColor textColor = dark ? QColor(0x9a, 0xa2, 0xac) : QColor(0x5f, 0x67, 0x70);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setFont(d->labelFont);

    const QFontMetrics& fm = d->labelMetrics;
    const bool vertical = (d->orientation == Vertical);
    const int levelCells = levelCellPixels();
    const int countCells = countCellPixels();
    const int levelAxis = levelAxisPixels();
    const int countAxis = countAxisPixels();

    //---- 灰度轴格子：每条线代表一个"每格 xPerCell 个灰度级" ----
    const int levelLabelWidth = fm.horizontalAdvance(QStringLiteral("000000"));
    const int levelLabelStep = (levelCells >= levelLabelWidth + 10)
        ? 1 : ((levelCells * 2 >= levelLabelWidth + 10) ? 2 : 3);

    for (int i = 0;; ++i) {
        const int pos = i * levelCells;
        if (pos > levelAxis) break;

        const int x = vertical ? d->plot.left() : d->plot.left() + pos;
        const int y = vertical ? d->plot.bottom() - pos : d->plot.top();
        if (vertical ? (y < d->plot.top()) : (x > d->plot.right())) break;

        if (d->gridVisible) {
            painter.setPen(QPen(gridColor, 1));
            if (vertical) {
                painter.drawLine(d->plot.left(), y, d->plot.right(), y);
            }
            else {
                painter.drawLine(x, d->plot.top(), x, d->plot.bottom());
            }
        }

        if ((i % levelLabelStep) != 0) continue;
        if (!d->levelLabelsVisible) continue;
        //竖向固定轴按像素反算灰度值，避免格子刻度与精确铺满后的坐标不一致
        const double labelValue = (vertical && !d->xAdjustable)
            ? xValueAt(y)
            : (d->xOrigin + double(i) * double(d->xPerCell));
        const QString label = formatNumber(labelValue);
        const int w = fm.horizontalAdvance(label);
        painter.setPen(QPen(textColor, 1));
        if (vertical) {
            //灰度刻度画在绘图区左侧，文字水平书写
            const double baseline = double(y) + (fm.ascent() - fm.descent()) / 2.0;
            const double tx = qMax(0.0, double(d->marginLeft) - double(w) - 6.0);
            painter.drawText(QPointF(tx, baseline), label);
        }
        else {
            //用点坐标+基线绘制：QRect 版本会走文本布局/换行路径，实测比点坐标慢一个数量级
            painter.drawText(QPointF(x - w / 2.0, d->plot.bottom() + 3 + fm.ascent()), label);
        }
    }

    //---- 计数轴格子：每条线代表一个"每格 yPerCell 个计数" ----
    const int countLabelWidth = fm.horizontalAdvance(QStringLiteral("000M"));
    const int countLabelStep = (countCells >= countLabelWidth + 10) ? 1 : 2;
    const int rows = int(std::ceil(double(countAxis) / double(countCells)));
    for (int j = 0; j <= rows; ++j) {
        const int pos = j * countCells;
        if (pos > countAxis) break;

        const int x = vertical ? d->plot.left() + pos : d->plot.left();
        const int y = vertical ? d->plot.bottom() : d->plot.bottom() - pos;

        if (d->gridVisible) {
            painter.setPen(QPen(gridColor, 1));
            if (vertical) {
                painter.drawLine(x, d->plot.top(), x, d->plot.bottom());
            }
            else {
                painter.drawLine(d->plot.left(), y, d->plot.right(), y);
            }
        }

        if (j == 0 || (j % countLabelStep) != 0) continue;
        if (!d->countLabelsVisible) continue;
        const QString label = formatNumber(double(j) * double(d->yPerCell));
        const int w = fm.horizontalAdvance(label);
        painter.setPen(QPen(textColor, 1));
        if (vertical) {
            //计数刻度画在绘图区底部
            const double x0 = qMin(double(d->plot.right() - w), double(x) - w / 2.0);
            painter.drawText(QPointF(x0, d->plot.bottom() + 3 + fm.ascent()), label);
        }
        else {
            const double baseline = double(y) + (fm.ascent() - fm.descent()) / 2.0;
            const double tx = qMax(0.0, double(d->marginLeft) - double(w) - 4.0);
            painter.drawText(QPointF(tx, baseline), label);
        }
    }

    painter.restore();
}

void CyHistogramView::buildSeriesGeometry(int channel, bool asBars, bool asArea,
    QVector<QLineF>& lines, QPolygonF& outline, QPolygonF& area) const {
    const PrivateData::Series& s = d->series[channel];
    const int n = int(s.front.size());
    if (n <= 0 || d->plot.width() < 2 || d->plot.height() < 2)
        return;
    if (s.step <= 0)
        return;

    const double span = visibleSpanX();
    if (span <= 0.0)
        return;

    const bool vertical = (d->orientation == Vertical);
    const double xMin = d->xOrigin;
    const double xMax = xMin + span;
    const double base = double(s.base);
    const double step = double(s.step);
    //灰度轴方向上的可用像素长度（横向=宽度，竖向=高度）
    const int lanes = qMax(1, levelAxisPixels());
    //计数轴 0 值所在的像素（横向=底边，竖向=左边）
    const double basePix = pixelOfValueY(0.0);

    int i0 = int(std::ceil((xMin - base) / step));
    int i1 = int(std::floor((xMax - base) / step));
    i0 = qBound(0, i0, n - 1);
    i1 = qBound(-1, i1, n - 1);
    if (i1 < i0)
        return;

    //灰度值 → 沿灰度轴的像素；计数 → 沿计数轴的像素
    const auto levelPix = [this, vertical](double v) {
        const double p = pixelOfValueX(v);
        return vertical
            ? qBound(double(d->plot.top()), p, double(d->plot.bottom()))
            : qBound(double(d->plot.left()), p, double(d->plot.right()));
    };
    const auto countPix = [this, vertical](double v) {
        const double p = pixelOfValueY(v);
        return vertical
            ? qBound(double(d->plot.left()), p, double(d->plot.right()))
            : qBound(double(d->plot.top()), p, double(d->plot.bottom()));
    };

    //灰度值 → 沿灰度轴的像素 + 计数 → 沿计数轴的像素，按当前方向摆放
    const auto pushTop = [&](double levelPixel, double countPixel) {
        outline.append(vertical ? QPointF(countPixel, levelPixel) : QPointF(levelPixel, countPixel));
    };

    //可见桶数不超过像素列数：逐桶绘制（放大后能看到每一个灰度级）
    if (i1 - i0 + 1 <= lanes) {
        //每个桶占半格（不足半像素时兜底 0.5），相邻桶首尾相接，阶梯线不会断开
        const double halfBinPx = std::max(0.5, step / span * double(lanes) * 0.5);
        outline.reserve((i1 - i0 + 2) * 2);
        if (asBars) lines.reserve(i1 - i0 + 1);
        for (int i = i0; i <= i1; ++i) {
            const double lp = levelPix(base + double(i) * step);
            const double v = s.front[size_t(i)];
            const double cp = countPix(v);
            if (asBars) {
                if (v > 0.0)
                    lines.append(vertical ? QLineF(basePix, lp, cp, lp) : QLineF(lp, basePix, lp, cp));
            }
            else if (d->stepLine) {
                //直方图用阶梯线：每个桶是一个常量高度（横向）或常量宽度（竖向）的段，
                //避免放大后被误读成线性插值。
                //端点吸附到整数像素（灰度轴取 floor 保证相邻桶首尾同列，计数轴取 round），
                //否则相邻桶的竖直/水平段会在拐角处错开一行，1px 线出现"点状"断口，
                //填充与描边也会差半像素。
                const double lpS = std::floor(lp - halfBinPx);
                const double lpE = std::floor(lp + halfBinPx);
                const double cpS = std::round(cp);
                //竖向时灰度轴向上增长（像素 y 递减），必须按"屏幕顺序"压点，
                //否则同一桶的两个端点顺序反了，轮廓会走成锯齿+斜线。
                if (vertical) {
                    pushTop(lpE, cpS);
                    pushTop(lpS, cpS);
                }
                else {
                    pushTop(lpS, cpS);
                    pushTop(lpE, cpS);
                }
            }
            else {
                pushTop(lp, cp);
            }
        }
    }
    else {
        //缩小时按屏幕列（竖向时按屏幕行）做峰值保持：绘制量恒等于控件一条边的像素数
        int i = i0;
        outline.reserve(lanes * 2 + 2);
        if (asBars) lines.reserve(lanes);
        for (int c = 0; c < lanes && i <= i1; ++c) {
            const double lr = xMin + double(c + 1) * span / double(lanes);
            int end = int(std::floor((lr - base) / step));
            if (end > i1) end = i1;
            if (end < i) continue;

            double peak = 0.0;
            for (int k = i; k <= end; ++k) {
                const double v = s.front[size_t(k)];
                if (v > peak) peak = v;
            }
            i = end + 1;

            const double lp = vertical
                ? (double(d->plot.bottom()) - double(c) - 0.5)
                : (double(d->plot.left()) + double(c) + 0.5);
            const double cp = countPix(peak);
            if (asBars) {
                if (peak > 0.0)
                    lines.append(vertical ? QLineF(basePix, lp, cp, lp) : QLineF(lp, basePix, lp, cp));
            }
            else if (d->stepLine) {
                //降采样后每列正好 1px，lp±0.5 本身是整数，floor 只是保证类型与取整一致
                const double lpS = std::floor(lp - 0.5);
                const double lpE = std::floor(lp + 0.5);
                const double cpS = std::round(cp);
                if (vertical) {
                    pushTop(lpE, cpS);
                    pushTop(lpS, cpS);
                }
                else {
                    pushTop(lpS, cpS);
                    pushTop(lpE, cpS);
                }
            }
            else {
                pushTop(lp, cp);
            }
        }
    }

    //轮廓填充用一条闭合多边形：上沿 → 落到计数 0 基线 → 沿基线回到起点。
    //逐列画小矩形时相邻矩形各自取整会漏掉 1px 列（陡坡上能看到白缝）；
    //一次多边形填充没有接缝，光栅化也只需一遍扫描线（实测比逐列矩形快 6~8 倍）。
    if (!asArea || outline.size() < 2)
        return;


    area = outline;
    area.reserve(outline.size() + 2);
    const double lastLevel = vertical ? outline.back().y() : outline.back().x();
    const double firstLevel = vertical ? outline.front().y() : outline.front().x();
    if (vertical) {
        area.append(QPointF(basePix, lastLevel));
        area.append(QPointF(basePix, firstLevel));
    }
    else {
        area.append(QPointF(lastLevel, basePix));
        area.append(QPointF(firstLevel, basePix));
    }
}

void CyHistogramView::drawSeries(QPainter& painter) {
    int visibleSeries = 0;
    for (int i = 0; i < Ch_Count; ++i) {
        if (d->series[i].visible && !d->series[i].front.empty())
            ++visibleSeries;
    }
    if (visibleSeries <= 0)
        return;

    const bool dark = isDarkTheme();
    const int autoAlpha = qBound(30, 190 / visibleSeries, 170);

    painter.save();
    painter.setClipRect(d->plot);
    for (int ch = 0; ch < Ch_Count; ++ch) {
        const PrivateData::Series& s = d->series[ch];
        if (!s.visible || s.front.empty())
            continue;

        const int style = (s.style >= Style_Bars) ? s.style : int(d->style);
        QVector<QLineF> lines;
        QPolygonF outline;
        QPolygonF area;
        buildSeriesGeometry(ch, style == Style_Bars, style == Style_Area, lines, outline, area);

        const QColor color = ensureContrast(s.color, dark);
        if (style == Style_Bars) {
            if (lines.isEmpty()) continue;
            painter.setRenderHint(QPainter::Antialiasing, false);
            painter.setPen(QPen(color, 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawLines(lines);
        }
        else {
            if (style == Style_Area && area.size() >= 3) {
                //与阶梯折线等价的闭合多边形：一次扫描线填充，没有逐列矩形的接缝
                QColor fill = color;
                fill.setAlpha(d->areaAlpha >= 0 ? d->areaAlpha : autoAlpha);
                painter.setRenderHint(QPainter::Antialiasing, false);
                painter.setPen(Qt::NoPen);
                painter.setBrush(fill);
                painter.drawPolygon(area);
            }

            if (outline.size() < 2) continue;

            //抗锯齿描边是光栅引擎上最贵的操作（512 点纯 Qt 实测 ~1.25ms）。
            //只有放大到少量桶（点少）时才开启以保持观感，密集直方图用 1px 无抗锯齿折线。
            const bool useAa = outline.size() <= 64;
            painter.setRenderHint(QPainter::Antialiasing, useAa);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(color, useAa ? 1.4 : 1.0));
            painter.drawPolyline(outline);
        }
    }
    painter.restore();
}

void CyHistogramView::drawSelection(QPainter& painter) {
    if (!d->selEnabled)
        return;

    const bool dark = isDarkTheme();
    const bool vertical = (d->orientation == Vertical);
    const double a = pixelOfValueX(d->selStart);
    const double b = pixelOfValueX(d->selEnd);

    painter.save();
    painter.setClipRect(d->plot);
    painter.setRenderHint(QPainter::Antialiasing, false);

    //选区外压暗，突出被选中区间
    const QColor dim = dark ? QColor(0, 0, 0, 90) : QColor(255, 255, 255, 110);
    QColor fill = d->selectionColor;
    fill.setAlpha(dark ? 70 : 46);
    QColor edge = d->selectionColor;
    edge.setAlpha(235);

    if (vertical) {
        const double lo = qBound(double(d->plot.top()), qMin(a, b), double(d->plot.bottom()));
        const double hi = qBound(double(d->plot.top()), qMax(a, b), double(d->plot.bottom()));

        if (hi < d->plot.bottom())
            painter.fillRect(QRectF(d->plot.left(), hi, d->plot.width(), d->plot.bottom() - hi), dim);
        if (lo > d->plot.top())
            painter.fillRect(QRectF(d->plot.left(), d->plot.top(), d->plot.width(), lo - d->plot.top()), dim);

        painter.fillRect(QRectF(d->plot.left(), lo, d->plot.width(), qMax(1.0, hi - lo)), fill);

        painter.setPen(QPen(edge, 2));
        painter.drawLine(QPointF(d->plot.left(), lo), QPointF(d->plot.right(), lo));
        painter.drawLine(QPointF(d->plot.left(), hi), QPointF(d->plot.right(), hi));

        //左侧握把，提示整段可拖动
        QColor grip = edge;
        grip.setAlpha(120);
        painter.setPen(QPen(grip, 2));
        painter.drawLine(QPointF(d->plot.left() + 1, lo), QPointF(d->plot.left() + 1, hi));
    }
    else {
        const double xl = qBound(double(d->plot.left()), qMin(a, b), double(d->plot.right()));
        const double xr = qBound(double(d->plot.left()), qMax(a, b), double(d->plot.right()));

        if (xl > d->plot.left()) {
            painter.fillRect(QRectF(d->plot.left(), d->plot.top(), xl - d->plot.left(), d->plot.height()), dim);
        }
        if (xr < d->plot.right()) {
            painter.fillRect(QRectF(xr, d->plot.top(), d->plot.right() - xr, d->plot.height()), dim);
        }

        painter.fillRect(QRectF(xl, d->plot.top(), qMax(1.0, xr - xl), d->plot.height()), fill);

        painter.setPen(QPen(edge, 2));
        painter.drawLine(QPointF(xl, d->plot.top()), QPointF(xl, d->plot.bottom()));
        painter.drawLine(QPointF(xr, d->plot.top()), QPointF(xr, d->plot.bottom()));

        //顶部握把，提示整段可拖动
        QColor grip = edge;
        grip.setAlpha(120);
        painter.setPen(QPen(grip, 2));
        painter.drawLine(QPointF(xl, d->plot.top() + 1), QPointF(xr, d->plot.top() + 1));
    }

    painter.restore();
}

void CyHistogramView::drawReadout(QPainter& painter) {
    if (!d->readoutVisible || d->plot.width() < 80)
        return;

    const bool dark = isDarkTheme();
    const QColor textColor = dark ? QColor(0xd6, 0xdb, 0xe1) : QColor(0x3d, 0x44, 0x4d);

    QStringList names;
    QStringList values;
    QVector<QColor> colors;

    const QString xName = d->xTitle.isEmpty() ? tr("Horizontal axis") : d->xTitle;
    names << xName;
    values << formatCount(d->readoutX);
    colors << textColor;

    for (int ch = 0; ch < Ch_Count; ++ch) {
        const PrivateData::Series& s = d->series[ch];
        if (!s.visible || s.front.empty())
            continue;
        names << (s.name.isEmpty() ? QStringLiteral("-") : s.name);
        values << formatCount(seriesValueAt(ch, d->readoutX));
        colors << ensureContrast(s.color, dark);
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(d->labelFont);
    const QFontMetrics& fm = d->labelMetrics;

    int nameWidth = 0;
    int valueWidth = 0;
    for (int i = 0; i < names.size(); ++i) {
        nameWidth = qMax(nameWidth, fm.horizontalAdvance(names[i]));
        valueWidth = qMax(valueWidth, fm.horizontalAdvance(values[i]));
    }

    const int lineHeight = fm.height() + 1;
    const int padX = 7;
    const int padY = 5;
    const int colGap = 10;
    const int boxWidth = qMin(d->plot.width() - 16, padX * 2 + nameWidth + colGap + valueWidth);
    const int boxHeight = padY * 2 + lineHeight * names.size();

    QRectF box(d->plot.left() + 10, d->plot.top() + 10, boxWidth, boxHeight);

    QColor boxBg = dark ? QColor(0x2a, 0x2e, 0x33, 232) : QColor(0xff, 0xff, 0xff, 232);
    QColor boxBorder = dark ? QColor(0x45, 0x4b, 0x52) : QColor(0xdc, 0xe1, 0xe7);
    painter.setPen(QPen(boxBorder, 1));
    painter.setBrush(boxBg);
    painter.drawRoundedRect(box, 4, 4);

    int y = int(box.top()) + padY;
    for (int i = 0; i < names.size(); ++i) {
        const double baseline = double(y) + (lineHeight - fm.height()) / 2.0 + fm.ascent();
        painter.setPen(QPen(i == 0 ? textColor : colors[i], 1));
        painter.drawText(QPointF(box.left() + padX, baseline), names[i]);
        painter.drawText(QPointF(box.left() + padX + nameWidth + colGap
            + valueWidth - fm.horizontalAdvance(values[i]), baseline), values[i]);
        y += lineHeight;
    }

    painter.restore();
}
