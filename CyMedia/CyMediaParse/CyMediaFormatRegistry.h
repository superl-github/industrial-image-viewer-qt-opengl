#pragma once
#include "CyMediaVideoParseBase.h"
#include <unordered_map>
#include <functional>
#include <string>

namespace CyMedia {

    class FormatRegistry {
    public:
        using Creator = std::function<std::unique_ptr<VideoParseBase>()>;

        static FormatRegistry& instance() {
            static FormatRegistry reg;
            return reg;
        }

        void registerFormat(VideoSuffix suffix, Creator creator) {
            m_map[suffix] = std::move(creator);
        }

        Creator find(VideoSuffix suffix) const {
            auto it = m_map.find(suffix);
            return it != m_map.end() ? it->second : nullptr;
        }

    private:
        FormatRegistry() = default;
        std::unordered_map<VideoSuffix, Creator> m_map;
    };

    // 自动注册宏（放在各格式 .cpp 文件顶部即可）
#define REGISTER_VIDEO_FORMAT(suffix, ClassName) \
    static const bool _reg_##ClassName = []{ \
        FormatRegistry::instance().registerFormat( \
            suffix, []{ return std::make_unique<ClassName>(); }); \
        return true; \
    }()

    // 同一个类注册多个后缀时用这个，RegName 必须唯一
#define REGISTER_VIDEO_FORMAT_EX(suffix, ClassName, RegName) \
    static const bool _reg_##RegName = []{ \
        FormatRegistry::instance().registerFormat( \
            suffix, []{ return std::make_unique<ClassName>(); }); \
        return true; \
    }()

}
