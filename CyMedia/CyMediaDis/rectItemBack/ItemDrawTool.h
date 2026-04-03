#pragma once
#include "CyDisDrawItem.h"

#include <QObject>
#include <QPointF>
#include <QLineF>
#include <QGraphicsView>

#include <functional>

namespace CyDisDrawItem {
    class ItemDrawTool : public QObject {
        Q_OBJECT

    public:
        explicit ItemDrawTool(ItemManager* manager, QGraphicsView* view, QObject* parent = nullptr);

        void setDraMode(DrawMode mode);
        DrawMode draMode() const { return m_mode; }

        void setReplaceMode(bool enable);
        bool replaceMode() const { return m_replaceMode; }

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override;

    private:
        void startDrawing(const QPointF& pos);
        void updatePreview(const QPointF& currentPos);
        void finishDrawing();

    private:
        ItemManager* m_manager = nullptr;
        QGraphicsView* m_view = nullptr;

        DrawMode m_mode = DrawMode::DRAW_None;
        bool m_replaceMode = true;

        BaseItem* m_previewItem = nullptr;
        BaseItem* m_lastItem = nullptr;
        QGraphicsItem* m_selectedItem = nullptr;
        QPointF m_startPos;
    };
}