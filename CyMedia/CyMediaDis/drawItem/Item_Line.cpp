#include "Item_Line.h"

#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QLayout>

namespace CyDisDrawItem {
    class CyMediaDisLineItem_Menu_geo : public QDialog {
        Q_OBJECT

    public:
        CyMediaDisLineItem_Menu_geo(QWidget* parent = nullptr);

    public:
        void setPara(QPointF P1, QPointF P2, QRectF max);
        QLine getSetPara();

        void flushTrans();

    private:
        void onLineTypeBoxChange(int index);

        void onX1ValueChange(int value);

        void onY1ValueChange(int value);

    private:
        void initGUI();

        QLabel* m_Xlabel = nullptr;
        QLabel* m_Ylabel = nullptr;
        QLabel* m_P1label = nullptr;
        QLabel* m_P2label = nullptr;
        QLabel* mLineTypeLabel = nullptr;

        QSpinBox* m_X1Box = nullptr;
        QSpinBox* m_Y1Box = nullptr;
        QSpinBox* m_X2Box = nullptr;
        QSpinBox* m_Y2Box = nullptr;
        QComboBox* mLineTypeBox = nullptr;

        QPushButton* m_OkBtn = nullptr;
        QPushButton* m_CancelBtn = nullptr;
    };

    enum customHandleType {
        StartPoint = 0,
        EndPoint
    };

    Item_Line::Item_Line(QGraphicsItem* parent /*= nullptr*/) {
        m_offsetP1 = QPoint(-50, 0);
        m_offsetP2 = QPoint(50, 0);
        setPos(0, 0);

        createHandles();
        updateHandles();
    }

    Item_Line::~Item_Line() {
        if (m_Menu) {
            m_Menu->close();
            delete m_Menu;
        }
    }

    QRectF Item_Line::boundingRect() const {
        return QRectF(
            qMin(m_offsetP1.x(), m_offsetP2.x()),
            qMin(m_offsetP1.y(), m_offsetP2.y()),
            qAbs(m_offsetP2.x() - m_offsetP1.x()),
            qAbs(m_offsetP2.y() - m_offsetP1.y()));
    }

    QRect Item_Line::boundingRectInScene() const {
        return boundingRect().toRect();
    }

    QPainterPath Item_Line::shape() const {
        QPainterPath path;
        QPainterPathStroker stroker;
        stroker.setWidth(10.0); // 可点击宽度
        path.moveTo(m_offsetP1);
        path.lineTo(m_offsetP2);
        return stroker.createStroke(path);
    }

    QPainterPath Item_Line::pathInScene() const {
        QPainterPath localPath;
        localPath.moveTo(m_offsetP1);
        localPath.lineTo(m_offsetP2);
        return mapToScene(localPath);
    }

    void Item_Line::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        painter->setRenderHint(QPainter::Antialiasing);

