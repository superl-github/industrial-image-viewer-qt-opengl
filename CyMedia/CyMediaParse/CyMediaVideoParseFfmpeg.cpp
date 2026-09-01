#include "CyMediaVideoParseFfmpeg.h"
#include "CyMediaFormatRegistry.h"
#include <filesystem>
#include <cstring>
#include <Windows.h>

namespace CyMedia {

    ParseResult VideoParseFfmpeg::onOpen(const std::filesystem::path& filePath, VideoParseInfo& parseInfo, bool format) {
        (void)format;
        // 1. 打开容器，兼容Windows中文路径（宽字符转系统ANSI编码）
        std::wstring wPath = filePath.wstring();
        int bufSize = WideCharToMultiByte(CP_ACP, 0, wPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string ansiPath(bufSize, '\0');
        WideCharToMultiByte(CP_ACP, 0, wPath.c_str(), -1, ansiPath.data(), bufSize, nullptr, nullptr);
        int ret = avformat_open_input(&m_fmtCtx, ansiPath.c_str(), nullptr, nullptr);
        if (ret < 0) {
            char errBuf[1024] = { 0 };
            av_strerror(ret, errBuf, sizeof(errBuf));
            printf("[FFmpeg解析器] avformat_open_input 失败，错误码：%d\n", ret);
            printf("[FFmpeg解析器] 失败原因：%s\n", errBuf);
            printf("[FFmpeg解析器] 尝试打开的路径：%s\n", ansiPath.c_str());
            return ParseResult::FILE_OPEN_FAIL;
        }
        // 2. 查找视频流与解码器
        m_streamIdx = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (m_streamIdx < 0) return ParseResult::FORMAT_ERROR;
        AVStream* stream = m_fmtCtx->streams[m_streamIdx];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) return ParseResult::UNSIPPORTED;
        m_codecCtx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(m_codecCtx, stream->codecpar);
        ret = avcodec_open2(m_codecCtx, codec, nullptr);
        if (ret < 0) {
            char errBuf[1024] = { 0 };
            av_strerror(ret, errBuf, sizeof(errBuf));
            printf("[FFmpeg解析器] avcodec_open2 失败，错误码：%d\n", ret);
            printf("[FFmpeg解析器] 失败原因：%s\n", errBuf);
            return ParseResult::FILE_OPEN_FAIL;
        }
        if (m_codecCtx->pix_fmt == AV_PIX_FMT_NONE) {
            m_codecCtx->pix_fmt = (AVPixelFormat)stream->codecpar->format;
        }
        if (m_codecCtx->pix_fmt == AV_PIX_FMT_NONE || m_codecCtx->pix_fmt < 0) {
            m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        }
        // 3. 分配帧与包资源
        m_decFrame = av_frame_alloc();
        m_rgbFrame = av_frame_alloc();
        m_packet = av_packet_alloc();
        const int width = m_codecCtx->width;
        const int height = m_codecCtx->height;
        av_image_alloc(m_rgbFrame->data, m_rgbFrame->linesize, width, height, AV_PIX_FMT_RGB24, 1);
        if (width <= 0 || height <= 0) return ParseResult::FORMAT_ERROR;
        if (m_codecCtx->pix_fmt == AV_PIX_FMT_NONE || m_codecCtx->pix_fmt < 0) return ParseResult::FORMAT_ERROR;
        m_swsCtx = sws_getContext(width, height, m_codecCtx->pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!m_swsCtx) return ParseResult::FORMAT_ERROR;
        m_timeBase = stream->time_base;
        m_frameRate = stream->avg_frame_rate;
        const float fps = av_q2d(m_frameRate);
        // 4. 计算总帧数
        uint64_t totalFrames = 0;
        if (stream->nb_frames > 0) totalFrames = stream->nb_frames;
        else if (stream->duration != AV_NOPTS_VALUE && fps > 1e-6f) totalFrames = static_cast<uint64_t>(av_rescale_q(stream->duration, m_timeBase, av_inv_q(m_frameRate)));
        totalFrames = totalFrames < 1 ? 1 : totalFrames;
        // 5. 填充基类 protected 成员
        m_frameInfo = ImageShowInfo(width, height, 8, RGB, static_cast<uint32_t>(width * height * 3), PIXEL_VALUE_INT);
        m_frameDataSize = static_cast<uint32_t>(width * height * 3);
        m_totalFrames = totalFrames;
        m_framerate = fps;
        m_lastFrameIdx = 0;
        // 6. 回填输出参数
        if (m_fmtCtx && m_fmtCtx->iformat) {
            const char* fmtName = m_fmtCtx->iformat->name;
            if (strstr(fmtName, "avi")) parseInfo.videoType = VideoSuffix::AVI;
            else parseInfo.videoType = VideoSuffix::MP4;
        }
        else {
            parseInfo.videoType = VideoSuffix::MP4;
        }
        // =================================================================
        parseInfo.frameInfo = m_frameInfo;
        parseInfo.dataOffset = 0;
        parseInfo.fps = fps;
        parseInfo.frameCount = static_cast<int>(totalFrames);
        int64_t fileSize = 0;
        try {
            fileSize = static_cast<int64_t>(std::filesystem::file_size(filePath));
        }
        catch (const std::filesystem::filesystem_error&) {
            fileSize = 0;
        }
        parseInfo.size = fileSize;
        return ParseResult::OK;
    }

