#include "CyMediaDisShow.h"


CyMedia::imgShowWidget::imgShowWidget(QWidget* parent /*= nullptr*/)
 : QWidget(parent) {
    if (!parent) setAttribute(Qt::WA_DeleteOnClose);

    //display
    ui_disPlay = new CyMedia::CyMediaDis(this);
    ui_disPlay->setToolBarVisible(true);
    ui_disPlay->setZoomScrollBarVisible(false);
    connect(ui_disPlay, &CyMedia::CyMediaDis::upPosPix, this, &imgShowWidget::onUpPosPix);
    //info
    ui_PosRGBLab = new QLabel(this);
    //Layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(ui_disPlay);
    mainLayout->addWidget(ui_PosRGBLab);
}

CyMedia::imgShowWidget::~imgShowWidget() {

}

CyMedia::CyMediaDis* CyMedia::imgShowWidget::display() const {
    return ui_disPlay;
}


void CyMedia::imgShowWidget::upImageData(CyMedia::ImageShowInfo info, void* data) {
    //创建时无法更新图像，窗口未显示上下文未创建
    ui_disPlay->upImageData(info, data);
}

void CyMedia::imgShowWidget::onUpPosPix(qint32 x, qint32 y, double r, double g, double b, bool signlR) {
    QString setStr = QString("pos(%1, %2) ").arg(x).arg(y);
    if (signlR) {
        setStr += QString("Gray: %1").arg(r);
    }
    else {
        setStr += QString("R G B: %1 %2 %3").arg(r).arg(g).arg(b);
    }

    ui_PosRGBLab->setText(setStr);
}
