#pragma once

#include "ui_CyMediaDisTest.h"
#include "CyMediaDis.h"

#include <QtWidgets/QMainWindow>
#include <QThread>

class CyMediaDisTest : public QMainWindow
{
    Q_OBJECT

public:
    CyMediaDisTest(QWidget *parent = nullptr);
    ~CyMediaDisTest();

private:
    void thread_up_image();

private:
    Ui::CyMediaDisTestClass ui;

    CyMedia::CyMediaDis* m_view = nullptr;

    QThread* mUpImageThread = nullptr;
    bool mUpImageThreadFlag = false;
};

