#include "Item_Point.h"

#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QLayout>
#include <QFormLayout>

namespace CyDisDrawItem {
    class CyMediaDisPointItem_Menu_geo : public QDialog {
        Q_OBJECT

    public:
        CyMediaDisPointItem_Menu_geo(QWidget* parent = nullptr);

    public:
        void setPara(QPointF pos, QPointF max);
        QPoint getSetRect();

        void flushTrans();

    private:
        void initGUI();

        QLabel* m_Xlabel = nullptr;
        QLabel* m_Ylabel = nullptr;

        QSpinBox* m_XBox = nullptr;
        QSpinBox* m_YBox = nullptr;

        QPushButton* m_OkBtn = nullptr;
        QPushButton* m_CancelBtn = nullptr;

    };

    CyDisDrawItem::Item_Point::Item_Point(QGraphicsItem* parent /*= nullptr*/)
        : BaseItem(parent) {
        setPos(0, 0); // 初始位置
        m_localRect = QRect{ 0, 0, 1, 1 };
        createHandles();
        updateHandles();
    }

    QRectF CyDisDrawItem::Item_Point::boundingRect() const {
        return m_localRect;
    }

    QRect CyDisDrawItem::Item_Point::boundingRectInScene() const {
        auto pos_I = pos().toPoint();
        return QRect(pos_I.x(), pos_I.y(), m_localRect.width(), m_localRect.height());
    }

    QPainterPath CyDisDrawItem::Item_Point::shape() const {
        QPainterPath path;
        path.addRect(m_localRect);
        return path;
    }

    QPainterPath CyDisDrawItem::Item_Point::pathInScene() const {
        return mapToScene(shape());
    }

    void CyDisDrawItem::Item_Point::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        painter->setRenderHint(QPainter::Antialiasing);
        QPen pen(m_contour_color_unselect, 0);     //边框未选中
        if (isSelected()) {
            pen.setColor(m_contour_color_select); //边框选中
        }
        painter->setPen(pen);
        painter->setBrush(Qt::transparent); // 填充颜色
        
        //绘制测试点
        painter->drawRect(m_localRect);

        //绘制外框
        qreal rectWidth = mOuterframeLenth * 2 + 1;
        painter->drawRect(QRectF{
            -mOuterframeLenth ,
            -mOuterframeLenth ,
            rectWidth ,
            rectWidth });

