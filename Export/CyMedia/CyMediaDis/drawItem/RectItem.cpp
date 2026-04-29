// ResizableRectItem.cpp
#include "RectItem.h"

#include <QPen>
#include <QBrush>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QLayout>
#include <QFormLayout>

namespace CyDisDrawItem {
    class CyMediaDisRectItem_Menu_geo : public QDialog {
        Q_OBJECT

    public:
        CyMediaDisRectItem_Menu_geo(QWidget* parent = nullptr);

    public:
        void setPara(QRectF current, QRectF max);
        QRect getSetRect();

        void flushTrans();
    private:
        void initGUI();

        QLabel* m_Xlabel = nullptr;
        QLabel* m_Ylabel = nullptr;
        QLabel* m_Wlabel = nullptr;
        QLabel* m_Hlabel = nullptr;

        QSpinBox* m_XBox = nullptr;
        QSpinBox* m_YBox = nullptr;
        QSpinBox* m_WBox = nullptr;
        QSpinBox* m_HBox = nullptr;

        QPushButton* m_OkBtn = nullptr;
        QPushButton* m_CancelBtn = nullptr;
    };

    RectItem::RectItem(QGraphicsItem* parent /*= nullptr*/)
        : BaseItem(parent) {
        //m_MinSize = QSize(HandleItem::handleSize() * 3, HandleItem::handleSize() * 3);
        m_MinSize = QSize(5, 5);
        m_localRect = { 0, 0, 100, 50 };
        setPos(0, 0); // 初始位置
        createHandles();
        updateHandles();
    }

    QRectF RectItem::boundingRect() const {

        return m_localRect; // 返回本地坐标系的矩形 
    }

    QRect RectItem::boundingRectInScene() const {
        auto pos_I = pos().toPoint();
        return QRect(pos_I.x(), pos_I.y(), m_localRect.width(), m_localRect.height());
    }

    QPainterPath RectItem::shape() const {
        QPainterPath path;
        path.addRect(m_localRect);
        return path;
    }

    QPainterPath RectItem::pathInScene() const {
        return mapToScene(shape());
    }

    void RectItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        painter->setRenderHint(QPainter::Antialiasing);
        QPen pen(m_contour_color_unselect, 0);     //边框未选中
        if (isSelected()) {
            pen.setColor(m_contour_color_select); //边框选中
        }
        painter->setPen(pen);
        painter->setBrush(Qt::transparent); // 填充颜色 
        painter->drawRect(m_localRect); // 绘制本地矩形 

