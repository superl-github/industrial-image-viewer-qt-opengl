#include "DrawItemTool.h"
#include <QMouseEvent>
#include <QLineF>
#include <QDebug>
#include <QTimer>
#include <cmath>
namespace CyDisDrawItem {
    DrawItemTool::DrawItemTool(ItemManager* manager, QGraphicsView* view, QObject* parent/* = nullptr*/)
        : QObject(parent)
        , m_manager(manager)
        , m_view(view) {
        qRegisterMetaType<CyDisDrawItem::BaseItem*>("CyDisDrawItem::BaseItem*");
        if (!m_manager || !m_view) {
            qWarning() << "DrawingTool: manager is null!";
        }
        else {
            m_view->viewport()->installEventFilter(this);
        }
    }
    DrawItemTool::~DrawItemTool() {
        if (m_view) {
            m_view->viewport()->removeEventFilter(this);
        }
    }
    void DrawItemTool::setThemeColor(QColor color) {
        mThemeColor = color;
    }
    void DrawItemTool::setDrawMode(ItemType mode) {
        if (m_mode == mode) return;
        // 清理预览
        if (m_previewItem && false == m_previewItem->isDrawFinished()) {
            m_manager->removeItem(m_previewItem);
            m_previewItem = nullptr;
        }
        // 重置拖拽状态
        m_isDragging = false;
        m_mode = mode;
        m_selectedItem = nullptr;
    }
    void DrawItemTool::setReplaceMode(bool enable) {
        m_replaceMode = enable;
        //m_manager->clearAll();
        // 关闭替换模式时，重置拖拽状态
        if (!enable) {
            m_isDragging = false;
        }
    }
    bool DrawItemTool::eventFilter(QObject* obj, QEvent* event) {
        if (!obj || !event) return QObject::eventFilter(obj, event);
        if (obj != m_view->viewport() || m_mode == ItemType::Invalid) {
            return QObject::eventFilter(obj, event);
        }

        // 存在预览Item时，将所有鼠标事件转发给Item自行处理
        if (m_previewItem) {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if (mouseEvent) {
                QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
                // 转发事件给Item
                m_previewItem->onDrawMouseEvent(event->type(), scenePos);
                // 检查Item是否标记绘制完成
                if (m_previewItem->isDrawFinished()) {
                    finishDrawing();
                    event->accept();
                    return true;
                }
                // 右键点击取消当前绘制
                if (event->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::RightButton) {
                    if (m_previewItem) {
                        m_manager->removeItem(m_previewItem);
                        m_previewItem = nullptr;
                    }
                    //m_mode = ItemType::Invalid;
                    m_isDragging = false; // 重置拖拽状态
                }
                event->accept();
                return true;
            }
        }
        // 无预览Item时，处理初始点击，创建预览Item
        QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if (!mouseEvent) {
            return QObject::eventFilter(obj, event);
        }
        QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
        m_selectedItem = nullptr;
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                if (mouseEvent->button() == Qt::LeftButton) {
                    // 检测是否执行选中某个item
                    QGraphicsItem* pressItem = nullptr;
                    auto pressRe = checkItemSelect(mouseEvent, &pressItem);
                    if (pressRe == press_Select) {
                        m_selectedItem = pressItem;
                        return false;
                    }
                    else if (pressRe == press_NotItem) {
                        return QObject::eventFilter(obj, event);
                    }
                    else {
                        ;//继续绘制
                    }
                    // 判断是否需要阈值
                    bool needThreshold = ItemFactory::requireDragThreshold(m_mode);
                    if (!needThreshold) {
                        // 不需要阈值，直接创建预览Item
                        m_previewItem = ItemFactory::createItem(m_mode);
                        if (m_previewItem) {
                            m_previewItem->setZValue(m_mode == ItemType::Point ? 10 : 1);
                            // 替换模式，移除之前的绘制结果
                            if (m_replaceMode) {
                                if (m_lastItem) {
                                    m_manager->removeItem(m_lastItem, false);
                                    lastItemRemoveWithNoSignal = true;
                                    m_lastItem = nullptr;
                                }
                            }
                            m_bIsDrawing = true;
                            m_previewItem->setSelectedContourColor(mThemeColor);
                            m_previewItem->setPreviewMode(true);
                            m_manager->addItem(m_previewItem);
                            // 转发初始点击事件给Item
                            m_previewItem->onDrawMouseEvent(event->type(), scenePos);
                        }
                    }
                    else {
                        // 需要阈值，记录起点，等待拖拽
                        m_dragStartPos = scenePos;
                        m_isDragging = true;
                    }
                    event->accept();
                    return true;
                }
            }break;

