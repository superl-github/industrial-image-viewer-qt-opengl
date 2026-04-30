#pragma once
#include "BaseItem.h"
#include "ItemFactory.h"
#include "ItemManager.h"

#include <QPointer>
#include <QObject>
#include <QPointF>
#include <QLineF>
#include <QGraphicsView>

#include <functional>

namespace CyDisDrawItem {
    class DrawItemTool : public QObject {
        Q_OBJECT

    public:
        explicit DrawItemTool(ItemManager* manager, QGraphicsView* view, QObject* parent = nullptr);
        ~DrawItemTool();

    public:
        void setThemeColor(QColor color);
        void setDrawMode(ItemType mode);
        ItemType draMode() const { return m_mode; }

        void setReplaceMode(bool enable);
        bool replaceMode() const { return m_replaceMode; }

    public:signals:
        void drawItem(CyDisDrawItem::BaseItem* item);

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override;

    private:
        void updatePreview(const QPointF& currentPos);
        void finishDrawing();

    private:
        QColor mThemeColor;
        ItemManager* m_manager = nullptr;
        QGraphicsView* m_view = nullptr;

        ItemType m_mode = ItemType::Invalid;
        bool m_replaceMode = true;

        //绘制实现相关
        QPointF m_dragStartPos;      // 鼠标按下的位置（scene 坐标）
        bool m_isDragging = false;   // 是否已开始拖拽绘制(按下鼠标)
        BaseItem* m_previewItem = nullptr;
        QPointer<CyDisDrawItem::BaseItem> m_lastItem = nullptr;
        QGraphicsItem* m_selectedItem = nullptr;
        static constexpr qreal kDragThreshold = 3.0; // 拖拽判定阈值（scene 坐标）
    };
}