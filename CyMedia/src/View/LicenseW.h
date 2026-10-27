#pragma once
#include "CyMediaBaseDef.h"

#include <QDialog>
#include <QTextBrowser>
#include <QPushButton>

namespace CyMedia {
    /**
     * @class CyMedia::AboutQTDialog.
     * @brief Qt Lincense
     */
    class CYMEDIA_LIB AboutQTDialog : public QDialog {
        Q_OBJECT
    public:
        explicit AboutQTDialog(QWidget* parent = nullptr);

    public:
        void flushTrans();

    private slots:
        void onAnchorClicked(const QUrl& url);
        void adjustSizeToContent();

    private:
        QString buildHtml() const;

    private:
        QTextBrowser* ui_textBrowser = nullptr;
        QPushButton* ui_closeBtn = nullptr;
        bool          m_updating = false;
    };
}