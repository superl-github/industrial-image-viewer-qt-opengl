// CyMediaI18n.cpp
#include "CyMediaI18n.h"
#include "common/CyMediaLog.h"

#include <QCoreApplication>
#include <QThread>
#include <QTranslator>

#include <atomic>

namespace CyMedia {

    namespace {
        constexpr const char* kDefaultPath = ":/resources/translate/";
    } // namespace

    CyMediaI18n* CyMediaI18n::instance() {
        static CyMediaI18n s_instance;
        return &s_instance;
    }

    CyMediaI18n::CyMediaI18n(QObject* parent)
        : QObject(parent)
        , m_pathPrefix(QString::fromLatin1(kDefaultPath)) {
        // 绑定到主线程，防止首次访问发生在工作线程导致信号调度错乱。
        if (auto* app = QCoreApplication::instance()) {
            if (thread() != app->thread()) {
                moveToThread(app->thread());
            }
        }
        // 信号携带 eLanguage，跨线程 queued connection 需要注册。
        qRegisterMetaType<CyMedia::eLanguage>("CyMedia::eLanguage");
    }

    CyMediaI18n::~CyMediaI18n() {
        // QPointer 可能在 app 先于本单例析构时已变为 null。
        if (m_translator) {
            if (auto* app = QCoreApplication::instance()) {
                QCoreApplication::removeTranslator(m_translator);
            }
            delete m_translator;
            m_translator = nullptr;
        }
    }

    void CyMediaI18n::setTranslatorPath(const QString& pathPrefix) {
        m_pathPrefix = normalizedPath(pathPrefix);
    }

    QString CyMediaI18n::translatorPath() const {
        return m_pathPrefix;
    }

    eLanguage CyMediaI18n::language() const {
        return static_cast<eLanguage>(m_languageValue.load(std::memory_order_acquire));
    }

    QString CyMediaI18n::normalizedPath(const QString& pathPrefix) const {
        if (pathPrefix.isEmpty()) return QString::fromLatin1(kDefaultPath);
        QString p = pathPrefix;
        if (!p.endsWith('/')) p += '/';
        return p;
    }

    QString CyMediaI18n::qmFilePathFor(eLanguage lang) const {
        switch (lang) {
        case CHINESE: return m_pathPrefix + "CyMedia_zh_CN.qm";
        case ENGLISH:
        default:      return m_pathPrefix + "CyMedia_en_US.qm";
        }
    }

    bool CyMediaI18n::setLanguage(eLanguage lang) {
        // 主线程强约束：QTranslator 安装会同步派发 LanguageChange 事件，
        // 跨线程操作会破坏 Qt 的事件循环假设。
        if (QCoreApplication::instance()
            && QThread::currentThread() != QCoreApplication::instance()->thread()) {
            CyMediaLog::instance().log_printf(LogLevel::WAR,
                "[CyMediaI18n] setLanguage() must be called from the main thread");
            return false;
        }

        const eLanguage cur = language();
        if (m_translator && cur == lang) {
            return true; // 已安装且语言一致
        }

        auto* app = QCoreApplication::instance();
        if (!app) {
            CyMediaLog::instance().log_printf(LogLevel::ERR,
                "[CyMediaI18n] QCoreApplication not created, cannot set language");
            return false;
        }

        // 先加载，失败不改动现状（避免"无 translator"窗口期）。
        auto* trans = new QTranslator(app);
        const QString qmPath = qmFilePathFor(lang);
        if (!trans->load(qmPath)) {
            CyMediaLog::instance().log_printf(LogLevel::WAR,
                "[CyMediaI18n] load translation failed: %s", qPrintable(qmPath));
            delete trans;
            return false;
        }

        // 卸载旧的（不触发 LanguageChange 派发，安全）。
        if (m_translator) {
            QCoreApplication::removeTranslator(m_translator);
            delete m_translator;
            m_translator = nullptr;
        }

        // 安装新的（Qt 会向所有顶层 widget 同步派发 LanguageChange）。
        QCoreApplication::installTranslator(trans);
        m_translator = trans;
        m_languageValue.store(static_cast<int>(lang), std::memory_order_release);

        // 兜底通知：用于不处理 LanguageChange 事件的自定义控件（如本项目面板）。
        emit languageChanged(lang);
        return true;
    }

    bool CyMediaI18n::installTranslation(const QString& qmFilePath) {
        if (QCoreApplication::instance()
            && QThread::currentThread() != QCoreApplication::instance()->thread()) {
            CyMediaLog::instance().log_printf(LogLevel::WAR,
                "[CyMediaI18n] installTranslation() must be called from the main thread");
            return false;
        }

        auto* app = QCoreApplication::instance();
        if (!app) return false;

        auto* trans = new QTranslator(app);
        if (!trans->load(qmFilePath)) {
            CyMediaLog::instance().log_printf(LogLevel::WAR,
                "[CyMediaI18n] load translation failed: %s", qPrintable(qmFilePath));
            delete trans;
            return false;
        }

        if (m_translator) {
            QCoreApplication::removeTranslator(m_translator);
            delete m_translator;
        }
        QCoreApplication::installTranslator(trans);
        m_translator = trans;

        // 语言未知（可能是自定义 qm），仅通知刷新，不改变 language() 语义。
        emit languageChanged(language());
        return true;
    }

    void CyMediaI18n::uninstallTranslation() {
        if (QCoreApplication::instance()
            && QThread::currentThread() != QCoreApplication::instance()->thread()) {
            return;
        }
        if (!m_translator) return;

        if (auto* app = QCoreApplication::instance()) {
            QCoreApplication::removeTranslator(m_translator);
        }
        delete m_translator;
        m_translator = nullptr;

        emit languageChanged(language());
    }

} // namespace CyMedia