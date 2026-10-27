#include "CyMediaRecTimeW.h"
#include <QPushButton>
#include <QLayout>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

class CyMediaRecTimeW::PrivateData
{
public:
    QPushButton* m_LedButton = Q_NULLPTR;
    QPushButton* m_RecTimeButton = Q_NULLPTR;

    bool            bLedChange = true;
};

CyMediaRecTimeW::CyMediaRecTimeW(QWidget* parent/* = nullptr*/, Qt::WindowFlags f/* = Qt::WindowFlags()*/)
    :QWidget(parent, f),
    p_data(new PrivateData) {
    setWindowFlags(Qt::FramelessWindowHint);
    initGui();
}

CyMediaRecTimeW::~CyMediaRecTimeW() {

}

void CyMediaRecTimeW::upRecTime(uint64_t time) {
    int hh = time / 60 / 60;
    int mm = time / 60 % 60;
    int ss = time % 60;
    if (p_data->bLedChange)
    {
        p_data->m_LedButton->setStyleSheet("border:none; border-radius:6px; background-color:red;");
        p_data->bLedChange = false;
    }
    else
    {
        p_data->m_LedButton->setStyleSheet("border:none; border-radius:6px; background-color:rgba(0,0,0,0);");
        p_data->bLedChange = true;
    }
    p_data->m_RecTimeButton->setText(QString("%1:%2:%3")
        .arg(hh, 2, 10, QChar('0'))
        .arg(mm, 2, 10, QChar('0'))
        .arg(ss, 2, 10, QChar('0')));
}

void CyMediaRecTimeW::upRecTime(float saved, float sum) {
    p_data->m_RecTimeButton->setText(QString("%1 / %2")
        .arg(QString::number(saved, 'f', 2))
        .arg(QString::number(sum, 'f', 2)));
    if (p_data->bLedChange == true) {
        p_data->m_LedButton->setStyleSheet("border:none; border-radius:6px; background-color:red;");
        p_data->bLedChange = false;
    }
}

void CyMediaRecTimeW::upRecTime_Timed(uint64_t saved, uint64_t sum) {
    int hh_e = saved / 60 / 60;
    int mm_e = saved / 60 % 60;
    int ss_e = saved % 60;

    int hh_s = sum / 60 / 60;
    int mm_s = sum / 60 % 60;
    int ss_s = sum % 60;

    if (p_data->bLedChange) {
        p_data->m_LedButton->setStyleSheet("border:none; border-radius:6px; background-color:red;");
        p_data->bLedChange = false;
    }
    else {
        p_data->m_LedButton->setStyleSheet("border:none; border-radius:6px; background-color:rgba(0,0,0,0);");
        p_data->bLedChange = true;
    }
    p_data->m_RecTimeButton->setText(QString("%1:%2:%3 / %4:%5:%6")
        .arg(hh_e, 2, 10, QChar('0'))
        .arg(mm_e, 2, 10, QChar('0'))
        .arg(ss_e, 2, 10, QChar('0'))
        .arg(hh_s, 2, 10, QChar('0'))
        .arg(mm_s, 2, 10, QChar('0'))
        .arg(ss_s, 2, 10, QChar('0')));
}

void CyMediaRecTimeW::initGui()
{
    setStyleSheet(
        "QWidget"
        "{"
        "background-color:black;"
        "}");
    //setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(true);
    setWindowOpacity(0.5);

    p_data->m_LedButton = new QPushButton(this);
    p_data->m_LedButton->blockSignals(true);
    p_data->m_LedButton->setMinimumSize(1, 12);
    p_data->m_LedButton->setMaximumSize(12, 12);
    p_data->m_LedButton->setText("");
    p_data->m_LedButton->setStyleSheet("border:none; border-radius:6px; background-color:lightgray;");

    p_data->m_RecTimeButton = new QPushButton(this);
    p_data->m_RecTimeButton->setStyleSheet("background-color:rgba(0,0,0,0); border:1px solid lightGray; color:white;");
    p_data->m_RecTimeButton->setFlat(true);
    p_data->m_RecTimeButton->blockSignals(true);
    QFont ft;
    ft.setFamily("Microsoft YaHei UI");
    ft.setPointSize(12);
    p_data->m_RecTimeButton->setFont(ft);
    p_data->m_RecTimeButton->setText("00:00:00 / 00:00:00");
    QFontMetrics fm(ft);
    int textWidth = fm.width(p_data->m_RecTimeButton->text());
    //p_data->m_RecTimeButton->setMinimumSize(textWidth + 6, fm.height() + 6);
    p_data->m_RecTimeButton->setMaximumSize(textWidth + 6, fm.height() + 6);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(3);
    mainLayout->setMargin(0);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(p_data->m_LedButton, 0);
    mainLayout->addWidget(p_data->m_RecTimeButton, 1);

    int width = p_data->m_RecTimeButton->maximumWidth() + p_data->m_LedButton->maximumWidth() + 3;
    this->setMinimumSize(width + 10, p_data->m_RecTimeButton->maximumHeight() + 10);
    this->setMaximumSize(width + 10, p_data->m_RecTimeButton->maximumHeight() + 10);
}