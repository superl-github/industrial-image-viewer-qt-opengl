#include "BaseItem.h"

#include <QTimer>
#include <QThread>
#include <QApplication>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>

namespace CyDisDrawItem {
    //====== class CyDisDrawItem::BaseItem ======

    BaseItem::BaseItem(QGraphicsItem* parent /*= nullptr*/)
        : QGraphicsObject(nullptr)
        , m_id(QUuid::createUuid()) {
        this->setFlags(
            QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsMovable |
            QGraphicsItem::ItemIsFocusable |
            QGraphicsItem::ItemSendsGeometryChanges);
        setAcceptHoverEvents(true);

        ////调试
        //connect(this, &BaseItem::geometryChanged, this, [this]() {
        //    CyDisDrawItem::BaseItem* senderItem = qobject_cast<CyDisDrawItem::BaseItem*>(sender());
        //    auto rect = senderItem->boundingRectInScene();
        //    printf("geometryChanged:(%d,%d) %d * %d\n", rect.x(), rect.y(), rect.width(), rect.height());
        //    });

        // 初始化闪烁定时器
        m_flickeringTimer = new QTimer(this);
        connect(m_flickeringTimer, &QTimer::timeout, this, &BaseItem::onFlickeringTimeout);
        m_flickeringTimer->setInterval(500); // 500ms切换一次，闪烁频率1Hz

        // ==== 文本子项 ====
        m_label = new QGraphicsSimpleTextItem(this);
        // 关键：忽略 view 的缩放/旋转变换 → 字号在屏幕上恒定
        m_label->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        // 不抢鼠标事件（右键/点击仍落到 BaseItem 上）
        m_label->setAcceptedMouseButtons(Qt::NoButton);
        m_label->setFlag(QGraphicsItem::ItemIsSelectable, false);
        m_label->setFlag(QGraphicsItem::ItemIsMovable, false);
        m_label->setBrush(m_contour_color_unselect);
        m_label->setVisible(false);
    }

    BaseItem::~BaseItem() {
        removeHandles();
        removeViewFilters();
    }

	QPainterPath BaseItem::shape() const {
        {
            QPainterPath path;
            path.addRect(boundingRect());
            return path;
        };
    }

    QPainterPath BaseItem::pathInScene() const {
        return QPainterPath();
    }

    bool BaseItem::onDrawMouseEvent(QEvent::Type type, const QPointF& scenePos) {
        Q_UNUSED(type);
        Q_UNUSED(scenePos);
        return false;
    }

    bool BaseItem::isDrawFinished() const {
        return false;
    }

    bool BaseItem::isPreViewMode() {
        return m_isPreviewMode;
    }

    void BaseItem::setPreviewMode(bool preview) {
        if (m_isPreviewMode == preview) return;

        m_isPreviewMode = preview;

        if (preview) {
            setFlag(ItemIsSelectable, false);
            setFlag(ItemIsMovable, false);
            setHandlesVisible(false);
        }
        else {
            // 正常模式：恢复交互
            setFlag(ItemIsSelectable, true);
            setFlag(ItemIsMovable, true);
            setHandlesVisible(isSelected());
        }
        updateTipLabel();// 预览模式隐藏 tip
    }

    void BaseItem::setHandleColor(QColor color) {
        if (m_handles.size() <= 0)
            return;

        for (auto* handle : m_handles) {
            handle->setColor(color);
        }

        if (m_handlesVisible) {
            update();
        }
    }

    void BaseItem::setHandlesVisible(bool visible) {
        if (m_handlesVisible == visible)
            return;
        m_handlesVisible = visible;
        for (auto* handle : m_handles) {
            handle->setVisible(visible && isSelected());
        }
    }

    void BaseItem::setUnSelectedContourColor(QColor color) {
        m_contour_color_unselect = color;
        if (m_label && !isSelected()) m_label->setBrush(color);
        update();
    }

