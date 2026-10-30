// CyMediaI18n.h
/**
 * @file CyMediaI18n.h
 * @brief 应用级国际化服务（单例）。
 * @ingroup Common
 *
 * @details
 *  - 管理全局 QTranslator 的安装/卸载，协调多实例 CyMediaDis 的语言切换；
 *  - 语言切换完成后发出 `languageChanged` 信号，供 UI 组件刷新文本；
 *  - 语言/路径状态跨所有 CyMediaDis 实例共享。
 *
 *  ## 线程约束
 *  - `setLanguage()`、`installTranslation()`、`uninstallTranslation()`、
 *    `setTranslatorPath()` **必须从主线程调用**（内部会断言）。
 *  - `language()`、`translatorPath()` 可在任意线程读取（原子/只读）。
 *
 *  ## 生命周期
 *  单例本身静态创建（进程退出时析构）；内部 QTranslator 由
 *  QCoreApplication 作 parent，随 QApplication 析构自动释放。
 *
 *  ## 使用
 *  @code
 *  CyMedia::CyMediaI18n::instance()->setTranslatorPath("F:/i18n/");
 *  CyMedia::CyMediaI18n::instance()->setLanguage(CyMedia::CHINESE);
 *  @endcode
 */
#pragma once

#include "common/Export.h"
#include "common/Language.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTranslator> 

#include <atomic>

namespace CyMedia {

    class CYMEDIA_LIB CyMediaI18n : public QObject {
        Q_OBJECT

    public:
        static CyMediaI18n* instance();

        /// 设置翻译文件目录前缀（默认 `:/resources/translate/`）。
        /// 只影响后续 setLanguage 使用；已安装的 translator 不受影响。
        void setTranslatorPath(const QString& pathPrefix);
        QString translatorPath() const;

        /// 切换语言：成功返回 true；失败时保持当前语言不变。
        bool setLanguage(eLanguage lang);
        eLanguage language() const;

        /// 高级：直接安装指定的 .qm 文件（不改变 language() 的语义值）。
        bool installTranslation(const QString& qmFilePath);
        /// 卸载当前翻译，恢复源语言。
        void uninstallTranslation();

        /// 当前已安装的 translator（无则返回 nullptr）。
        QTranslator* translator() const { return m_translator; }

    signals:
        /// 语言切换完成（来自 setLanguage / installTranslation 内部）。
        /// 接收方应刷新自身文本（重绘/重设 label 等），并做空指针保护。
        void languageChanged(CyMedia::eLanguage newLang);

    private:
        explicit CyMediaI18n(QObject* parent = nullptr);
        ~CyMediaI18n() override;
        Q_DISABLE_COPY(CyMediaI18n)

            QString qmFilePathFor(eLanguage lang) const;
        QString normalizedPath(const QString& pathPrefix) const;

    private:
        mutable QString m_pathPrefix;
        std::atomic<int> m_languageValue{ static_cast<int>(ENGLISH) };
        QPointer<QTranslator> m_translator;
    };

} // namespace CyMedia

Q_DECLARE_METATYPE(CyMedia::eLanguage)