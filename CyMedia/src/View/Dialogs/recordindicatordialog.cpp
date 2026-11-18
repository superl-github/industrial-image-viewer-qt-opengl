#include "recordindicatordialog.h"
#include "../Widgets/ledindicator.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

RecordIndicatorDialog::RecordIndicatorDialog(const QString& text, QWidget* parent)
    : QDialog(parent) {
    // ---- 窗口属性：无边框 / 置顶 / 不抢焦点 / 不进任务栏 ----
    setWindowFlags(Qt::FramelessWindowHint
        | Qt::WindowStaysOnTopHint
        | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    // ---- LED ----
    m_led = new LedIndicator(10, this);

    // ---- 文本 ----
    m_label = new QLabel(text, this);
    m_label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_label->setAttribute(Qt::WA_TranslucentBackground, true);

    QPalette pal = m_label->palette();
    pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
    m_label->setPalette(pal);

    QFont font = m_label->font();
    font.setBold(true);
    font.setPointSize(14);
    m_label->setFont(font);

    // ---- 布局 ----
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 14, 6);
    layout->setSpacing(8);
    layout->addWidget(m_led, 0, Qt::AlignVCenter);
    layout->addWidget(m_label, 0, Qt::AlignVCenter);

    adjustSize();
}

// ==================== 用户接口 ====================

void RecordIndicatorDialog::updateText(const QString& text, bool blink/* = false*/) {
    m_label->setText(text);
    if (blink) m_led->toggle();            // 亮->灭 / 灭->亮
    adjustSize();
}

bool RecordIndicatorDialog::isLedVisible() const {
    // 用 isHidden() 而不是 isVisible()：
    // 窗口还没 show 时 isVisible() 恒为 false，无法反映“是否被显式隐藏”。
    return !m_led->isHidden();
}

void RecordIndicatorDialog::setLedVisible(bool visible) {
    m_led->setVisible(visible);
    adjustSize();
}

bool RecordIndicatorDialog::isDraggable() {
    return m_Draggable;
}

void RecordIndicatorDialog::setDraggable(bool flag) {
    m_Draggable = flag;
}

// ==================== 外观配置 ====================

void RecordIndicatorDialog::setBackgroundColor(const QColor& color) {
    m_bgColor = color;
    update();
}

void RecordIndicatorDialog::setTextColor(const QColor& color) {
    QPalette pal = m_label->palette();
    pal.setColor(QPalette::WindowText, color);
    m_label->setPalette(pal);
}

void RecordIndicatorDialog::setTextFont(const QFont& font) {
    m_label->setFont(font);
    adjustSize();
}

void RecordIndicatorDialog::setLedColor(const QColor& color) {
    m_led->setOnColor(color);
    QColor off = color;
    off.setAlpha(60);
    m_led->setOffColor(off);
}

void RecordIndicatorDialog::setCornerRadius(int radius) {
    m_radius = qMax(0, radius);
    update();
}

// ==================== 绘制背景 ====================

void RecordIndicatorDialog::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_bgColor);

    const QRectF r(rect());
    if (m_radius > 0)
        painter.drawRoundedRect(r, m_radius, m_radius);
    else
        painter.drawRect(r);
}

// ==================== 鼠标拖动（可选） ====================

void RecordIndicatorDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPos() - pos();
        event->accept();
    }
    else {
        QDialog::mousePressEvent(event);
    }
}

void RecordIndicatorDialog::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton) && m_Draggable) {
        move(event->globalPos() - m_dragOffset);
        event->accept();
    }
    else {
        QDialog::mouseMoveEvent(event);
    }
}

void RecordIndicatorDialog::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}