    void BaseItem::setSelectedContourColor(QColor color) {
        m_contour_color_select = color;
        if (m_label && isSelected()) m_label->setBrush(color);
        update();
    }

    bool BaseItem::trackingGeometry() {
        return m_bTrackGeometryChange;
    }

    void BaseItem::setTrackingGeometry(bool track) {
        m_bTrackGeometryChange = track;
    }

    void BaseItem::pathToMask(const QPainterPath& scenePath, QImage& maskImage) {
        if (maskImage.isNull() || maskImage.format() != QImage::Format_Grayscale8) {
            return;
        }

        // 清空为黑色背景
        maskImage.fill(Qt::black);

        if (scenePath.isEmpty()) return;

        QPainter painter(&maskImage);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.drawPath(scenePath);
    }

	void BaseItem::registerCreateContextMenuFunc(ItemCreateContexMenuCallBack func) {
        m_createContextMenuFunc = func;
	}

	void BaseItem::registerContextMenuTriggerFunc(ItemContexMenuTriger func, void* pUser) {
        m_ContextMenuTriigerFunc = func;
        m_ContextMenuTriigerFunc_user = pUser;
	}

    bool BaseItem::flickeringEnable() {
        return m_flickeringEnable;
    }

    void BaseItem::setFlickeringEnable(bool enable) {
        if (m_flickeringEnable == enable)
            return;

        m_flickeringEnable = enable;
        if (enable) {
            // 保存原始颜色
            m_oldContourColorUnselect = m_contour_color_unselect;
            m_oldContourColorSelect = m_contour_color_select;

            // 初始状态显示用户设置的闪烁颜色
            m_flickeringState = true;
            m_contour_color_unselect = m_flickeringColor;
            m_contour_color_select = m_flickeringColor;
            
            //启动定时器
            if (QThread::currentThread() != qApp->thread()) {
                QMetaObject::invokeMethod(this, [this]() mutable {
                    m_flickeringTimer->start();
                    });
            }
            else {
                m_flickeringTimer->start();
            }
        }
        else {
            // 停止定时器
            if (QThread::currentThread() != qApp->thread()) {
                QMetaObject::invokeMethod(this, [this]() mutable {
                    m_flickeringTimer->stop();
                    });
            }
            else {
                m_flickeringTimer->stop();
            }

            // 恢复原始颜色
            m_contour_color_unselect = m_oldContourColorUnselect;
            m_contour_color_select = m_oldContourColorSelect;
        }

        if (QThread::currentThread() == qApp->thread()) {
            // 文本
            if (m_label) m_label->setBrush(isSelected() ? m_contour_color_select : m_contour_color_unselect);
            update();
        }
        else {
            QTimer::singleShot(0, this, [this]() {
                // 文本
                if (m_label) m_label->setBrush(isSelected() ? m_contour_color_select : m_contour_color_unselect);
                });
        }
    }

    QColor BaseItem::flickeringColor() {
        return m_flickeringColor;
    }

    void BaseItem::setFlickeringColor(QColor color) {
        if (m_flickeringColor == color)
            return;

        m_flickeringColor = color;

        // 如果当前正在闪烁且处于显示用户颜色的状态，立即更新
        if (m_flickeringEnable && m_flickeringState) {
            m_contour_color_unselect = color;
            m_contour_color_select = color;
            update();
        }
    }

    void BaseItem::setShowTip(bool show) {
        if (m_showTip == show) return;
        m_showTip = show;
        updateTipLabel();
        if (QThread::currentThread() == qApp->thread()) update();
    }

    void BaseItem::setTipText(QString text) {
        if (m_tipText == text) return;
        m_tipText = std::move(text);
        updateTipLabel();
        if (m_showTip && QThread::currentThread() == qApp->thread()) update();
    }

    void BaseItem::setTipFont(const QFont& font) {
        if (!m_label) return;
        m_label->setFont(font);
        updateTipLabel();
    }

    QFont BaseItem::tipFont() const {
        return m_label ? m_label->font() : QFont();
    }