    void VideoParseFfmpeg::onClose() {
        if (m_swsCtx)   sws_freeContext(m_swsCtx);
        if (m_rgbFrame) {
            av_freep(&m_rgbFrame->data[0]);
            av_frame_free(&m_rgbFrame);
        }
        if (m_decFrame)  av_frame_free(&m_decFrame);
        if (m_packet)    av_packet_free(&m_packet);
        if (m_codecCtx)  avcodec_free_context(&m_codecCtx);
        if (m_fmtCtx)    avformat_close_input(&m_fmtCtx);
        m_swsCtx = nullptr;
        m_decFrame = nullptr;
        m_rgbFrame = nullptr;
        m_packet = nullptr;
        m_codecCtx = nullptr;
        m_fmtCtx = nullptr;
        m_streamIdx = -1;
        m_lastFrameIdx = 0;

        m_eofFrame.clear();
        m_eofed = false;
    }

    bool VideoParseFfmpeg::onReadFrame(uint64_t index, uint8_t* buffer) {
        if (!m_fmtCtx || index < 1 || !buffer)
            return false;

        // 1. 如果已经到达文件尾，直接返回缓存的最后一帧（并更新当前帧号）
        if (m_eofed && index >= m_eofedPos) {
            if (!m_eofFrame.empty()) {
                memcpy(buffer, m_eofFrame.data(), m_frameDataSize);
                m_lastFrameIdx = index;          // 让位置继续前进
                return true;
            }
            return false;
        }
        
        // 2. 非顺序访问 => 执行 Seek 定位（并重置 EOF 状态）
        if (index != m_lastFrameIdx + 1) {
            int64_t target_ts = av_rescale_q(
                static_cast<int64_t>(index - 1),
                av_inv_q(m_frameRate),
                m_timeBase
            );
            av_seek_frame(m_fmtCtx, m_streamIdx, target_ts, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(m_codecCtx);
            m_lastFrameIdx = 0;
        }

        // 3. 解码循环（直到拿到目标帧或文件结束）
        bool gotTarget = false;
        int retryCount = 0;
        const int MAX_RETRY = 5;

        while (!gotTarget) {
            // 3.1 尝试从解码器取帧
            int recvRet = avcodec_receive_frame(m_codecCtx, m_decFrame);
            if (recvRet == 0) {
                uint64_t curIdx = (m_decFrame->pts != AV_NOPTS_VALUE) ?
                    static_cast<uint64_t>(av_rescale_q(m_decFrame->pts, m_timeBase, av_inv_q(m_frameRate))) :
                    m_lastFrameIdx + 1;

                if (curIdx >= index) {
                    // 转换并拷贝到 buffer
                    sws_scale(m_swsCtx,
                        m_decFrame->data, m_decFrame->linesize,
                        0, m_codecCtx->height,
                        m_rgbFrame->data, m_rgbFrame->linesize);
                    // 拷贝 RGB 数据（逐行处理）
                    const int w = m_codecCtx->width;
                    const int h = m_codecCtx->height;
                    for (int y = 0; y < h; ++y) {
                        memcpy(buffer + static_cast<size_t>(y) * w * 3,
                            m_rgbFrame->data[0] + static_cast<size_t>(y) * m_rgbFrame->linesize[0],
                            static_cast<size_t>(w) * 3);
                    }
                    m_lastFrameIdx = index;
                    return true;
                }
                continue;  // 继续解码下一帧
            }
            else if (recvRet == AVERROR(EAGAIN)) {
                // 需要更多数据，继续读包
            }
            else {
                // 解码错误，跳过
                avcodec_flush_buffers(m_codecCtx);
                if (++retryCount > MAX_RETRY) {
                    if (!m_eofFrame.empty()) {
                        memcpy(buffer, m_eofFrame.data(), m_frameDataSize);
                        m_lastFrameIdx = index;
                        return true;
                    }
                    return false;
                }
                continue;
            }

            // 3.2 读取数据包
            int readRet = av_read_frame(m_fmtCtx, m_packet);
            if (readRet < 0) {
                if (readRet == AVERROR_EOF) {
                    // ===== 到达文件真实结尾 =====
                    // 尝试取出最后一帧
                    avcodec_send_packet(m_codecCtx, nullptr);
                    int lastRet = avcodec_receive_frame(m_codecCtx, m_decFrame);
                    if (lastRet == 0 && m_decFrame->data[0]) {
                        sws_scale(m_swsCtx,
                            m_decFrame->data, m_decFrame->linesize,
                            0, m_codecCtx->height,
                            m_rgbFrame->data, m_rgbFrame->linesize);
                        const int w = m_codecCtx->width;
                        const int h = m_codecCtx->height;
                        for (int y = 0; y < h; ++y) {
                            memcpy(buffer + static_cast<size_t>(y) * w * 3,
                                m_rgbFrame->data[0] + static_cast<size_t>(y) * m_rgbFrame->linesize[0],
                                static_cast<size_t>(w) * 3);
                        }
                        m_eofFrame.assign(buffer, buffer + m_frameDataSize);
                    }

                    // 标记 EOF，后续请求直接返回缓存
                    if (false == m_eofed) {
                        m_eofed = true;
                        m_eofedPos = index;
                    }
                    m_lastFrameIdx = index;   // 记录本次请求的帧号，使位置正常前进

                    // 如果有缓存，拷贝并返回成功
                    if (!m_eofFrame.empty()) {
                        memcpy(buffer, m_eofFrame.data(), m_frameDataSize);
                        return true;
                    }
                    return false;
                }
                else {
                    // 临时读错误，重试
                    if (++retryCount > MAX_RETRY) {
                        return false;
                    }
                    continue;
                }
            }

            // 忽略非视频包
            if (m_packet->stream_index != m_streamIdx) {
                av_packet_unref(m_packet);
                continue;
            }

            // 发送包给解码器
            avcodec_send_packet(m_codecCtx, m_packet);
            av_packet_unref(m_packet);
            retryCount = 0;
        }

        return false;
    }
} // namespace CyMedia
