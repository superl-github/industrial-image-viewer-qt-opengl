// CyMediaVideoParseBase.h
#pragma once
#include "CyMediaBaseDef.h"
#include <filesystem>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <queue>

namespace CyMedia {
    /**
     * @brief 视频解析器抽象基类。
     * @details 定义了视频文件解析的标准接口，支持主动获取（同步）和
     *          被动回调（异步推流）两种工作模式。派生类需实现具体容器（RAW/AVI/MP4）的解析逻辑。
     * @note 所有接口均为线程安全设计，派生类内部需自行加锁保护共享资源。
     */
    class VideoParseBase {
    public:
        virtual ~VideoParseBase();

        //==================== 文件控制 ====================
        /**
         * @brief 打开视频文件。
         * @param filePath 文件绝对路径或相对路径（UTF-8编码）。
         * @param parseInfo 存放解析的视频文件信息。
         * @param format true:按照parseInfo指定的信息解析视频文件(raw)。
         * @return CyMedia::ParseResult。
         */
        CyMedia::ParseResult open(const std::filesystem::path& filePath, CyMedia::VideoParseInfo& parseInfo, bool format = false);

        /**
         * @brief 关闭当前打开的文件，释放所有资源（包括线程和缓存）。
         * @details 关闭后对象可复用，再次调用 open() 即可解析新文件。
         */
        void close();

        /**
         * @brief 检查文件是否已成功打开且处于有效状态。
         * @return 已打开且有效返回 true，否则 false。
         */
        bool isOpen();

        //==================== 元数据 ====================
        /**
         * @brief 获取当前视频帧的图像属性。
         * @return ImageShowInfo 结构体，包含宽、高、位深、像素格式等。
         *         若文件未打开，返回默认空结构（宽度为0）。
         * @see CyMedia::ImageShowInfo
         */
        ImageShowInfo getImageInfo();
        /**
         * @brief 获取视频文件总帧数。
         * @return 总帧数（索引从 1 开始）。
         */
        uint64_t getFrameCount();
        /**
         * @brief 获取视频原始帧率。
         * @return 帧率（帧/秒），若无法获取则返回 0.0f。
         */
        float getFramerate();

        //==================== 同步获取 ====================
        /**
         * @brief 获取指定索引位置的帧数据。
         * @details 该操作直接从文件中读取，不影响异步播放进度（currentPos 不变）。
         *          调用者需保证 outData 已预分配足够内存，或传入空 vector 由内部自动分配。
         * @param index 帧索引（从 1 开始，范围 1 ~ getFrameCount()）。
         * @param outData 输出缓冲区（引用），函数执行后包含完整的帧数据。
         * @return 成功返回 true，失败（索引越界或文件错误）返回 false。
         */
        bool getFrame(uint64_t index, std::vector<uint8_t>& outData);

        //==================== 异步播放 ====================
        /**
         * @brief 注册帧数据回调函数。
         * @details 注册后，当异步播放线程推送一帧时，该回调将被触发。
         *          回调执行期间，数据指针有效，但回调返回后数据可能被覆盖，外部需及时拷贝。
         * @param callback 回调函数对象（可为 nullptr 以取消回调）。
         * @param userData 用户自定义数据，会在回调时原样透传。
         */
        void registerFrameCallback(CyMedia::FrameCallback callback, void* userData = nullptr);

        /**
         * @brief 开始或恢复异步播放（非阻塞）。
         * @details 调用后内部线程开始按帧率推送数据。若当前处于暂停状态，将自动恢复。
         */
        void play();



        void setAlignTarget(uint32_t targetFrame);
        /**
         * @brief 暂停推流。
         */
        void pause();

        /**
         * @brief 获取当前暂停状态。
         * @return 暂停中返回 true，播放中返回 false。
         */
        bool isPaused() const;

        /**
         * @brief 跳转到指定帧（异步模式下）。
         * @details 跳转后，下一次 push 将从该帧开始。若当前处于播放状态，会丢弃积压的帧。
         * @param pos 目标帧索引（从 1 开始）。
         * @return 成功返回 true，索引超限返回 false。
         */
        bool seek(uint64_t pos);

        /**
         * @brief 获取当前异步播放进度（最近一次推送的帧索引）。
         * @return 当前帧索引（从 1 开始），若从未推送则返回 0。
         */
        uint64_t getCurrentPosition() const;

        /**
         * @brief 设置播放速度倍率。
         * @param speed 倍率（必须 > 0）。1.0f 为原速，2.0f 为两倍速，0.5f 为半速。
         * @note 速度改变会影响帧间隔时间，内部计时器将自动重校准。
         */
        void setSpeed(float speed);

      

    protected:
        // ===== 子类必须实现的纯虚接口 =====
        virtual ParseResult onOpen(const std::filesystem::path& filePath,
            VideoParseInfo& parseInfo, bool format) = 0;
        virtual void onClose() = 0;

        /// 读取指定帧到 buffer（buffer 已由基类分配好，大小 = frameDataSize）
        /// @return true=成功
        virtual bool onReadFrame(uint64_t index, uint8_t* buffer) = 0;

        /// 子类可选覆写：返回单帧字节数（默认用 m_frameDataSize）
        virtual uint32_t getFrameDataSize() const { return m_frameDataSize; }

        // 受保护的元数据，由子类在 onOpen 中填充
        ImageShowInfo m_frameInfo{};
        uint64_t      m_totalFrames = 0;
        float         m_framerate = 0.0f;
        uint32_t      m_frameDataSize = 0;
        VideoParseInfo m_parseInfo{};

    private:
        void playbackThread();

            // ========== 新增：回调分发线程与队列 ==========
            void callbackDispatchThread();
            // 队列条目：完整拷贝帧数据，脱离解码线程缓冲区生命周期
            struct FrameQueueItem
            {
                ImageShowInfo info;
                std::vector<uint8_t> frameData;
                uint64_t framePos;
            };

            std::queue<FrameQueueItem> m_frameQueue;
            mutable std::mutex m_frameQueueMtx;
            std::condition_variable m_frameQueueCv;
            size_t m_maxQueueSize = 4;   // 最大缓存4帧，超过自动丢旧帧
            std::thread m_callbackThread;
            std::atomic<bool> m_callbackThreadExit{ false };
            // ==============================================

         



        // 播放状态
        std::atomic<bool>     m_isOpen{ false };
        std::atomic<bool>     m_playing{ false };
        std::atomic<bool>     m_paused{ true };
        std::atomic<uint64_t> m_currentPos{ 1 };
        std::atomic<uint64_t> m_lastCallbackPos{ 0 };  // 最后一个成功回调的帧号
        std::atomic<uint32_t> m_alignTarget{ 0 };  // 上层指定的对齐目标，0=不使用
        std::atomic<bool>     m_needAlign{ false };     // 暂停后恢复播放时需要对齐
        std::atomic<float>    m_speed{ 1.0f };

        // 回调
        FrameCallback m_callback = nullptr;
        void* m_userData = nullptr;
        mutable std::mutex m_cbMutex;  // 仅保护 callback 指针

        // 线程
        std::thread           m_thread;
        std::atomic<bool>     m_threadExit{ false };
        std::mutex            m_playMutex;
        std::condition_variable m_playCv;

        // 同步读取锁（与异步线程互斥访问 IO）
        std::mutex m_ioMutex;

        // 帧缓冲（双缓冲：一个给异步线程，一个给同步 getFrame）
        std::vector<uint8_t> m_asyncBuffer;
        std::vector<uint8_t> m_syncBuffer;
    };

} // namespace CyMedia