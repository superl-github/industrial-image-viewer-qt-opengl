#pragma once
#include <QDialog>
#include <QColor>
#include <QPoint>

class QLabel;
class LedIndicator;

/**
 * 无边框录制提示框：左边 LED，右边文本。
 * 接口：
 *   updateText(text)     更新文本，同时翻转 LED 亮/灭
 *   setLedVisible(bool)  设置 LED 可见性（不是亮/灭状态）
 *   isLedVisible()       查询 LED 是否可见
 */
class RecordIndicatorDialog : public QDialog {
    Q_OBJECT
public:
    explicit RecordIndicatorDialog(const QString& text = QString(),
        QWidget* parent = nullptr);

    // ---- 用户接口 ----
    void updateText(const QString& text, bool blink = false);
    
    bool isLedVisible() const;
    void setLedVisible(bool visible);

    bool isDraggable();
    void setDraggable(bool flag);
    
    // ---- 外观配置 ----
    void setBackgroundColor(const QColor& color);
    void setTextColor(const QColor& color);
    void setTextFont(const QFont& font);
    void setLedColor(const QColor& color);
    void setCornerRadius(int radius);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    LedIndicator* m_led = nullptr;
    QLabel* m_label = nullptr;

    QColor m_bgColor{ 0, 0, 0, 160 };   // 背景色（含 alpha）
    int    m_radius = 8;              // 圆角半径

    bool m_Draggable = false;
    QPoint m_dragOffset;
    bool   m_dragging = false;
};