#include "CyMediaVideoParseBase.h"
#include <algorithm>
#include <cmath>


namespace CyMedia {
    VideoParseBase::~VideoParseBase() {
        ;
    }

    CyMedia::ParseResult VideoParseBase::open(const std::filesystem::path& fp, CyMedia::VideoParseInfo& info, bool format /*= false*/) {
        close();
        auto ret = onOpen(fp, info, format);
        if (ret != ParseResult::OK) return ret;

        m_parseInfo = info;
        m_isOpen = true;
        m_currentPos = 1;
        m_playing = false;
        m_paused = true;

        m_asyncBuffer.resize(m_frameDataSize);
        m_syncBuffer.resize(m_frameDataSize);

        // 启动后台线程（始终创建，通过 CV 休眠等待）
        m_threadExit = false;
        m_thread = std::thread(&VideoParseBase::playbackThread, this);
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

    void VideoParseBase::play() {
        if (!m_isOpen) return;
        {
            std::lock_guard<std::mutex> lk(m_playMutex);
            m_playing = true; 
            m_paused = false;
        }
        m_playCv.notify_one();
    }

    void VideoParseBase::pause() {
        if (m_paused) return;
        { std::lock_guard<std::mutex> lk(m_playMutex); m_paused = true; }
        m_playCv.notify_one();
    }

    bool VideoParseBase::isPaused() const {
        return m_paused;
    }

    bool VideoParseBase::seek(uint64_t pos) {
        if (!m_isOpen || pos < 1 || pos > m_totalFrames) return false;
        m_currentPos = pos;
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

            if (ok) {
                FrameCallback cb = nullptr; void* ud = nullptr;
                {
                    std::lock_guard<std::mutex> lk(m_cbMutex);
                    cb = m_callback; ud = m_userData;
                }
                if (cb) cb(m_frameInfo, m_asyncBuffer.data(), pos, ud);
                m_currentPos = pos + 1;
            }

            // 精确帧率睡眠
            std::this_thread::sleep_until(deadline);
        }
    }
}
