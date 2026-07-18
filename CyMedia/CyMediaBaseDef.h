#pragma once
#include <stdint.h>
#include <functional>
#include <string>

# if defined(CYMEDIA_EXPORT)
#  define CYMEDIA_LIB __declspec(dllexport)
# else
#  define CYMEDIA_LIB __declspec(dllimport)
# endif

namespace CyMedia {
    using LogCallback = std::function<void(const std::string&, void*)>;

    /**
    * @brief    Supported Languages
    */
    enum eLanguage {
        CHINESE,
        ENGLISH
    };

    /**
    * @brief    Pixel Format
    * @details  Maximum 31 bits per color channel.
    */
    enum ePixType {
        MONO = 0,
        BAYERRG,
        BAYERGR,
        BAYERBG,
        BAYERGB,
        RGB,
        RGBA,
        MONO10P,
        MONO10P_GVSP,
        MONO12P,
        MONO12P_GVSP,
        MONO_OVERSIZE,
    };

    /**
     * @brief Pixel Data Type
     */
    enum ePixelValueType {
        PIXEL_VALUE_INT = 0,
        PIXEL_VALUE_F32,
        PIXEL_VALUE_F64,
    };

    /**
     * @brief   Supported Image Formats
    */
    enum eSaveImageFormat {
        ImageFormat_BMP,
        ImageFormat_RAW
    };

    /**
     * @brief Image Operation Types
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

    enum DemosaicingMethod {
        DEMOSAIC_NONE,
        DEMOSAIC_BILINEAR,
        DEMOSAIC_MALVA,
        DEMOSAIC_AHD,
    };

    enum StretchType {
        stretch_None = 0,
        stretch_Gray,
        stretch_HSV,
        stretch_Lab,
    };

    enum ImageSuffix {
        IMAGE_SUFFIX_RAW = 0,
        IMAGE_SUFFIX_BMP,
        IMAGE_SUFFIX_TIFF,
        IMAGE_SUFFIX_PNG,
        IMAGE_SUFFIX_JPEG,
        IMAGE_SUFFIX_INVALID
    };

    enum VideoSuffix {
        VIDEO_SUFFIX_RAW = 0,
        VIDEO_SUFFIX_INVALID,
    };

#pragma pack(push,1)
    /**
     * @brief   Single-Frame Data Structure
     */
    struct ImageShowInfo {
        int32_t width = 0;       ///< The number of pixels in a single row of the image
        int32_t height = 0;      ///< The number of pixels in a column of the image
        int8_t  bit = 8;         ///< Bit depth of a single color channel
        ePixType format = MONO;   ///< Image Format
        ePixelValueType special_pixel = PIXEL_VALUE_INT;
        uint32_t length = 0;  ///< Data Length (Bytes)

        ImageShowInfo(int w, int h, int b, ePixType f, uint32_t l = 0, ePixelValueType sv = PIXEL_VALUE_INT)
            : width(w), height(h), bit(static_cast<int8_t>(b)), format(f), special_pixel(sv) {
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

				case CyMedia::RGBA: {
					return 3;
				}break;
            }
            return 1;
        }

		int bytesPerLine() {
			int bytesNumber = width;
			if (format == MONO10P) {
				return (bytesNumber * 10 + 7) / 8;;
			}
			else if (format == MONO10P_GVSP) {
				return (bytesNumber * 10 + 7) / 8;
			}
			else if (format == MONO12P) {
				return (bytesNumber * 12 + 7) / 8;
			}
			else if (format == MONO12P_GVSP) {
				return (bytesNumber * 12 + 7) / 8;
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
			switch (special_pixel) {
			    case CyMedia::PIXEL_VALUE_F32: {
				    pixelLen = 4;
			    }break;

			    case CyMedia::PIXEL_VALUE_F64: {
				    pixelLen = 8;
			    }break;
			}

			return bytesNumber * pixelLen * channel();
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

            switch (special_pixel) {
                case CyMedia::PIXEL_VALUE_F32: {
                    pixelLen = 4;
                }break;

                case CyMedia::PIXEL_VALUE_F64: {
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
        bool isRGBA() const {
            return format == RGBA;
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