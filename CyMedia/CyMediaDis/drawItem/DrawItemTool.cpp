#include "DrawItemTool.h"
#include "../CyDMediaDisScen.h"

#include <QMouseEvent>
#include <QLineF>
#include <QDebug>
#include <QTimer>

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

        m_mode = mode;
        m_selectedItem = nullptr;
    }

    void DrawItemTool::setReplaceMode(bool enable) {
        m_replaceMode = enable;

        //m_manager->clearAll();
        if (m_lastItem) {
            m_manager->removeItem(m_lastItem);
            m_lastItem = nullptr;
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
                }
                event->accept();
                return true;
            }
        }
        // 无预览Item时，处理初始点击，创建预览Item
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
                    QGraphicsItem* item = qobject_cast<CyDMediaDisScen*>(m_manager->scene())->itemAtWithoutBack(scenePos, QTransform());
                    if (item) {
                        m_selectedItem = item;
                    }
                    else {
                        m_selectedItem = nullptr;
                        // 替换模式，移除之前的绘制结果
                        if (m_replaceMode) {
                            if (m_lastItem) {
                                m_manager->removeItem(m_lastItem);
                                m_lastItem = nullptr;
                            }
                        }
                        // 创建预览Item
                        m_previewItem = ItemFactory::createItem(m_mode);
                        if (m_previewItem) {
                            m_previewItem->setSelectedContourColor(mThemeColor);
                            m_previewItem->setPreviewMode(true);
                            m_manager->addItem(m_previewItem);
                            // 转发初始点击事件给Item
                            m_previewItem->onDrawMouseEvent(event->type(), scenePos);
                        }
                    }
                }
            }break;

            case QEvent::MouseButtonRelease: {
                if (m_previewItem && )
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
        if (!m_previewItem) return;

        m_previewItem->setPreviewMode(false);
        m_previewItem->setSelected(true);
        m_lastItem = m_previewItem;
        m_previewItem = nullptr;

        emit drawItem(m_lastItem);
    }
}
