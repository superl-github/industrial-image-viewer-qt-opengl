#include "CyMediaVideoParseRaw.h"
#include <cstring>
#include <chrono>
#include <winsock.h>

#pragma comment(lib, "ws2_32.lib")

namespace CyMedia {
// RAW 文件头部结构
#pragma pack(push, 1)
    struct RawFileHeader {
        uint8_t version;      // 版本号
        uint8_t nBit;         // 位深
        uint32_t nColor;      // 像素格式（可直接转为 ePixType）
        uint32_t nWidth;      // 宽度
        uint32_t nHeight;     // 高度
        uint32_t rate;        // 帧率 * 100
        uint32_t frameLength; // 单帧字节数
    };
#pragma pack(pop)

    VideoParseRaw::VideoParseRaw() = default;
    VideoParseRaw::~VideoParseRaw() { close(); }

    int VideoParseRaw::open(const std::filesystem::path& filePath, CyMedia::VideoParseInfo& parseInfo, bool format/* = false*/) {
        close(); // 确保关闭之前的

        m_file.open(filePath, std::ios::in | std::ios::binary);
        if (!m_file.is_open()) return 1;

        // 1. 获取文件大小
        m_file.seekg(0, std::ios::end);
        uint64_t fileSize = m_file.tellg();
        m_file.seekg(0, std::ios::beg);

        // 2. 读取头部
        RawFileHeader header;
        m_heardOffset = format ? parseInfo.dataOffset : sizeof(RawFileHeader);
        if (fileSize < m_heardOffset) {
            m_file.close();
            return 2;
        }
        if (format) {
            header.version = 1;
            header.nBit = parseInfo.frameInfo.bit;
            header.nColor = parseInfo.frameInfo.format;
            header.nWidth = parseInfo.frameInfo.width;
            header.nHeight = parseInfo.frameInfo.height;
            header.rate = parseInfo.fps * 100;
            header.frameLength = parseInfo.frameInfo.length;
            m_heardOffset = 0;
        }
        else {
            m_file.read(reinterpret_cast<char*>(&header), sizeof(RawFileHeader));
            header.nColor = htonl(header.nColor);
            header.nWidth = htonl(header.nWidth);
            header.nHeight = htonl(header.nHeight);
            header.rate = htonl(header.rate);
            header.frameLength = htonl(header.frameLength);
            m_heardOffset = sizeof(RawFileHeader);
        }
        
        // 3. 校验数据完整性（帧数是否为整数）
        uint64_t dataSize = fileSize;
        if (false == format) {
            dataSize -= sizeof(RawFileHeader);
        }
        uint32_t calcFrames = static_cast<uint32_t>(dataSize / header.frameLength);
        //按照格式解析帧数必须大于1，否则是图像
        if (format && calcFrames <= 1) {
            m_file.close();
            return 2;
        }
        if (dataSize != (calcFrames * header.frameLength)) {
            m_file.close();
            return 2;
        }

        // 4. 填充 CyMedia::ImageShowInfo（统一数据结构）
        m_frameInfo = ImageShowInfo(
            static_cast<int32_t>(header.nWidth),
            static_cast<int32_t>(header.nHeight),
            static_cast<int8_t>(header.nBit),
            static_cast<ePixType>(header.nColor), // 强转即可
            0,                                    // length 留空，upLenth 自动计算
            PIXEL_VALUE_INT
        );
        // 注意：打包格式（10P/12P）的 length 计算依赖 upLenth，这里我们手动设置 frameDataSize
        // 但 header.frameLength 是文件中的实际长度，我们以它为准
        m_frameDataSize = header.frameLength;
        // 同时修正 ImageShowInfo 的 length 为实际文件长度，防止计算歧义
        m_frameInfo.length = header.frameLength;

        // 5. 填充其他元数据
        m_totalFrames = calcFrames;
        m_framerate = static_cast<float>(header.rate) / 100.0f;
        m_frameIntervalMs = static_cast<uint64_t>(1000.0 / m_framerate);

        // 6. 初始化缓存
        m_frameBuffer.resize(m_frameDataSize);

        m_filePath = filePath;
        m_currentPos = 1;
        m_bPlay = true;     // 打开即就绪
        m_bPause = true;    // 默认暂停

        // 7. 启动异步线程（如果回调已注册）
        // 线程将在 play() 时真正开始循环，这里仅创建
        if (!m_threadRunning) {
            m_threadRunning = true;
            m_thread = std::thread(threadProc, this);
        }

        //填充文件信息
        parseInfo.videoType = VIDEO_SUFFIX_RAW;
        parseInfo.frameInfo = m_frameInfo;
        parseInfo.dataOffset = m_heardOffset;
        parseInfo.fps = m_framerate;
        parseInfo.frameCount = calcFrames;
        parseInfo.size = fileSize;

        return 0;
    }

