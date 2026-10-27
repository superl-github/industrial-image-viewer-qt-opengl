/**
 * @file ImagePipeline.h
 * @brief 与 UI 解耦的图像接收、缓存、处理线程与像素采样管线。
 * @ingroup Display
 *
 * @details
 *  `ImagePipeline` 是显示控件的"服务端"：
 *  - 接收任意线程推入的帧，做参数校验与有界深拷贝缓存；
 *  - 维护独立工作线程，按需启动/停止；
 *  - 调用 `ImageProcessor` 完成 Bayer/YUV/MJPG/打包格式的 CPU 转换；
 *  - 维护原始帧副本，供鼠标像素采样、重处理（拉伸/直方图）使用；
 *  - 统计数据帧率与"慢数据源"状态；
 *  - 通过 `Sink` 回调把处理后的帧交给 UI/渲染层。
 *
 *  该层不包含任何 QWidget / QGraphicsView / OpenGL 类型，便于单独测试与复用。
 *  渲染、标注、面板刷新由 `Sink` 的实现方（显示控件控制器）负责。
 */
#pragma once

#include "CyMediaBaseDef.h"
#include "View/DisplayTypes.h"

#include <cstdint>

namespace CyMedia {
    /**
     * @brief 一帧已完成 CPU 处理、等待渲染/展示的数据。
     * @note `data` 只在 `Sink::onFrame()` 调用期间有效，实现方不得保存指针。
     */
    struct PipelineFrame {
        ImageShowInfo info;                 ///< 处理后的图像信息
        uint8_t* data = nullptr;            ///< 处理后的数据（可能指向管线内部缓冲）
        ImageColorOpe colorOpe;             ///< 本帧使用的色彩处理参数
        bool isSource = false;              ///< 是否来自已缓存原始帧的重处理
        bool updateDisplay = true;          ///< 是否需要上传/刷新显示
        bool updateStretch = true;          ///< 是否需要更新拉伸面板
        bool stretchEnabled = false;        ///< 拉伸面板当前是否可见
        StretchRange stretch;               ///< 拉伸区间
    };

    /**
     * @class CyMedia::ImagePipeline
     * @brief 图像接收/缓存/处理线程/统计的纯服务端管线。
     */
    class ImagePipeline {
    public:
        /**
         * @brief 管线输出接收器。
         * @warning `onFrame()` 在工作线程执行；实现方负责线程安全与 UI 投递。
         */
        class Sink {
        public:
            virtual ~Sink() = default;

            /// 渲染端是否已准备好（例如 OpenGL 已初始化）。工作线程会等待。
            virtual bool isReady() const = 0;

            /// 收到一帧处理完成的数据。
            virtual void onFrame(const PipelineFrame& frame) = 0;

            /// 直接模式下需要外部重新提供一帧（对应 needUpImage 信号）。
            virtual void onNeedImage() {}
        };

        /// 每帧处理选项（由 UI 层在推帧前更新）。
        struct Options {
            DemosaicingMethod demosaic = DEMOSAIC_BILINEAR;
            YUVTransMethod yuv = BT601;
            StretchType stretch = stretch_None;
            StretchRange stretchRange;
            bool stretchPanelVisible = false;
            bool thumbnailVisible = false;
            bool forceCpuBayer = false; ///< AHD 未在 OpenGL 实现，必须 CPU 处理
            bool forceCpuYuv = false;
        };

    public:
        explicit ImagePipeline(Sink* sink);
        ~ImagePipeline();

        ImagePipeline(const ImagePipeline&) = delete;
        ImagePipeline& operator=(const ImagePipeline&) = delete;

        //==================== 输入 ====================
        /// 推入一帧；data 在返回后即可释放。
        bool submit(const ImageShowInfo& info, void* data, bool force = false);

        /// 设置缓存队列深度（1~10），会按需重启工作线程。
        void setStackSize(uint32_t num);
        uint32_t stackSize() const;

        /// 直接模式：不排队，在调用线程同步处理。
        void setDirect(bool direct);
        bool direct() const;

        /// 请求用缓存的原始帧重处理（拉伸/直方图参数变化时）。
        void requestReprocess(bool updateDisplay, bool updateStretch = true, bool force = false);

        /// 停止线程、清空队列、丢弃原始帧。
        void clear();

        //==================== 状态 ====================
        bool hasImage() const;
        ImageShowInfo imageInfo() const;
        /// 最近一帧信息引用（无锁，供既有 API 返回引用；不要跨线程长期持有）。
        const ImageShowInfo& imageInfoRef() const;
        double dataFps() const;
        bool isDataSlow() const;

        /// 用缓存原始帧采样像素颜色（线程安全）。
        bool samplePixel(int x, int y, double* r, double* g, double* b, bool* gray);

        //==================== 配置/回调 ====================
        void setOptions(const Options& opt);
        Options options() const;

        void setImageCallback(ImageFrameCallback cb, void* user);
        void stop();

    private:
        class Impl;
        Impl* d = nullptr;
    };
}
