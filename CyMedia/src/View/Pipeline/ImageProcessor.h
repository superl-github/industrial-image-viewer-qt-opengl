/**
 * @file ImageProcessor.h
 * @brief 与 UI 无关的 CPU 图像格式转换器。
 * @ingroup Display
 *
 * @details
 *  原实现把这些逻辑埋在 `CyMediaDis::privateData::Thread_ImageData_SpecialOpe`
 *  中，与 QWidget / OpenGL 渲染器混在一起。现在抽成纯 CPU 组件：
 *  - Bayer → RGB（双线性/Malvar/AHD）
 *  - YUV → RGB / Y
 *  - MJPG → RGB / Gray
 *  - MONO10P/12P → 解包为 MONO
 *
 *  该类不依赖 Qt UI，也不持有线程，可被管线、测试或其他模块复用。
 */
#pragma once

#include "common/ImageTypes.h"

#include <cstdint>
#include <vector>

namespace CyMedia {
    /**
     * @class CyMedia::ImageProcessor
     * @brief 有状态（复用缓冲区）的 CPU 图像格式转换器。
     * @note 一个实例只应被一个线程使用；管线和直接模式各持有一个实例。
     */
    class ImageProcessor {
    public:
        /// 每帧转换策略。
        struct Context {
            ImageColorOpe colorOpe;           ///< Bayer/YUV/拉伸参数
            bool bayerToRgbOnCpu = false;     ///< 是否在 CPU 做 Bayer 去马赛克
            bool yuvToRgbOnCpu = false;       ///< 是否在 CPU 做 YUV→RGB
        };

        ImageProcessor() = default;

        /**
         * @brief 就地转换特殊格式。
         * @param[in,out] info 图像信息；转换后会更新 format/bit/length。
         * @param[in,out] data 图像数据；需要 CPU 转换时会指向内部复用缓冲区。
         * @param ctx 转换策略。
         * @return 成功返回 true；未知格式保持原样并返回 true。
         */
        bool process(ImageShowInfo& info, void*& data, const Context& ctx);

        /// 内部工作缓冲区当前大小（字节），用于诊断/日志。
        size_t scratchSize() const { return m_scratch.size(); }

    private:
        bool ensureScratch(uint32_t needLen);

    private:
        std::vector<uint8_t> m_scratch;
    };
}