        //绘制延长线
        double line_len = (mOuterframeLenth * 2 + 5) * 2.5;
        painter->drawLine(QPointF(-mOuterframeLenth, 0), QPointF(-mOuterframeLenth - line_len, 0));
        painter->drawLine(QPointF(mOuterframeLenth + 1, 0), QPointF(rectWidth + line_len, 0));
        painter->drawLine(QPointF(0, -mOuterframeLenth), QPointF(0, -mOuterframeLenth - line_len));
        painter->drawLine(QPointF(0, mOuterframeLenth + 1), QPointF(0, rectWidth + line_len));
    }

    void CyDisDrawItem::Item_Point::setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals /*= true*/) {
        //约束不越界
        QRect sceneRect(p1, p2);
        QRect newRect = constrainToSceneByPos(sceneRect);
        setPos(newRect.topLeft());
        updateHandles();
        prepareGeometryChange();
        update(); // 触发重绘

        if (needSignals) {
            emit geometryChanged();
        }
    }

    void Item_Point::setPainterPathInScene(QPainterPath path, bool needSignals /*= true*/) {
        QPoint p = path.boundingRect().topLeft().toPoint();
        // 复用已有的点设置逻辑
        setBoundingRectInScene(p, p, needSignals);
    }

    bool Item_Point::onDrawMouseEvent(QEvent::Type type, const QPointF& scenePos) {
        if (type == QEvent::MouseButtonPress) {
            setBoundingRectInScene(scenePos.toPoint(), scenePos.toPoint(), true);
            m_drawFinished = true;
            return true;
        }
        return false;
    }

    bool Item_Point::isDrawFinished() const {
        return m_drawFinished;
    }

    QPoint CyDisDrawItem::Item_Point::constrainToSceneByPos(const QPoint& pos) const {
        if (!scene())
            return pos;

        QSize scenSize = scene()->sceneRect().toRect().size();
        int32_t newLeft = std::clamp(pos.x(), 0, scenSize.width() - 1);
        int32_t newTop = std::clamp(pos.y(), 0, scenSize.height() - 1);

        QPoint newPos(newLeft, newTop);

        return std::move(newPos);
    }

    bool CyDisDrawItem::Item_Point::changeByHandle(CyDisDrawItem::HandlePosition handletype, int id, QPointF mousePos, QPointF delta) {
        return false;
    }

    void CyDisDrawItem::Item_Point::createHandles() {
        ;
    }

    QPoint CyDisDrawItem::Item_Point::getHandlePos(CyDisDrawItem::HandlePosition type, int id) {
        return QPoint(0, 0);
    }

    QPoint CyDisDrawItem::Item_Point::getHandlePosInScene(CyDisDrawItem::HandlePosition type, int id) {
        return pos().toPoint();
    }

    void CyDisDrawItem::Item_Point::onContextMenuCreate(QMenu& menu) {
        QAction* act = menu.addAction(tr("Geometric shapes"));
        act->setData(0);
    }

    void CyDisDrawItem::Item_Point::onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) {
        if (!act || event) return;
        switch (act->data().toUInt()) {
            case 0: {
                CyMediaDisPointItem_Menu_geo menuW;
                menuW.setWindowTitle(act->text());
                menuW.setPara(
                    pos(),
                    {qreal(scene()->sceneRect().width()), qreal(scene()->sceneRect().height()) });
                auto sel = menuW.exec();
                if (sel == menuW.Accepted) {
                    auto setPos= menuW.getSetRect();
                    setBoundingRectInScene(setPos, setPos);
                }
            }break;

            default: break;
        }
    }

    QRect CyDisDrawItem::Item_Point::constrainToSceneByPos(const QRect& r) const {
        if (!scene())
            return r;

        QSize scenSize = scene()->sceneRect().toRect().size();
        auto currentSize = r.size();
        qreal newLeft = std::clamp(r.x(), 0, scenSize.width() - 1);
        qreal newTop = std::clamp(r.y(), 0, scenSize.height() - 1);

        QRect newSceneRect(newLeft, newTop, currentSize.width(), currentSize.height());

        return std::move(newSceneRect);
    }




    CyMediaDisPointItem_Menu_geo::CyMediaDisPointItem_Menu_geo(QWidget* parent /*= nullptr*/)
        : QDialog(parent) {
        initGUI();
    }

    void CyMediaDisPointItem_Menu_geo::setPara(QPointF pos, QPointF max) {
        m_XBox->setMaximum(max.x());
        m_XBox->setValue(pos.x());

        m_YBox->setMaximum(max.y());
        m_YBox->setValue(pos.y());
    }

    QPoint CyMediaDisPointItem_Menu_geo::getSetRect() {
        return { m_XBox->value(), m_YBox->value() };
    }

    void CyMediaDisPointItem_Menu_geo::flushTrans() {
        m_Xlabel->setText("x");
        m_Ylabel->setText("x");

        m_OkBtn->setText(tr("confirm"));
        m_CancelBtn->setText(tr("cancel"));
    }

    void CyMediaDisPointItem_Menu_geo::initGUI() {
        m_Xlabel = new QLabel(this);
        m_Ylabel = new QLabel(this);

        m_XBox = new QSpinBox(this);
        m_YBox = new QSpinBox(this);

        m_OkBtn = new QPushButton(this);
        m_CancelBtn = new QPushButton(this);

        QFormLayout* propertyLayout = new QFormLayout();
        propertyLayout->setWidget(0, QFormLayout::LabelRole, m_Xlabel);
        propertyLayout->setWidget(1, QFormLayout::LabelRole, m_Ylabel);

        propertyLayout->setWidget(0, QFormLayout::FieldRole, m_XBox);
        propertyLayout->setWidget(1, QFormLayout::FieldRole, m_YBox);

        QHBoxLayout* BtnLayout = new QHBoxLayout();
        BtnLayout->addWidget(m_OkBtn);
        BtnLayout->addWidget(m_CancelBtn);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(propertyLayout);
        mainLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
        mainLayout->addLayout(BtnLayout);

        flushTrans();

        connect(m_OkBtn, &QPushButton::clicked, this, [this]() {
            this->accept();
            });
        connect(m_CancelBtn, &QPushButton::clicked, this, [this]() {
            this->reject();
            });
    }
}

#include "Item_Point.moc"
