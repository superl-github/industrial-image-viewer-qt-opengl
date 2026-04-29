#include "Item_Ellipse.h"

#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QLayout>
#include <QFormLayout>
#include <QPushButton>

namespace CyDisDrawItem {
    class CyMediaDisEllipseItem_Menu_geo : public QDialog {
        Q_OBJECT
    public:
        CyMediaDisEllipseItem_Menu_geo(QWidget* parent = nullptr);
    public:
        void setPara(const QRectF& current, const QRectF& max);
        QRect getSetRect();
        void flushTrans();

    private:
        void initGUI();
        QLabel* m_Centerlabel = nullptr;
        QLabel* m_Wlabel = nullptr;
        QLabel* m_Hlabel = nullptr;
        QSpinBox* m_XBox = nullptr;
        QSpinBox* m_YBox = nullptr;
        QSpinBox* m_WBox = nullptr;
        QSpinBox* m_HBox = nullptr;
        QPushButton* m_OkBtn = nullptr;
        QPushButton* m_CancelBtn = nullptr;
    };

    Item_Ellipse::Item_Ellipse(QGraphicsItem* parent /*= nullptr*/)
        : BaseItem(parent) {
        m_MinSize = QSize(HandleItem::handleSize() * 3, HandleItem::handleSize() * 3);
        m_localRect = QRect(0, 0, 100, 60); // 默认宽高
        setPos(0, 0);
        createHandles();
        updateHandles();
    }

	QRectF Item_Ellipse::boundingRect() const {
        return m_localRect;
    }

    QRect Item_Ellipse::boundingRectInScene() const {
        auto pos_I = pos().toPoint();
        return QRect(pos_I.x(), pos_I.y(), m_localRect.width(), m_localRect.height());
    }

    QPainterPath Item_Ellipse::shape() const {
        QPainterPath path;
        path.addEllipse(m_localRect);
        return path;
    }

    QPainterPath Item_Ellipse::pathInScene() const {
        return mapToScene(shape());
    }

