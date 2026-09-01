#include "CyMediaVideoParseBase.h"
#include <algorithm>
#include <cmath>
#include <queue>      
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>

namespace CyMedia {
    VideoParseBase::~VideoParseBase() {
        ;
    }

    CyMedia::ParseResult VideoParseBase::open(const std::filesystem::path& fp, CyMedia::VideoParseInfo& info, bool format /*= false*/) {
        close();
        auto ret = onOpen(fp, info, format);
        if (ret != ParseResult::OK) return ret;

        m_parseInfo = info;

        if (m_framerate <= 0.0f) {
            m_framerate = 25.0f;
        }

        m_isOpen = true;
        m_currentPos = 1;
        m_playing = false;
        m_paused = true;

        m_asyncBuffer.resize(m_frameDataSize);
        m_syncBuffer.resize(m_frameDataSize);

        // 启动后台线程（始终创建，通过 CV 休眠等待）
        m_threadExit = false;
        m_thread = std::thread(&VideoParseBase::playbackThread, this);

        m_callbackThreadExit = false;
        m_callbackThread = std::thread(&VideoParseBase::callbackDispatchThread, this);

        return ParseResult::OK;
    }

    void VideoParseBase::close() {
        {
            std::lock_guard<std::mutex> lk(m_playMutex);
            m_threadExit = true;
            m_playing = false;
        }
        m_playCv.notify_all();

        if (m_thread.joinable()) m_thread.join();


        // ========== 停止回调分发线程 ==========
        {
            std::lock_guard<std::mutex> qlk(m_frameQueueMtx);
            m_callbackThreadExit = true;
            // 清空队列，无需处理残留帧
            while (!m_frameQueue.empty()) m_frameQueue.pop();
        }
        m_frameQueueCv.notify_all();
        if (m_callbackThread.joinable()) {
            m_callbackThread.join();
        }


        { std::lock_guard<std::mutex> lk(m_ioMutex); onClose(); }

        m_isOpen = false;
        m_asyncBuffer.clear();
        m_syncBuffer.clear();
        m_callback = nullptr;
    }

    bool VideoParseBase::isOpen() {
        return m_isOpen;
    }

    CyMedia::ImageShowInfo VideoParseBase::getImageInfo() {
        return m_frameInfo;
    }

    uint64_t VideoParseBase::getFrameCount() {
        return m_totalFrames;
    }

    float VideoParseBase::getFramerate() {
        return m_framerate;
    }

    bool VideoParseBase::getFrame(uint64_t index, std::vector<uint8_t>& out) {
        if (!m_isOpen || index < 1 || index > m_totalFrames) return false;
        std::lock_guard<std::mutex> lk(m_ioMutex);
        if (!onReadFrame(index, m_syncBuffer.data())) return false;
        out.assign(m_syncBuffer.begin(), m_syncBuffer.end());
        return true;
    }

    void VideoParseBase::registerFrameCallback(CyMedia::FrameCallback cb, void* ud /*= nullptr*/) {
        std::lock_guard<std::mutex> lk(m_cbMutex);
        m_callback = cb; m_userData = ud;
    }

    void VideoParseBase::setAlignTarget(uint32_t targetFrame) {
        m_alignTarget = targetFrame;
        m_needAlign = true;
    }

    void VideoParseBase::play() {
        if (!m_isOpen) return; {
            std::lock_guard<std::mutex> lk(m_playMutex);
            if (m_needAlign) {
                uint64_t target = m_alignTarget.load();
                if (target > 0) {
                    // 优先用上层传入的显示位置对齐
                    if (m_currentPos > target + 1) {
                        m_currentPos = target + 1;
                    }
                }
                else {
                    // 没有外部目标时，回退到用回调位置对齐
                    uint64_t cbPos = m_lastCallbackPos.load();
                    if (cbPos > 0 && m_currentPos > cbPos + 1) {
                        m_currentPos = cbPos + 1;
                    }
                }
                m_needAlign = false;
                m_alignTarget = 0;  // 用完清零，只生效一次
            }
            m_playing = true;
            m_paused = false;
        }
        m_playCv.notify_one();
    }

    void VideoParseBase::pause() {
        if (m_paused) return;
        { std::lock_guard<std::mutex> lk(m_playMutex); m_paused = true; }
        m_playCv.notify_one();
        m_needAlign = true;  // 标记：下次 play 需要帧号对齐
        // 清空队列：暂停后不再回调残留帧，保证 m_lastCallbackPos 就是暂停位置
        {
            std::lock_guard<std::mutex> lk(m_frameQueueMtx);
            while (!m_frameQueue.empty()) m_frameQueue.pop();
        }
    }

    bool VideoParseBase::isPaused() const {
        return m_paused;
    }

