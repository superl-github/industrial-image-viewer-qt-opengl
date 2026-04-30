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

        /**
         * @brief 该Item是否需要拖拽阈值才能开始创建（替换模式下）
         * 对于单次拖拽型Item（线、矩形、椭圆）返回true，需要移动超过阈值才创建
         * 对于多次点击型Item（点、多边形）返回false，按下即可直接开始创建
         */
        static bool requireDragThreshold(CyDisDrawItem::ItemType type);
    };
}