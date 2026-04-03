#include "CyDMediaDisScen.h"

#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QPainter>
#include <QGraphicsView>

class CyDMediaDisScen::PrivateData {
public:
    Qt::CursorShape arrCursor = Qt::ArrowCursor;
    bool acceptDrop = false;
    
    bool bHideText = false;

    QGraphicsItem* backDis = nullptr;
};

CyDMediaDisScen::CyDMediaDisScen(qreal x, qreal y, qreal width, qreal height, QObject* parent /*= nullptr*/)
    : QGraphicsScene(x, y, width, height, parent)
    , d(new CyDMediaDisScen::PrivateData()) {

}

CyDMediaDisScen::~CyDMediaDisScen()
{

}

bool CyDMediaDisScen::acceptDrops()
{
    return d->acceptDrop;
}

void CyDMediaDisScen::setAcceptDrops(bool accept)
{
    d->acceptDrop = accept;
}

bool CyDMediaDisScen::tipTextVisible() {
    return !d->bHideText;
}

void CyDMediaDisScen::setTipTextVisible(bool visi) {
    d->bHideText = !visi;
}

void CyDMediaDisScen::setBackDis(QGraphicsItem* back) {
    if (d->backDis) {
        removeItem(d->backDis);
    }

    addItem(back);
    d->backDis = back;
}

QGraphicsItem* CyDMediaDisScen::BackDis() {
    return d->backDis;
}

QGraphicsItem* CyDMediaDisScen::itemAtWithoutBack(const QPointF& pos, const QTransform& deviceTransform) {
    auto item = itemAt(pos, deviceTransform);
    if (item == d->backDis)
        return nullptr;
    return item;
}

void CyDMediaDisScen::drawBackground(QPainter* painter, const QRectF& rect)
{
    QBrush brush;
    brush.setColor(Qt::black);
    brush.setStyle(Qt::SolidPattern);
    painter->setBrush(brush);
    painter->drawRect(rect);

    if (false == d->bHideText) {
        QFont font;
        font.setPointSize(28);
        font.setFamily("Microsoft YaHei");
        QPen pen;
        pen.setColor(Qt::lightGray);
        painter->setPen(pen);
        painter->setFont(font);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawText(rect, Qt::AlignCenter, QString("No Image"));
    }

    QGraphicsScene::drawBackground(painter, rect);
}

void CyDMediaDisScen::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    return QGraphicsScene::mousePressEvent(event);
}

void CyDMediaDisScen::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    //更新PosGray
    if (event->scenePos().x() >= 0 && event->scenePos().x() <= this->width() && 
        event->scenePos().y() >= 0 && event->scenePos().y() <= this->height()) {
        emit mousePosChange(event->scenePos().x(), event->scenePos().y());
    }

    return QGraphicsScene::mouseMoveEvent(event);
}

void CyDMediaDisScen::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    return QGraphicsScene::mouseReleaseEvent(event);
}

void CyDMediaDisScen::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    return QGraphicsScene::mouseDoubleClickEvent(event);
}

void CyDMediaDisScen::keyPressEvent(QKeyEvent* event)
{
    return QGraphicsScene::keyPressEvent(event);
}

void CyDMediaDisScen::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    if (d->acceptDrop)
        event->acceptProposedAction();
    else
        event->ignore();

}

void CyDMediaDisScen::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    if (d->acceptDrop) {
        if (event->mimeData()->hasUrls()) {
            QList<QUrl> urls = event->mimeData()->urls();
            emit urlsDrop(urls);
        }
    }
}