    bool VideoParseBase::seek(uint64_t pos) {
        if (!m_isOpen || pos < 1 || pos > m_totalFrames) return false;
        m_currentPos = pos;
        m_needAlign = false;
        {
            std::lock_guard<std::mutex> lk(m_frameQueueMtx);
            while (!m_frameQueue.empty()) m_frameQueue.pop();
        }

        return true;
    }

    uint64_t VideoParseBase::getCurrentPosition() const {
        return m_currentPos;
    }

    void VideoParseBase::setSpeed(float s) {
        if (s > 0.f) m_speed = s;
    }

    void VideoParseBase::playbackThread() {
        while (!m_threadExit) {
            // 等待播放信号
            {
                std::unique_lock<std::mutex> lk(m_playMutex);
                m_playCv.wait(lk, [this] {
                    return m_threadExit || (m_playing && !m_paused);
                    });
            }
            if (m_threadExit) break;

            // 帧率控制
            auto interval = std::chrono::microseconds(
                static_cast<int64_t>(1'000'000.0 / (m_framerate * m_speed.load())));
            auto deadline = std::chrono::steady_clock::now() + interval;

            uint64_t pos = m_currentPos.load();
            if (pos > m_totalFrames) {
                m_paused = true;
                continue;
            }

            // IO 操作加锁，与同步 getFrame 互斥
            bool ok = false;
            {
                std::lock_guard<std::mutex> lk(m_ioMutex);
                ok = onReadFrame(pos, m_asyncBuffer.data());
            }
            FrameQueueItem newItem;
            newItem.info = m_frameInfo;
            newItem.framePos = pos;
            if (ok) {
                newItem.frameData = m_asyncBuffer; // 完整拷贝帧数据，脱离原缓冲区
            }
            {
                    std::lock_guard<std::mutex> lk(m_frameQueueMtx);
                    // 队列满则丢弃最旧的帧，保证实时性，不阻塞解码线程
                    while (m_frameQueue.size() >= m_maxQueueSize) {
                        m_frameQueue.pop();
                    }
                    m_frameQueue.push(std::move(newItem));
                }
                m_frameQueueCv.notify_one(); // 唤醒回调线程
            m_currentPos = pos + 1;
            // ==============================================

            // 精确帧率睡眠
            std::this_thread::sleep_until(deadline);
        }
    }

    void VideoParseBase::callbackDispatchThread() {
        // 回调帧率控制：记录下次回调的时间点
        auto nextCallbackTime = std::chrono::steady_clock::now();

        while (!m_callbackThreadExit) {
            FrameQueueItem item;
            bool hasItem = false;
            // 等待队列数据或退出信号
            {
                std::unique_lock<std::mutex> lk(m_frameQueueMtx);
                m_frameQueueCv.wait(lk, [this]() {
                    return m_callbackThreadExit || !m_frameQueue.empty();
                    });
                if (m_callbackThreadExit) break;
                if (!m_frameQueue.empty()) {
                    item = std::move(m_frameQueue.front());
                    m_frameQueue.pop();
                    hasItem = true;
                }
            }
            if (!hasItem) {
                // 队列空了（暂停清队列等），重置计时器，避免恢复后追赶式回调
                nextCallbackTime = std::chrono::steady_clock::now();
                continue;
            }
            // ========== 按帧率控制回调间隔 ==========
            auto interval = std::chrono::microseconds(
                static_cast<int64_t>(1'000'000.0 / (m_framerate * m_speed.load())));
            nextCallbackTime += interval;
            auto now = std::chrono::steady_clock::now();
            if (now > nextCallbackTime) {
                // 回调函数执行太慢，已经落后于预定时间，重置基准
                // 避免 sleep_until 立刻返回导致疯狂追赶
                nextCallbackTime = now;
            }
            std::this_thread::sleep_until(nextCallbackTime);
            // ==============================================

            // 取出回调函数
            FrameCallback cb = nullptr;
            void* ud = nullptr;
            {
                std::lock_guard<std::mutex> lk(m_cbMutex);
                cb = m_callback;
                ud = m_userData;
            }
            // 执行业务回调
            if (cb) {
                // ===== 帧率验证打印 =====
                static auto lastCbTime = std::chrono::steady_clock::now();
                static uint64_t lastCbFrame = 0;
                auto now = std::chrono::steady_clock::now();
                int deltaMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCbTime).count();
                lastCbTime = now;
                //printf("[回调帧率] frame=%llu 距上次=%dms 理论=%.0fms\n", (unsigned long long)item.framePos, deltaMs, 1000.0 / m_framerate);
                cb(item.info, item.frameData.data(), static_cast<int>(item.framePos), ud);
                m_lastCallbackPos = item.framePos;
            }
        }
    }
}


