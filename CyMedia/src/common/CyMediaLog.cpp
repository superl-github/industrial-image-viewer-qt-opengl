#include "CyMediaLog.h"
#include "stdarg.h"


CyMediaLog& CyMediaLog::CyMediaLog::instance() {
    static CyMediaLog instance;  // 初始化时会读取配置的路径
    return instance;
}

void CyMediaLog::setLogCallback(CyMedia::LogCallback cb, void* pUser /*= nullptr*/) {
    m_logCallback = std::move(cb);
    m_logCallback_user = pUser;
}


CyMedia::LogLevel CyMediaLog::level() {
    return m_level;
}

void CyMediaLog::setLevel(CyMedia::LogLevel level) {
    m_level = level;
}


void CyMediaLog::log_print(CyMedia::LogLevel level, const std::string& msg) {
    if (m_level == CyMedia::LogLevel::OFF) return;
    // 只输出 >= 当前阈值的日志（TRACE < DEBUG < INFO < WAR < ERR < CRITICAL）。
    if (!m_logCallback || level < m_level) return;
    m_logCallback(level, msg, m_logCallback_user);
}


void CyMediaLog::log_printf(CyMedia::LogLevel level, const char* fmt, ...) {
    if (m_level == CyMedia::LogLevel::OFF) return;
    if (!m_logCallback || level < m_level) return;

    // 标准 C 可变参数格式化
    char buffer[1024] = { 0 };
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap); // 格式化到 buffer
    va_end(ap);

    // 转给回调（纯 std::string）
    CyMediaLog::instance().log_print(level, std::string(buffer));
}


void CyMediaLog::log_printf_Trace(const char* fmt, ...) {
    return log_printf(CyMedia::LogLevel::TRACE, fmt);
}


void CyMediaLog::log_printf_Debug(const char* fmt, ...) {
    return log_printf(CyMedia::LogLevel::DEBUG, fmt);
}


void CyMediaLog::log_printf_Info(const char* fmt, ...) {
    return log_printf(CyMedia::LogLevel::INFO, fmt);
}


void CyMediaLog::log_printf_Warning(const char* fmt, ...) {
    return log_printf(CyMedia::LogLevel::WAR, fmt);
}


void CyMediaLog::log_printf_Error(const char* fmt, ...) {
    return log_printf(CyMedia::LogLevel::ERR, fmt);
}


void CyMediaLog::log_printf_Critical(const char* fmt, ...) {
    return log_printf(CyMedia::LogLevel::CRITICAL, fmt);
}

CyMediaLog::CyMediaLog() {

}

