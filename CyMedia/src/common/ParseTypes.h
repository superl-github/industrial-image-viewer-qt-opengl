/**
 * @file ParseTypes.h
 * @brief 文件解析相关的枚举与帧回调类型。
 * @ingroup Common
 */
#pragma once

#include "ImageTypes.h"

namespace CyMedia {
    /// 图像文件后缀类型。
    enum class ImageSuffix : uint32_t {
        RAW = 0,  ///< .raw
        BMP,      ///< .bmp
        TIFF,     ///< .tiff/.tif
        PNG,      ///< .png
        JPEG,     ///< .jpg/.jpeg
        INVALID,  ///< 无效/未识别
    };

    /// 视频文件后缀类型。
    enum class VideoSuffix : uint32_t {
        RAWV = 0, ///< .raw
        AVI,      ///< .avi
        MP4,      ///< .mp4
        INVALID,  ///< 无效/未识别
    };

    /// 解析结果。
    enum class ParseResult : uint32_t {
        OK = 0,
        FILE_OPEN_FAIL,
        FORMAT_ERROR,
        UNSUPPORTED,
        INVALID_PARAM,
        IO_ERROR
    };

    /// 视频文件头信息。
    typedef struct _VideoParseInfo {
        VideoSuffix videoType;   ///< 视频类型
        ImageShowInfo frameInfo; ///< 帧图像信息
        uint32_t dataOffset;     ///< 数据偏移
        float fps;               ///< 帧率
        int frameCount;          ///< 总帧数
        int64_t size;            ///< 文件大小（字节）
    } VideoParseInfo;

    /**
     * @brief 视频帧回调。
     * @param info 当前帧信息。
     * @param data 帧数据（只读，生命周期由解析器管理）。
     * @param count 帧序号。
     * @param user 注册回调时传入的用户指针。
     */
    using FrameCallback = void (*)(const ImageShowInfo& info, const uint8_t* data,
        int count, void* user);
}
