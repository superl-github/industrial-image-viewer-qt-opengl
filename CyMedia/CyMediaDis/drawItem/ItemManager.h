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

    public:
        void flushTrans();

        bool trackingGeometry();
        void setTrackingGeometry(bool track);

        void addItem(BaseItem* item);
        QUuid addItemByType(CyDisDrawItem::ItemType itemType);
        QUuid addItemByTypeWidthPath(CyDisDrawItem::ItemType itemType, QPainterPath path);
        void removeItem(BaseItem* item, bool needSignal = true);
        void sendRemove(QUuid id);
        void clearAll();

        BaseItem* getItem(QUuid id);

        QList<BaseItem*> items() const { return m_items; }
        QUuid selectedItem() const;
        QUuid getLaseItem();

        QGraphicsScene* scene() const { return m_scene; }

    signals:
        void itemAdded(QUuid id);
        void itemRemoved(QUuid id);
        void selectionChanged(BaseItem* item);

    private slots:
        void onItemSelectionChanged();

    private:
        QList<BaseItem*> m_items;
        QMap<QUuid, BaseItem*> mIdAndItemMap;

        QGraphicsScene* m_scene = nullptr;
        BaseItem* m_selectedItem = nullptr;

        bool m_trackingGeometryChange = false;
    };
}