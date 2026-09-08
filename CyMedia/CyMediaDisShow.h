#pragma once
#include "CyMediaBaseDef.h"
#include "CyMediaDis.h"

#include <QWidget>
#include <QLabel>
#include <QMenu>

namespace CyMedia {
    class imgShowWidget : public QWidget {
        Q_OBJECT

    public:
        imgShowWidget(QWidget* parent = nullptr);
        ~imgShowWidget();

    public:
        CyMedia::CyMediaDis* display() const;

        void upImageData(CyMedia::ImageShowInfo info, void* data);

    private:
        void onUpPosPix(qint32 x, qint32 y, double r, double g, double b, bool signlR);

    private:
        CyMedia::CyMediaDis* ui_disPlay = nullptr;
        QLabel* ui_PosRGBLab = nullptr;
    };
}
