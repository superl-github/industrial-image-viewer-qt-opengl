#include "CyMediaDisTest.h"

#include <random>
#include <chrono>

CyMediaDisTest::CyMediaDisTest(QWidget *parent)
    : QMainWindow(parent) {
    ui.setupUi(this);
    m_view = new CyMediaDis(this);
    m_view->setGrayStretchVisible(true);
    m_view->setGrayTestVisible(true);

    QGridLayout* mainLayout = new QGridLayout(this->centralWidget());
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_view);

    mUpImageThread = new QThread(this);
    connect(mUpImageThread, &QThread::started, this, &CyMediaDisTest::thread_up_image, Qt::DirectConnection);
    mUpImageThread->start();
}

CyMediaDisTest::~CyMediaDisTest() {
    if (mUpImageThread->isRunning()) {
        mUpImageThreadFlag = false;
        mUpImageThread->wait();
        mUpImageThread->terminate();
    }
}

void CyMediaDisTest::thread_up_image() {
    //随机数
    std::random_device rd;
    std::default_random_engine engind(rd());
    std::uniform_int_distribution<int> distr(0, 255);

    //初始化数据
    CyMedia::ImageShowInfo tinfo{ 5000, 5000, 8, CyMedia::RGB };
    uint8_t* pImage = new uint8_t[5000 * 5000 * 3];

    QString filename = "C:\\Users\\Administrator\\Desktop\\RGB_RAW_20260205_091735.raw";
    QFile openFile(filename);
    if (openFile.open(QIODevice::ReadOnly)) {
        auto readCode = openFile.readAll();
        openFile.close();
        uint8_t* pCode = (uint8_t*)readCode.data();
        for (int h = 0; h < 360; h++) {
            memcpy(&pImage[h * 5000 * 3], &pCode[h * 360 * 3], 360 * 3);
        }
    }
    for (int h = 0; h < 360; h++) {
        memset(&pImage[(h * 5000 + 360) * 3], 0x00, 5000 - 360 * 3);
    }

    mUpImageThreadFlag = true;
    while (mUpImageThreadFlag) {
        int tempR, tempG, tempB;
        for (int h = 360; h < 5000; h++) {
            tempR = distr(engind);
            tempG = distr(engind);
            tempB = distr(engind);
            for (int w = 0; w < 5000; w++) {
                pImage[(h * 5000 + w) * 3 + 0] = tempR;
                pImage[(h * 5000 + w) * 3 + 1] = tempG;
                pImage[(h * 5000 + w) * 3 + 2] = tempB;
            }
        }

        m_view->upImageData(tinfo, pImage);

        QThread::msleep(100);
    }

    delete[] pImage;
    mUpImageThread->quit();
}

