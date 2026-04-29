#include "ItemFactory.h"

#include "PointItem.h"
#include "RectItem.h"
#include "LineItem.h"
#include "EllipseItem.h"

namespace CyDisDrawItem {
    BaseItem* ItemFactory::createBySceneDraw(CyDisDrawItem::ItemType type, QPointF startPos, QPointF endPos) {
        BaseItem* item = nullptr;
        switch (type) {
            case CyDisDrawItem::ItemType::Invalid:
                break;

            case CyDisDrawItem::ItemType::Point: {
                item = new PointItem();
            }
            break;

            case CyDisDrawItem::ItemType::Rectangle: {
                item = new RectItem();
            }break;

            case CyDisDrawItem::ItemType::Line: {
                item = new LineItem();
            }break;

            case CyDisDrawItem::ItemType::Ellipse: {
                item = new EllipseItem();
            }break;

            default:
                break;
        }

        if (item) {
            item->setBoundingRectInScene(startPos.toPoint(), endPos.toPoint(), false);
        }
        return item;
    }
}