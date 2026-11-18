#include "CyCellStepper.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
    const int kBtnW = 22;
    const int kBtnH = 18;
    const int kBtnWVertical = 22;
    const int kBtnHVertical = 13;
    const int kGapVertical = 3;

    QColor blend(const QColor& base, const QColor& over, double ratio)
    {
        const double r = ratio;
        return QColor(
            int(base.red() * (1.0 - r) + over.red() * r),
            int(base.green() * (1.0 - r) + over.green() * r),
            int(base.blue() * (1.0 - r) + over.blue() * r),
            base.alpha());
    }
}

struct CyCellStepper::PrivateData {
    Orientation orientation = Horizontal;
    QString caption;
    int value = 1;
    int minimum = 1;
    int maximum = 1000000;
    int reservedWidth = 44;
    QColor themeColor = QColor(0x2a, 0xa3, 0xc6);

    QLabel* label = nullptr;
    QToolButton* minusBtn = nullptr;
    QToolButton* plusBtn = nullptr;

    //防止"样式变化 -> 重新设置样式 -> 再次收到样式变化"的无限递归
    bool applyingStyle = false;
    QString lastStyle;
};

CyCellStepper::CyCellStepper(Orientation orientation, QWidget* parent /*= nullptr*/)
    : QWidget(parent)
    , d(new PrivateData) {
    d->orientation = orientation;

    d->label = new QLabel(this);
    //右对齐：数值紧贴按钮，预留宽度只影响左侧空白，数值变化时按钮不移动
    d->label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont f = d->label->font();
    f.setBold(true);
    d->label->setFont(f);
    d->label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    auto makeButton = [this](const QString& text, const QString& tip) {
        auto* btn = new QToolButton(this);
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setAutoRaise(true);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setCursor(Qt::PointingHandCursor);
        QFont bf = btn->font();
        bf.setBold(true);
        bf.setPointSizeF(qMax(8.0, bf.pointSizeF()));
        btn->setFont(bf);
        return btn;
    };

    d->plusBtn = makeButton(QStringLiteral("+"), QStringLiteral("\u00d72"));
    d->minusBtn = makeButton(QStringLiteral("\u2212"), QStringLiteral("\u00f72"));

    if (d->orientation == Horizontal) {
        //左减右加
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(3);
        layout->addWidget(d->label);
        layout->addWidget(d->minusBtn);
        layout->addWidget(d->plusBtn);

        d->minusBtn->setFixedSize(kBtnW, kBtnH);
        d->plusBtn->setFixedSize(kBtnW, kBtnH);
    }
    else {
        //上加下减
        auto* buttons = new QVBoxLayout();
        buttons->setContentsMargins(0, 0, 0, 0);
        buttons->setSpacing(kGapVertical);
        buttons->addWidget(d->plusBtn);
        buttons->addWidget(d->minusBtn);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
        layout->addWidget(d->label);
        layout->addLayout(buttons, 0);

        d->minusBtn->setFixedSize(kBtnWVertical, kBtnHVertical);
        d->plusBtn->setFixedSize(kBtnWVertical, kBtnHVertical);
    }

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(d->plusBtn, &QToolButton::clicked, this, [this]() { applyStep(true); });
    connect(d->minusBtn, &QToolButton::clicked, this, [this]() { applyStep(false); });

    applyStyle();
    refresh();
}

CyCellStepper::~CyCellStepper() {
    delete d;
    d = nullptr;
}

void CyCellStepper::setCaption(const QString& caption) {
    d->caption = caption;
    //按"标签 + 6 位数值"预留宽度：避免自动 Y 变化时按钮左右跳动，
    //更长的数值仍可让 QLabel 按 sizeHint 自然变宽（不会截断）。
    const QFontMetrics fm(d->label->font());
    const QString sample = caption.isEmpty()
        ? QStringLiteral("000000")
        : QStringLiteral("%1 000000").arg(caption);
    d->reservedWidth = fm.horizontalAdvance(sample) + 4;
    refresh();
}

