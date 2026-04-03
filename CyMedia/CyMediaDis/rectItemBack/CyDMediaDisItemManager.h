#pragma once

#include <QObject>
#include <QGraphicsScene>
#include <QList>

#include "drawItem/CyMediaDisBaseItem.h"

class CyDMediaDisItemManager : public QObject {
    Q_OBJECT

public:
    explicit CyDMediaDisItemManager(QGraphicsScene* scene, QObject* parent = nullptr);
    ~CyDMediaDisItemManager();

    // 禁止拷贝
    CyDMediaDisItemManager(const CyDMediaDisItemManager&) = delete;
    CyDMediaDisItemManager& operator=(const CyDMediaDisItemManager&) = delete;

public:
    static CyMediaDisBaseItem* createByScenDraw(CyMediaDisBaseItem::ItemType type, QPointF startPos, QPointF endPos);

    // 核心管理接口
    void addItem(CyMediaDisBaseItem* item);
    void removeItem(CyMediaDisBaseItem* item);
    void clearAll();

    // 查询接口
    QList<CyMediaDisBaseItem*> items() const { return m_items; }
    CyMediaDisBaseItem* selectedItem() const;

    QGraphicsScene* scene() const { return m_scene; }

signals:
    void itemAdded(CyMediaDisBaseItem* item);
    void selectionChanged(CyMediaDisBaseItem* item);

private slots:
    void onItemSelectionChanged();

private:
    QGraphicsScene* m_scene = nullptr;
    QList<CyMediaDisBaseItem*> m_items;
    CyMediaDisBaseItem* m_selectedItem = nullptr;
};