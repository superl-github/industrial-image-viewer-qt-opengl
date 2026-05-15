#include "DrawItemTool.h"
#include "../CyDMediaDisScen.h"
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
                }
                // 右键点击取消当前绘制
                if (event->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::RightButton) {
                    m_manager->removeItem(m_previewItem);
                    m_previewItem = nullptr;
                    m_mode = ItemType::Invalid;
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
        QGraphicsItem* item = qobject_cast<CyDMediaDisScen*>(m_manager->scene())->itemAtWithoutBack(scenePos, QTransform());
        if (item) {
            m_selectedItem = item;
            return QObject::eventFilter(obj, event);
        }
        m_selectedItem = nullptr;

        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                if (mouseEvent->button() == Qt::LeftButton) {
                    // 替换模式，移除之前的绘制结果
                    if (m_replaceMode) {
                        if (m_lastItem) {
                            m_manager->removeItem(m_lastItem, false);
                            lastItemRemoveWithNoSignal = true;
                            m_lastItem = nullptr;
                        }
                    }

                    // 判断是否需要阈值
                    bool needThreshold = ItemFactory::requireDragThreshold(m_mode);
                    if (!needThreshold) {
                        // 不需要阈值，直接创建预览Item
                        m_previewItem = ItemFactory::createItem(m_mode);
                        if (m_previewItem) {
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
                }
                else if (mouseEvent->button() == Qt::RightButton) {
                    if (m_lastItem) {
                        m_manager->removeItem(m_lastItem);
                        m_lastItem = nullptr;
                    }
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
}
//#include "DrawItemTool.moc"
