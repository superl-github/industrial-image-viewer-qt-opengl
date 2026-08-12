#include "CyMediaVideoParse.h"
#include "CyMediaVideoParseRaw.h"

#include <algorithm>
#include <cctype>

#include <QFile>
#include <QFileInfo>


namespace CyMedia {
    class VideoParser::Private {
    public:
        std::unique_ptr<VideoParseBase> m_impl = nullptr;
    };


    VideoParser::VideoParser() {
        d = new Private;
    }


    VideoParser::~VideoParser() {
        if (d->m_impl) d->m_impl->close();
        delete d;
    }


    std::string VideoParser::videoTypeStr(CyMedia::VideoSuffix type) {
        switch (type) {
            case CyMedia::VIDEO_SUFFIX_RAW: return "raw";
        }
        return "Undefined";
    }


    CyMedia::VideoSuffix VideoParser::getvideoTypeByPath(const std::string& filePath) {
        std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == "raw" || ext == "rawv") {
            return VIDEO_SUFFIX_RAW;
        }

        return VIDEO_SUFFIX_INVALID;
    }


    int VideoParser::open(const std::filesystem::path& filePath, CyMedia::VideoParseInfo& parseInfo, bool format/* = false*/) {
        close(); // 释放旧的解析器

        auto videoType = getvideoTypeByPath(filePath.string());

        if (videoType == VIDEO_SUFFIX_RAW) {
            d->m_impl = std::make_unique<VideoParseRaw>();
        }
        else {
            return 3; // 不支持的格式
        }

        return d->m_impl->open(filePath, parseInfo, format);
    }


    void VideoParser::close() {
        if (d->m_impl) {
            d->m_impl->close();
            d->m_impl.reset();
        }
    }


    bool VideoParser::isOpen() const {
        return d->m_impl && d->m_impl->isOpen();
    }


    CyMedia::ImageShowInfo VideoParser::getImageInfo() const {
        return d->m_impl ? d->m_impl->getImageInfo() : ImageShowInfo();
    }


    uint32_t VideoParser::getFrameCount() const {
        return d->m_impl ? d->m_impl->getFrameCount() : 0;
    }


    float VideoParser::getFramerate() const
    {
        return d->m_impl ? d->m_impl->getFramerate() : 0.0f;
    }


    bool VideoParser::getFrame(uint32_t index, std::vector<uint8_t>& outData) {
        return d->m_impl ? d->m_impl->getFrame(index, outData) : false;
    }


    void VideoParser::registerFrameCallback(CyMedia::FrameCallback callback, void* userData /*= nullptr*/) {
        if (d->m_impl) d->m_impl->registerFrameCallback(callback, userData);
    }


    void VideoParser::play() {
        if (d->m_impl) d->m_impl->play();
    }


    void VideoParser::setPause(bool pause) {
        if (d->m_impl) d->m_impl->setPause(pause);
    }


    bool VideoParser::isPaused() const {
        return d->m_impl ? d->m_impl->isPaused() : true;
    }


    bool VideoParser::seek(uint32_t pos) {
        return d->m_impl ? d->m_impl->seek(pos) : false;
    }


    uint32_t VideoParser::getCurrentPosition() const {
        return d->m_impl ? d->m_impl->getCurrentPosition() : 0;
    }


    void VideoParser::setSpeed(float speed) {
        if (d->m_impl) d->m_impl->setSpeed(speed);
    }
}
