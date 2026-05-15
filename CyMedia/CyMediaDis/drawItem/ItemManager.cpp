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

    bool ItemManager::trackingGeometry() {
        return m_trackingGeometryChange;
    }

    void ItemManager::setTrackingGeometry(bool track) {
        if (m_trackingGeometryChange != track) {
            m_trackingGeometryChange = track;
            for (auto item : m_items) {
                item->setTrackingGeometry(m_trackingGeometryChange);
            }
        }
    }

    void ItemManager::addItem(BaseItem* item) {
        if (!item || !m_scene) return;
        if (m_items.contains(item)) return;

        item->setTrackingGeometry(m_trackingGeometryChange);
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
        //处理删除事件
		connect(item, &BaseItem::removeThis,
			this, &ItemManager::onItemRemoveClicked,
			Qt::UniqueConnection);

        emit itemAdded(item->id());
    }

    QUuid ItemManager::addItemByType(CyDisDrawItem::ItemType itemType) {
        auto tempItem = ItemFactory::createItem(itemType);
        addItem(tempItem);
        return tempItem->id();
	}

	QUuid ItemManager::addItemByTypeWidthPath(CyDisDrawItem::ItemType itemType, QPainterPath path) {
        BaseItem* tempItem = nullptr;
        //查找是否有相同Item
        for (auto item : m_items) {
            if (item->itemType() == itemType && item->pathInScene() == path) {
                return item->id();
            }
        }
        tempItem = ItemFactory::createItem(itemType);
        if (tempItem) {
            tempItem->setPainterPathInScene(path);
            addItem(tempItem);
            return tempItem->id();
        }
        return nullptr;
	}

	void ItemManager::removeItem(BaseItem* item, bool needSignal/* = true*/) {
        if (!item || !m_items.contains(item)) return;

        m_scene->removeItem(item);
        mIdAndItemMap.remove(item->id());
        m_items.removeOne(item);
        disconnect(item, &BaseItem::selectedChanged, this, nullptr);

        // 如果是当前选中项，清空
        if (m_selectedItem == item) m_selectedItem = nullptr;

        if (needSignal) emit itemRemoved(item->id());

        emit itemSelectionChanged(item->id(), false);
        item->deleteLater();
    }

	void ItemManager::removeItem(QUuid id, bool needSignal /*= true*/) {
        auto item = getItem(id);
        if (item) {
            removeItem(item);
        }
	}

	void ItemManager::sendRemove(QUuid id) {
        emit itemRemoved(id);
    }

    void ItemManager::clearAll() {
        mIdAndItemMap.clear();
		for (auto* item : m_items) {
            disconnect(item, &BaseItem::selectedChanged, this, nullptr);
			item->setSelected(false);
		}

        for (auto* item : m_items) {
            m_scene->removeItem(item);
        }

        for (auto* item : m_items) {
            emit itemRemoved(item->id());
            item->deleteLater();
        }

        m_items.clear();
        
        m_selectedItem = nullptr;
    }

	int ItemManager::itemCount() {
        return m_items.count();
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

    QUuid ItemManager::selectedItem() const {
        return m_selectedItem->id();
    }

    QUuid ItemManager::getLaseItem() {
        if (m_items.size() <= 0)
            return QUuid();
        else {
            return m_items[m_items.size() - 1]->id();
        }
    }

    void ItemManager::onItemSelectionChanged() {
        BaseItem* senderItem = qobject_cast<CyDisDrawItem::BaseItem*>(sender());
        if (!senderItem) return;

        if (senderItem->isSelected()) {
            if (m_selectedItem && m_selectedItem != senderItem) {
                m_selectedItem->setSelected(false);
            }
            m_selectedItem = senderItem;
            emit itemSelectionChanged(senderItem->id(), true);
        }
        else if (m_selectedItem == senderItem) {
            m_selectedItem = nullptr;
            emit itemSelectionChanged(senderItem->id(), false);
        }
    }

	void ItemManager::onItemRemoveClicked(QUuid id) {
        removeItem(id, true);
	}

}