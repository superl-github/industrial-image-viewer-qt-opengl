/**
 * @file CyHistogramView.h
 * @brief 高性能自绘直方图/曲线控件（柱形、折线、半透明轮廓三种风格，支持横向/竖向渲染）。
 * @ingroup Display
 *
 * @details
 *  本控件替代原先基于 QCustomPlot 的直方图绘制路径，面向"每帧都要刷新"的工业
 *  图像面板场景，设计目标：
 *  - **快**：数据在工作线程写入后台缓冲，GUI 线程交换后直接按屏幕列采样绘制，
 *    不做任何 QCP 数据容器/图元/选择测试；
 *  - **准**：横轴上"一个格子固定像素长度"，每格代表多少灰度级/计数由编辑框
 *    精确设定，X/Y 轴最大范围可通过接口限制；
 *  - **现代**：三种显示风格（竖线柱形、折线、折线 + 半透明覆盖区域），
 *    暗色/亮色主题自适应，鼠标位置读数固定显示在左上角而不是跟随鼠标的
 *    ToolTip 气泡。
 *
 *  ## 渲染方向（竖向渲染机制）
 *  - `Horizontal`（默认）：灰度级沿横轴，计数沿纵轴，柱子自下向上生长；
 *  - `Vertical`：灰度级沿纵轴（自下而上，0 在最底），计数沿横轴（自左向右），
 *    柱子自左向右生长。适用于"图像右侧灰度映射条旁挂一个竖向直方图"这类布局
 *    （灰度拉伸面板）——此时 `setXAdjustable(false)` 可让灰度轴固定铺满量程，
 *    同时保留计数方向的自适应。
 *  - 坐标轴文字始终水平绘制，不随方向旋转，保证可读性。
 *
 *  ## 坐标轴模型
 *  - 横向：格子宽度固定为 `cellPixelSize().width()` 像素，每格代表
 *    `xPerCell()` 个灰度级（≥1）；可视范围为
 *    `[xOrigin, xOrigin + 格子数 * xPerCell]`；
 *  - 纵向：格子高度固定为 `cellPixelSize().height()` 像素，每格代表
 *    `yPerCell()` 个计数；计数轴始终从 0 开始（直方图计数非负）；
 *  - `setAutoX` / `setAutoY` 打开时，按数据范围自动求每格数值（自动取整到
 *    1/2/3/4/5/8×10ⁿ 这类"好看的刻度"），用户手动缩放后自动关闭。
 *  - `Vertical` 只是交换两条轴在屏幕上的朝向，`x*` 系列接口始终描述"灰度级"
 *    （数值轴），`y*` 系列接口始终描述"计数"（数值轴），语义不随方向变化。
 *
 *  ## 采样策略
 *  可见桶数 ≤ 像素列数时逐桶绘制（放大后能看清每一个灰度级）；
 *  否则按屏幕列做"峰值保持"降采样，每列只保留该列内的最大计数，
 *  既保留尖峰又让绘制量恒等于控件宽度（竖向模式下按屏幕行降采样）。
 *
 *  ## 轮廓填充
 *  `Style_Area` 把整条采样轨迹构成**一条闭合多边形**（上沿 → 计数 0 基线 →
 *  回到起点），一次 `drawPolygon()` 填充；描边复用同一条上沿折线，填充与轮廓
 *  始终严丝合缝。不逐桶画小矩形的原因：相邻矩形各自取整会漏掉 1px 列，陡坡
 *  上能看到白缝，而且逐矩形绘制要走 N 次光栅填充入口，比单条多边形慢 6~8 倍。
 *
 *  ## 线程模型
 *  `setSeriesData()` 可从任意线程调用；内部互斥量保护后台缓冲，并通过排队
 *  调用把刷新请求投递到控件所属线程。其余接口必须在 GUI 线程调用。
 *
 *  @note 本控件需要以 CyMedia 动态库形式对外复用，故类声明使用 `CYMEDIA_LIB`
 *        （由 `CYMEDIA_EXPORT` 宏决定 dllexport / dllimport）。
 *
 *  @author LLF
 *  @date   November 2026
 *  @version 2.2
 */
#pragma once

#include "../../common/Export.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QLineF>
#include <QMutex>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QWidget>

#include <atomic>
#include <vector>

class QPainter;

class CYMEDIA_LIB CyHistogramView : public QWidget {
    Q_OBJECT

public:
    /// 渲染方向。
    enum Orientation {
        Horizontal = 0, ///< 横向：灰度级为横轴、计数为纵轴（默认）
        Vertical = 1,   ///< 竖向：灰度级为纵轴（自下而上）、计数为横轴（自左向右）
    };

    /// 显示风格。
    enum DisplayStyle {
        Style_Bars = 0, ///< 柱形：每个采样点画一条竖直线（等价柱状图，但无需计算矩形）
        Style_Line = 1, ///< 折线：把采样点连成折线
        Style_Area = 2, ///< 半透明轮廓：折线 + 下方覆盖区域（单条闭合多边形填充，任务管理器 CPU 图风格）
    };

