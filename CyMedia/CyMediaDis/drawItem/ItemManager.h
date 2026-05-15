#pragma once
#include <QObject>
#include <QList>

#include "BaseItem.h"

namespace CyDisDrawItem {
    class ItemManager : public QObject {
        Q_OBJECT

    public:
        explicit ItemManager(QGraphicsScene* scene, QObject* parent = nullptr);
        ~ItemManager();

        // 禁止拷贝
        ItemManager(const ItemManager&) = delete;
        ItemManager& operator=(const ItemManager&) = delete;

    public:signals:
		void itemAdded(QUuid id);
		void itemRemoved(QUuid id);
		void itemSelectionChanged(QUuid id, bool selected);

    public:
        void flushTrans();

        bool trackingGeometry();
        void setTrackingGeometry(bool track);

        void addItem(BaseItem* item);
        QUuid addItemByType(CyDisDrawItem::ItemType itemType);
        QUuid addItemByTypeWidthPath(CyDisDrawItem::ItemType itemType, QPainterPath path);
        void removeItem(BaseItem* item, bool needSignal = true);
        void removeItem(QUuid id, bool needSignal = true);
        void sendRemove(QUuid id);
        void clearAll();

        int itemCount();
        BaseItem* getItem(QUuid id);

        QList<BaseItem*>& items() { return m_items; }
        QUuid selectedItem() const;
        QUuid getLaseItem();

        QGraphicsScene* scene() const { return m_scene; }

    private slots:
        void onItemSelectionChanged();
        void onItemRemoveClicked(QUuid id);

    private:
        QList<BaseItem*> m_items;
        QMap<QUuid, BaseItem*> mIdAndItemMap;

        QGraphicsScene* m_scene = nullptr;
        BaseItem* m_selectedItem = nullptr;

        bool m_trackingGeometryChange = false;
    };
}