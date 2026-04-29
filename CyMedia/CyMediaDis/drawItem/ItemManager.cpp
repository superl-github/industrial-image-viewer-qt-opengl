#include "ItemManager.h"
#include "ItemFactory.h"

#include <QDebug>

namespace CyDisDrawItem {
    //====== class CyDisDrawItem::ItemManager ======
    ItemManager::ItemManager(QGraphicsScene* scene, QObject* parent /*= nullptr*/)
        : QObject(parent)
        , m_scene(scene) {
        if (!m_scene) {
            qWarning() << "DrawingManager: scene is null!";
        }
    }

    ItemManager::~ItemManager() {
        clearAll();
    }

	void ItemManager::flushTrans() {
        ;
	}

	void ItemManager::addItem(BaseItem* item) {
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
        mIdAndItemMap[item->id()] = item;

        // 监听选中状态变化
        connect(item, &BaseItem::selectedChanged,
            this, &ItemManager::onItemSelectionChanged,
            Qt::UniqueConnection);

        emit itemAdded(item->id());
    }

    QUuid ItemManager::addItemByType(CyDisDrawItem::ItemType itemType) {
        auto tempItem = ItemFactory::createItem(itemType);
        addItem(tempItem);
        return tempItem->id();
	}

	QUuid ItemManager::addItemByTypeWidthPath(CyDisDrawItem::ItemType itemType, QPainterPath path) {
        auto tempItem = ItemFactory::createItem(itemType);
        tempItem->setPainterPathInScene(path);
        return tempItem->id();
	}

	void ItemManager::removeItem(BaseItem* item) {
        if (!item || !m_items.contains(item)) return;

        m_scene->removeItem(item);
        mIdAndItemMap.remove(item->id());
        m_items.removeOne(item);
        disconnect(item, &BaseItem::selectedChanged, this, nullptr);

        // 如果是当前选中项，清空
        if (m_selectedItem == item) {
            m_selectedItem = nullptr;
            emit selectionChanged(nullptr);
        }
        emit itemRemoved(item->id());
        item->deleteLater();
    }

    void ItemManager::clearAll() {
        mIdAndItemMap.clear();
        for (auto* item : m_items) {
            item->setSelected(false);
        }

        for (auto* item : m_items) {
            m_scene->removeItem(item);
        }

        for (auto* item : m_items) {
            emit itemRemoved(item->id());
            disconnect(item, &BaseItem::selectedChanged, this, nullptr);
            item->deleteLater();
        }

        m_items.clear();
        
        m_selectedItem = nullptr;
        emit selectionChanged(nullptr);
    }

    CyDisDrawItem::BaseItem* ItemManager::getItem(QUuid id) {
        auto it = mIdAndItemMap.find(id);
        if (it == mIdAndItemMap.end()) {
            return nullptr;
        }
        else {
            return it.value();
        }
    }

    BaseItem* ItemManager::selectedItem() const {
        return m_selectedItem;
    }

    void ItemManager::onItemSelectionChanged() {
        BaseItem* senderItem = qobject_cast<CyDisDrawItem::BaseItem*>(sender());
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
}