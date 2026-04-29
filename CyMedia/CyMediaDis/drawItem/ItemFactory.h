#pragma once
#include "BaseItem.h"
#include <QString>
#include <QVariantMap>

namespace CyDisDrawItem {
    class BaseItem;
}

namespace CyDisDrawItem {
    class ItemFactory {
    public:
        static BaseItem* createBySceneDraw(CyDisDrawItem::ItemType type, QPointF startPos, QPointF endPos);
        static BaseItem* createItem(CyDisDrawItem::ItemType type);
    };
}