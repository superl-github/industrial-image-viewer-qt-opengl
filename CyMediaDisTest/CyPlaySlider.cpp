#include "CyPlaySlider.h"

#include <QBoxLayout>
#include <QMouseEvent>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>


//class CustomSlider------------------------------------------------------------
CustomSlider::CustomSlider(QWidget* parent /* = nullptr */)
:QSlider(parent),
m_HandleTrack(false) {
    setOrientation(Qt::Orientation::Horizontal);
    setRange(0, 1000);
    setTickPosition(QSlider::TicksAbove);
    setStyleSheet(QString(
        "QSlider::groove:horizontal{"
        "height:10px;"
        "border-radius:6px;"
        "border:0px"
        "}"

        "QSlider::handle:horizontal{"
        "width:12px;"
        "margin-top:-5px;"
        "margin-left:0px;"
        "margin-bottom:-5px;"
        "margin-right:-0px;"
        "border-radius:6px;"
        "background:rgb(193,204,208);"
        "}"

        "QSlider::sub-page:horizontal{"
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #74E6FC,stop:1 #31515B)/*rgb(90, 49, 255)*/;"
        "}"

        "QSlider::add-page:horizontal{"
        "background:#F9F9FB;"
        "}")
    );
}

CustomSlider::~CustomSlider() {

}

void CustomSlider::mousePressEvent(QMouseEvent* e) {
    if (e->buttons() == Qt::LeftButton) {
        int x = e->pos().x();
        int value = (x * 1.0 / width()) * (maximum() - minimum()) + minimum();
        setValue(value);
        emit handleJump(value);
        setSliderDown(true);
    }
    else {
        QSlider::mousePressEvent(e);
    }
}

void CustomSlider::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() == Qt::LeftButton && isSliderDown()) {
        int x = e->pos().x();
        int value = (x * 1.0 / width()) * (maximum() - minimum()) + minimum();
        if (value > maximum()) value = maximum();
        else if (value < minimum()) value = minimum();
        setValue(value);
        if (m_LastMoveValue != value) {
            if (m_HandleTrack) {
                emit handleMoved(value);
            }
            m_LastMoveValue = value;
        }
    }

    QSlider::mouseMoveEvent(e);
}

void CustomSlider::mouseReleaseEvent(QMouseEvent* e) {
    if (isSliderDown()) {
        setSliderDown(false);
        if (m_HandleTrack == false) {
            emit handleMoved(value());
        }
        emit handleJumpDone();
    }
    else {
        QSlider::mouseReleaseEvent(e);
    }
}

void CustomSlider::setHandleTrack(bool Track) {
    m_HandleTrack = Track;
}





class CyPlaySlider::PrivateData {
public:
    enum SliderStatus  {
        SLIDER_AUTO, SLIDER_DRAG
    };
    int             m_dRate = 1;
    CustomSlider*   m_slider = 0;
    QLabel*         ui_playTime_Lab = 0;
    SliderStatus    CurrentSliderStatus = SLIDER_AUTO;
};

CyPlaySlider::CyPlaySlider(QWidget* parent/* = nullptr*/)
:QWidget(parent),
d(new PrivateData) {
    d->m_slider = new CustomSlider(this);
    connect(d->m_slider, &CustomSlider::handleJump, this, &CyPlaySlider::handleJump);
    connect(d->m_slider, &CustomSlider::handleMoved, this, &CyPlaySlider::handleMoved);
    connect(d->m_slider, &CustomSlider::handleJumpDone, this, &CyPlaySlider::handleJumpDone);

    d->ui_playTime_Lab = new QLabel(this);
    //边框
    d->ui_playTime_Lab->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QFont font = d->ui_playTime_Lab->font();
    font.setPointSize(8);
    font.setFamily(QString("Microsoft YaHei UI"));
    font.setBold(true);
    d->ui_playTime_Lab->setFont(font);
    d->ui_playTime_Lab->setText("000:00`000");
    d->ui_playTime_Lab->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
    d->ui_playTime_Lab->setEnabled(false);
    //Layout
    QHBoxLayout* m_Layout = new QHBoxLayout(this);
    m_Layout->setContentsMargins(5, 0, 5, 0);
    m_Layout->setSpacing(5);
    m_Layout->addWidget(d->m_slider);
    m_Layout->addWidget(d->ui_playTime_Lab);
    setLayout(m_Layout);
}

