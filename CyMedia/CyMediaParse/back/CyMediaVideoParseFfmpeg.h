#pragma once
#include "CyMediaVideoParseBase.h"
#include <filesystem>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace CyMedia {
    /**
     * @brief 基于 FFmpeg 的通用视频解析器（同时支持 MP4 / AVI）
     * @details FFmpeg 自动检测容器格式，一个类处理所有 FFmpeg 支持的格式，
     *          输出统一为 RGB888，与上层渲染逻辑完全兼容。
     */
    class VideoParseFfmpeg final : public VideoParseBase
    {
    protected:
        ParseResult onOpen(const std::filesystem::path& filePath,
            VideoParseInfo& parseInfo, bool format) override;
        void onClose() override;
        bool onReadFrame(uint64_t index, uint8_t* buffer) override;

    private:
        AVFormatContext* m_fmtCtx = nullptr;
        AVCodecContext* m_codecCtx = nullptr;
        AVFrame* m_decFrame = nullptr;
        AVFrame* m_rgbFrame = nullptr;
        SwsContext* m_swsCtx = nullptr;
        AVPacket* m_packet = nullptr;
        int               m_streamIdx = -1;
        AVRational        m_timeBase = { 0, 0 };
        AVRational        m_frameRate = { 0, 0 };
        uint64_t          m_lastFrameIdx = 0;

        uint64_t          m_logicalFrame = 0;   // 逻辑帧计数（按数据包计数，与容器索引对齐）
        bool              m_hasLastFrame = false; // 是否有缓存的有效画面
    };
}