        // 绘制中心点
        // painter->drawEllipse(m_localRect.center(), 2, 2);
    }

    void RectItem::setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals/* = true*/) {
        //约束不越界
        QRect sceneRect(p1, p2);
        QRect newRect = constrainToSceneByPos(sceneRect);

        m_localRect = QRect(0, 0, newRect.width(), newRect.height());
        setPos(newRect.topLeft());
        updateHandles();
        prepareGeometryChange();
        update(); // 触发重绘

        if (needSignals) {
            emit geometryChanged();
        }
    }

    QPoint RectItem::constrainToSceneByPos(const QPoint& pos) const {
        if (!scene())
            return pos;

        QSize scenSize = scene()->sceneRect().toRect().size();

        int32_t newLeft = std::clamp(pos.x(), 0, scenSize.width() - m_localRect.width());
        int32_t newTop = std::clamp(pos.y(), 0, scenSize.height() - m_localRect.height());

        QPoint newPos(newLeft, newTop);

        return std::move(newPos);
    }

    bool RectItem::changeByHandle(CyDisDrawItem::HandlePosition handletype, QPointF mousePos, QPointF delta) {
        ////判断增量
        //if (abs(delta.x()) < 1.0 &&
        //    abs(delta.y()) < 1.0) {
        //    return false;
        //}
        QPoint mousePos_I = mousePos.toPoint();
        QRect currentSceneRect = boundingRectInScene();
        QSize scenSize = scene()->sceneRect().toRect().size();
        int32_t newLeft = currentSceneRect.x();
        int32_t newTop = currentSceneRect.y();
        int32_t newRight = currentSceneRect.x() + currentSceneRect.width();
        int32_t newBottom = currentSceneRect.y() + currentSceneRect.height();

        if (handletype == CyDisDrawItem::Center) {
            // 平移模式：整个矩形移动
            newLeft += delta.x();
            newTop += delta.y();
            newRight += delta.x();
            newBottom += delta.y();
        }
        else {
            // 缩放模式：根据手柄调整对应边
            switch (handletype) {
            case CyDisDrawItem::TopLeft:     newLeft = mousePos_I.x(); newTop = mousePos_I.y(); break;
            case CyDisDrawItem::Top:         newTop = mousePos_I.y(); break;
            case CyDisDrawItem::TopRight:    newRight = mousePos_I.x(); newTop = mousePos_I.y(); break;
            case CyDisDrawItem::Left:        newLeft = mousePos_I.x(); break;
            case CyDisDrawItem::Right:       newRight = mousePos_I.x(); break;
            case CyDisDrawItem::BottomLeft:  newLeft = mousePos_I.x(); newBottom = mousePos_I.y(); break;
            case CyDisDrawItem::Bottom:      newBottom = mousePos_I.y(); break;
            case CyDisDrawItem::BottomRight: newRight = mousePos_I.x(); newBottom = mousePos_I.y(); break;
            default: break;
            }
        }
        setBoundingRectInScene(QPoint(newLeft, newTop), QPoint(newRight, newBottom), false);
        return true;
    }

    void RectItem::createHandles() {
        static const QList<CyDisDrawItem::HandlePosition> positions = {
            CyDisDrawItem::TopLeft,
            CyDisDrawItem::Top,
            CyDisDrawItem::TopRight,
            CyDisDrawItem::Left,
            /*CyMediaDisHandleItem::Center,*/
            CyDisDrawItem::Right,
            CyDisDrawItem::BottomLeft,
            CyDisDrawItem::Bottom,
            CyDisDrawItem::BottomRight
        };

        for (auto pos : positions) {
            auto* handle = new HandleItem(pos, this);
            m_handles.append(handle);
        }
        setHandlesVisible(m_handlesVisible);
    }

    QPoint RectItem::getHandlePos(CyDisDrawItem::HandlePosition type) {
        int32_t l = 0;
        int32_t t = 0;
        int32_t r = m_localRect.width();
        int32_t b = m_localRect.height();

        switch (type) {
        case CyDisDrawItem::TopLeft: return QPoint(l, t);
        case CyDisDrawItem::Top: return QPoint((l + r) / 2, t);
        case CyDisDrawItem::TopRight: return QPoint(r, t);
        case CyDisDrawItem::Left: return QPoint(l, (t + b) / 2);
        case CyDisDrawItem::Center: break;
        case CyDisDrawItem::Right: return QPoint(r, (t + b) / 2);
        case CyDisDrawItem::BottomLeft: return QPoint(l, b);
        case CyDisDrawItem::Bottom: return QPoint((l + r) / 2, b);
        case CyDisDrawItem::BottomRight: return QPoint(r, b);
        default:
            break;
        }

        return QPoint(0, 0);
    }

    QPoint RectItem::getHandlePosInScene(CyDisDrawItem::HandlePosition type) {
        int32_t l = 0;
        int32_t t = 0;
        int32_t r = m_localRect.width();
        int32_t b = m_localRect.height();

        switch (type) {
        case CyDisDrawItem::TopLeft: return mapToScene(QPointF(l, t)).toPoint();
        case CyDisDrawItem::Top: return mapToScene(QPointF((l + r) / 2, t)).toPoint();
        case CyDisDrawItem::TopRight: return mapToScene(QPointF(r, t)).toPoint();
        case CyDisDrawItem::Left: return mapToScene(QPointF(l, (t + b) / 2)).toPoint();
        case CyDisDrawItem::Center: break;
        case CyDisDrawItem::Right: return mapToScene(QPointF(r, (t + b) / 2)).toPoint();
        case CyDisDrawItem::BottomLeft: return mapToScene(QPointF(l, b)).toPoint();
        case CyDisDrawItem::Bottom: return mapToScene(QPointF((l + r) / 2, b)).toPoint();
        case CyDisDrawItem::BottomRight: return mapToScene(QPointF(r, b)).toPoint();
        default:
            break;
        }

        return pos().toPoint();
    }

    void RectItem::onContextMenuCreate(QMenu& menu) {
        QAction* act = menu.addAction(tr("Geometric shapes"));
        act->setData(0);
    }

    void RectItem::onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) {
        switch (act->data().toUInt()) {
            case 0: {
                CyMediaDisRectItem_Menu_geo menuW;
                menuW.flushTrans();
                menuW.setWindowTitle(act->text());
                menuW.setPara(
                    boundingRectInScene(),
                    { 0.0, 0.0, scene()->sceneRect().width(), scene()->sceneRect().height() });
                auto sel = menuW.exec();
                if (sel == menuW.Accepted) {
                    auto setRect = menuW.getSetRect();
                    setBoundingRectInScene(setRect.topLeft(), setRect.bottomRight());
                }
            }break;

            default: break;
        }
    }

    QRect RectItem::constrainToSceneByPos(const QRect& r) const {
        if (!scene())
            return r;

        QSize scenSize = scene()->sceneRect().toRect().size();

        int32_t newLeft = std::clamp(r.x(), 0, scenSize.width() - m_MinSize.width());
        int32_t newTop = std::clamp(r.y(), 0, scenSize.height() - m_MinSize.height());
        int32_t newRight = std::clamp(r.right(), newLeft + m_MinSize.width(), scenSize.width());
        int32_t newBottom = std::clamp(r.bottom(), newTop + m_MinSize.height(), scenSize.height());

        QRect newSceneRect(newLeft, newTop, newRight - newLeft, newBottom - newTop);

        return newSceneRect;
    }




    CyMediaDisRectItem_Menu_geo::CyMediaDisRectItem_Menu_geo(QWidget* parent /*= nullptr*/)
        : QDialog(parent) {
        initGUI();
    }

    void CyMediaDisRectItem_Menu_geo::setPara(QRectF current, QRectF max) {
        m_XBox->setMaximum(max.x());
        m_XBox->setValue(current.x());

        m_YBox->setMaximum(max.y());
        m_YBox->setValue(current.y());

        m_WBox->setMaximum(max.width());
        m_WBox->setValue(current.width());

        m_HBox->setMaximum(max.height());
        m_HBox->setValue(current.height());
    }

    QRect CyMediaDisRectItem_Menu_geo::getSetRect() {
        return QRect{
            m_XBox->value(),
            m_YBox->value(),
            m_WBox->value(),
            m_HBox->value() };
    }

    void CyMediaDisRectItem_Menu_geo::flushTrans() {
        m_Xlabel->setText("x");
        m_Ylabel->setText("x");
        m_Wlabel->setText(tr("width"));
        m_Hlabel->setText(tr("height"));

        m_OkBtn->setText(tr("confirm"));
        m_CancelBtn->setText(tr("cancel"));
    }

    void CyMediaDisRectItem_Menu_geo::initGUI() {
        m_Xlabel = new QLabel(this);
        m_Ylabel = new QLabel(this);
        m_Wlabel = new QLabel(this);
        m_Hlabel = new QLabel(this);

        m_XBox = new QSpinBox(this);
        m_YBox = new QSpinBox(this);
        m_WBox = new QSpinBox(this);
        m_HBox = new QSpinBox(this);

        m_OkBtn = new QPushButton(this);
        m_CancelBtn = new QPushButton(this);

        QFormLayout* propertyLayout = new QFormLayout();
        propertyLayout->setWidget(0, QFormLayout::LabelRole, m_Xlabel);
        propertyLayout->setWidget(1, QFormLayout::LabelRole, m_Ylabel);
        propertyLayout->setWidget(2, QFormLayout::LabelRole, m_Wlabel);
        propertyLayout->setWidget(3, QFormLayout::LabelRole, m_Hlabel);

        propertyLayout->setWidget(0, QFormLayout::FieldRole, m_XBox);
        propertyLayout->setWidget(1, QFormLayout::FieldRole, m_YBox);
        propertyLayout->setWidget(2, QFormLayout::FieldRole, m_WBox);
        propertyLayout->setWidget(3, QFormLayout::FieldRole, m_HBox);

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
#include "RectItem.moc"
