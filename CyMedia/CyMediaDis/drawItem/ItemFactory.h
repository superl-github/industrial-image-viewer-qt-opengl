/**
 * @file ItemFactory.h
 * @brief 图形项工厂类，负责根据类型创建具体的图形项实例。
 *
 * 提供静态工厂方法 createItem，根据 ItemType 构造对应的图形项对象（如 Item_Rect、Item_Polygon）。
 * 创建后的对象为“空”状态，需由调用方（如 DrawItemTool）进一步初始化（设置位置或转发鼠标事件）。
 *
 * 注意：
 * - 旧版工厂方法 createBySceneDraw 已废弃（仅用于过时的 ItemDrawTool），请统一使用 createItem。
 * - 类型判定函数 requireDragThreshold 将在后续版本移至 BaseItem 虚函数，避免硬编码。
 */
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