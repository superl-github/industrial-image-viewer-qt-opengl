#include "ledindicator.h"

#include <QPainter>

LedIndicator::LedIndicator(int diameter, QWidget* parent)
    : QWidget(parent)
    , m_diameter(diameter) {
    setFixedSize(m_diameter + 2 * m_margin, m_diameter + 2 * m_margin);
    // 鼠标事件穿透，避免挡住对话框拖动
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

bool LedIndicator::isOn() const {
    return m_on;
}

void LedIndicator::setOn(bool on) {
    if (on != m_on) {
        m_on = on;
        update();
    }
}

void LedIndicator::toggle() {
    setOn(!m_on);
}

void LedIndicator::setOnColor(const QColor& color) {
    m_onColor = color;
    update();
}

void LedIndicator::setOffColor(const QColor& color) {
    m_offColor = color;
    update();
}

void LedIndicator::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    const QRectF full(rect());
    const QRectF core = full.adjusted(m_margin, m_margin, -m_margin, -m_margin);

    if (m_on) {                             // 亮起时加一圈淡淡的光晕
        QColor glow = m_onColor;
        glow.setAlpha(60);
        painter.setBrush(glow);
        painter.drawEllipse(full);
    }

    painter.setBrush(m_on ? m_onColor : m_offColor);
    painter.drawEllipse(core);
}