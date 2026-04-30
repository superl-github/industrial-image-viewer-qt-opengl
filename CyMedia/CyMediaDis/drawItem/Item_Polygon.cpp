#include "Item_Polygon.h"
#include <QPen>
#include <QBrush>
#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QFormLayout>
#include <algorithm>
namespace CyDisDrawItem {
    class CyMediaDisPolygonItem_Menu_geo : public QDialog {
        Q_OBJECT
    public:
        CyMediaDisPolygonItem_Menu_geo(QWidget* parent = nullptr);
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
    Item_Polygon::Item_Polygon(QGraphicsItem* parent /*= nullptr*/)
        : BaseItem(parent) {
        m_MinSize = QSize(5, 5);
        setPos(0, 0);
        createHandles();
        updateHandles();
    }
    QRectF Item_Polygon::boundingRect() const {
        if (m_points.isEmpty()) {
            return QRectF(0, 0, 0, 0);
        }
        qreal minX = m_points[0].x(), maxX = m_points[0].x();
        qreal minY = m_points[0].y(), maxY = m_points[0].y();
        for (const auto& p : m_points) {
            minX = qMin(minX, p.x());
            maxX = qMax(maxX, p.x());
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }
        if (!m_drawFinished && !m_tempPoint.isNull()) {
            // 绘制过程中包含临时预览点
            minX = qMin(minX, m_tempPoint.x());
            maxX = qMax(maxX, m_tempPoint.x());
            minY = qMin(minY, m_tempPoint.y());
            maxY = qMax(maxY, m_tempPoint.y());
        }
        return QRectF(minX, minY, maxX - minX, maxY - minY);
    }
    QRect Item_Polygon::boundingRectInScene() const {
        auto pos_I = pos().toPoint();
        auto localBBox = boundingRect();
        return QRect(pos_I.x() + localBBox.x(), pos_I.y() + localBBox.y(),
            localBBox.width(), localBBox.height());
    }
    QPainterPath Item_Polygon::shape() const {
        QPainterPath path;
        if (m_points.size() < 1) {
            return path;
        }
        path.moveTo(m_points[0]);
        for (int i = 1; i < m_points.size(); ++i) {
            path.lineTo(m_points[i]);
        }
        if (m_drawFinished) {
            path.closeSubpath(); // 完成后自动闭合
        }
        else if (!m_tempPoint.isNull()) {
            path.lineTo(m_tempPoint); // 绘制中添加预览边
        }
        return path;
    }
    QPainterPath Item_Polygon::pathInScene() const {
        return mapToScene(shape());
    }
    void Item_Polygon::setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals /*= true*/) {
        // 多边形不使用两点式接口，空实现以兼容基类定义
        Q_UNUSED(p1);
        Q_UNUSED(p2);
        Q_UNUSED(needSignals);
    }
    void Item_Polygon::setPainterPathInScene(QPainterPath path, bool needSignals /*= true*/) {
        // 从路径中加载顶点，用于反序列化
        m_points.clear();
        for (int i = 0; i < path.elementCount(); ++i) {
            QPainterPath::Element e = path.elementAt(i);
            m_points.append(e);
        }
        if (m_points.isEmpty()) return;
        // 计算包围盒
        QRectF bbox = path.boundingRect();
        // 设置Item位置，将顶点转换为本地坐标
        setPos(bbox.topLeft());
        for (auto& p : m_points) {
            p -= bbox.topLeft();
        }
        m_drawFinished = true;
        // 重新创建控制点以匹配新的顶点数量
        createHandles();
        prepareGeometryChange();
        update();
        if (needSignals) {
            emit geometryChanged();
        }
    }
    void Item_Polygon::updateHandles() {
        if (m_handles.isEmpty())
            return;
        // 重写更新逻辑，为每个控制点传递正确的id
        for (auto* handle : std::as_const(m_handles)) {
            QPoint handlePos = getHandlePos(handle->position(), handle->id());
            handle->setPos(handlePos);
        }
    }
    bool Item_Polygon::onDrawMouseEvent(QEvent::Type type, const QPointF& scenePos) {
        if (type == QEvent::MouseButtonPress) {
            // 左键点击添加正式顶点
            if (m_points.isEmpty()) {
                // 第一个点，设置Item原点
                setPos(scenePos);
                m_points.append(QPointF(0, 0));
                // 更新控制点
                createHandles();
            }
            else {
                QPointF localTemp = mapFromScene(scenePos);
                // 检查是否接近第一个点，若是则闭合完成绘制
                if (m_points.size() >= 2) {
                    qreal dx = localTemp.x() - m_points[0].x();
                    qreal dy = localTemp.y() - m_points[0].y();
                    const qreal closeThreshold = 10.0; // 闭合检测阈值（像素）
                    if (dx * dx + dy * dy < closeThreshold * closeThreshold) {
                        // 接近起点，不添加新点，直接闭合完成绘制
                        m_drawFinished = true;
                        // 更新控制点
                        createHandles();
                        prepareGeometryChange();
                        updateHandles();
                        update();
                        return true;
                    }
                }
                // 正常添加新顶点
                m_points.append(localTemp);
                // 更新控制点以匹配新的顶点数量
                createHandles();
            }
            prepareGeometryChange();
            update();
            return true;
        }
        else if (type == QEvent::MouseMove) {
            // 鼠标移动更新临时预览点
            m_tempPoint = mapFromScene(scenePos);
            prepareGeometryChange();
            update();
            return true;
        }
        else if (type == QEvent::MouseButtonDblClick) {
            // 双击结束绘制，至少需要2个点
            if (m_points.size() >= 2) {
                m_drawFinished = true;
                // 更新控制点
                createHandles();
                prepareGeometryChange();
                updateHandles();
                update();
            }
            return true;
        }
        return false;
    }
    bool Item_Polygon::isDrawFinished() const {
        return m_drawFinished;
    }
    void Item_Polygon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->setRenderHint(QPainter::Antialiasing);
        QPen pen(m_contour_color_unselect, 0);
        if (isSelected()) {
            pen.setColor(m_contour_color_select);
        }
        // 绘制过程中使用虚线预览
        if (!m_drawFinished) {
            pen.setStyle(Qt::DashLine);
        }
        painter->setPen(pen);
        painter->setBrush(Qt::transparent);
        painter->drawPath(shape());
        // 绘制过程中显示顶点标记
        if (!m_drawFinished) {
            painter->setBrush(pen.color());
            for (const auto& p : m_points) {
                painter->drawEllipse(p, 3, 3);
            }
        }
    }
    QPoint Item_Polygon::constrainToSceneByPos(const QPoint& pos) const {
        if (!scene())
            return pos;
        QSize scenSize = scene()->sceneRect().toRect().size();
        auto localBBox = boundingRect();
        int32_t newLeft = std::clamp(pos.x(), 0, (int)(scenSize.width() - localBBox.width()));
        int32_t newTop = std::clamp(pos.y(), 0, (int)(scenSize.height() - localBBox.height()));
        return QPoint(newLeft, newTop);
    }

    bool Item_Polygon::changeByHandle(CyDisDrawItem::HandlePosition handletype, int id, QPointF mousePos, QPointF delta) {
        if (!scene()) return false;

        if (handletype == CyDisDrawItem::Center) {
            // 保留整体平移逻辑，支持整体移动
            moveBy(delta.x(), delta.y());
            updateHandles();
            prepareGeometryChange();
            update();
            if (m_bTrackGeometryChange) emit geometryChanged();
            return true;
        }
        else if (handletype == CyDisDrawItem::Free) {
            // 处理顶点控制点的移动
            if (id < 0 || id >= m_points.size()) {
                return false;
            }

            // 边界判断：约束鼠标位置到场景范围内
            QRectF sceneRect = scene()->sceneRect();
            qreal constrainedX = std::clamp(mousePos.x(), sceneRect.left(), sceneRect.right());
            qreal constrainedY = std::clamp(mousePos.y(), sceneRect.top(), sceneRect.bottom());
            QPointF constrainedMousePos(constrainedX, constrainedY);

            // 转换为本地坐标
            QPointF localPos = mapFromScene(constrainedMousePos);

            // 更新对应顶点
            m_points[id] = localPos;

            // 更新控制点位置
            updateHandles();
            prepareGeometryChange();
            update();
            if (m_bTrackGeometryChange) emit geometryChanged();

            return true;
        }

        return false;
    }
    void Item_Polygon::createHandles() {
        // 清空旧的控制点，释放资源
        qDeleteAll(m_handles);
        m_handles.clear();

        // 为每个顶点创建Free类型的控制点，id为顶点索引
        for (int i = 0; i < m_points.size(); ++i) {
            auto* handle = new HandleItem(CyDisDrawItem::Free, this);
            handle->setId(i); // 设置控制点的id，用于区分不同顶点
            m_handles.append(handle);
        }
        setHandlesVisible(m_handlesVisible);
    }
    QPoint Item_Polygon::getHandlePos(CyDisDrawItem::HandlePosition type, int id) {
        if (type == CyDisDrawItem::Free && id >= 0 && id < m_points.size()) {
            return m_points[id].toPoint();
        }
        return QPoint(0, 0);
    }
    QPoint Item_Polygon::getHandlePosInScene(CyDisDrawItem::HandlePosition type, int id) {
        return mapToScene(getHandlePos(type, id)).toPoint();
    }
    void Item_Polygon::onContextMenuCreate(QMenu& menu) {
        QAction* act = menu.addAction(tr("Geometric shapes"));
        act->setData(0);
    }
    void Item_Polygon::onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) {
        if (!act || event) return;
        switch (act->data().toUInt()) {
        case 0: {
            CyMediaDisPolygonItem_Menu_geo menuW;
            menuW.flushTrans();
            menuW.setWindowTitle(act->text());
            menuW.setPara(
                boundingRectInScene(),
                { 0.0, 0.0, scene()->sceneRect().width(), scene()->sceneRect().height() });
            auto sel = menuW.exec();
            if (sel == menuW.Accepted) {
                auto setRect = menuW.getSetRect();
                QRect oldRect = boundingRectInScene();
                // 缩放整个多边形到新的矩形
                qreal sx = (qreal)setRect.width() / oldRect.width();
                qreal sy = (qreal)setRect.height() / oldRect.height();
                for (auto& p : m_points) {
                    p.setX(p.x() * sx);
                    p.setY(p.y() * sy);
                }
                setPos(setRect.topLeft());
                updateHandles();
                prepareGeometryChange();
                update();
            }
        }break;
        default: break;
        }
    }
    QRect Item_Polygon::constrainToSceneByPos(const QRect& r) const {
        if (!scene())
            return r;
        QSize scenSize = scene()->sceneRect().toRect().size();
        int32_t newLeft = std::clamp(r.x(), 0, scenSize.width() - m_MinSize.width());
        int32_t newTop = std::clamp(r.y(), 0, scenSize.height() - m_MinSize.height());
        int32_t newRight = std::clamp(r.x() + r.width(), newLeft + m_MinSize.width(), scenSize.width());
        int32_t newBottom = std::clamp(r.y() + r.height(), newTop + m_MinSize.height(), scenSize.height());
        return QRect(newLeft, newTop, newRight - newLeft, newBottom - newTop);
    }
    CyMediaDisPolygonItem_Menu_geo::CyMediaDisPolygonItem_Menu_geo(QWidget* parent /*= nullptr*/)
        : QDialog(parent) {
        initGUI();
    }
    void CyMediaDisPolygonItem_Menu_geo::setPara(QRectF current, QRectF max) {
        m_XBox->setMaximum((int)max.width());
        m_XBox->setValue((int)current.x());
        m_YBox->setMaximum((int)max.height());
        m_YBox->setValue((int)current.y());
        m_WBox->setMaximum((int)max.width());
        m_WBox->setValue((int)current.width());
        m_HBox->setMaximum((int)max.height());
        m_HBox->setValue((int)current.height());
    }
    QRect CyMediaDisPolygonItem_Menu_geo::getSetRect() {
        return QRect{
            m_XBox->value(),
            m_YBox->value(),
            m_WBox->value(),
            m_HBox->value() };
    }
    void CyMediaDisPolygonItem_Menu_geo::flushTrans() {
        m_Xlabel->setText("x");
        m_Ylabel->setText("y");
        m_Wlabel->setText(tr("width"));
        m_Hlabel->setText(tr("height"));
        m_OkBtn->setText(tr("confirm"));
        m_CancelBtn->setText(tr("cancel"));
    }
    void CyMediaDisPolygonItem_Menu_geo::initGUI() {
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
#include "Item_Polygon.moc"
