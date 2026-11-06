#include "CyMediaLog.h"

#include <cstdio>


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


void CyMediaLog::log_vprintf(CyMedia::LogLevel level, const char* fmt, va_list ap) {
    if (m_level == CyMedia::LogLevel::OFF) return;
    if (!m_logCallback || level < m_level) return;
    if (!fmt) return;

    // 标准 C 可变参数格式化（统一由 log_printf / log_printf_* 汇聚到这里）。
    char stackbuf[1024];
    //int n = vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap);
    //if (n < 0 || (n >= (int)sizeof(buffer))) return;
    //m_logCallback(level, std::string_view(buffer), m_logCallback_user);
    //return;
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(stackbuf, sizeof(stackbuf), fmt, ap);
    if (n < 0) {         // 格式化失败
        va_end(ap2);
        return;
    }

    if ((size_t)n < sizeof(stackbuf)) {
        // 未截断：零堆分配，零 memset
        m_logCallback(level, std::string_view(stackbuf, (size_t)n), m_logCallback_user);
        va_end(ap2);
        return;
    }

    // 超长：n 是完整长度（不含 '\0'），按实际大小重来一次（低频路径）
    std::vector<char> buf((size_t)n + 1);
    vsnprintf(buf.data(), buf.size(), fmt, ap2);
    m_logCallback(level, std::string_view(buf.data(), (size_t)n), m_logCallback_user);
    va_end(ap2);
}


void CyMediaLog::log_printf(CyMedia::LogLevel level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(level, fmt, ap);
    va_end(ap);
}


void CyMediaLog::log_printf_Trace(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(CyMedia::LogLevel::TRACE, fmt, ap);
    va_end(ap);
}


void CyMediaLog::log_printf_Debug(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(CyMedia::LogLevel::DEBUG, fmt, ap);
    va_end(ap);
}


void CyMediaLog::log_printf_Info(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(CyMedia::LogLevel::INFO, fmt, ap);
    va_end(ap);
}


void CyMediaLog::log_printf_Warning(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(CyMedia::LogLevel::WAR, fmt, ap);
    va_end(ap);
}


void CyMediaLog::log_printf_Error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(CyMedia::LogLevel::ERR, fmt, ap);
    va_end(ap);
}


void CyMediaLog::log_printf_Critical(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vprintf(CyMedia::LogLevel::CRITICAL, fmt, ap);
    va_end(ap);
}

CyMediaLog::CyMediaLog() {

}