CyPlaySlider::~CyPlaySlider() {
    if (d) {
        delete d;
        d = 0;
    }
}

void CyPlaySlider::setValue(int value) {
    switch (d->CurrentSliderStatus) {
        case PrivateData::SLIDER_AUTO: {
            d->m_slider->setValue(value);
            double timeValue = (double)value / (double)d->m_dRate;
            d->ui_playTime_Lab->setText(QString("%0:%1:%3")
                .arg(QString::number(int(timeValue / 60)))
                .arg(int(timeValue) % 60, 2, 10, QChar('0'))
                .arg(int((timeValue - int(timeValue)) * 1000), 3, 10, QChar('0'))
            );
        }break;

        case PrivateData::SLIDER_DRAG: {

        }break;

        default:
            break;
    }
}

int CyPlaySlider::value() {
    return d->m_slider->value();
}

void CyPlaySlider::setTimeLableVisible(bool visio) {
    d->ui_playTime_Lab->setVisible(visio);
}

bool CyPlaySlider::timeLableisVisible() {
    return d->ui_playTime_Lab->isVisible();
}

void CyPlaySlider::setRate(int Rate) {
    d->m_dRate = Rate;
    int value = d->m_slider->value();
    double timeValue = (double)value / (double)d->m_dRate;
    d->ui_playTime_Lab->setText(QString("%0:%1:%2")
        .arg(QString::number(int(timeValue / 60)))
        .arg(int(timeValue) % 60, 2, 10, QChar('0'))
        .arg(int((timeValue - int(timeValue)) * 1000), 3, 10, QChar('0'))
    );
}

int CyPlaySlider::Rate() {
    return d->m_dRate;
}

void CyPlaySlider::setRange(int min, int max) {
    d->m_slider->setRange(min, max);
}

int CyPlaySlider::maxmum() {
    return d->m_slider->maximum();
}

int CyPlaySlider::minimum() {
    return d->m_slider->minimum();
}

void CyPlaySlider::setHandleTracking(bool tracking) {
    d->m_slider->setHandleTrack(tracking);
}

bool CyPlaySlider::HandleTrack() {
    return d->m_slider->HandleTrack();
}

void CyPlaySlider::setTickInterval(int ti) {
    d->m_slider->setTickInterval(ti);
}

int CyPlaySlider::tickInterval() {
    return d->m_slider->tickInterval();
}

void CyPlaySlider::handleJump(int value) {
    d->m_slider->setSliderPosition(value);
    double timevalue = (double)value / (double)d->m_dRate;
    d->ui_playTime_Lab->setText(QString("%0:%1:%2")
        .arg(QString::number(int(timevalue / 60)))
        .arg(int(timevalue) % 60, 2, 10, QChar('0'))
        .arg(int((timevalue - int(timevalue)) * 1000), 3, 10, QChar('0'))
    );
    d->CurrentSliderStatus = PrivateData::SLIDER_DRAG;
    emit sliderDrag(value);
}

void CyPlaySlider::handleMoved(int value) {
    if (d->CurrentSliderStatus == PrivateData::SLIDER_DRAG) {
        d->m_slider->setSliderPosition(value);
        double timevalue = (double)value / (double)d->m_dRate;
        d->ui_playTime_Lab->setText(QString("%0:%1:%2")
            .arg(QString::number(int(timevalue / 60)))
            .arg(int(timevalue) % 60, 2, 10, QChar('0'))
            .arg(int((timevalue - int(timevalue)) * 1000), 3, 10, QChar('0'))
        );
        emit sliderMoved(value);
    }
}

void CyPlaySlider::handleJumpDone() {
    d->CurrentSliderStatus = PrivateData::SLIDER_AUTO;
    emit sliderRelease();
}
