#include "CyMediaDisLog.h"


CyMediaDisLog& CyMediaDisLog::CyMediaDisLog::instance() {
    static CyMediaDisLog instance;  // 初始化时会读取配置的路径
    return instance;
}

void CyMediaDisLog::setLogCallback(CyMedia::LogCallback cb, void* pUser /*= nullptr*/) {
    m_logCallback = std::move(cb);
    m_logCallback_user = pUser;
}

void CyMediaDisLog::setLevel(CyMedia::LogLevel level) {
    m_level = level;
}


void CyMediaDisLog::log_print(CyMedia::LogLevel level, const std::string& msg) {
    if (!m_logCallback || level < m_level) return;
    m_logCallback(level, msg, m_logCallback_user);
}

CyMediaDisLog::CyMediaDisLog() {

}

