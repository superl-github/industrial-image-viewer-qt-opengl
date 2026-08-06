#pragma once
#include "CyMediaVideoParseBase.h"
#include <fstream>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

namespace CyMedia {
    /**
 * @brief RAW 格式视频解析器
 * @details 解析特定格式的 .raw 视频文件（头部包含 22 字节的 CyRawVideoInfo）
 */
    class VideoParseRaw : public VideoParseBase {
    public:
        VideoParseRaw();
        ~VideoParseRaw() override;

        // 基类接口实现
        int open(const std::filesystem::path& filePath, CyMedia::VideoParseInfo& parseInfo, bool format = false) override;
        void close() override;
        bool isOpen() const override;

        ImageShowInfo getImageInfo() const override;
        uint32_t getFrameCount() const override;
        float getFramerate() const override;

        bool getFrame(uint32_t index, std::vector<uint8_t>& outData) override;

        void registerFrameCallback(CyMedia::FrameCallback callback, void* userData = nullptr) override;

        void play() override;
        void setPause(bool pause) override;
        bool isPaused() const override;
        bool seek(uint32_t pos) override;
        uint32_t getCurrentPosition() const override;
        void setSpeed(float speed) override;

    private:
        // 内部线程函数（静态，适配 std::thread）
        static void threadProc(VideoParseRaw* pThis);

        // 内部读取一帧数据到缓存
        bool readFrameData(uint32_t pos, uint8_t* buffer);

    private:
        // 文件与元数据
        std::fstream m_file;
        uint32_t m_heardOffset = 0;
        std::filesystem::path m_filePath;
        ImageShowInfo m_frameInfo;      // 统一使用新定义的图像信息
        uint32_t m_totalFrames = 0;
        float m_framerate = 0.0f;
        uint32_t m_frameDataSize = 0;   // 单帧字节数

        // 帧缓存
        std::vector<uint8_t> m_frameBuffer;

        // 播放控制
        std::atomic<bool> m_bPlay{ false };
        std::atomic<bool> m_bPause{ true };
        std::atomic<uint32_t> m_currentPos{ 1 };
        std::atomic<uint32_t> m_lastCallbackPos{ 0 };

        // 帧率控制
        float m_speed = 1.0f;
        uint64_t m_frameIntervalMs = 0; // 微秒
        std::chrono::time_point<std::chrono::high_resolution_clock> m_timer;

        // 回调
        CyMedia::FrameCallback m_callback = nullptr;
        void* m_userData = nullptr;

        // 线程与同步
        std::thread m_thread;
        std::atomic<bool> m_threadRunning{ false };
        std::mutex m_mutex;
    };
}