            case QEvent::MouseMove: {
                if (m_isDragging) {
                    // 检查拖拽距离是否超过阈值
                    qreal dx = scenePos.x() - m_dragStartPos.x();
                    qreal dy = scenePos.y() - m_dragStartPos.y();
                    qreal distance = std::sqrt(dx * dx + dy * dy);
                    if (distance >= kDragThreshold) {
                        // 超过阈值，创建预览Item
                        m_previewItem = ItemFactory::createItem(m_mode);
                        if (m_previewItem) {
                            // 替换模式，移除之前的绘制结果
                            if (m_replaceMode) {
                                if (m_lastItem) {
                                    m_manager->removeItem(m_lastItem, false);
                                    lastItemRemoveWithNoSignal = true;
                                    m_lastItem = nullptr;
                                }
                            }
                            m_bIsDrawing = true;
                            m_previewItem->setSelectedContourColor(mThemeColor);
                            m_previewItem->setPreviewMode(true);
                            m_manager->addItem(m_previewItem);
                            // 先转发按下事件
                            m_previewItem->onDrawMouseEvent(QEvent::MouseButtonPress, m_dragStartPos);
                            // 再转发当前移动事件
                            m_previewItem->onDrawMouseEvent(QEvent::MouseMove, scenePos);
                        }
                        // 结束拖拽等待状态
                        m_isDragging = false;
                    }
                }
            }break;

            case QEvent::MouseButtonRelease: {
                if (m_isDragging) {
                    // 点击了但是没移动超过阈值，取消拖拽状态，不创建Item
                    m_isDragging = false;
                }
                if (lastItemRemoveWithNoSignal) {
                    lastItemRemoveWithNoSignal = false;
                    m_manager->sendRemove(lastItemid);
                }
            }break;

            default:
                break;
        }
        return QObject::eventFilter(obj, event);
    }
    void DrawItemTool::updatePreview(const QPointF& currentPos) {
        if (!m_previewItem) return;
        if (m_mode == Point) {
            m_previewItem->setBoundingRectInScene(currentPos.toPoint(), currentPos.toPoint());
        }
        else {
            m_previewItem->setBoundingRectInScene(m_dragStartPos.toPoint(), currentPos.toPoint());
        }
    }
    void DrawItemTool::finishDrawing() {
        m_bIsDrawing = false;
        if (!m_previewItem) return;
        m_previewItem->setPreviewMode(false);
        m_previewItem->setSelected(true);
        m_lastItem = m_previewItem;
        lastItemid = m_previewItem->id();
        m_previewItem = nullptr;
        emit drawItem(m_lastItem);
    }

    qreal DrawItemTool::computeHitTol(QGraphicsItem* item) const {
        if (!item || !m_view) return 2.0;

        constexpr qreal kScreenTolPx = 3.5;

        // item 局部坐标 → viewport 像素坐标 的完整变换
        QTransform itemToViewport = item->sceneTransform() * m_view->viewportTransform();

        // 取局部 x 轴单位向量在 viewport 中的长度 = 每局部单位对应多少像素
        // 用 hypot 而不是只看 m11，是为了兼容 item 或 view 有旋转的情况
        qreal pixelsPerLocal = std::hypot(itemToViewport.m11(), itemToViewport.m12());
        if (pixelsPerLocal < 1e-6) return kScreenTolPx;

        qreal tol = kScreenTolPx / pixelsPerLocal;

        // 按 item 尺寸限幅（避免小图形容差覆盖整体）
        QRectF br = item->boundingRect();
        if (!br.isEmpty()) {
            qreal minSide = qMin(br.width(), br.height());
            tol = qMin(tol, minSide * 0.25);
        }

        // 按 scene 尺寸限幅（避免极端缩放时容差爆炸）
        if (item->scene()) {
            QRectF sr = item->scene()->sceneRect();
            qreal sceneMin = qMin(sr.width(), sr.height());
            tol = qMin(tol, sceneMin * 0.01);
        }

        return qMax(tol, 0.5);
    }

    bool DrawItemTool::hitOnBorder(QGraphicsItem* item, const QPointF& scenePos, qreal tol /*= 6.0*/) {
        if (!item) return false;
        QPointF local = item->mapFromScene(scenePos);
        QPainterPath outline = item->shape();           // 实心路径
        QPainterPathStroker stroker;
        stroker.setWidth(tol * 2);
        QPainterPath band = stroker.createStroke(outline);
        return band.contains(local);
    }

    DrawItemTool::PressItemResult DrawItemTool::checkItemSelect(QMouseEvent* mouseEvent, QGraphicsItem** selectItem) {
        QGraphicsItem* item = m_view->itemAt(mouseEvent->pos());
        //未选中Item
        if (!item) return DrawItemTool::press_Ignore;
        if (selectItem) *selectItem = item;
        QGraphicsObject* graphObj = item->toGraphicsObject();
        if (graphObj == nullptr) return DrawItemTool::press_NotItem;

        //不是点绘制，执行选中
        if (m_mode != Point) return DrawItemTool::press_Select;

        //点可以绘制在其他区域上，条件：
        // 1、Ctrl未按下
        // 2、选中的图形不是点
        BaseItem* baseItem = dynamic_cast<BaseItem*>(graphObj);
        if (!(mouseEvent->modifiers() & Qt::ControlModifier) &&
            (baseItem && baseItem->itemType() != ItemType::Point)) {
            //判断是否选中边框
            if (hitOnBorder(item, m_view->mapToScene(mouseEvent->pos()), computeHitTol(item))) {
                return DrawItemTool::press_Select;   // 点边框 → 选中
            }
            return DrawItemTool::press_Ignore;
        }

        return DrawItemTool::press_Select;
    }

}
//#include "DrawItemTool.moc"
