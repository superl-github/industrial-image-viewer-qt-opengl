#pragma once
#include "CyMediaBaseDef.h"

class CyMediaDisLog{
public:
    // 获取单例实例（线程安全）
    static CyMediaDisLog& instance();
    // 禁止拷贝和赋值
    CyMediaDisLog(const CyMediaDisLog&) = delete;
    CyMediaDisLog& operator=(const CyMediaDisLog&) = delete;

public:
    void setLogCallback(CyMedia::LogCallback cb, void* pUser = nullptr);

    CyMedia::LogLevel level();
    void setLevel(CyMedia::LogLevel level);

    void log_print(CyMedia::LogLevel level, const std::string& msg);
    void log_printf(CyMedia::LogLevel level, const char* fmt, ...);

    void log_printf_Trace(const char* fmt, ...);
    void log_printf_dDebug(const char* fmt, ...);
    void log_printf_Info(const char* fmt, ...);
    void log_printf_Warning(const char* fmt, ...);
    void log_printf_Error(const char* fmt, ...);
    void log_printf_Critical(const char* fmt, ...);

private:
    CyMediaDisLog();

private:
    CyMedia::LogCallback m_logCallback = nullptr;
    void* m_logCallback_user = nullptr;
    CyMedia::LogLevel m_level = CyMedia::LogLevel::INFO;
};
