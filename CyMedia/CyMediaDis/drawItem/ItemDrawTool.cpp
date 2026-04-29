#include "ItemDrawTool.h"

#include <QMouseEvent>
#include <QLineF>
#include <QDebug>
#include <QTimer>

#include "../CyDMediaDisScen.h"

namespace CyDisDrawItem {

    void ItemDrawTool::setThemeColor(QColor color) {
        mThemeColor = color;
    }

    ItemDrawTool::ItemDrawTool(ItemManager* manager, QGraphicsView* view, QObject* parent/* = nullptr*/)
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

    void ItemDrawTool::setDrawMode(ItemType mode) {
        if (m_mode == mode) return;

        // 清理预览
        if (m_previewItem) {
            m_manager->removeItem(m_previewItem);
            m_previewItem = nullptr;
        }

        m_mode = mode;
        m_isDragging = false;
        m_selectedItem = nullptr;
    }

    void ItemDrawTool::setReplaceMode(bool enable) {
        m_replaceMode = enable;

        //m_manager->clearAll();
        if (m_lastItem) {
            m_manager->removeItem(m_lastItem);
            m_lastItem = nullptr;
        }
    }

    bool ItemDrawTool::eventFilter(QObject* obj, QEvent* event) {
        if (obj != m_view->viewport() || m_mode == ItemType::Invalid) {
            return QObject::eventFilter(obj, event);
        }

        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
                    QGraphicsItem* item = qobject_cast<CyDMediaDisScen*>(m_manager->scene())->itemAtWithoutBack(scenePos, QTransform());

                    if (item) {
                        m_selectedItem = item;
                        // 注意：此时不应重置 m_isDragging，因为根本没开始绘制
                    }
                    else {
                        // 开始潜在的绘制流程：记录起点，但不创建预览
                        m_dragStartPos = scenePos;
                        m_isDragging = true;          // 尚未拖动
                        m_selectedItem = nullptr;
                        //单个模式，移除之前的
                        if (m_replaceMode) {
                            //m_manager->clearAll();
                            if (m_lastItem) {
                                m_manager->removeItem(m_lastItem);
                                m_lastItem = nullptr;
                            }
                        }

                        if (m_mode == Point) {
                            startDrawing(scenePos);
                        }
                    }
                }
            }break;

            case QEvent::MouseMove: {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
                if (m_isDragging) {
                    //创建
                    if (!m_previewItem) {
                        // 判断是否满足“拖拽开始”条件
                        QPointF delta = scenePos - m_dragStartPos;
                        if (delta.manhattanLength() >= kDragThreshold) {
                            startDrawing(scenePos);
                        }
                    }
                    //更新
                    else if (m_previewItem) {
                        updatePreview(scenePos);
                    }
                }
            }break;

            case QEvent::MouseButtonRelease: {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    if (m_isDragging && m_previewItem) {
                        // 完成绘制
                        finishDrawing();
                    }
                    else {
                        // 未拖拽：视为点击
                        // 可选：清空选中（根据需求）
                        // m_manager->scene()->clearSelection();
                    }
                    m_isDragging = false;
                }
            }break;

            default:
                break;
        }

        return QObject::eventFilter(obj, event);
    }

    void ItemDrawTool::startDrawing(const QPointF& pos) {
        // 创建预览项：起点 = startPos，当前点也设为 startPos（后续由 updatePreview 更新）
        m_previewItem = ItemFactory::createBySceneDraw(
            m_mode, m_dragStartPos, pos
        );

        if (m_previewItem) {
            m_previewItem->setSelectedContourColor(mThemeColor);
            m_previewItem->setPreviewMode(true);
            m_manager->addItem(m_previewItem);
        }
    }

    void ItemDrawTool::updatePreview(const QPointF& currentPos) {
        if (!m_previewItem) return;

        if (m_mode == Point) {
            m_previewItem->setBoundingRectInScene(currentPos.toPoint(), currentPos.toPoint());
        }
        else {
            m_previewItem->setBoundingRectInScene(m_dragStartPos.toPoint(), currentPos.toPoint());
        }
    }

    void ItemDrawTool::finishDrawing() {
        if (!m_previewItem) return;

        m_previewItem->setPreviewMode(false);
        m_manager->addItem(m_previewItem); // 可能只是从 preview 列表移到正式列表
        m_previewItem->setSelected(true);
        m_lastItem = m_previewItem;
        m_previewItem = nullptr;

        emit drawItem(m_lastItem);
    }
}
