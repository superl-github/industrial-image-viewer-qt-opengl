#include "ItemDrawTool.h"

#include <QMouseEvent>
#include <QLineF>
#include <QDebug>

namespace CyDisDrawItem {
    ItemDrawTool::ItemDrawTool(ItemManager* manager, QGraphicsView* view, QObject* parent/* = nullptr*/)
        : QObject(parent)
        , m_manager(manager)
        , m_view(view) {

        if (!m_manager || !m_view) {
            qWarning() << "DrawingTool: manager is null!";
        }
        else {
            m_view->viewport()->installEventFilter(this);
        }
    }

    void ItemDrawTool::setDraMode(DrawMode mode) {
        if (m_mode == mode) return;

        // 如果正在绘制，取消预览
        if (m_previewItem) {
            m_manager->scene()->removeItem(m_previewItem);
            delete m_previewItem;
            m_previewItem = nullptr;
        }

        m_mode = mode;
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
        if (obj != m_view->viewport() || m_mode == DrawMode::DRAW_None) {
            return QObject::eventFilter(obj, event);
        }

        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
                QGraphicsItem* item = m_manager->scene()->itemAt(scenePos, QTransform());
                if (item && item != m_previewItem) {
                    m_selectedItem = item;
                }
                else {
                    startDrawing(scenePos);
                }
            }
        }break;

        case QEvent::MouseMove: {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
            updatePreview(scenePos);
        }break;

        case QEvent::MouseButtonRelease: {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                finishDrawing();
            }
        }break;

        default:
            break;
        }

        return QObject::eventFilter(obj, event);
    }

    void ItemDrawTool::startDrawing(const QPointF& pos) {
        m_startPos = pos;
        m_previewItem = m_manager->createByScenDraw(CyDisDrawItem::ItemType(m_mode), m_startPos, m_startPos);
        if (m_previewItem) {
            if (m_replaceMode) {
                //m_manager->clearAll();
                if (m_lastItem) {
                    m_manager->removeItem(m_lastItem);
                    m_lastItem = nullptr;
                }
            }
            m_previewItem->setPreviewMode(true);
            m_manager->scene()->addItem(m_previewItem);
        }
    }

    void ItemDrawTool::updatePreview(const QPointF& currentPos) {
        if (!m_previewItem)
            return;

        m_previewItem->setBoundingRectInScene(m_startPos, currentPos);
    }

    void ItemDrawTool::finishDrawing() {
        if (!m_previewItem) return;

        m_previewItem->setPreviewMode(false);
        m_manager->addItem(m_previewItem); // 安全添加（内部防重复）
        m_previewItem->setSelected(true);

        m_lastItem = m_previewItem;
        m_previewItem = nullptr;
    }
}
