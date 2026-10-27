/**
 * @file ImageTypes.h
 * @brief 图像像素格式、色彩处理参数与单帧图像描述。
 * @ingroup Common
 *
 * @details
 *  本文件只包含图像/像素相关的枚举与值类型，不依赖 Qt UI、日志或文件解析。
 *  需要完整公共定义时可包含 `CyMediaBaseDef.h` 聚合头。
 */
#pragma once

#include "Export.h"

#include <stdint.h>

namespace CyMedia {
    //==================== 色彩空间转换常量 ====================
    constexpr float THRESHOLD_SRGB = 0.04045f;    ///< sRGB → 线性 RGB 阈值
    constexpr float THRESHOLD_LINEAR = 0.0031308f; ///< 线性 RGB → sRGB 阈值
    constexpr float THRESHOLD_F = 0.008856f;       ///< CIELAB f(t) 阈值
    constexpr float THRESHOLD_F_INV = 6.0f / 29.0f; ///< CIELAB 逆 f(t) 阈值

    //==================== 图像格式 ====================
    /**
     * @brief 像素格式。
     * @details 单色与打包格式遵循 GenICam；Bayer 支持四种排列；YUV 覆盖
     *          Packed / Planar / Semi-Planar。每通道最大 31 位。
     */
    enum ePixType {
        MONO = 0,       ///< 单色（灰度）

        MONO10P,        ///< 10 位单色打包（5 字节 4 像素）
        MONO10P_GVSP,   ///< 10 位单色 GVSP 打包
        MONO12P,        ///< 12 位单色打包（3 字节 2 像素）
        MONO12P_GVSP,   ///< 12 位单色 GVSP 打包

        MONO_OVERSIZE,  ///< 单色扩展标记（内部使用）

        RGB,            ///< RGB
        RGBA,           ///< RGBA
        BGR,            ///< BGR

        BAYERRG,        ///< Bayer RGGB
        BAYERGR,        ///< Bayer GRBG
        BAYERBG,        ///< Bayer BGGR
        BAYERGB,        ///< Bayer GBRG

        FOURCC_YUY2,    ///< YUYV 4:2:2 packed
        FOURCC_YVYU,    ///< YVYU 4:2:2 packed
        FOURCC_I422,    ///< YUV 4:2:2 planar
        FOURCC_YV16,    ///< YVU 4:2:2 planar
        FOURCC_I420,    ///< YUV 4:2:0 planar
        FOURCC_YV12,    ///< YVU 4:2:0 planar
        FOURCC_NV12,    ///< YUV 4:2:0 semi-planar
        FOURCC_NV21,    ///< YVU 4:2:0 semi-planar

        MJPG,           ///< JPEG 压缩帧
    };

    /// 像素值数据类型。
    enum ePixelValueType {
        PIXEL_VALUE_INT = 0, ///< 整型（默认）
        PIXEL_VALUE_F32,     ///< 32 位浮点
        PIXEL_VALUE_F64,     ///< 64 位浮点
    };

    /// 图像保存格式。
    enum eSaveImageFormat {
        ImageFormat_BMP, ///< Windows Bitmap
        ImageFormat_RAW  ///< 原始数据（可带文件头）
    };

    /// 两图逐像素运算类型。
    enum ImageMath {
        Image1AddImage2,
        Image1SubImage2,
        Image2SubImage1,
        Image1TimesImage2,
        Image1DivideImage2,
        Image2DivideImage1,
    };

    /// RGBA 通道索引。
    enum RGBChannel {
        Ch_R,
        Ch_G,
        Ch_B,
        Ch_A,
    };

    /// Bayer 去马赛克算法。
    enum DemosaicingMethod {
        DEMOSAIC_NONE,      ///< 不插值（按单色显示）
        DEMOSAIC_BILINEAR,  ///< 双线性（最快）
        DEMOSAIC_MALVA,     ///< Malvar（质量/速度均衡）
        DEMOSAIC_AHD,       ///< AHD（质量最高）
    };

    /// YUV 转 RGB 方法。
    enum YUVTransMethod {
        YUVTRANS_Y = 0, ///< 仅使用 Y 通道
        BT601,          ///< BT.601 YUV → RGB
    };

    /// 灰度/对比度拉伸类型。
    enum StretchType {
        stretch_None = 0, ///< 不拉伸
        stretch_Gray,     ///< 灰度直接拉伸
        stretch_HSV,      ///< HSV V 通道拉伸
        stretch_Lab,      ///< Lab L* 通道拉伸
    };

    //==================== 核心结构 ====================
#pragma pack(push, 1)
    /**
     * @brief 图像色彩处理参数。
     * @details 统一传递 Bayer、YUV 与拉伸策略。
     */
    struct ImageColorOpe {
        DemosaicingMethod bayerFunc = DEMOSAIC_NONE;
        YUVTransMethod YUVFunc = BT601;
        StretchType stretchType = stretch_None;
        int stretch_S = 0;
        int stretch_E = 0;
    };

    /**
     * @brief 单帧图像元数据与数据长度。
     * @details 提供通道数、行字节数、总长度及格式判断辅助方法。
     */
    struct ImageShowInfo {
        int32_t width = 0;              ///< 宽（像素）
        int32_t height = 0;             ///< 高（像素）
        int8_t  bit = 8;                ///< 每通道位深（1~31）
        ePixType format = MONO;         ///< 像素格式
        ePixelValueType special_pixel = PIXEL_VALUE_INT; ///< 像素值类型
        uint32_t length = 0;            ///< 数据总长度（字节）

