#include "ImageProcessor.h"

#include "Calculate/Calculate.h"
#include "../../common/CyMediaLog.h"

#include <cstring>

namespace CyMedia {
    bool ImageProcessor::ensureScratch(uint32_t needLen) {
        if (needLen == 0) {
            CyMediaLog::instance().log_printf_Error(
                "[ImageProcessor] invalid scratch size: 0");
            return false;
        }
        // 只在容量不足时增长，避免每帧清零整个缓冲区。
        if (m_scratch.size() < needLen) {
            m_scratch.resize(needLen);
        }
        return true;
    }

    bool ImageProcessor::process(ImageShowInfo& info, void*& data, const Context& ctx) {
        if (!data) {
            CyMediaLog::instance().log_printf_Error(
                "[ImageProcessor] null frame data, format=%d bit=%d",
                int(info.format), int(info.bit));
            return false;
        }

        // Bayer 关闭去马赛克：直接按单色显示原始 Bayer 数据。
        if (info.isBayer() && ctx.colorOpe.bayerFunc == DEMOSAIC_NONE) {
            info.format = MONO;
            return true;
        }

        if (info.isBayer()) {
            if (!ctx.bayerToRgbOnCpu) {
                return true; // 交给 OpenGL 着色器处理
            }
            const uint32_t rgbLen = info.length * 3;
            if (!ensureScratch(rgbLen)) return false;
            if (!CyMediaCalc::bayer2RGB(info, static_cast<uint8_t*>(data),
                m_scratch.data(), ctx.colorOpe.bayerFunc)) {
                CyMediaLog::instance().log_printf_Error(
                    "[ImageProcessor] bayer2RGB failed: %dx%d bit=%d pattern=%d method=%d",
                    info.width, info.height, int(info.bit), int(info.format),
                    int(ctx.colorOpe.bayerFunc));
                return false;
            }
            info.length = rgbLen;
            info.format = RGB;
            data = m_scratch.data();
            return true;
        }

        if (info.isYUV()) {
            if (!ctx.yuvToRgbOnCpu) {
                return true; // 交给 OpenGL 着色器处理
            }
            const uint32_t rgbLen = info.width * info.height * 3;
            if (!ensureScratch(rgbLen)) return false;
            if (!CyMediaCalc::YUV2RGB(info, static_cast<uint8_t*>(data),
                m_scratch.data(), ctx.colorOpe.YUVFunc)) {
                CyMediaLog::instance().log_printf_Error(
                    "[ImageProcessor] YUV2RGB failed: %dx%d format=%d method=%d",
                    info.width, info.height, int(info.format), int(ctx.colorOpe.YUVFunc));
                return false;
            }
            if (ctx.colorOpe.YUVFunc == YUVTRANS_Y) {
                info.length = info.width * info.height;
                info.format = MONO;
            }
            else {
                info.length = rgbLen;
                info.format = RGB;
            }
            data = m_scratch.data();
            return true;
        }

        if (info.format == MJPG) {
            const uint32_t rgbLen = info.width * info.height * 3;
            if (!ensureScratch(rgbLen)) return false;
            bool isGray = false;
            if (!CyMediaCalc::JPG2RGB(info, data, m_scratch.data(), isGray)) {
                CyMediaLog::instance().log_printf_Error(
                    "[ImageProcessor] JPG2RGB failed: %dx%d bit=%d len=%u",
                    info.width, info.height, int(info.bit), info.length);
                return false;
            }
            if (isGray) {
                info.length = info.width * info.height;
                info.format = MONO;
            }
            else {
                info.length = rgbLen;
                info.format = RGB;
            }
            data = m_scratch.data();
            return true;
        }

        // 10/12 位打包单色：解包为普通 MONO。
        switch (info.format) {
            case MONO10P_GVSP:
            case MONO10P:
            case MONO12P_GVSP:
            case MONO12P: {
                ImageShowInfo tempInfo = info;
                tempInfo.format = MONO;
                tempInfo.bit = (info.format == MONO10P || info.format == MONO10P_GVSP) ? 10 : 12;
                tempInfo.upLenth();
                if (!ensureScratch(tempInfo.length)) return false;
                if (!CyMediaCalc::monoUnPack(info, static_cast<uint8_t*>(data), m_scratch.data())) {
                    CyMediaLog::instance().log_printf_Error(
                        "[ImageProcessor] monoUnPack failed: %dx%d format=%d len=%u",
                        info.width, info.height, int(info.format), info.length);
                    return false;
                }
                info = tempInfo;
                data = m_scratch.data();
                return true;
            }
            default:
                break;
        }

        return true;
    }
}
