#include "ItemManager.h"
#include "ItemFactory.h"
#include "../../common/CyMediaLog.h"

namespace CyDisDrawItem {
    //====== class CyDisDrawItem::ItemManager ======
    ItemManager::ItemManager(QGraphicsScene* scene, QObject* parent /*= nullptr*/)
        : QObject(parent)
        , m_scene(scene) {
        if (!m_scene) {
            CyMediaLog::instance().log_printf_Error(
                "[Annotation] ItemManager created with null scene, item operations are disabled");
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
        if (!item) return;
        if (!m_scene) {
            CyMediaLog::instance().log_printf_Error(
                "[Annotation] addItem failed: scene is null");
            delete item;
            return;
        }
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
        CyMediaLog::instance().log_printf_Debug(
            "[Annotation] item added: id=%s type=%d count=%d",
            item->id().toString().toUtf8().constData(), int(item->itemType()), m_items.size());
    }

    QUuid ItemManager::addItemByType(CyDisDrawItem::ItemType itemType) {
        auto tempItem = ItemFactory::createItem(itemType);
        if (!tempItem) {
            CyMediaLog::instance().log_printf_Error(
                "[Annotation] create item failed: unsupported type %d", int(itemType));
            return QUuid();
        }
        if (!m_scene) {
            CyMediaLog::instance().log_printf_Error(
                "[Annotation] addItem failed: scene is null");
            delete tempItem;
            return QUuid();
        }
        addItem(tempItem);
        return tempItem->id();
	}

	QUuid ItemManager::addItemByTypeWidthPath(CyDisDrawItem::ItemType itemType, QPainterPath path) {
        BaseItem* tempItem = nullptr;
        //查找是否有相同Item
        for (auto item : m_items) {
            if (item->itemType() == itemType && item->pathInScene() == path) {
                CyMediaLog::instance().log_printf_Debug(
                    "[Annotation] duplicate item reused: type=%d id=%s",
                    int(itemType), item->id().toString().toUtf8().constData());
                return item->id();
            }
        }
        tempItem = ItemFactory::createItem(itemType);
        if (tempItem) {
            if (!m_scene) {
                CyMediaLog::instance().log_printf_Error(
                    "[Annotation] addItem(path) failed: scene is null");
                delete tempItem;
                return QUuid();
            }
            tempItem->setPainterPathInScene(path);
            addItem(tempItem);
            return tempItem->id();
        }
        CyMediaLog::instance().log_printf_Error(
            "[Annotation] create item failed: unsupported type %d", int(itemType));
        return QUuid();
	}

	void ItemManager::removeItem(BaseItem* item, bool needSignal/* = true*/) {
        if (!item || !m_items.contains(item)) return;
        if (!m_scene) {
            CyMediaLog::instance().log_printf_Error(
                "[Annotation] removeItem(%s) failed: scene is null",
                item->id().toString().toUtf8().constData());
            return;
        }

        m_scene->removeItem(item);
        mIdAndItemMap.remove(item->id());
        m_items.removeOne(item);
        disconnect(item, &BaseItem::selectedChanged, this, nullptr);

        // 如果是当前选中项，清空
        if (m_selectedItem == item) m_selectedItem = nullptr;

        if (needSignal) emit itemRemoved(item->id());

        emit itemSelectionChanged(item->id(), false);
        CyMediaLog::instance().log_printf_Debug(
            "[Annotation] item removed: id=%s count=%d",
            item->id().toString().toUtf8().constData(), m_items.size());
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
        if (!m_items.isEmpty()) {
            CyMediaLog::instance().log_printf_Debug(
                "[Annotation] clear all items (%d)", m_items.size());
        }
        mIdAndItemMap.clear();
		for (auto* item : m_items) {
            disconnect(item, &BaseItem::selectedChanged, this, nullptr);
			item->setSelected(false);
		}

        for (auto* item : m_items) {
            if (m_scene) m_scene->removeItem(item);
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
        return m_selectedItem ? m_selectedItem->id() : QUuid();
    }

    QUuid ItemManager::getLastItem() {
        if (m_items.size() <= 0) return QUuid();
        else return m_items[m_items.size() - 1]->id();
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