    void VideoParseRaw::close() {
        m_bPlay = false;
        if (m_file.is_open()) m_file.close();

        // 停止线程
        if (m_thread.joinable()) {
            m_threadRunning = false;
            m_thread.join();
        }

        m_totalFrames = 0;
        m_frameDataSize = 0;
        m_frameBuffer.clear();
        m_callback = nullptr;
    }

    bool VideoParseRaw::isOpen() const {
        return m_file.is_open();
    }

    ImageShowInfo VideoParseRaw::getImageInfo() const {
        return m_frameInfo;
    }
    uint32_t VideoParseRaw::getFrameCount() const { return m_totalFrames; }
    float VideoParseRaw::getFramerate() const { return m_framerate; }

    // ---- 主动获取帧（同步） ----
    bool VideoParseRaw::getFrame(uint32_t index, std::vector<uint8_t>& outData) {
        if (!m_file.is_open() || index < 1 || index > m_totalFrames) return false;

        // 重新分配输出缓冲区
        outData.resize(m_frameDataSize);
        return readFrameData(index, outData.data());
    }

    // ---- 异步播放控制 ----
    void VideoParseRaw::registerFrameCallback(CyMedia::FrameCallback callback, void* userData) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callback = callback;
        m_userData = userData;
    }

    void VideoParseRaw::play() {
        if (!m_file.is_open()) return;
        m_bPlay = true;
        m_bPause = false;
        m_timer = std::chrono::high_resolution_clock::now(); // 重置计时
    }

    void VideoParseRaw::setPause(bool pause) {
        m_bPause = pause;
        //if (!pause) {
        //    // 恢复时重置计时，防止跳跃
        //    m_timer = std::chrono::high_resolution_clock::now();
        //}
    }

    bool VideoParseRaw::isPaused() const { return m_bPause; }

    bool VideoParseRaw::seek(uint32_t pos) {
        if (pos < 1 || pos > m_totalFrames) return false;
        m_currentPos = pos;
        m_lastCallbackPos = 0; // 强制下一次回调触发
        return true;
    }

    uint32_t VideoParseRaw::getCurrentPosition() const { return m_currentPos; }

    void VideoParseRaw::setSpeed(float speed) {
        if (speed <= 0) return;
        m_speed = speed;
        m_frameIntervalMs = static_cast<uint64_t>(1000.0 / (m_framerate * m_speed));
    }

    // ---- 内部实现 ----
    bool VideoParseRaw::readFrameData(uint32_t pos, uint8_t* buffer) {
        if (!buffer || pos < 1 || pos > m_totalFrames) return false;
        uint64_t offset = m_heardOffset + static_cast<uint64_t>(pos - 1) * m_frameDataSize;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_file.seekg(offset, std::ios::beg);
        m_file.read(reinterpret_cast<char*>(buffer), m_frameDataSize);
        return !m_file.fail();
    }

    void VideoParseRaw::threadProc(VideoParseRaw* pThis) {
        while (pThis->m_threadRunning) {
            //播放
            if (pThis->m_bPlay && false == pThis->m_bPause) {
                // 帧率控制
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - pThis->m_timer).count();
                if (elapsed >= pThis->m_frameIntervalMs) {
                    // 读取当前帧
                    uint32_t current = pThis->m_currentPos.load();
                    bool upRe = pThis->readFrameData(current, pThis->m_frameBuffer.data());
                    if (upRe && pThis->m_callback) {
                        pThis->m_callback(pThis->m_frameInfo, pThis->m_frameBuffer.data(), current, pThis->m_userData);
                        pThis->m_currentPos++;
                    }
                    pThis->m_timer = now;
                }
                else {
                    // 未到时间，休眠一小段时间减少 CPU 占用
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                //播放到尾部暂停
                if (pThis->m_currentPos > pThis->m_totalFrames) {
                    pThis->m_currentPos = pThis->m_totalFrames;
                    pThis->m_bPause = true;
                }
            }
            else {
                // 暂停或未播放状态，休眠
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
}

