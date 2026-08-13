#pragma once
#include "CyMediaVideoParseBase.h"
#include <fstream>
#include <filesystem>

namespace CyMedia {
    /**
 * @brief RAW 格式视频解析器
 * @details 解析特定格式的 .raw 视频文件（头部包含 22 字节的 CyRawVideoInfo）
 */
    class VideoParseRaw final : public VideoParseBase {
    protected:
        ParseResult onOpen(const std::filesystem::path& filePath,
            VideoParseInfo& parseInfo, bool format) override;
        void onClose() override;
        bool onReadFrame(uint64_t index, uint8_t* buffer) override;

    private:
        std::fstream m_file;
        uint32_t     m_headerOffset = 0;

        // 跨平台 ntohl 替代
        static uint32_t swapBE32(uint32_t v);
    };
}