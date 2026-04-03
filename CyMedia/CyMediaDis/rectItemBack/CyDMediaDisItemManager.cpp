#include "CyDMediaDisItemManager.h"
#include <QGraphicsScene>
#include <QDebug>

#include "drawItem/RectItem.h"
#include "drawItem/LineItem.h"
#include "drawItem/CyMediaDisEllipseItem.h"

CyDMediaDisItemManager::CyDMediaDisItemManager(QGraphicsScene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene) {
    if (!m_scene) {
        qWarning() << "DrawingManager: scene is null!";
    }
}

CyDMediaDisItemManager::~CyDMediaDisItemManager() {
    clearAll();
}

CyMediaDisBaseItem* CyDMediaDisItemManager::createByScenDraw(CyMediaDisBaseItem::ItemType type, QPointF startPos, QPointF endPos) {
    CyMediaDisBaseItem* item = nullptr;
    switch (type) {
        case CyMediaDisBaseItem::ItemType::Invalid:
            break;

        case CyMediaDisBaseItem::ItemType::Rectangle: {
            item = new CyMediaDisRectItem();
            item->setBoundingRectInScene(startPos, endPos,false);
        }break;

        case CyMediaDisBaseItem::ItemType::Line: {
            item = new CyMediaDisLineItem();
            item->setBoundingRectInScene(startPos, endPos, false);
        }break;

        case CyMediaDisBaseItem::ItemType::Ellipse: {
            item = new CyMediaDisEllipseItem();
            item->setBoundingRectInScene(startPos, endPos, false);
        }break;
        default:
            break;
    }

    return item;
}

void CyDMediaDisItemManager::addItem(CyMediaDisBaseItem* item) {
    if (!item || !m_scene) return;
    if (m_items.contains(item)) return;

    // 添加到场景
    if (item->scene() != m_scene) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        m_scene->addItem(item);
    }
    

    // 加入管理列表
    m_items.append(item);

    // 监听选中状态变化
    connect(item, &CyMediaDisBaseItem::selectedChanged,
        this, &CyDMediaDisItemManager::onItemSelectionChanged,
        Qt::UniqueConnection);

    emit itemAdded(item);
}

void CyDMediaDisItemManager::removeItem(CyMediaDisBaseItem* item) {
    if (!item || !m_items.contains(item)) return;

    m_scene->removeItem(item);
    m_items.removeOne(item);
    disconnect(item, &CyMediaDisBaseItem::selectedChanged, this, nullptr);

    // 如果是当前选中项，清空
    if (m_selectedItem == item) {
        m_selectedItem = nullptr;
        emit selectionChanged(nullptr);
    }
    item->deleteLater();
}

void CyDMediaDisItemManager::clearAll() {
    for (auto* item : m_items) {
        item->setSelected(false);
    }

    for (auto* item : m_items) {
        m_scene->removeItem(item);
    }

    for (auto* item : m_items) {
        disconnect(item, &CyMediaDisBaseItem::selectedChanged, this, nullptr);
        item->deleteLater();
    }

    m_items.clear();
    m_selectedItem = nullptr;
    emit selectionChanged(nullptr);
}

CyMediaDisBaseItem* CyDMediaDisItemManager::selectedItem() const {
    return m_selectedItem;
}

void CyDMediaDisItemManager::onItemSelectionChanged() {
    CyMediaDisBaseItem* senderItem = qobject_cast<CyMediaDisBaseItem*>(sender());
    if (!senderItem) return;

    if (senderItem->isSelected()) {
        m_selectedItem = senderItem;
        emit selectionChanged(senderItem);
    }
    else if (m_selectedItem == senderItem) {
        m_selectedItem = nullptr;
        emit selectionChanged(nullptr);
    }
}
