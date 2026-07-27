/**
 * @file CyMediaDisGrayStretch.h
 * @brief 灰度拉伸（对比度增强）交互控件
 *
 * @details
 * 本类提供了一个基于直方图的可视化交互控件，用于调整图像的灰度/亮度拉伸范围，
 * 以增强图像对比度。核心功能包括：
 *
 * 1. 直方图展示
 *    - 支持 Mono、Bayer、RGB、YUV 等多种图像格式的灰度/亮度直方图绘制。
 *    - 直方图 Y 轴范围自动适配，并支持可选的缩放/拖动（默认禁用）。
 *
 * 2. 拉伸区间交互
 *    - 在直方图上叠加一个可拖拽的矩形选区，选区左右边界分别对应拉伸的起始和结束值。
 *    - 用户可通过鼠标拖拽整体平移选区，或拖拽左右边缘调整区间宽度。
 *    - 选区宽度受最小限制（默认 10 个灰度级），防止无效区间。
 *    - 同时提供 QSpinBox 微调框，支持键盘精确输入起始/结束值。
 *
 * 3. 自动拉伸
 *    - 提供开关按钮，开启后自动根据直方图分布计算最优拉伸范围（由 CyMediaCalc 库实现），
 *      并自动覆盖手动选区。
 *
 * 4. 拉伸类型选择
 *    - 支持三种拉伸策略：Gray（灰度直接拉伸）、HSV（基于 V 亮度通道）、Lab（基于 L* 明度通道）。
 *    - 针对彩色图像，可有效避免拉伸导致的色偏问题。
 *
 * @note 设计要点
 *    - 本控件不直接修改图像数据，仅输出拉伸参数（起始/结束值）和拉伸类型。
 *    - 外部模块需监听 stretchParaChange 信号，获取参数后执行实际的像素拉伸计算。
 *    - 控件内部通过 needImage 信号主动请求外部更新图像数据（当显示或自动拉伸状态变化时）。
 *
 * @note 交互细节
 *    - 鼠标悬停在直方图曲线上时会显示 ToolTip，提示当前灰度值和像素数量。
 *    - 鼠标移至选区边缘时光标变为水平双向箭头，移至内部时变为移动十字箭头。
 *
 * @note 外部依赖
 *    - 依赖 QCustomPlot 进行直方图绘制。
 *    - 依赖 CyMediaCalc 命名空间下的直方图计算和自动拉伸参数计算函数。
 *
 * @see CyMediaCalc::computeGrayHistogram, CyMediaCalc::computeGrayStretchPara
 * 
 * @author LLF
 * @date   July 2026
 * @version 1.0
 */
#pragma once
#include "../CyMediaBaseDef.h"

#include <QWidget>
#include <QTabWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QBoxLayout>

class CyMediaDisGrayStretch : public QWidget {
    Q_OBJECT

public:
    struct StretchValue {
        int32_t start;
        int32_t end;
        int32_t max;
    };

public:
    CyMediaDisGrayStretch(QWidget* parent = nullptr);
    ~CyMediaDisGrayStretch();

public:signals:
    void needImage();
    void stretchParaChange();

private:signals:
    void transImageType(int index);
    void upStretchRange(int start, int end);
    void upHisRange(int minX, int maxX, int maxY);
    void upEditRange(bool bitChange = false);

public:
    void flushTrans();
    void setThemeColor(QColor color);

    //histogram
    bool upImageData(CyMedia::ImageShowInfo& info, uint8_t* data, CyMedia::ImageColorOpe colorOpe);

    bool isZoomble();
    void setZoomble(bool zoom);

    bool axisToolTipVisible();
    void setAxisToolTipVisible(bool visi);

    //stretch
    CyMedia::StretchType stretchtype();

    StretchValue stretchValue();

    bool isAutoStretch();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event)override;
    void showEvent(QShowEvent* event)override;

    bool mouseOnSelectRect(QEvent* event);

    void plotMouseMove(QPoint mousePos);
    bool plotToolTips(double xValue, double yValue);

private:
    void onStartValueEdit();
    void onEndValueEdit();

    void onAutoStretchChange(bool enable);
    void onStretchTypeChange(int idx);

    void upSelectRectChange(int32_t start, int32_t end, bool needSignals = true);
    void onUpSelectRectRange(int32_t start, int32_t end);
    void onUpEditorNumber(int32_t  start, int32_t end);

    void updateCursor(const QPointF& pos);

private:
    void initGUI();
    QString getPosToolTip(double xValue, double yValue);

    void onTransImageType(int index);
    void onUpStretchRange(int start, int end);
    void onUpHisRange(int minX, int maxX, int maxY);
    void onUpEditRange(bool bitChange = false);

private:
    struct PrivateData;
    PrivateData* d = nullptr;
};