        ImageShowInfo(int w, int h, int b, ePixType f,
            uint32_t l = 0, ePixelValueType sv = PIXEL_VALUE_INT)
            : width(w), height(h), bit(static_cast<int8_t>(b)), format(f), special_pixel(sv) {
            if (l == 0) {
                upLenth();
            }
            else {
                length = l;
            }
        }

        ImageShowInfo() = default;

        /// 返回颜色通道数（RGBA 返回 4）。
        uint8_t channel() const {
            switch (format) {
                case MONO:
                case BAYERRG:
                case BAYERGR:
                case BAYERBG:
                case BAYERGB:
                case MONO10P:
                case MONO10P_GVSP:
                case MONO12P:
                case MONO12P_GVSP:
                case MONO_OVERSIZE:
                    return 1;
                case RGB:
                    return 3;
                case RGBA:
                    return 4;
                default:
                    return 1;
            }
        }

        /// 返回每行字节数。
        int bytesPerLine() {
            int bytesNumber = width;
            if (format == MONO10P || format == MONO10P_GVSP) {
                return (bytesNumber * 10 + 7) / 8;
            }
            if (format == MONO12P || format == MONO12P_GVSP) {
                return (bytesNumber * 12 + 7) / 8;
            }

            int pixelLen = 0;
            if (bit <= 8) pixelLen = 1;
            else if (bit <= 16) pixelLen = 2;
            else if (bit <= 31) pixelLen = 4;

            switch (special_pixel) {
                case PIXEL_VALUE_F32: pixelLen = 4; break;
                case PIXEL_VALUE_F64: pixelLen = 8; break;
                default: break;
            }

            return bytesNumber * pixelLen * channel();
        }

        /// 根据宽、高、位深、格式计算并更新 `length`。
        uint32_t upLenth() {
            int pixelNum = width * height;
            if (format == MONO10P || format == MONO10P_GVSP) {
                length = (pixelNum * 10 + 7) / 8;
                return length;
            }
            if (format == MONO12P || format == MONO12P_GVSP) {
                length = (pixelNum * 12 + 7) / 8;
                return length;
            }
            if (isYUV()) {
                switch (format) {
                    case FOURCC_YUY2:
                    case FOURCC_YVYU:
                    case FOURCC_I422:
                    case FOURCC_YV16:
                        length = pixelNum * 2;
                        return length;
                    case FOURCC_I420:
                    case FOURCC_YV12:
                    case FOURCC_NV12:
                    case FOURCC_NV21:
                        length = pixelNum * 3 / 2;
                        return length;
                    default:
                        length = pixelNum;
                        return length;
                }
            }

            int pixelLen = 0;
            if (bit <= 8) pixelLen = 1;
            else if (bit <= 16) pixelLen = 2;
            else if (bit <= 31) pixelLen = 4;

            switch (special_pixel) {
                case PIXEL_VALUE_F32: pixelLen = 4; break;
                case PIXEL_VALUE_F64: pixelLen = 8; break;
                default: break;
            }

            length = pixelNum * pixelLen * channel();
            return length;
        }

        /// 是否为单色（不含 Bayer/YUV）。
        bool isMono() const {
            return format >= MONO && format <= MONO_OVERSIZE;
        }

        /// 在给定色彩参数下，最终显示是否为单色。
        bool isMono(const ImageColorOpe& colorPara) const {
            return isMono()
                || (isBayer() && colorPara.bayerFunc == DEMOSAIC_NONE)
                || (isYUV() && colorPara.YUVFunc == YUVTRANS_Y);
        }

        bool isRGB() const { return format == RGB || format == BGR; }
        bool isRGBA() const { return format == RGBA; }
        bool isBayer() const { return format >= BAYERRG && format <= BAYERGB; }
        bool isYUV() const { return format >= FOURCC_YUY2 && format <= FOURCC_NV21; }

        bool isYUV_Packed() const {
            return format >= FOURCC_YUY2 && format <= FOURCC_YVYU;
        }
        bool isYUV_Planar() const {
            return format >= FOURCC_I422 && format <= FOURCC_YV12;
        }
        bool isYUV_SemiPlanar() const {
            return format >= FOURCC_NV12 && format <= FOURCC_NV21;
        }
    };

    /// 二维整数坐标。
    typedef struct _Pos {
        int32_t x = 0;
        int32_t y = 0;
    } Pos;

    /// RGB 像素（32 位整型）。
    typedef struct _RGBPixel {
        uint32_t r = 0;
        uint32_t g = 0;
        uint32_t b = 0;
    } RgbPixel;

    /// RGBA 像素（32 位整型）。
    typedef struct _RGBAPixel {
        uint32_t r = 0;
        uint32_t g = 0;
        uint32_t b = 0;
        uint32_t a = 0;
    } RgbaPixel;

    /// RGB 像素（单精度浮点）。
    typedef struct _RGBPixelF {
        float r = 0;
        float g = 0;
        float b = 0;
    } RgbPixelF;

    /// RGBA 像素（单精度浮点）。
    typedef struct _RGBAPixelF {
        float r = 0;
        float g = 0;
        float b = 0;
        float a = 0;
    } RgbaPixelF;
#pragma pack(pop)

    /**
     * @brief 安全读取指定坐标的像素值（带边界检查）。
     * @return 越界返回 0；多通道图像返回第一个通道。
     */
    static inline uint32_t safeAt(const uint8_t* data, int8_t bit, int32_t w, int32_t h,
        int32_t x, int32_t y) {
        if (x < 0 || x >= w || y < 0 || y >= h)
            return 0;
        if (bit <= 8)
            return ((const uint8_t*)data)[y * w + x];
        if (bit <= 16)
            return ((const uint16_t*)data)[y * w + x];
        if (bit <= 31)
            return ((const uint32_t*)data)[y * w + x];
        return 0;
    }
}

