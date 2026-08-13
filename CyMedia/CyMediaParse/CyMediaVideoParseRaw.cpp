#include "CyMediaVideoParseRaw.h"
#include "CyMediaFormatRegistry.h"

#include <cstring>
#include <algorithm>

#pragma pack(push, 1)
struct RawFileHeader {
    uint8_t  version;
    uint8_t  nBit;
    uint32_t nColor;
    uint32_t nWidth;
    uint32_t nHeight;
    uint32_t rate;
    uint32_t frameLength;
};
#pragma pack(pop)

namespace CyMedia {
    uint32_t VideoParseRaw::swapBE32(uint32_t v) {
        const uint8_t* b = reinterpret_cast<const uint8_t*>(&v);
        return (static_cast<uint32_t>(b[0]) << 24) |
            (static_cast<uint32_t>(b[1]) << 16) |
            (static_cast<uint32_t>(b[2]) << 8) |
            static_cast<uint32_t>(b[3]);
    }

    ParseResult VideoParseRaw::onOpen(const std::filesystem::path& filePath, VideoParseInfo& parseInfo, bool format) {
        m_file.open(filePath, std::ios::in | std::ios::binary);
        if (!m_file.is_open()) return ParseResult::FILE_OPEN_FAIL;

        m_file.seekg(0, std::ios::end);
        const uint64_t fileSize = static_cast<uint64_t>(m_file.tellg());
        m_file.seekg(0, std::ios::beg);

        RawFileHeader hdr{};
        if (format) {
            m_headerOffset = parseInfo.dataOffset;
            hdr.version = 1;
            hdr.nBit = parseInfo.frameInfo.bit;
            hdr.nColor = static_cast<uint32_t>(parseInfo.frameInfo.format);
            hdr.nWidth = static_cast<uint32_t>(parseInfo.frameInfo.width);
            hdr.nHeight = static_cast<uint32_t>(parseInfo.frameInfo.height);
            hdr.rate = static_cast<uint32_t>(parseInfo.fps * 100);
            hdr.frameLength = parseInfo.frameInfo.length;
        }
        else {
            if (fileSize < sizeof(RawFileHeader)) { m_file.close(); return ParseResult::FORMAT_ERROR; }
            m_file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
            hdr.nColor = swapBE32(hdr.nColor);
            hdr.nWidth = swapBE32(hdr.nWidth);
            hdr.nHeight = swapBE32(hdr.nHeight);
            hdr.rate = swapBE32(hdr.rate);
            hdr.frameLength = swapBE32(hdr.frameLength);
            m_headerOffset = sizeof(RawFileHeader);
        }

        // ===== 健壮性校验 =====
        if (hdr.nWidth == 0 || hdr.nHeight == 0 || hdr.frameLength == 0 || hdr.rate == 0) {
            m_file.close(); return ParseResult::FORMAT_ERROR;
        }

        const uint64_t dataSize = fileSize - m_headerOffset;
        const uint32_t calcFrames = static_cast<uint32_t>(dataSize / hdr.frameLength);

        if (format && calcFrames <= 1) { m_file.close(); return ParseResult::FORMAT_ERROR; }
        if (dataSize != static_cast<uint64_t>(calcFrames) * hdr.frameLength) {
            m_file.close(); return ParseResult::FORMAT_ERROR;
        }

        // 填充基类受保护成员
        m_frameInfo = ImageShowInfo(
            static_cast<int32_t>(hdr.nWidth), static_cast<int32_t>(hdr.nHeight),
            static_cast<int8_t>(hdr.nBit), static_cast<ePixType>(hdr.nColor),
            hdr.frameLength, PIXEL_VALUE_INT);
        m_frameDataSize = hdr.frameLength;
        m_totalFrames = calcFrames;
        m_framerate = static_cast<float>(hdr.rate) / 100.0f;

        // 回填输出参数
        parseInfo.videoType = VideoSuffix::RAWV;
        parseInfo.frameInfo = m_frameInfo;
        parseInfo.dataOffset = m_headerOffset;
        parseInfo.fps = m_framerate;
        parseInfo.frameCount = calcFrames;
        parseInfo.size = static_cast<int64_t>(fileSize);

        return ParseResult::OK;
    }

    void VideoParseRaw::onClose() {
        if (m_file.is_open()) m_file.close();
        m_headerOffset = 0;
    }

    bool VideoParseRaw::onReadFrame(uint64_t index, uint8_t* buffer) {
        if (!buffer || index < 1 || index > m_totalFrames) return false;
        const uint64_t offset = m_headerOffset
            + static_cast<uint64_t>(index - 1) * m_frameDataSize;
        m_file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        m_file.read(reinterpret_cast<char*>(buffer), m_frameDataSize);
        return !m_file.fail();
    }
    REGISTER_VIDEO_FORMAT(VideoSuffix::RAWV, VideoParseRaw);
}