    bool BaseItem::eventFilter(QObject* obj, QEvent* event) {
        switch (event->type()) {
        case QEvent::Wheel:      // 滚轮缩放
        case QEvent::Resize:     // 窗口大小变化
        case QEvent::Scroll:     // 滚动条拖动
        case QEvent::MouseButtonDblClick: // 双击自适应等场景
        case QEvent::KeyPress:   // 可能有快捷键缩放
            // 事件的默认处理会修改 viewportTransform，
            // 所以延后到事件循环下一轮再刷新标签位置
            QTimer::singleShot(0, this, [this]() { updateTipLabel(); });
            break;
        default:
            break;
        }
        return QGraphicsObject::eventFilter(obj, event);
    }

    void BaseItem::updateHandles() {
        if (m_handles.size()) {
            for (auto* handle : std::as_const(m_handles)) {
                handle->setPos(getHandlePos(handle->position()));
            }
        }
        updateTipLabel();// 子类几何变化都会调 updateHandles()，顺带把 label 也刷了
    }

    void BaseItem::removeHandles() {
        for (auto* handle : std::as_const(m_handles)) {
            if (handle->scene()) {
                handle->scene()->removeItem(handle);
            }
            delete handle;
        }
    }

    QVariant BaseItem::itemChange(GraphicsItemChange change, const QVariant& value) {
        if (change == QGraphicsItem::ItemPositionChange) {
            if (scene()) {
                QPoint newPos = value.toPointF().toPoint();
                newPos = constrainToSceneByPos(newPos);
                newPos.setX(int(newPos.x()));
                newPos.setY(int(newPos.y()));
                return newPos; // 返回修正后的 scene 位置
            }
        }
        else if (change == QGraphicsItem::ItemSelectedHasChanged) {
            setHandlesVisible(value.toBool());
            if (m_label) {
                m_label->setBrush(value.toBool() ? m_contour_color_select
                    : m_contour_color_unselect);
            }
            emit selectedChanged();
        }
        else if (change == QGraphicsItem::ItemSceneHasChanged) {
            removeViewFilters();
            // 视图可能在场景赋值后才注册，延后一拍安装
            QTimer::singleShot(0, this, [this]() {
                installViewFilters();
                updateTipLabel();
                });
        }
        else if (change == ItemPositionHasChanged) {
            // 只要位置变了（且可移动），就标记
            m_positionChangedDuringDrag = true;
            if (m_bTrackGeometryChange) {
                emit geometryChanged();
            }
        }

        return QGraphicsItem::itemChange(change, value);
    }