        QPen pen(isSelected() ? m_contour_color_select : m_contour_color_unselect, 0);
        painter->setPen(pen); // 线条样式 
        painter->drawLine(m_offsetP1, m_offsetP2); // 绘制本地线段 
    }

    void Item_Line::setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals /*= true*/) {
        setLineInScene(QLine(p1, p2), true);
    }

    QLine Item_Line::lineInScene() const {
        return QLine(mapToScene(m_offsetP1).toPoint(), mapToScene(m_offsetP2).toPoint());
    }

    void Item_Line::setLineInScene(const QLine& line, bool needSignals/* = true*/) {
        if (!scene())
            return;

        // 约束端点不越界
        QPoint newP1 = line.p1();
        QPoint newP2 = line.p2();
        QSize sceneSize = scene()->sceneRect().toRect().size();
        newP1.setX(std::clamp(newP1.x(), 0, sceneSize.width()));
        newP1.setY(std::clamp(newP1.y(), 0, sceneSize.height()));
        newP2.setX(std::clamp(newP2.x(), 0, sceneSize.width()));
        newP2.setY(std::clamp(newP2.y(), 0, sceneSize.height()));
        QLine newLine = QLine(newP1, newP2);


        // 计算新的中心点
        QPoint newCenter = newLine.center();

        // 移动整个item到新中心点
        setPos(newCenter);

        // 计算新的偏移量
        m_offsetP1 = newLine.p1() - newCenter;
        m_offsetP2 = newLine.p2() - newCenter;

        // 更新手柄位置
        updateHandles();

        // 通知图形系统几何形状已改变
        prepareGeometryChange();
        update(); // 触发重绘

        // 发出信号
        if (needSignals) {
            emit geometryChanged();
        }
    }

    void Item_Line::setPainterPathInScene(QPainterPath path, bool needSignals /*= true*/) {
        if (path.isEmpty() || path.elementCount() < 2) {
            return;
        }
        // 从场景路径中提取线段的两个端点
        QPointF p1 = path.elementAt(0);
        QPointF p2 = path.elementAt(1);
        // 复用已有的线段设置逻辑
        setLineInScene(QLineF(p1, p2).toLine(), needSignals);
    }

    QVariant Item_Line::itemChange(GraphicsItemChange change, const QVariant& value) {
        QVariant result = BaseItem::itemChange(change, value);

        // 检查是否是被添加到了场景
        if (change == ItemSceneChange && value.value<QGraphicsScene*>() != nullptr) {
            // 此时 item 即将被添加到一个有效的场景中
            QGraphicsScene* newScene = value.value<QGraphicsScene*>();

            // 检查当前 pos() 是否还是默认的 (0,0)
            // 这可以避免重复初始化
            if (pos() == QPointF(0, 0)) {
                // 将线段的中心点设置为场景的中心
                QPointF sceneCenter = newScene->sceneRect().center();
                setPos(sceneCenter);
                // 手柄位置会随着 pos() 改变而自动更新，因为它们的父 item 移动了
                // 但我们最好显式调用一下以确保万无一失
                updateHandles();
            }
        }

        return result;
    }

    QPoint Item_Line::constrainToSceneByPos(const QPoint& pos) const {
        if (!scene()) return pos;

        QRect currentBBox = boundingRect().toRect(); // 本地包围盒

        // 场景边界
        const QRect sceneRect = scene()->sceneRect().toRect();
        int32_t sceneLeft = sceneRect.x();
        int32_t sceneRight = sceneLeft + sceneRect.width();
        int32_t sceneTop = sceneRect.top();
        int32_t sceneBottom = sceneTop + sceneRect.height();

        // 计算包围盒在场景中的范围
        int32_t currentLeft = pos.x() + currentBBox.x();
        int32_t currentRight = pos.x() + currentBBox.x() + currentBBox.width();
        int32_t currentTop = pos.y() + currentBBox.y();
        int32_t currentBottom = pos.y() + currentBBox.y() + currentBBox.height();

        // 计算需要的平移量（delta）
        int32_t dx = 0, dy = 0;

        // 水平方向：如果右边超了，向左推；如果左边超了，向右推
        if (currentRight > sceneRight) {
            dx = sceneRight - currentRight; // 负值，向左移
        }
        if (currentLeft < sceneLeft) {
            // 注意：可能上面已经向左移了，这里要叠加
            dx += sceneLeft - (currentLeft + dx); // 补足左边缺口
        }
        // 垂直方向
        if (currentBottom > sceneBottom) {
            dy = sceneBottom - currentBottom;
        }
        if (currentTop < sceneTop) {
            dy += sceneTop - (currentTop + dy);
        }

        // 应用平移
        QPoint constrainedPos = pos + QPoint(dx, dy);

        return constrainedPos;
    }

    bool Item_Line::changeByHandle(CyDisDrawItem::HandlePosition handletype, QPointF mousePos, QPointF delta) {
        if (!scene()) return false;
        QPoint mousePos_I = mousePos.toPoint();
        QLine currentLine = lineInScene();
        QPoint currentCenter = currentLine.center();
        QPoint newP1 = currentLine.p1();
        QPoint newP2 = currentLine.p2();

        if (handletype == CyDisDrawItem::Center) {
            QPointF newPos = pos() + delta;
            QPointF constrainedPos = constrainToSceneByPos(newPos.toPoint());

            // 如果位置没变，无需更新
            if (constrainedPos == pos()) {
                return false;
            }

            setPos(constrainedPos);
            updateHandles(); // 手柄随 item 移动自动更新，但显式调用更安全
            prepareGeometryChange(); // 虽然几何没变，但位置变了，boundingRect 在场景中变了
            update();
            //emit geometryChanged(sceneBoundingRect());
            return true;
        }
        else if (handletype == CyDisDrawItem::TopLeft) {
            // 拖动起点
            newP1 = mousePos_I;
        }
        else if (handletype == CyDisDrawItem::BottomRight) {
            // 拖动终点
            newP2 = mousePos_I;
        }
        else {
            return false; // 不支持其他手柄
        }

        setLineInScene(QLine(newP1, newP2), false);
        return true;
    }

    void Item_Line::createHandles() {
        m_handles.clear();
        // 0: Start (TopLeft), 1: End (BottomRight), 2: Center
        m_handles.append(new HandleItem(CyDisDrawItem::TopLeft, this));
        m_handles.append(new HandleItem(CyDisDrawItem::BottomRight, this));
        m_handles.append(new HandleItem(CyDisDrawItem::Center, this));
        updateHandles();
        setHandlesVisible(m_handlesVisible);
    }

    QPoint Item_Line::getHandlePos(CyDisDrawItem::HandlePosition type) {
        switch (type) {
        case CyDisDrawItem::TopLeft: return m_offsetP1;
        case CyDisDrawItem::Center: return QPoint(0, 0);
        case CyDisDrawItem::BottomRight: return m_offsetP2;
        default:
            break;
        }

        return QPoint(0, 0);
    }

    QPoint Item_Line::getHandlePosInScene(CyDisDrawItem::HandlePosition type) {
        switch (type) {
        case CyDisDrawItem::TopLeft: return mapToScene(m_offsetP1).toPoint();
        case CyDisDrawItem::Center: return pos().toPoint();
        case CyDisDrawItem::BottomRight: return mapToScene(m_offsetP2).toPoint();
        default:
            break;
        }

        return pos().toPoint();
    }

    void Item_Line::onContextMenuCreate(QMenu& menu) {
        QAction* act = menu.addAction(tr("Geometric shapes"));
        act->setData(0);
    }

    void Item_Line::onContexMenu(QAction* act, QGraphicsSceneContextMenuEvent* event) {
        switch (act->data().toUInt()) {
        case 0: {
            if (!m_Menu) {
                m_Menu = new CyMediaDisLineItem_Menu_geo();
            }
            m_Menu->flushTrans();
            m_Menu->setWindowTitle(act->text());
            auto scenePath = pathInScene();
            m_Menu->setPara(
                scenePath.elementAt(0),
                scenePath.elementAt(1),
                { 0.0, 0.0, scene()->sceneRect().width(), scene()->sceneRect().height() });
            auto sel = m_Menu->exec();
            if (sel == m_Menu->Accepted) {
                setLineInScene(m_Menu->getSetPara());
            }
        }break;

        default: break;
        }

    }


    CyMediaDisLineItem_Menu_geo::CyMediaDisLineItem_Menu_geo(QWidget* parent /*= nullptr*/)
        :QDialog(parent) {
        initGUI();
    }

    void CyMediaDisLineItem_Menu_geo::setPara(QPointF P1, QPointF P2, QRectF max) {
        m_X1Box->setMaximum(max.width());
        m_X2Box->setMaximum(max.width());
        m_Y1Box->setMaximum(max.height());
        m_Y2Box->setMaximum(max.height());

        m_X1Box->setValue(P1.x());
        m_X2Box->setValue(P2.x());
        m_Y1Box->setValue(P1.y());
        m_Y2Box->setValue(P2.y());

        m_Xlabel->setText(QString("X(0-%1)").arg(max.width()));
        m_Ylabel->setText(QString("Y(0-%1)").arg(max.height()));
    }

    QLine CyMediaDisLineItem_Menu_geo::getSetPara() {

        return QLine(QPoint(m_X1Box->value(), m_Y1Box->value()), QPoint(m_X2Box->value(), m_Y2Box->value()));
    }

    void CyMediaDisLineItem_Menu_geo::flushTrans() {
        m_Xlabel->setText("X");
        m_Ylabel->setText("Y");
        m_P1label->setText(tr("point1"));
        m_P2label->setText(tr("point2"));
        mLineTypeLabel->setText(tr("Line type"));

        mLineTypeBox->setItemText(0, tr("Free"));
        mLineTypeBox->setItemText(1, tr("Horizon"));
        mLineTypeBox->setItemText(2, tr("vertical"));

        m_OkBtn->setText(tr("confirm"));
        m_CancelBtn->setText(tr("cancel"));
    }

    void CyMediaDisLineItem_Menu_geo::onLineTypeBoxChange(int index) {
        m_Y2Box->setVisible(index != 1);
        m_X2Box->setVisible(index != 2);
        if (index == 1) {
            m_Y2Box->setValue(m_Y1Box->value());
        }
        else if (index == 2) {
            m_X2Box->setValue(m_X1Box->value());
        }
    }

    void CyMediaDisLineItem_Menu_geo::onX1ValueChange(int value) {
        if (mLineTypeBox->currentIndex() == 2) {
            m_X2Box->setValue(value);
        }
    }

    void CyMediaDisLineItem_Menu_geo::onY1ValueChange(int value) {
        if (mLineTypeBox->currentIndex() == 1) {
            m_Y2Box->setValue(value);
        }
    }

    void CyMediaDisLineItem_Menu_geo::initGUI() {
        m_Xlabel = new QLabel(this);
        m_Ylabel = new QLabel(this);
        m_P1label = new QLabel(this);
        m_P2label = new QLabel(this);
        mLineTypeLabel = new QLabel(this);

        m_X1Box = new QSpinBox(this);
        m_Y1Box = new QSpinBox(this);
        m_X2Box = new QSpinBox(this);
        m_Y2Box = new QSpinBox(this);

        connect(m_X1Box, QOverload<int>::of(&QSpinBox::valueChanged), this, &CyMediaDisLineItem_Menu_geo::onX1ValueChange);
        connect(m_Y1Box, QOverload<int>::of(&QSpinBox::valueChanged), this, &CyMediaDisLineItem_Menu_geo::onY1ValueChange);

        mLineTypeBox = new QComboBox(this);
        mLineTypeBox->addItem("");
        mLineTypeBox->addItem("");
        mLineTypeBox->addItem("");
        connect(mLineTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CyMediaDisLineItem_Menu_geo::onLineTypeBoxChange);

        m_OkBtn = new QPushButton(this);
        m_CancelBtn = new QPushButton(this);

        QGridLayout* propertyLayout = new QGridLayout();
        propertyLayout->addWidget(m_Xlabel, 0, 1);
        propertyLayout->addWidget(m_Ylabel, 0, 2);

        propertyLayout->addWidget(m_P1label, 1, 0);
        propertyLayout->addWidget(m_X1Box, 1, 1);
        propertyLayout->addWidget(m_Y1Box, 1, 2);

        propertyLayout->addWidget(m_P2label, 2, 0);
        propertyLayout->addWidget(m_X2Box, 2, 1);
        propertyLayout->addWidget(m_Y2Box, 2, 2);

        propertyLayout->addWidget(mLineTypeLabel, 3, 0);
        propertyLayout->addWidget(mLineTypeBox, 3, 1, 1, 2);

        QHBoxLayout* BtnLayout = new QHBoxLayout();
        BtnLayout->addWidget(m_OkBtn);
        BtnLayout->addWidget(m_CancelBtn);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(propertyLayout);
        mainLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
        mainLayout->addLayout(BtnLayout);

        connect(m_OkBtn, &QPushButton::clicked, this, [this]() {
            this->accept();
            });
        connect(m_CancelBtn, &QPushButton::clicked, this, [this]() {
            this->reject();
            });

        m_Xlabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_Ylabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
}
#include "LineItem.moc"