    /// 颜色通道索引，与灰度测量/拉伸面板的通道顺序保持一致。
    enum Channel {
        Ch_Gray = 0,
        Ch_Red = 1,
        Ch_Green = 2,
        Ch_Blue = 3,
        Ch_Count = 4,
    };

    /// `setSeriesStyle()` 的特殊值：跟随控件的整体显示风格。
    static const int Style_Inherit = -1;

public:
    explicit CyHistogramView(QWidget* parent = nullptr);
    ~CyHistogramView() override;

    CyHistogramView(const CyHistogramView&) = delete;
    CyHistogramView& operator=(const CyHistogramView&) = delete;

    //==================== 数据（线程安全） ====================
    /**
     * @brief 设置某个通道的直方图数据（计数）。
     * @param channel 通道索引（Ch_Gray..Ch_Blue）。
     * @param values  计数数组，values[i] 对应灰度级 `binBase + i * binStep`。
     * @param count   桶数量。
     * @param binBase 第一个桶对应的灰度级。
     * @param binStep 相邻桶的灰度级步长（通常为 1）。
     * @note 可从工作线程调用；内部拷贝数据，调用方无需保留缓冲。
     */
    void setSeriesData(int channel, const double* values, int count, int binBase = 0, int binStep = 1);

    /// 清空某个通道的数据。
    void clearSeries(int channel);
    /// 清空所有通道的数据。
    void clearAllSeries();

    /// 设置通道直方图可见性
    void setSeriesVisible(int channel, bool visible);
    /// 通道直方图可见性
    bool isSeriesVisible(int channel) const;

    /// 设置通道直方图颜色
    void setSeriesColor(int channel, const QColor& color);
    /// 获取通道直方图颜色
    QColor seriesColor(int channel) const;

    /// 设置单通道显示风格；`Style_Inherit` 表示跟随 `displayStyle()`。
    void setSeriesStyle(int channel, int style);
    int seriesStyle(int channel) const;

    /// 设置通道在读数/图例中显示的名称（由上层用 tr() 传入）。
    void setChannelName(int channel, const QString& name);
    QString channelName(int channel) const;

    //==================== 外观 ====================
    /// 设置直方图类型
    void setDisplayStyle(DisplayStyle style);
    /// 获取当前显示直方图类型
    DisplayStyle displayStyle() const;

    /// 轮廓风格下覆盖区域的透明度（-1 表示按可见通道数自动取值）。
    void setAreaAlpha(int alpha);
    int areaAlpha() const;

    /// 左上角固定读数的显示开关。
    void setReadoutVisible(bool visible);
    bool readoutVisible() const;

    /// 是否显示格子网格。
    void setGridVisible(bool visible);
    bool gridVisible() const;

    /**
     * @brief 坐标轴刻度文字开关（网格线仍由 `setGridVisible` 控制）。
     * @details 竖向直方图嵌在灰度映射条旁时通常不需要刻度文字，可单独关闭。
     */
    void setAxisLabelsVisible(bool levelLabels, bool countLabels);
    bool levelLabelsVisible() const;
    bool countLabelsVisible() const;

    /**
     * @brief 折线/轮廓是否用阶梯线（直方图语义：每个桶是常量高度）。
     * @details 直方图应保持 true；时序数据（如点模式的采样序列）设为 false，
     *          按样本点直线连接更自然。
     */
    void setStepLine(bool enabled);
    bool stepLine() const;

    /// 主题色（选区、选中状态等）。
    void setThemeColor(const QColor& color);
    QColor themeColor() const;

    /// 坐标轴标题，同时用于左上角读数第一行。
    void setAxisTitles(const QString& xTitle, const QString& yTitle);

    //==================== 渲染方向 ====================
    /**
     * @brief 设置渲染方向（竖向渲染入口）。
     * @details 仅改变两条轴在屏幕上的朝向，不改变数值语义：
     *          `x*` 永远描述灰度级/数值轴，`y*` 永远描述计数/幅度轴。
     *          切换方向后内部会重新计算边距、自动缩放并立即重绘。
     */
    void setOrientation(Orientation orientation);
    /// 当前渲染方向。
    Orientation orientation() const;

    //==================== 坐标轴 / 格子 ====================
    void setCellPixelSize(const QSize& size);
    QSize cellPixelSize() const;

    int xPerCell() const;
    /// 设置每格灰度级数量（≥1），`keepAnchor` 为真时保持锚点处的灰度值不动。
    void setXPerCell(int levels, bool keepAnchor = true);

    int yPerCell() const;
    /// 设置每格计数数量（≥1），`keepAnchor` 为真时保持锚点处的计数不动。
    void setYPerCell(int counts, bool keepAnchor = true);

    /// X 轴最小范围（灰度的下界，通常为 0）。
    void setXMinRange(int minLevels);
    int xMinRange() const;

