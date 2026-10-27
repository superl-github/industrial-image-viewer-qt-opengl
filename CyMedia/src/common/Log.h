/**
 * @file Log.h
 * @brief 日志级别与日志回调类型。
 * @ingroup Common
 */
#pragma once

#include <functional>
#include <string>

namespace CyMedia {
    /// 日志级别，数值越大越严重。
    enum LogLevel {
        TRACE = 0,
        DEBUG,
        INFO,
        WAR,
        ERR,
        CRITICAL,
        OFF,
    };

    /**
     * @brief 日志回调。
     * @param level 日志级别。
     * @param message 日志文本。
     * @param user 注册回调时传入的用户指针。
     */
    using LogCallback = std::function<void(LogLevel, const std::string&, void*)>;
}