    void BaseItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            setCursor(Qt::ClosedHandCursor);
            m_positionChangedDuringDrag = false; // 每次按下重置
        }
        QGraphicsItem::mousePressEvent(event);
    }

    void BaseItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
        QGraphicsObject::mouseMoveEvent(event);
    }

    void BaseItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
        QGraphicsObject::mouseReleaseEvent(event);

        if (event->button() == Qt::LeftButton) {
            setCursor(Qt::ArrowCursor);
            if (m_positionChangedDuringDrag) {
                m_positionChangedDuringDrag = false;
                if (false == m_bTrackGeometryChange) {
                    emit geometryChanged();
                }
            }
        }
    }

    void BaseItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
        QMenu menu;
        auto menuMap = onContextMenuCreate(menu);
        if (menu.actions().size() <= 0)
            return QGraphicsObject::contextMenuEvent(event);

        QAction* selectedAction = menu.exec(event->screenPos());
        if (!selectedAction) return;
        auto findeType = menuMap.find(selectedAction);
        if (findeType != menuMap.end()) {
            onContexMenu(findeType.value(), event);
        }
        else {
            if (m_ContextMenuTriigerFunc) m_ContextMenuTriigerFunc(m_id, selectedAction->data().toInt(), m_ContextMenuTriigerFunc_user);
        }
        event->accept();
    }

	void BaseItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
        setCursor(Qt::ArrowCursor);
        QGraphicsObject::hoverEnterEvent(event);
    }

    void BaseItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
        setCursor(Qt::ArrowCursor);
        QGraphicsObject::hoverLeaveEvent(event);
    }

	QString BaseItem::getContextStr(ContextMenuType contexType) {
		switch (contexType) {
		case CyDisDrawItem::BaseItem::Contex_Geometric: return tr("Geometric shapes");
		case CyDisDrawItem::BaseItem::Contex_Delete: return tr("Delete");
		}
		return ("None");
	}

	bool BaseItem::getContextSupport(ContextMenuType contexType) {
		switch (contexType) {
		    case CyDisDrawItem::BaseItem::Contex_Geometric: return true;
		    case CyDisDrawItem::BaseItem::Contex_Delete: return true;
		}
		return false;
	}

    QMap<QAction*, BaseItem::ContextMenuType> BaseItem::onContextMenuCreate(QMenu& menu) {
        QMap<QAction*, BaseItem::ContextMenuType> menuMap;
		for (int i = 0; i < Contex_End; i++) {
            if (getContextSupport(ContextMenuType(i))) {
				QAction* act = menu.addAction(getContextStr(ContextMenuType(i)));
				act->setData(i);
                menuMap.insert(act, BaseItem::ContextMenuType(i));
            }
		}
        if (m_createContextMenuFunc) {
            m_createContextMenuFunc(&menu);
        }

        return menuMap;
	}

    void BaseItem::updateTipLabel() {
        if (!m_label) return;

        const bool visible = m_showTip && !m_tipText.isEmpty() && !m_isPreviewMode;
        if (m_label->text() != m_tipText)
            m_label->setText(m_tipText);
        m_label->setVisible(visible);
        if (!visible) return;

        const QPointF anchor = boundingRect().center();
        const QSizeF  labelPx = m_label->boundingRect().size(); // 屏幕像素尺寸
        
        // 拿不到视图时退化为居中
        if (!scene() || scene()->views().isEmpty()) {
            m_label->setPos(anchor - QPointF(labelPx.width() / 2.0, labelPx.height() / 2.0));
            return;
        }
        QGraphicsView* view = scene()->views().first();
        const QTransform itemToViewport = sceneTransform() * view->viewportTransform();
        if (!itemToViewport.isInvertible()) return;
        const QTransform viewportToItem = itemToViewport.inverted();
        const QRectF  vpRect = view->viewport()->rect();
        const QPointF anchorVp = itemToViewport.map(anchor);  // 锚点在视口坐标的位置
        const qreal   gap = 6.0;                          // 图形与标签之间的间距（像素）

        // 图形靠左半屏 → 放右边；靠右半屏 → 放左边
        const bool inLeftHalf = anchorVp.x() < vpRect.center().x();
        qreal x = inLeftHalf ? (anchorVp.x() + gap)
            : (anchorVp.x() - gap - labelPx.width());

        const bool inTopHalf = anchorVp.y() < vpRect.center().y();
        qreal y = inTopHalf ? (anchorVp.y() + gap)
            : (anchorVp.y() - gap - labelPx.height());

        // 最后再把标签夹进视口，避免贴边时溢出半个像素
        x = qBound(vpRect.left(), x, vpRect.right() - labelPx.width());
        y = qBound(vpRect.top(), y, vpRect.bottom() - labelPx.height());

        // 屏幕坐标 → 父项坐标
        m_label->setPos(viewportToItem.map(QPointF(x, y)));
    }

    void BaseItem::onFlickeringTimeout() {
        if (!m_flickeringEnable)
            return;

        // 切换闪烁状态
        m_flickeringState = !m_flickeringState;

        if (m_flickeringState) {
            // 显示用户设置的颜色
            m_contour_color_unselect = m_flickeringColor;
            m_contour_color_select = m_flickeringColor;
        }
        else {
            // 显示灰色
            m_contour_color_unselect = Qt::gray;
            m_contour_color_select = Qt::gray;
        }
        //文本
        if (m_label) m_label->setBrush(isSelected() ? m_contour_color_select
            : m_contour_color_unselect);

        update();
    }

    void BaseItem::installViewFilters() {
        if (!scene()) return;
        const auto views = scene()->views();
        for (auto* view : views) {
            if (view && view->viewport()) {
                view->viewport()->installEventFilter(this);
            }
        }
    }

    void BaseItem::removeViewFilters() {
        // 遍历所有可能的 viewport（包括 scene 已为空的边界情况）
        const auto views = scene() ? scene()->views() : QList<QGraphicsView*>();
        for (auto* view : views) {
            if (view && view->viewport()) {
                view->viewport()->removeEventFilter(this);
            }
        }
    }






    //====== class CyDisDrawItem::HandleItem ======
    HandleItem::HandleItem(HandlePosition pos, BaseItem* parent)
        : QGraphicsItem(parent)
        , m_parent(parent)
        , m_type(pos)
        , m_isResizing(false) {
        setFlag(QGraphicsItem::ItemIsMovable, false); // 由父类控制位置
        setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        setAcceptedMouseButtons(Qt::LeftButton);
        setCursor(Qt::SizeFDiagCursor); // 默认，后面会根据方向调整

        // 设置光标
        /*switch (pos) {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Center:
            setCursor(Qt::SizeAllCursor);
            break;
        }*/
        setCursor(Qt::PointingHandCursor);
    }

    int HandleItem::handleSize() {
        return m_handleSize;
    }

    QPointF HandleItem::posFromRect(const QRectF& rect) {
        switch (m_type) {
        case TopLeft:      return rect.topLeft();
        case Top:          return QPointF(rect.center().x(), rect.top());
        case TopRight:     return rect.topRight();
        case Left:         return QPointF(rect.left(), rect.center().y());
        case Center:       return rect.center();
        case Right:        return QPointF(rect.right(), rect.center().y());
        case BottomLeft:   return rect.bottomLeft();
        case Bottom:       return QPointF(rect.center().x(), rect.bottom());
        case BottomRight:  return rect.bottomRight();
        default:           return rect.center();
        }
    }

    QRectF HandleItem::boundingRect() const {
        return QRectF(-m_handleSize / 2, -m_handleSize / 2, m_handleSize, m_handleSize);
    }

    void HandleItem::setColor(QColor color) {
        m_color = color;
    }

    void HandleItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        painter->setPen(Qt::black);
        painter->setBrush(m_color);
        painter->drawRect(boundingRect());
        //painter->drawEllipse(boundingRect());
    }

    void HandleItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            if (m_parent && !m_parent->isSelected()) {
                m_parent->setSelected(true);
            }

            m_isResizing = true;
            m_dragStartScenePos = event->scenePos();
            grabMouse();
            event->accept();
        }
    }

    void HandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
        if (!m_isResizing) return;
        if (!m_parent || !m_parent->scene()) {
            event->ignore();
            return;
        }

        m_parent->changeByHandle(m_type, m_id, event->scenePos(), event->scenePos() - m_dragStartScenePos);

        m_dragStartScenePos = m_parent->getHandlePosInScene(m_type);
        event->accept();
    }

    void HandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
        if (m_isResizing) {
            m_isResizing = false;
            ungrabMouse();
            // 最终 emit 信号
            if (m_parent && false == m_parent->trackingGeometry()) {
                m_parent->emit geometryChanged();
            }
            event->accept();
        }
    }

    void HandleItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
        if (!m_parent) {
            QGraphicsItem::contextMenuEvent(event); return;
        }
        // 点击手柄右键，先确保父item选中，和直接点item本体行为一致
        if (!m_parent->isSelected()) {
            m_parent->setSelected(true);
        }
        // 将右键事件转发给父BaseItem，复用已经写好的contextMenuEvent逻辑
        m_parent->contextMenuEvent(event);
        event->accept();
    }

}