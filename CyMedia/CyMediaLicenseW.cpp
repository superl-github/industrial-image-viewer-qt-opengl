#include "CyMediaLicenseW.h"

#include <QGridLayout>
#include <QDesktopServices>
#include <QFile>

namespace {
    constexpr auto kLgplUrl = "https://www.gnu.org/licenses/lgpl-2.1.html";
    constexpr auto kTemplatePath = ":/qrc/HTML/about_qt.html";

    QString loadTemplate() {
        QFile f(QString::fromLatin1(kTemplatePath));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        return QString::fromUtf8(f.readAll());
    }

    QString qtSubmodulesUrl() {
        const QString ver = QStringLiteral(QT_VERSION_STR);
        const QString majorMinor = ver.section(QLatin1Char('.'), 0, 1);
        return QStringLiteral("https://download.qt.io/archive/qt/%1/%2/submodules/")
            .arg(majorMinor, ver);
    }

    void substitute(QString& tpl,
        std::initializer_list<QPair<QString, QString>> vars) {
        for (const auto& v : vars)
            tpl.replace(v.first, v.second);
    }
}

namespace CyMedia {
    AboutQTDialog::AboutQTDialog(QWidget* parent /*= nullptr*/)
        : QDialog(parent) {
        
        ui_textBrowser = new QTextBrowser(this);
        ui_textBrowser->setReadOnly(true);
        ui_textBrowser->setOpenLinks(false);
        ui_textBrowser->setOpenExternalLinks(false);
        connect(ui_textBrowser, &QTextBrowser::anchorClicked, this, &AboutQTDialog::onAnchorClicked);
        connect(ui_textBrowser->document(), &QTextDocument::contentsChanged,
            this, &AboutQTDialog::adjustSizeToContent);

        ui_closeBtn = new QPushButton(this);
        connect(ui_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

        QGridLayout* layout = new QGridLayout;
        layout->addWidget(ui_textBrowser, 0, 0, 1, 2);
        layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Maximum), 1, 0, 1, 1);
        layout->addWidget(ui_closeBtn, 1, 1, 1, 1);
        layout->setSpacing(2);
        layout->setContentsMargins(4, 4, 4, 4);
        setLayout(layout);
        setMinimumWidth(640);

        flushTrans();
    }

    void AboutQTDialog::flushTrans() {
        m_updating = true;
        ui_textBrowser->setHtml(buildHtml());
        ui_closeBtn->setText(tr("close"));
        m_updating = false;

        adjustSizeToContent();
    }

    void AboutQTDialog::onAnchorClicked(const QUrl& url) {
        QDesktopServices::openUrl(url);
    }

    void AboutQTDialog::adjustSizeToContent() {
        if (m_updating)   // setHtml 期间不布局
            return;

        // 获取文档理想尺寸
        //QSize docSz = ui_textBrowser->document()->size().toSize();
        QTextDocument* doc = ui_textBrowser->document();
        doc->adjustSize(); // 强制执行布局计算
        QSizeF docSizeF = doc->size();
        QSize docSz = docSizeF.toSize();
        // 预留边距
        int w = qMax(640, docSz.width() + 20);
        int h = docSz.height() + 2 + ui_closeBtn->height();

        // 限制最大高度，文本过长时出现滚动条，防止窗口超出屏幕
        const int maxH = 700;
        h = qMin(h, maxH);

        ui_textBrowser->setMaximumHeight(docSz.height() + 10);
        this->resize(w, h);
    }

    QString AboutQTDialog::buildHtml() const {
        QString html = loadTemplate();
        if (html.isEmpty())
            return html;

        const QString qtVer = QStringLiteral(QT_VERSION_STR);
        const QString qtUrl = qtSubmodulesUrl();
        const QString lgpl = QString::fromLatin1(kLgplUrl);

        substitute(html, {
        { "@QT_VERSION@",        qtVer },
        { "@QT_DOWNLOAD_URL@",   qtUrl },
        { "@LGPL_URL@",          lgpl  },

        { "@TEXT_INTRO@",
          tr("This application uses the Qt framework.\n"
              "Qt related libraries are governed by the GNU Lesser General Public License v2.1 (LGPLv2.1).") },

        { "@TEXT_COPYRIGHT@",
          tr("Qt Copyright (C) The Qt Company Ltd. and contributors.") },
        { "@TEXT_QT_VERSION@",
          tr("Qt version used: Qt %1.").arg(qtVer) },

        { "@TEXT_SOURCE@",
          tr("Source code for Qt %1 can be obtained at:").arg(qtVer) },

        { "@TEXT_LINKING@",
          tr("This application dynamically links Qt libraries. Qt library "
             "files (*.dll) and Qt plugin folders are distributed separately.") },
        { "@TEXT_REPLACE@",
          tr("End users may replace or update Qt related libraries and "
             "plugin files.") },
        { "@TEXT_NO_MODIFY@",
          tr("No modifications have been made to Qt source code.") },

        { "@TEXT_LICENSE_LINK@",
          tr("The full GNU LGPLv2.1 license text is available at:") },

        { "@TEXT_FOOTNOTE@",
          tr("For other third-party components, please refer to "
             "LICENSE_QT_CN.txt, LICENSE_QT_EN.txt and LGPL-2.1.txt in the "
             "installation directory.") },
            });

        return html;
    }

}