    /// X 轴最大范围（内部接口）：灰度级上限，例如 8bit 为 255。
    void setXMaxRange(int maxLevels);
    int xMaxRange() const;

    /// Y 轴最大范围（内部接口）：每格计数的上限，用于限制自动/手动缩放。
    void setYMaxRange(double maxCount);
    double yMaxRange() const;

    bool autoX() const;
    void setAutoX(bool on);
    bool autoY() const;
    void setAutoY(bool on);

    /**
     * @brief 是否允许用户调整 X 轴（缩放/平移/设置每格数值）。
     * @details 关闭后 X 轴始终自动铺满 `[xMinRange, xMaxRange]`，
     *          滚轮/拖动/`setXPerCell` 都不再改变它（灰度拉伸面板使用）。
     */
    void setXAdjustable(bool adjustable);
    bool xAdjustable() const;

    /// 自动 Y 时使用的参考峰值；≤0 表示由数据自身决定。
    void setYHintMax(double value);
    double yHintMax() const;

    /// 按 X 轴范围重新铺满格子宽度。
    void fitX();
    /// 以控件中心为锚点缩放（factor<1 放大，>1 缩小）。
    void zoomX(double factor);
    void zoomY(double factor);
    /// 恢复自动 X / 自动 Y。
    void resetView();

    //==================== 选区（灰度拉伸遮盖罩） ====================
    void setSelectionEnabled(bool enabled);
    bool isSelectionEnabled() const;
    void setSelection(double start, double end);
    QPointF selection() const;
    void setSelectionMinWidth(double width);
    void setSelectionColor(const QColor& color);

    //==================== 查询 ====================
    /// 像素坐标 → X/Y 数值。
    double xValueAt(int pixelX) const;
    double yValueAt(int pixelY) const;
    /// 某个通道在 x 处（最近桶）的计数；无数据或越界返回 0。
    double seriesValueAt(int channel, double x) const;
    /// 绘图区（不含坐标轴标签）矩形。
    QRect plotRect() const;

    /**
     * @brief 数值 → 控件像素坐标。
     * @param level 灰度级/数值轴的值。
     * @param count 计数/幅度轴的值。
     * @return 与当前渲染方向一致的点；供上层绘制阈值线、标记等叠加图形。
     */
    QPointF pixelOfValue(double level, double count) const;

signals:
    /// 用户拖动/编辑选区后发出（程序调用 setSelection 不会触发）。
    void selectionChanged(double start, double end);
    /// 自动模式被用户操作改变。
    void autoYChanged(bool on);
    void autoXChanged(bool on);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void changeEvent(QEvent* event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    void onDataReady();
    void updateLayout();
    bool isDarkTheme() const;

    /// 灰度轴（数值轴）一个格子在屏幕上的像素长度。
    int levelCellPixels() const;
    /// 计数轴（幅度轴）一个格子在屏幕上的像素长度。
    int countCellPixels() const;
    /// 当前方向下，画笔坐标系里"灰度轴方向"的可用像素长度。
    int levelAxisPixels() const;
    /// 当前方向下，画笔坐标系里"计数轴方向"的可用像素长度。
    int countAxisPixels() const;

    QFont axisFont() const;
    const QFontMetrics& axisMetrics() const;
    void drawGrid(QPainter& painter);
    void drawSeries(QPainter& painter);
    void drawSelection(QPainter& painter);
    void drawReadout(QPainter& painter);

    /**
     * @brief 构造一条通道的绘制几何。
     * @param asBars  为真时只生成柱形竖线（`lines`）。
     * @param asArea  为真时额外生成闭合填充多边形（`area`）。
     * @param outline 上沿/左沿折线，描边用；阶梯模式下每桶两个端点。
     * @param area    闭合多边形：`outline` + 落到基线再沿基线返回的两个点。
     */
    void buildSeriesGeometry(int channel, bool asBars, bool asArea,
        QVector<QLineF>& lines, QPolygonF& outline, QPolygonF& area) const;

    double pixelOfValueX(double value) const;
    double pixelOfValueY(double value) const;
    double visibleSpanX() const;
    double visibleSpanY() const;
    int maxXPerCell() const;
    int maxYPerCell() const;

    /// 鼠标/像素坐标里"灰度轴方向"的坐标（横向取 x，竖向取 y）。
    double levelCoordOf(const QPoint& pos) const;
    double levelCoordOf(int pixelX, int pixelY) const;
    /// 计数轴方向的像素长度（用于把拖动像素换算成数值）。
    int countAxisPixelLength() const;

    void clampX();
    void applyAutoX(bool force);
    void applyAutoY(bool force);

    int hitTestSelection(const QPoint& pos) const; ///< 0 无, 1 左, 2 右, 3 主体

    static QString formatNumber(double value);
    static QString formatCount(double value);
    static int niceCellStep(double raw);

private:
    struct PrivateData;
    PrivateData* d = nullptr;
};