QString CyCellStepper::caption() const {
    return d->caption;
}

void CyCellStepper::setValue(int value) {
    value = qBound(d->minimum, value, d->maximum);
    if (d->value == value)
        return;
    d->value = value;
    refresh();
}

int CyCellStepper::value() const {
    return d->value;
}

void CyCellStepper::setRange(int minimum, int maximum) {
    if (maximum < minimum) maximum = minimum;
    if (d->minimum == minimum && d->maximum == maximum)
        return;
    d->minimum = minimum;
    d->maximum = maximum;
    d->value = qBound(d->minimum, d->value, d->maximum);
    refresh();
}

int CyCellStepper::minimum() const {
    return d->minimum;
}

int CyCellStepper::maximum() const {
    return d->maximum;
}

void CyCellStepper::setThemeColor(const QColor& color) {
    d->themeColor = color;
    applyStyle();
}

QColor CyCellStepper::themeColor() const {
    return d->themeColor;
}

void CyCellStepper::applyStep(bool zoomIn) {
    const int old = d->value;
    int next = old;
    if (zoomIn) {
        const qint64 doubled = qint64(old) * 2;
        next = int(qMin<qint64>(d->maximum, doubled));
    }
    else {
        next = qMax(d->minimum, old / 2);
    }

    if (next == old) {
        refresh();
        return;
    }
    d->value = next;
    refresh();
    emit valueChanged(next);
}

void CyCellStepper::refresh() {
    if (!d->label)
        return;

    const QString text = d->caption.isEmpty()
        ? QString::number(d->value)
        : QStringLiteral("%1 %2").arg(d->caption, QString::number(d->value));
    d->label->setText(text);
    d->label->setMinimumWidth(qMax(d->reservedWidth, d->label->fontMetrics().horizontalAdvance(text)));

    d->minusBtn->setEnabled(d->value > d->minimum);
    d->plusBtn->setEnabled(d->value < d->maximum);
}

void CyCellStepper::applyStyle() {
    //父控件设置 stylesheet 时，Qt 会给所有子控件投递 StyleChange；
    //本函数内部又会 setStyleSheet，若不做保护会无限递归导致栈溢出。
    if (d->applyingStyle) return;

    const bool dark = palette().color(QPalette::Window).lightness() < 128;

    QColor base = palette().color(QPalette::Window);
    if (!base.isValid()) base = dark ? QColor(0x2b, 0x2e, 0x33) : QColor(0xf0, 0xf0, 0xf0);

    const QColor hover = blend(base, d->themeColor, dark ? 0.45 : 0.22);
    const QColor pressed = blend(base, d->themeColor, dark ? 0.70 : 0.42);
    const QColor textColor = palette().color(QPalette::WindowText);
    const QColor disabled = palette().color(QPalette::Mid);

    const QString qss = QStringLiteral(
        "QToolButton{ border:none; border-radius:3px; padding:0px; color:%1; background:transparent; }"
        "QToolButton:hover{ background:%2; }"
        "QToolButton:pressed{ background:%3; }"
        "QToolButton:disabled{ color:%4; background:transparent; }")
        .arg(textColor.name(),
            QStringLiteral("rgba(%1,%2,%3,%4)").arg(hover.red()).arg(hover.green()).arg(hover.blue()).arg(255),
            QStringLiteral("rgba(%1,%2,%3,%4)").arg(pressed.red()).arg(pressed.green()).arg(pressed.blue()).arg(255),
            disabled.name());

    if (d->lastStyle == qss) return;
    d->lastStyle = qss;

    d->applyingStyle = true;
    setStyleSheet(qss);
    d->applyingStyle = false;

    QLabel* label = d->label;
    label->setStyleSheet(QStringLiteral("QLabel{ color:%1; }").arg(textColor.name()));
}

void CyCellStepper::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::FontChange
        || event->type() == QEvent::StyleChange) {
        applyStyle();
        if (event->type() == QEvent::FontChange && d->label) {
            setCaption(d->caption); // 字体变化后重新按新度量预留宽度
        }
    }
}
