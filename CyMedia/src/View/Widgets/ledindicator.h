#pragma once
#include <QWidget>
#include <QColor>

/**
 * 圆形 LED 指示灯，只有“亮”和“灭”两种状态。
 * 注意：亮/灭状态与控件的可见性是两个独立概念。
 */
class LedIndicator : public QWidget {
    Q_OBJECT
public:
    explicit LedIndicator(int diameter = 10, QWidget* parent = nullptr);

    bool isOn() const;
    void setOn(bool on);
    void toggle();

    void setOnColor(const QColor& color);
    void setOffColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int    m_margin = 2;          // 预留光晕空间
    int    m_diameter = 10;
    bool   m_on = false;
    QColor m_onColor{ 255, 59, 48 };        // 亮：录制红
    QColor m_offColor{ 255, 59, 48, 60 };   // 灭：同色暗化
};