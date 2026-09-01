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

    void VideoParseFfmpeg::onClose()
    {
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
    }

    bool VideoParseFfmpeg::onReadFrame(uint64_t index, uint8_t* buffer) {
        if (!m_fmtCtx || index < 1 || index > m_totalFrames || !buffer)
            return false;
        if (index != m_lastFrameIdx + 1) {
            const int64_t ts = av_rescale_q(
                static_cast<int64_t>(index - 1),
                av_inv_q(m_frameRate),
                m_timeBase
            );
            av_seek_frame(m_fmtCtx, m_streamIdx, ts, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(m_codecCtx);
            m_lastFrameIdx = 0;
            // ===== 跳转后重置逻辑帧和缓存标记 =====
            m_logicalFrame = 0;
            m_hasLastFrame = false;
        }

        bool eofReached = false;
        bool hasDecodedFrame = false;
        int errorCount = 0;
        const int MAX_ERROR_SKIP = 5; // 最多连续跳过5个坏帧
        while (true) {
            //取帧
            const int recvRet = avcodec_receive_frame(m_codecCtx, m_decFrame);
            if (recvRet == 0) {
                hasDecodedFrame = true;
                errorCount = 0; // 成功解码，重置错误计数
                m_hasLastFrame = true;

                uint64_t curIdx = 0;
                if (m_decFrame->pts != AV_NOPTS_VALUE) {
                    curIdx = static_cast<uint64_t>(av_rescale_q(m_decFrame->pts, m_timeBase, av_inv_q(m_frameRate)));
                }
                else {
                    curIdx = m_lastFrameIdx + 1;
                }
                if (m_logicalFrame >= index - 1
                    || curIdx >= index - 1
                    || (eofReached && m_hasLastFrame)) {
                    sws_scale(m_swsCtx,
                        m_decFrame->data, m_decFrame->linesize,
                        0, m_codecCtx->height,
                        m_rgbFrame->data, m_rgbFrame->linesize);

                    const int w = m_codecCtx->width;
                    const int h = m_codecCtx->height;
                    for (int y = 0; y < h; ++y) {
                        std::memcpy(
                            buffer + static_cast<uint64_t>(y) * w * 3,
                            m_rgbFrame->data[0] + static_cast<uint64_t>(y) * m_rgbFrame->linesize[0],
                            static_cast<size_t>(w) * 3
                        );
                    }
                    m_lastFrameIdx = index;
                    return true;
                }
                continue;
            }
            else if (recvRet == AVERROR(EAGAIN)) {
                ;// 需要更多数据，往下走读包
            }
            else {
                // ===== 解码错误跳过，不直接退出 =====
                errorCount++;
                //printf("[解码警告] 跳过第%d个坏帧，错误码：%d\n", errorCount, recvRet);
                if (errorCount >= MAX_ERROR_SKIP) {
                    //printf("[解码错误] 连续%d帧解码失败，终止\n", MAX_ERROR_SKIP);
                    break;
                }
                avcodec_flush_buffers(m_codecCtx); // 清空解码器缓存，跳过坏包

                // 已经解出过帧就兜底返回，避免卡帧
                if (hasDecodedFrame && eofReached) {
                    sws_scale(m_swsCtx,
                        m_decFrame->data, m_decFrame->linesize,
                        0, m_codecCtx->height,
                        m_rgbFrame->data, m_rgbFrame->linesize);

                    const int w = m_codecCtx->width;
                    const int h = m_codecCtx->height;
                    for (int y = 0; y < h; ++y) {
                        std::memcpy(
                            buffer + static_cast<uint64_t>(y) * w * 3,
                            m_rgbFrame->data[0] + static_cast<uint64_t>(y) * m_rgbFrame->linesize[0],
                            static_cast<size_t>(w) * 3
                        );
                    }
                    m_lastFrameIdx = index;
                    return true;
                }
                continue;
            }
            if (eofReached) {
                if (m_hasLastFrame) {
                    sws_scale(m_swsCtx,
                        m_decFrame->data, m_decFrame->linesize,
                        0, m_codecCtx->height,
                        m_rgbFrame->data, m_rgbFrame->linesize);

                    const int w = m_codecCtx->width;
                    const int h = m_codecCtx->height;
                    for (int y = 0; y < h; ++y) {
                        std::memcpy(
                            buffer + static_cast<uint64_t>(y) * w * 3, 
                            m_rgbFrame->data[0] + static_cast<uint64_t>(y) * m_rgbFrame->linesize[0],
                            static_cast<size_t>(w) * 3);
                    }
                    m_lastFrameIdx = index;
                    return true;
                }
                break;
            }
            int ret = av_read_frame(m_fmtCtx, m_packet);
            if (ret < 0) {
                // ===== 区分真正EOF和读错误 =====
                if (ret == AVERROR_EOF) {
                    // 真正文件结束，发送空包flush解码器
                    avcodec_send_packet(m_codecCtx, nullptr);
                    eofReached = true;
                    continue;
                }
                else {
                    // 临时读错误，重试一次
                    errorCount++;
                    if (errorCount < MAX_ERROR_SKIP) {
                        printf("[读包警告] 读包出错，错误码：%d，重试\n", ret);
                        continue;
                    }
                    else {
                        // 多次失败，当成EOF处理
                        avcodec_send_packet(m_codecCtx, nullptr);
                        eofReached = true;
                        continue;
                    }
                }
            }

            if (m_packet->stream_index != m_streamIdx) {
                av_packet_unref(m_packet);
                continue;
            }

            avcodec_send_packet(m_codecCtx, m_packet);
            // ===== 视频包计数，一个包对应一个时间槽 =====
            m_logicalFrame++;
            av_packet_unref(m_packet);
        }
        return false;
    }
} // namespace CyMedia
