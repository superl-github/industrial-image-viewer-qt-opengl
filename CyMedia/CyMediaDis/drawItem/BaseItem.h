#pragma once
#include "CyDisDrawItem.h"

namespace CyDisDrawItem {
    //====== class CyDisDrawItem::BaseItem ======
    class HandleItem;
    class CYMEDIA_LIB BaseItem : public QGraphicsObject {
        Q_OBJECT

    public:
        enum ContextMenuType {
            Contex_Geometric = 0,
            Contex_Delete,
            Contex_End,
        };

    public:
        using ItemCreateContexMenuCallBack = std::function<void(QMenu* menu)>;
        using ItemContexMenuTriger = std::function<void(QUuid id, int actId, void* puser)>;

    public:
        explicit BaseItem(QGraphicsItem* parent = nullptr);
        virtual ~BaseItem();

	public:signals:
		void removeThis(QUuid id);

		void geometryChanged();
		void selectedChanged();

    public:
        // Intrinsic Override
        QUuid id() const { return m_id; }
        void setid(QUuid& id) { m_id = id; }
        virtual ItemType itemType() const = 0;
        virtual QRectF boundingRect() const = 0;
        virtual QRect boundingRectInScene() const = 0;
        virtual QPainterPath shape() const override;
        virtual QPainterPath pathInScene() const = 0;

        // Update shape
        virtual void setBoundingRectInScene(const QPoint p1, const QPoint p2, bool needSignals = true) = 0;
        virtual void setPainterPathInScene(QPainterPath path, bool needSignals = true) = 0;

        // draw
        virtual bool onDrawMouseEvent(QEvent::Type type, const QPointF& scenePos);
        virtual bool isDrawFinished() const;
        virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) = 0;

        // Style / function
        bool isPreViewMode();
        virtual void setPreviewMode(bool preview);
        virtual void setHandleColor(QColor color);
        virtual void setHandlesVisible(bool visible);
        void setUnSelectedContourColor(QColor color);
        void setSelectedContourColor(QColor color);

        bool trackingGeometry();
        void setTrackingGeometry(bool track);

        void pathToMask(const QPainterPath& scenePath, QImage& maskImage);

        void registerCreateContextMenuFunc(ItemCreateContexMenuCallBack func);
        void registerContextMenuTriggerFunc(ItemContexMenuTriger func, void* pUser);

        // Flickering
        bool flickeringEnable();
        void setFlickeringEnable(bool enable);

        QColor flickeringColor();
        void setFlickeringColor(QColor color);

    protected:
        QUuid m_id;

        QColor m_contour_color_unselect = Qt::gray;
        QColor m_contour_color_select = QColor(0x2a, 0xa3, 0xc6);
        QColor m_handleColor = Qt::white;

        // true:Real-time tracking: Immediately sends an update if the position or shape changes. 
        // false: Sends a signal only when movement/resizing is complete.
        bool m_bTrackGeometryChange = false;
        bool m_positionChangedDuringDrag = false;

        bool m_isPreviewMode = false;

        QList<HandleItem*> m_handles;
        bool m_handlesVisible = true;

        ItemCreateContexMenuCallBack m_createContextMenuFunc = nullptr;
        ItemContexMenuTriger m_ContextMenuTriigerFunc = nullptr;
        void* m_ContextMenuTriigerFunc_user = nullptr;

        virtual QPoint getHandlePos(HandlePosition type, int id = 0) = 0;
        virtual QPoint getHandlePosInScene(HandlePosition type, int id = 0) = 0;
        virtual void updateHandles();
        virtual void removeHandles();

        virtual QPoint constrainToSceneByPos(const QPoint& pos) const = 0;

        virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
        virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
        virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event)override;
        
        virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

		//Right-click Menu
        QString getContextStr(ContextMenuType contexType);
        virtual bool getContextSupport(ContextMenuType contexType);
        QMap<QAction*, BaseItem::ContextMenuType> onContextMenuCreate(QMenu& menu);
        virtual void onContexMenu(ContextMenuType type, QGraphicsSceneContextMenuEvent* event) = 0;
        
    private slots:
        void onFlickeringTimeout();

    private:
        // Blink-related members
        bool m_flickeringEnable = false;
        QColor m_flickeringColor = Qt::red;
        bool m_flickeringState = true;
        QTimer* m_flickeringTimer = nullptr;
        QColor m_oldContourColorUnselect;
        QColor m_oldContourColorSelect;

        friend class HandleItem;
        virtual bool changeByHandle(HandlePosition handletype, int id, QPointF mousePos, QPointF delta) = 0;
    };



    //====== class CyDisDrawItem::HandleItem ======
    class HandleItem : public QGraphicsItem {
    public:
        explicit HandleItem(HandlePosition pos, BaseItem* parent);

    public:
        static int handleSize();

        HandlePosition position() const { return m_type; }

        int id() { return m_id; }
        void setId(int id) { m_id = id; }

        QPointF posFromRect(const QRectF& rect);
        QRectF boundingRect() const override;
        void setColor(QColor color);

    private:
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    private:
        static const int m_handleSize = 8;
        HandlePosition m_type;
        int m_id = 0;
        BaseItem* m_parent;
        QColor m_color = Qt::white;

        bool m_isResizing = false;
        // Used to calculate the offset for the Center handle.
        QPointF m_dragStartScenePos;
    };
}