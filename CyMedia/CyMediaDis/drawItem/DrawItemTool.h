/**
 * @file DrawItemTool.h
 * @brief 鼠标交互绘制工具（新版），通过事件过滤器将用户操作转发给预览图形项自身处理。
 *
 * 本工具是绘图模块的核心交互控制器，设计目标为支持所有图形类型（包括复杂的多边形）：
 * - 通过 installEventFilter 监听 QGraphicsView 的视口事件。
 * - 对于不需要拖拽阈值的类型（点、多边形），按下鼠标即创建预览项并转发事件。
 * - 对于需要拖拽阈值的类型（矩形、线、椭圆），记录起点，移动超过阈值后才创建预览项。
 * - **关键设计**：创建预览项后，后续所有鼠标事件（移动、释放、双击）均直接转发给
 *   m_previewItem->onDrawMouseEvent，由图形项自身维护绘制状态机（如多边形累加顶点）。
 * - 支持替换模式（setReplaceMode），绘制新图形前自动移除上一个图形。
 * - 绘制完成（isDrawFinished 返回 true）或右键取消时，自动清理预览项并发出 drawItem 信号。
 *
 * 后续优化建议：将内部逻辑拆分为 startDrawing/updatePreview/finishDrawing 以提升可读性，
 * 但核心的事件转发机制必须保留。
 */
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

        bool isDrawing() const { return m_bIsDrawing; }

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
        bool m_bIsDrawing = false;
        QPointF m_dragStartPos;      // 鼠标按下的位置（scene 坐标）
        bool m_isDragging = false;   // 是否已开始拖拽绘制(按下鼠标)
        BaseItem* m_previewItem = nullptr;
        QPointer<CyDisDrawItem::BaseItem> m_lastItem = nullptr;
        bool lastItemRemoveWithNoSignal = false;
        QUuid lastItemid;
        QGraphicsItem* m_selectedItem = nullptr;
        static constexpr qreal kDragThreshold = 3.0; // 拖拽判定阈值（scene 坐标）
    };
}