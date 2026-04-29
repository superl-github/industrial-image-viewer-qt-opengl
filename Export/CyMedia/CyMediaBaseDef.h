#pragma once
#include <stdint.h>
#include <functional>
#include <string>

namespace CyMedia {
    using LogCallback = std::function<void(const std::string&, void*)>;

    /**
    * @brief    支持的显示语言
    */
    enum eLanguage {
        CHINESE,
        ENGLISH
    };

    /**
    * @brief    像素格式
    * @details  单个颜色通道最大16位
    */
    enum ePixType {
        MONO = 0,
        BAYERRG,
        BAYERGR,
        BAYERBG,
        BAYERGB,
        RGB,
        MONO10P,
        MONO10P_GVSP,
        MONO12P,
        MONO12P_GVSP,
        MONO_OVERSIZE,
    };

    /**
     * @brief 像素数据类型
     */
    enum eSpecialValueType {
        IMGVALUE_None,
        IMGVALUE_F32,
        IMGVALUE_F64,
    };

    /**
     * @brief   支持的存图格式
    */
    enum eSaveImageFormat {
        ImageFormat_BMP,
        ImageFormat_RAW
    };

    /**
     * @brief 图像运算类型
    */
    enum ImageMath {
        Image1AddImage2,
        Image1SubImage2,
        Image2SubImage1,
        Image1TimesImage2,
        Image1DivideImage2,
        Image2DivideImage1,
    };

    enum RGBChannel {
        Ch_R,
        Ch_G,
        Ch_B,
    };

    enum DemosaicMethod {
        BAYERSOUCE,
        BILINEAR,
        MALVAR,
        AHD,
    };

    enum StretchType {
        stretch_None = 0,
        stretch_Gray,
        stretch_HSV,
        stretch_Lab,
    };

#pragma pack(push,1)
    /**
     * @brief   单帧数据结构体
     */
    struct ImageShowInfo {
        int32_t width = 0;       ///< 图像一行的像素个数
        int32_t height = 0;      ///< 图像一列的像素个数
        int8_t  bit = 8;         ///< 单个颜色通道的位宽
        ePixType format = MONO;   ///< 图像格式
        eSpecialValueType special_value = IMGVALUE_None;
        uint32_t length = 0;  ///< 数据长度(字节)

        ImageShowInfo(int w, int h, int b, ePixType f, uint32_t l = 0, eSpecialValueType sv = IMGVALUE_None)
            : width(w), height(h), bit(static_cast<int8_t>(b)), format(f), special_value(sv) {
            if (l == 0) {
                upLenth();
            }
            else {
                length = l;
            }
        }
        ImageShowInfo() = default;

        uint8_t channel() const {
            switch (format) {
                case CyMedia::MONO:
                case CyMedia::BAYERRG:
                case CyMedia::BAYERGR:
                case CyMedia::BAYERBG:
                case CyMedia::BAYERGB:
                case CyMedia::MONO10P:
                case CyMedia::MONO10P_GVSP:
                case CyMedia::MONO12P:
                case CyMedia::MONO12P_GVSP:
                case CyMedia::MONO_OVERSIZE: {
                    return 1;
                }break;

                case CyMedia::RGB: {
                    return 3;
                }break;
            }
            return 1;
        }

        void upLenth() {
            int pixelNum = width * height;
            if (format == MONO10P) {
                 length = (pixelNum * 10 + 7) / 8;
                 return;
            }
            else if (format == MONO10P_GVSP) {
                length = (pixelNum * 10 + 7) / 8;
                return;
            }
            else if (format == MONO12P) {
                length = (pixelNum * 12 + 7) / 8;
                return;
            }
            else if (format == MONO12P_GVSP) {
                length = (pixelNum * 12 + 7) / 8;
                return;
            }

            int pixelLen = 0;
            if (bit <= 8) {
                pixelLen = 1;
            }
            else if (bit <= 16) {
                pixelLen = 2;
            }
            else if (bit <= 31) {
                pixelLen = 4;
            }

            switch (special_value) {
                case CyMedia::IMGVALUE_None: {
                    ;
                }break;

                case CyMedia::IMGVALUE_F32: {
                    pixelLen = 4;
                }break;

                case CyMedia::IMGVALUE_F64: {
                    pixelLen = 8;
                }break;
            }

            length = pixelNum * pixelLen * channel();
        }

        bool isMono() const{
            return format == MONO ||
                format == MONO10P ||
                format == MONO10P_GVSP ||
                format == MONO12P ||
                format == MONO12P_GVSP ||
                format == MONO_OVERSIZE;
        }
        bool isBayer() const {
            return format == BAYERRG ||
                format == BAYERGR ||
                format == BAYERBG ||
                format == BAYERGB;
        }
        bool isRGB() const {
            return format == RGB;
        }

    };

    typedef struct _RawImageHeadInfo {
        ImageShowInfo frameInfo;
        float fps;
        int frameCount;
        int64_t size;
    }RawImageHeadInfo;

    typedef struct _Pos {
        int32_t x = 0;
        int32_t y = 0;
    }Pos;

    typedef struct _RGBPixel {
        int32_t r = 0;
        int32_t g = 0;
        int32_t b = 0;
    }RgbPixel;

    typedef struct _RGBAPixel {
        int32_t r = 0;
        int32_t g = 0;
        int32_t b = 0;
        int32_t a = 0;
    }RgbaPixel;

    typedef struct _RGBPixelF {
        float r = 0;
        float g = 0;
        float b = 0;
    }RgbPixelF;

    typedef struct _RGBAPixelF {
        float r = 0;
        float g = 0;
        float b = 0;
        float a = 0;
    }RgbaPixelF;

    
#pragma pack(pop)

    static inline int32_t safeAt(const uint8_t* data, int8_t bit, int32_t w, int32_t h, int32_t x, int32_t y) {
        if (x < 0 || x >= w || y < 0 || y >= h)
            return 0;
        if (bit <= 8)
            return ((uint8_t*)data)[y * w + x];
        else if (bit <= 16)
            return ((uint8_t*)data)[y * w + x];
        else if (bit <= 31)
            return ((uint32_t*)data)[y * w + x];
        return 0;
    }
}