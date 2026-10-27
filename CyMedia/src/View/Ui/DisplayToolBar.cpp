#include "DisplayToolBar.h"

#include <QHBoxLayout>
#include <QPalette>
#include <QPushButton>

namespace CyMedia {
    namespace {
        /// 正方形工具栏按钮，高度始终等于宽度。
        class SquareButton : public QPushButton {
        public:
            explicit SquareButton(QWidget* parent = nullptr) : QPushButton(parent) {}
            int heightForWidth(int w) const override { return w; }
            QSize sizeHint() const override { return QSize(26, 26); }
        };

        QPushButton* makeToolButton(QWidget* parent, const QString& iconPrefix,
            const QString& iconFile, const QString& hoverFile, const QString& tooltip) {
            auto* btn = new SquareButton(parent);
            btn->setText("");
            btn->setToolTip(tooltip);
            btn->setStyleSheet(QString(
                "QPushButton{border:none;background:transparent;image:url(%1%2);}"
                "QPushButton:hover{border:none;background:transparent;image:url(%1%3);}"
                "QPushButton:pressed{border:none;background:transparent;image:url(%1%3);}")
                .arg(iconPrefix, iconFile, hoverFile));
            return btn;
        }
    }

    class DisplayToolBar::PrivateData {
    public:
        QPushButton* zoomIn = nullptr;
        QPushButton* zoomOut = nullptr;
        QPushButton* zoomRaw = nullptr;
        QPushButton* zoomFit = nullptr;
        QPushButton* flipH = nullptr;
        QPushButton* flipV = nullptr;
    };

    DisplayToolBar::DisplayToolBar(const QString& iconPrefix, QWidget* parent /*= nullptr*/)
        : QWidget(parent)
        , d(new PrivateData) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setStyleSheet(
            "QWidget{border:none;background-color:black;}"
            "QToolTip{background-color:white;color:black;}");

        // DisplayToolBar 是 QWidget 子类。若不显式开启 WA_StyledBackground，
        // QWidget{background-color:black} 不会绘制在工具栏自身，布局间隙/右侧
        // 会露出父窗口的白色背景，表现为按钮之间的白色长方形。
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(true);
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::black);
        setPalette(pal);

        d->zoomIn = makeToolButton(this, iconPrefix, "ZommIn.png", "ZoomIn-hover.png", tr("Zoom In"));
        d->zoomOut = makeToolButton(this, iconPrefix, "ZoomOut.png", "ZoomOut-hover.png", tr("Zoom Out"));
        d->zoomRaw = makeToolButton(this, iconPrefix, "zoonRaw.png", "zoonRaw-hover.png", tr("Actual Size"));
        d->zoomFit = makeToolButton(this, iconPrefix, "zoonFit.png", "zoonFit-hover.png", tr("Fit to window"));
        d->flipH = makeToolButton(this, iconPrefix, "FlipH.png", "FlipH-hover.png", tr("Flip Horizontal"));
        d->flipV = makeToolButton(this, iconPrefix, "FlipV.png", "FlipV-hover.png", tr("Flip Vertical"));

        connect(d->zoomIn, &QPushButton::clicked, this, &DisplayToolBar::zoomInRequested);
        connect(d->zoomOut, &QPushButton::clicked, this, &DisplayToolBar::zoomOutRequested);
        connect(d->zoomRaw, &QPushButton::clicked, this, &DisplayToolBar::zoomRawRequested);
        connect(d->zoomFit, &QPushButton::clicked, this, &DisplayToolBar::zoomFitRequested);
        connect(d->flipH, &QPushButton::clicked, this, &DisplayToolBar::flipHorizontalRequested);
        connect(d->flipV, &QPushButton::clicked, this, &DisplayToolBar::flipVerticalRequested);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(d->zoomIn);
        layout->addWidget(d->zoomOut);
        layout->addWidget(d->zoomRaw);
        layout->addWidget(d->zoomFit);
        layout->addWidget(d->flipH);
        layout->addWidget(d->flipV);
    }

    DisplayToolBar::~DisplayToolBar() = default;
}
