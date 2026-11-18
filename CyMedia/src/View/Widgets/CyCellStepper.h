/**
 * @file CyCellStepper.h
 * @brief "标签 + 数值"融合显示，配一对 ×2 / ÷2 步进按钮的紧凑控件。
 * @ingroup Display
 *
 * @details
 *  用于直方图坐标轴的"每格数值"设置，替代 QSpinBox：
 *  - 数值直接显示在标签里（例如 `X/格 25`），不再单独占用输入框；
 *  - 只有一个按钮组：点击 = 原值 ×2（放大）或 ÷2（缩小），到达上下限时按钮自动置灰；
 *  - 两种排列：
 *    - `Horizontal`：按钮左右排列，**左减右加**（X 轴格子）；
 *    - `Vertical`：按钮上下排列，**上加下减**（Y 轴格子）。
 *
 *  数值本身仍由使用方校验/收敛（例如直方图控件会把每格数值夹到合法范围），
 *  控件只负责"×2/÷2、显示、上下限置灰"。
 *
 *  @author LLF
 *  @date   November 2026
 *  @version 1.0
 */
#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

class QLabel;
class QToolButton;

class CyCellStepper : public QWidget {
    Q_OBJECT

public:
    /// 按钮组排列方式。
    enum Orientation {
        Horizontal = 0, ///< 左右排列：左减右加
        Vertical = 1,   ///< 上下排列：上加下减
    };

public:
    explicit CyCellStepper(Orientation orientation = Horizontal, QWidget* parent = nullptr);
    ~CyCellStepper() override;

    CyCellStepper(const CyCellStepper&) = delete;
    CyCellStepper& operator=(const CyCellStepper&) = delete;

    /// 标签文字（会与数值拼成一行，例如 "X/cell 25"）。
    void setCaption(const QString& caption);
    QString caption() const;

    /// 设置当前数值；不会发出 valueChanged。
    void setValue(int value);
    int value() const;

    void setRange(int minimum, int maximum);
    int minimum() const;
    int maximum() const;

    void setThemeColor(const QColor& color);
    QColor themeColor() const;

signals:
    /// 用户点击按钮导致数值变化后发出（已包含 ×2/÷2 与上下限收敛）。
    void valueChanged(int value);

protected:
    void changeEvent(QEvent* event) override;

private:
    void applyStep(bool zoomIn);
    void refresh();
    void applyStyle();

private:
    struct PrivateData;
    PrivateData* d = nullptr;
};
