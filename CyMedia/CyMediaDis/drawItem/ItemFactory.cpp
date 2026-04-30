#include "ItemFactory.h"

#include "Item_Point.h"
#include "Item_Rect.h"
#include "Item_Line.h"
#include "Item_Ellipse.h"
#include "Item_Polygon.h"

namespace CyDisDrawItem {
    BaseItem* ItemFactory::createBySceneDraw(CyDisDrawItem::ItemType type, QPointF startPos, QPointF endPos) {
        BaseItem* item = nullptr;
        switch (type) {
            case CyDisDrawItem::ItemType::Invalid:
                break;

            case CyDisDrawItem::ItemType::Point: {
                item = new Item_Point();
            }break;
            case CyDisDrawItem::ItemType::Rectangle: {
                item = new Item_Rect();
            }break;
            case CyDisDrawItem::ItemType::Line: {
                item = new Item_Line();
            }break;
            case CyDisDrawItem::ItemType::Ellipse: {
                item = new Item_Ellipse();
            }break;
            case CyDisDrawItem::ItemType::Polygon: {
                item = new Item_Polygon();
            }break;

            default:
                break;
        }

        if (item) {
            item->setBoundingRectInScene(startPos.toPoint(), endPos.toPoint(), false);
        }
        return item;
    }

	CyDisDrawItem::BaseItem* ItemFactory::createItem(CyDisDrawItem::ItemType type) {
		BaseItem* item = nullptr;
		switch (type) {
		    case CyDisDrawItem::ItemType::Invalid:
			    break;

		    case CyDisDrawItem::ItemType::Point: {
			    item = new Item_Point();
		    }break;
		    case CyDisDrawItem::ItemType::Rectangle: {
			    item = new Item_Rect();
		    }break;
		    case CyDisDrawItem::ItemType::Line: {
			    item = new Item_Line();
		    }break;
		    case CyDisDrawItem::ItemType::Ellipse: {
			    item = new Item_Ellipse();
		    }break;
            case CyDisDrawItem::ItemType::Polygon: {
                item = new Item_Polygon();
            }break;

		    default:
			    break;
		}

        return item;
	}

    bool ItemFactory::requireDragThreshold(CyDisDrawItem::ItemType type) {
        switch (type) {
            case CyDisDrawItem::Invalid:
                return false;
            case CyDisDrawItem::Rectangle:
            case CyDisDrawItem::Line:
            case CyDisDrawItem::Ellipse:
                return true;
            case CyDisDrawItem::Point:
            case CyDisDrawItem::Polygon:
                return false;
        }

        return false;
    }
}