    void Item_Ellipse::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->setRenderHint(QPainter::Antialiasing);
        QPen pen(m_contour_color_unselect, 0);
        if (isSelected()) {
            pen.setColor(m_contour_color_select);
        }
        painter->setPen(pen);
        painter->setBrush(Qt::transparent);
        painter->drawEllipse(m_localRect);
    }

    void Item_Ellipse::setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals /*= true*/) {
        QRectF sceneRect(p1, p2);
        QRectF newRect = constrainToSceneByPos(sceneRect.toRect());
        m_localRect = QRect(0, 0, newRect.width(), newRect.height());
        setPos(newRect.topLeft());
        updateHandles();
        prepareGeometryChange();
        update();
        if (needSignals) {
            emit geometryChanged();
        }
    }

    void Item_Ellipse::setPainterPathInScene(QPainterPath path, bool needSignals /*= true*/) {
        QRectF sceneRect = path.boundingRect();
        QRectF newRect = constrainToSceneByPos(sceneRect.toRect());
        m_localRect = QRect(0, 0, newRect.width(), newRect.height());
        setPos(newRect.topLeft());
        updateHandles();
        prepareGeometryChange();
        update();
        if (needSignals) {
            emit geometryChanged();
        }
    }

    QPoint Item_Ellipse::constrainToSceneByPos(const QPoint& pos) const {
        if (!scene()) return pos;
        QSize sceneSize = scene()->sceneRect().toRect().size();
        int32_t newLeft = std::clamp(pos.x(), 0, sceneSize.width() - m_localRect.width());
        int32_t newTop = std::clamp(pos.y(), 0, sceneSize.height() - m_localRect.height());
        return QPoint(newLeft, newTop);
    }

    bool Item_Ellipse::changeByHandle(CyDisDrawItem::HandlePosition handletype, QPointF mousePos, QPointF delta) {
        if (!scene()) return false;
        QRect sceneRect = boundingRectInScene();
        int32_t l = sceneRect.x(), t = sceneRect.y();
        int32_t r = sceneRect.x() + sceneRect.width(), b = sceneRect.y() + sceneRect.height();
        printf("currentRect{%d %d %d %d}\n", l, t, r, b);

        QPoint adjustedMouse = mousePos.toPoint();
        printf("adjustedMouse{%d %d}\n", adjustedMouse.x(), adjustedMouse.y());
        if (isCornerHandle(handletype)) {
            QPoint handleLocal = getHandlePos(handletype);
            // 对应的矩形角点（本地）
            QPoint rectCornerLocal;
            switch (handletype) {
            case CyDisDrawItem::TopLeft:     rectCornerLocal = QPoint(0, 0); break;
            case CyDisDrawItem::TopRight:    rectCornerLocal = QPoint(m_localRect.width(), 0); break;
            case CyDisDrawItem::BottomLeft:  rectCornerLocal = QPoint(0, m_localRect.height()); break;
            case CyDisDrawItem::BottomRight: rectCornerLocal = QPoint(m_localRect.width(), m_localRect.height()); break;
            default: break;
            }
            // 本地 offset = 矩形角点 - 椭圆点
            QPoint localOffset = rectCornerLocal - handleLocal;
            // 因为 item 无旋转，场景 offset = 本地 offset
            adjustedMouse += localOffset;
        }
        switch (handletype) {
            case CyDisDrawItem::Top: t = adjustedMouse.y(); break;
            case CyDisDrawItem::Bottom: b = adjustedMouse.y(); break;
            case CyDisDrawItem::Left: l = adjustedMouse.x(); break;
            case CyDisDrawItem::Right: r = adjustedMouse.x(); break;

            case CyDisDrawItem::TopLeft: l = adjustedMouse.x(); t = adjustedMouse.y(); break;
            case CyDisDrawItem::TopRight: r = adjustedMouse.x(); t = adjustedMouse.y(); break;
            case CyDisDrawItem::BottomLeft: l = adjustedMouse.x(); b = adjustedMouse.y(); break;
            case CyDisDrawItem::BottomRight: r = adjustedMouse.x(); b = adjustedMouse.y(); break;
            default:
                return false;
        }

        if (l > r) std::swap(l, r);
        if (t > b) std::swap(t, b);
        if (r - l < m_MinSize.width()) { qreal cx = (l + r) / 2; l = cx - m_MinSize.width() / 2; r = cx + m_MinSize.width() / 2; }
        if (b - t < m_MinSize.height()) { qreal cy = (t + b) / 2; t = cy - m_MinSize.height() / 2; b = cy + m_MinSize.height() / 2; }
        printf("newRect{%d %d %d %d}\n", l, t, r, b);
        setBoundingRectInScene(QPoint(l, t), QPoint(r, b), false);
        printf("setBoundingRectInScene{%.0f %.0f %.0f %.0f}\n\n", pos().x(), pos().y(), pos().x() + m_localRect.width(), pos().y() + m_localRect.height());
        return true;
    }

    void Item_Ellipse::createHandles() {
        static const QList<CyDisDrawItem::HandlePosition> positions = {
            CyDisDrawItem::Top,
            CyDisDrawItem::Left,
            CyDisDrawItem::Bottom,
            CyDisDrawItem::Right,

            CyDisDrawItem::TopLeft,
            CyDisDrawItem::TopRight,
            CyDisDrawItem::BottomLeft,
            CyDisDrawItem::BottomRight
        };
        for (auto pos : positions) {
            auto* handle = new HandleItem(pos, this);
            m_handles.append(handle);
        }
        setHandlesVisible(m_handlesVisible);
    }

    QPoint Item_Ellipse::getHandlePos(CyDisDrawItem::HandlePosition type) {
        qreal l = 0, t = 0, r = m_localRect.width(), b = m_localRect.height();
        QPointF rectPoint;
        switch (type) {
        case CyDisDrawItem::TopLeft:     rectPoint = QPoint(l, t); break;
        case CyDisDrawItem::Top:         return QPoint((l + r) / 2, t);
        case CyDisDrawItem::TopRight:    rectPoint = QPoint(r, t); break;
        case CyDisDrawItem::Left:        return QPoint(l, (t + b) / 2);
        case CyDisDrawItem::Center:      return QPoint((l + r) / 2, (t + b) / 2);
        case CyDisDrawItem::Right:       return QPoint(r, (t + b) / 2);
        case CyDisDrawItem::BottomLeft:  rectPoint = QPoint(l, b); break;
        case CyDisDrawItem::Bottom:      return QPoint((l + r) / 2, b);
        case CyDisDrawItem::BottomRight: rectPoint = QPoint(r, b); break;
        default: return QPoint(0, 0);
        }

        return projectToEllipse(rectPoint, m_localRect).toPoint();
    }

    QPoint Item_Ellipse::getHandlePosInScene(CyDisDrawItem::HandlePosition type) {
        return mapToScene(getHandlePos(type)).toPoint();
    }

    void Item_Ellipse::onContextMenuCreate(QMenu& menu) {
        QAction* act = menu.addAction(tr("Geometric shapes"));
        act->setData(0);
    }

    void Item_Ellipse::onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) {
        if (act && act->data().toUInt() == 0) {
            CyMediaDisEllipseItem_Menu_geo menuW;
            menuW.flushTrans();
            menuW.setWindowTitle(act->text());
            menuW.setPara(
                boundingRectInScene(),
                scene()->sceneRect());
            if (menuW.exec() == QDialog::Accepted) {
                QRect scencRect = menuW.getSetRect();
                setBoundingRectInScene(scencRect.topLeft(), scencRect.bottomRight());
            }
        }
    }

    QRect Item_Ellipse::constrainToSceneByPos(const QRect& r) const {
        if (!scene()) return r;
        QSize sceneSize = scene()->sceneRect().toRect().size();
        int32_t newLeft = std::clamp(r.x(), 0, sceneSize.width() - m_MinSize.width());
        int32_t newTop = std::clamp(r.y(), 0, sceneSize.height() - m_MinSize.height());
        int32_t newRight = std::clamp(r.x() + r.width(), newLeft + m_MinSize.width(), sceneSize.width());
        int32_t newBottom = std::clamp(r.y() + r.height(), newTop + m_MinSize.height(), sceneSize.height());
        return QRect(newLeft, newTop, newRight - newLeft, newBottom - newTop);
    }

    QPointF Item_Ellipse::projectToEllipse(const QPointF& point, const QRectF& rect) {
        if (rect.isEmpty()) return rect.topLeft();

        qreal cx = rect.x() + rect.width() / 2.0;
        qreal cy = rect.y() + rect.height() / 2.0;
        qreal a = rect.width() / 2.0;
        qreal b = rect.height() / 2.0;

        if (a < 1e-6 || b < 1e-6) return QPointF(cx, cy);

        qreal dx = point.x() - cx;
        qreal dy = point.y() - cy;

        // 特殊情况：中心点
        if (std::abs(dx) < 1e-6 && std::abs(dy) < 1e-6) {
            // 默认返回右中点
            return QPointF(cx + a, cy);
        }

        // 计算从中心到 point 的射线与椭圆的交点
        // 公式：(dx * t)^2 / a^2 + (dy * t)^2 / b^2 = 1
        // => t = 1 / sqrt( (dx/a)^2 + (dy/b)^2 )
        qreal dx_a = dx / a;
        qreal dy_b = dy / b;
        qreal t = 1.0 / std::sqrt(dx_a * dx_a + dy_b * dy_b);

        return QPointF(cx + dx * t, cy + dy * t);
    }



    bool Item_Ellipse::isCornerHandle(CyDisDrawItem::HandlePosition type) {
        return (type == CyDisDrawItem::TopLeft ||
            type == CyDisDrawItem::TopRight ||
            type == CyDisDrawItem::BottomLeft ||
            type == CyDisDrawItem::BottomRight);
    }

    CyMediaDisEllipseItem_Menu_geo::CyMediaDisEllipseItem_Menu_geo(QWidget* parent)
        : QDialog(parent) {
        initGUI();
    }

    void CyMediaDisEllipseItem_Menu_geo::setPara(const QRectF& current, const QRectF& max)
    {
        m_XBox->setRange(0, static_cast<int>(max.width()));
        m_YBox->setRange(0, static_cast<int>(max.height()));
        m_WBox->setRange(0, static_cast<int>(max.width()));
        m_HBox->setRange(0, static_cast<int>(max.height()));

        m_XBox->setValue(static_cast<int>(current.center().x()));
        m_YBox->setValue(static_cast<int>(current.center().y()));
        m_WBox->setValue(current.width());
        m_HBox->setValue(current.height());
    }

    QRect CyMediaDisEllipseItem_Menu_geo::getSetRect() {
        qreal cx = m_XBox->value();
        qreal cy = m_YBox->value();
        qreal w = m_WBox->value();
        qreal h = m_HBox->value();

        // 以 (cx, cy) 为中心构造矩形
        return QRect(cx - w / 2.0, cy - h / 2.0, w, h);
    }

    void CyMediaDisEllipseItem_Menu_geo::flushTrans() {
        m_Centerlabel->setText("Circle Center");
        m_Wlabel->setText(tr("Width"));
        m_Hlabel->setText(tr("Height"));
        m_OkBtn->setText(tr("Confirm"));
        m_CancelBtn->setText(tr("Cancel"));
    }

    void CyMediaDisEllipseItem_Menu_geo::initGUI()
    {
        m_Centerlabel = new QLabel(this);
        m_Wlabel = new QLabel(this);
        m_Hlabel = new QLabel(this);
        m_XBox = new QSpinBox(this);
        m_YBox = new QSpinBox(this);
        m_WBox = new QSpinBox(this);
        m_HBox = new QSpinBox(this);
        m_OkBtn = new QPushButton(this);
        m_CancelBtn = new QPushButton(this);

        QGridLayout* centerBoxLyout = new QGridLayout();
        centerBoxLyout->addWidget(m_XBox, 0, 0);
        centerBoxLyout->addWidget(m_YBox, 0, 1);

        QFormLayout* layout = new QFormLayout();
        layout->addRow(m_Centerlabel, centerBoxLyout);
        layout->addRow(m_Wlabel, m_WBox);
        layout->addRow(m_Hlabel, m_HBox);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->addWidget(m_OkBtn);
        btnLayout->addWidget(m_CancelBtn);

        QVBoxLayout* main = new QVBoxLayout(this);
        main->addLayout(layout);
        main->addStretch();
        main->addLayout(btnLayout);

        connect(m_OkBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(m_CancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    }
}
#include "Item_Ellipse.moc"