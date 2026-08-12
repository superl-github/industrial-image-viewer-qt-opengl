/**
 * @file CyMediaBaseDef.h
 * @brief CyMedia 库的核心基础类型、枚举、结构体及全局定义。
 * @details 本文件定义了整个库所依赖的基础数据类型，包括像素格式、图像信息结构、
 *          色彩空间转换常量以及通用回调函数类型。所有上层模块（如 CyMediaDis）
 *          均依赖此文件中的定义。
 * @author LLF
 * @version 1.0
 */

#pragma once
#include <stdint.h>
#include <functional>
#include <string>
#include <filesystem>

 //==================== 库导出/导入宏 ====================
# if defined(CYMEDIA_EXPORT)
#  define CYMEDIA_LIB __declspec(dllexport)
# else
#  define CYMEDIA_LIB __declspec(dllimport)
# endif

/**
 * @brief namespace CyMedia.
 * @details 定义了整个库所依赖的基础数据类型，包括像素格式、图像信息结构、
 *          色彩空间转换常量以及通用回调函数类型。所有上层模块（如 CyMediaDis）
 *          均依赖此命名空间中的定义。
 */
namespace CyMedia {
    //================ 版本号 ====================
    const char VERSION[] = "V 1.2.3.preview";

    //==================== 色彩空间转换常量 ====================
    /**
     * @brief sRGB 非线性转换阈值 (0.04045)。
     * @details 用于将 sRGB 转换为线性 RGB 时的分段函数阈值。
     */
    const float THRESHOLD_SRGB = 0.04045f;
    /**
     * @brief 线性 RGB 转换阈值 (0.0031308)。
     * @details 用于将线性 RGB 转换为 sRGB 时的分段函数阈值。
     */
    const float THRESHOLD_LINEAR = 0.0031308f;
    /**
     * @brief 颜色空间转换函数 f(t) 阈值 (0.008856)。
     * @details 用于 CIELAB 等颜色空间转换中的阈值判定。
     */
    const float THRESHOLD_F = 0.008856f;
    /**
     * @brief 逆 f(t) 函数阈值 (6.0/29.0)。
     * @details 用于 CIELAB 等颜色空间逆转换中的阈值判定。
     */
    const float THRESHOLD_F_INV = 6.0f / 29.0f;

    /**
     * @defgroup CyMediaBaseTypes CyMedia 基础类型定义
     * @brief 包含所有核心数据结构、枚举和类型别名。
     * @{
     */

    //==================== 公共枚举定义 ====================
    /**
     * @brief 支持的语言类型。
     */
    enum eLanguage {
        CHINESE, ///< 简体中文
        ENGLISH  ///< 英语
    };

    enum LogLevel {
        TRACE = 0,
        DEBUG,
        INFO,
        WAR,
        ERR,
        CRITICAL,
        OFF,
    };

    /**
     * @brief 像素格式（Pixel Format）。
     * @details 定义所有支持的图像数据排列格式。
     *          - 单色（MONO）及打包格式（MONO10P/12P）遵循 GenICam 标准。
     *          - Bayer 格式支持 RGGB/GRBG/BGGR/GBRG 四种排列。
     *          - YUV 格式涵盖 Packed (YUYV, YVYU)、Planar (I420, YV12) 和 Semi-Planar (NV12, NV21)。
     *          @note 每个颜色通道最大支持 31 位深度。
     */
    enum ePixType {
        MONO = 0,       ///< 单色（灰度）

        MONO10P,        ///< 10位单色打包格式（5字节存储4像素）
        MONO10P_GVSP,   ///< 10位单色 GVSP 打包格式（兼容 GigE Vision）
        MONO12P,        ///< 12位单色打包格式（3字节存储2像素）
        MONO12P_GVSP,   ///< 12位单色 GVSP 打包格式

        MONO_OVERSIZE,  ///< 单色扩展标记（内部使用，非实际格式）

        RGB,            ///< RGB 彩色（顺序 R-G-B）
        RGBA,           ///< RGBA 彩色（带 Alpha 通道）

        BAYERRG,        ///< Bayer 阵列 RGGB 排列
        BAYERGR,        ///< Bayer 阵列 GRBG 排列
        BAYERBG,        ///< Bayer 阵列 BGGR 排列
        BAYERGB,        ///< Bayer 阵列 GBRG 排列

        //==================== YUV FourCC 格式 ====================
        FOURCC_YUY2,       ///< YUYV 4:2:2 打包格式 (Y0 U Y1 V)
        FOURCC_YVYU,       ///< YVYU 4:2:2 打包格式 (Y0 V Y1 U)
        FOURCC_I422,       ///< YUV 4:2:2 Planar 格式 (Y, U, V 三个平面)
        FOURCC_YV16,       ///< YVU 4:2:2 Planar 格式 (Y, V, U 三个平面)
        FOURCC_I420,       ///< YUV 4:2:0 Planar 格式 (Y, U, V 三个平面)
        FOURCC_YV12,       ///< YVU 4:2:0 Planar 格式 (Y, V, U 三个平面)
        FOURCC_NV12,       ///< YUV 4:2:0 Semi-Planar 格式 (Y 平面 + UV 交错平面)
        FOURCC_NV21,       ///< YVU 4:2:0 Semi-Planar 格式 (Y 平面 + VU 交错平面)
    };

    /**
     * @brief 像素值数据类型。
     * @details 用于区分图像数据是整型还是浮点型，影响像素值的解释方式。
     */
    enum ePixelValueType {
        PIXEL_VALUE_INT = 0,    ///< 整型数据（默认）
        PIXEL_VALUE_F32,        ///< 32位单精度浮点数据
        PIXEL_VALUE_F64,        ///< 64位双精度浮点数据
    };

    /**
     * @brief 支持的图像文件保存格式。
     */
    enum eSaveImageFormat {
        ImageFormat_BMP,    ///< Windows Bitmap 格式
        ImageFormat_RAW     ///< 原始数据格式（无压缩、包含头信息）
    };

    /**
     * @brief 图像数学运算类型（两图像间逐像素运算）。
     */
    enum ImageMath {
        Image1AddImage2,     ///< 图像1 + 图像2
        Image1SubImage2,     ///< 图像1 - 图像2
        Image2SubImage1,     ///< 图像2 - 图像1
        Image1TimesImage2,   ///< 图像1 * 图像2
        Image1DivideImage2,  ///< 图像1 / 图像2
        Image2DivideImage1,  ///< 图像2 / 图像1
    };

    /**
     * @brief RGBA 颜色通道索引。
     */
    enum RGBChannel {
        Ch_R, ///< 红色通道
        Ch_G, ///< 绿色通道
        Ch_B, ///< 蓝色通道
        Ch_A, ///< 透明通道
    };

    /**
     * @brief Bayer 去马赛克（Demosaicing）算法枚举。
     */
    enum DemosaicingMethod {
        DEMOSAIC_NONE,      ///< 不进行去马赛克（直接输出 Bayer 原始数据，视为单色）
        DEMOSAIC_BILINEAR,  ///< 双线性插值（速度最快，细节较模糊）
        DEMOSAIC_MALVA,     ///< Malvar 自适应插值（质量与速度均衡）
        DEMOSAIC_AHD,       ///< 自适应同质定向插值（质量最高，计算量大）
    };

    /**
     * @brief YUV 转 RGB 的转换方法。
     */
    enum YUVTransMethod {
        YUVTRANS_Y = 0,///< 仅使用 Y 通道（亮度），忽略 UV（输出灰度）
        BT601, ///< 标准 YUV 转 RGB（BT.601）      
    };

    /**
     * @brief 灰度拉伸（对比度增强）类型。
     */
    enum StretchType {
        stretch_None = 0, ///< 不进行拉伸
        stretch_Gray,     ///< 直接对灰度值进行拉伸（可能引起色偏）
        stretch_HSV,      ///< 在 HSV 空间的 V 通道进行拉伸（保持色调）
        stretch_Lab,      ///< 在 Lab 空间的 L* 通道进行拉伸（更符合人眼感知）
    };

    /**
     * @brief 图像文件后缀类型。
     */
    enum ImageSuffix {
        IMAGE_SUFFIX_RAW = 0,  ///< .raw
        IMAGE_SUFFIX_BMP,      ///< .bmp
        IMAGE_SUFFIX_TIFF,     ///< .tiff/.tif
        IMAGE_SUFFIX_PNG,      ///< .png
        IMAGE_SUFFIX_JPEG,     ///< .jpg/.jpeg
        IMAGE_SUFFIX_INVALID   ///< 无效或未识别的格式
    };

    /**
     * @brief 视频文件后缀类型。
     */
    enum VideoSuffix {
        VIDEO_SUFFIX_RAW = 0,  ///< .raw
        VIDEO_SUFFIX_INVALID   ///< 无效或未识别的格式
    };

    //==================== 核心数据结构 ====================
#pragma pack(push,1)
    /**
     * @brief 图像色彩处理参数集合。
     * @details 用于统一传递 Bayer 去马赛克、YUV 转换和拉伸策略。
     */
    struct ImageColorOpe {
        CyMedia::DemosaicingMethod bayerFunc;
        CyMedia::YUVTransMethod YUVFunc;
        StretchType stretchType;
        int stretch_S = 0;
        int stretch_E = 0;
    };

    /**
     * @brief 单帧图像信息结构体（元数据 + 长度）。
     * @details 描述图像的所有属性，并提供了便捷的方法来计算通道数、行字节数以及数据总长度。
     *          对于打包格式（MONO10P/12P），`upLenth()` 会按位计算实际字节数。
     */
    struct ImageShowInfo {
        int32_t width = 0;              ///< 图像宽度（像素列数）
        int32_t height = 0;             ///< 图像高度（像素行数）
        int8_t  bit = 8;                ///< 单个颜色通道的位深度（1~31）
        ePixType format = MONO;         ///< 像素格式（见 ePixType）
        ePixelValueType special_pixel = PIXEL_VALUE_INT; ///< 像素值数据类型（整型/浮点）
        uint32_t length = 0;            ///< 图像数据总长度（字节数）

        /**
         * @brief 带参数的构造函数。
         * @param w 宽度。
         * @param h 高度。
         * @param b 位深度。
         * @param f 像素格式。
         * @param l 数据长度（若为 0，则自动通过 upLenth() 计算）。
         * @param sv 像素值数据类型。
         */
        ImageShowInfo(int w, int h, int b, ePixType f, uint32_t l = 0, ePixelValueType sv = PIXEL_VALUE_INT)
            : width(w), height(h), bit(static_cast<int8_t>(b)), format(f), special_pixel(sv) {
            if (l == 0) {
                upLenth();
            }
            else {
                length = l;
            }
        }
        /** @brief 默认构造函数。 */
        ImageShowInfo() = default;

        /**
         * @brief 计算颜色通道数。
         * @details 根据 `format` 返回实际的通道数量。
         *          - 单色/Bayer/MONO打包：返回 1。
         *          - RGB：返回 3。
         *          - RGBA：返回 3（此处忽略 Alpha 通道仅返回 RGB 通道数，符合常规图像处理习惯）。
         * @return 通道数（1/3/4）。
         */
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
					return 4;
				}break;
            }
            return 1;
        }

        /**
         * @brief 计算图像每行的字节数（行步长）。
         * @details 考虑了像素格式、位深度、特殊像素类型（浮点）以及通道数。
         *          对于 10/12 位打包格式，按位计算行字节数。
         * @return 每行字节数。
         */
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

        /**
         * @brief 根据当前参数（宽、高、位深、格式）计算并更新 `length` 字段。
         * @details 特别注意 10/12 位打包格式的按位计算逻辑。
         * @return 返回计算后的长度
         */
        uint32_t upLenth() {
            int pixelNum = width * height;
            if (format == MONO10P) {
                 length = (pixelNum * 10 + 7) / 8;
                 return length;
            }
            else if (format == MONO10P_GVSP) {
                length = (pixelNum * 10 + 7) / 8;
                return length;
            }
            else if (format == MONO12P) {
                length = (pixelNum * 12 + 7) / 8;
                return length;
            }
            else if (format == MONO12P_GVSP) {
                length = (pixelNum * 12 + 7) / 8;
                return length;
            }

            int pixelLen = 0;
            if (bit <= 8) {
                pixelLen = 1;
                if (isYUV()) {
                    pixelLen = 2;
                }
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
            return length;
        }

        //==================== 格式判定辅助函数 ====================

        /** @brief 判断是否为单色（Mono）格式（不含 Bayer/YUV）。 */
        bool isMono() const{
            return format >= MONO && format <= MONO_OVERSIZE;
        }
        /**
         * @brief 在特定色彩处理参数下判断最终显示结果是否为单色。
         * @param colorPara 色彩处理参数（含去马赛克和 YUV 转换策略）。
         * @return 若为单色返回 true，否则 false。
         */
        bool isMono(const CyMedia::ImageColorOpe& colorPara) const {
            return isMono() ||
                (isBayer() && colorPara.bayerFunc == DEMOSAIC_NONE) ||
                (isYUV() && colorPara.YUVFunc == YUVTRANS_Y);
        }
        /** @brief 判断是否为 RGB 格式。 */
        bool isRGB() const {
            return format == RGB;
        }
        /** @brief 判断是否为 RGBA 格式。 */
        bool isRGBA() const {
            return format == RGBA;
        }
        /** @brief 判断是否为 Bayer 格式。 */
        bool isBayer() const {
            return format >= BAYERRG && format <= BAYERGB;
        }
        /** @brief 判断是否为 YUV 格式（涵盖 Packed/Planar/Semi-Planar）。 */
        bool isYUV() const {
            return format >= FOURCC_YUY2 && format <= FOURCC_NV21;
        }
        /** @brief 判断是否为 YUV Packed（打包）格式（如 YUYV, YVYU）。 */
        bool isYUV_Packed() const {
            return format >= FOURCC_YUY2 && format <= FOURCC_YVYU;
        }
        /** @brief 判断是否为 YUV Planar（三平面）格式（如 I420, YV12）。 */
        bool isYUV_Planar() const {
            return format >= FOURCC_I422 && format <= FOURCC_YV12;
        }
        /** @brief 判断是否为 YUV Semi-Planar（半平面）格式（如 NV12, NV21）。 */
        bool isYUV_SemiPlanar() const {
            return format >= FOURCC_NV12 && format <= FOURCC_NV21;
        }
    };

    /**
    * @brief 视频文件头信息。
    * @details 用于接收视频文件分析的信息。
    */
    typedef struct _VideoParseInfo {
        VideoSuffix videoType;   ///< 视频文件类型
        ImageShowInfo frameInfo; ///< 帧图像信息
        uint32_t dataOffset;     ///< 视频数据偏移
        float fps;               ///< 帧率（帧/秒）
        int frameCount;          ///< 总帧数
        int64_t size;            ///< 文件总大小（字节）
    }VideoParseInfo;

    /**
     * @brief 二维坐标点（整数）。
     */
    typedef struct _Pos {
        int32_t x = 0;
        int32_t y = 0;
    }Pos;

    /**
     * @brief RGB 像素（32位整型，各通道范围 0~2^32-1）。
     */
    typedef struct _RGBPixel {
        uint32_t r = 0;
        uint32_t g = 0;
        uint32_t b = 0;
    }RgbPixel;

    /**
     * @brief RGBA 像素（32位整型，含 Alpha 通道）。
     */
    typedef struct _RGBAPixel {
        uint32_t r = 0;
        uint32_t g = 0;
        uint32_t b = 0;
        uint32_t a = 0;
    }RgbaPixel;

    /**
     * @brief RGB 像素（单精度浮点，范围通常 0.0~1.0）。
     */
    typedef struct _RGBPixelF {
        float r = 0;
        float g = 0;
        float b = 0;
    }RgbPixelF;

    /**
     * @brief RGBA 像素（单精度浮点，含 Alpha）。
     */
    typedef struct _RGBAPixelF {
        float r = 0;
        float g = 0;
        float b = 0;
        float a = 0;
    }RgbaPixelF;
    /** @} */
    
#pragma pack(pop)
    //==================== 回调函数类型 ====================

    /**
     * @brief 日志回调函数类型。
     * @param msg 日志消息内容。
     * @param pUser 用户自定义指针，用于传递上下文。
     */
    using LogCallback = std::function<void(CyMedia::LogLevel, const std::string&, void*)>;

    /**
     * @brief 视频帧数据回调函数类型。
     * @details 用于异步播放模式，每解析到一帧图像时触发。
     *          回调函数内禁止执行耗时操作，以免阻塞推流线程。
     * @param info 当前帧的图像属性（CyMedia::ImageShowInfo）。
     * @param data 指向帧图像数据的只读指针（生命周期由解析器管理）。
     * @param userData 用户注册时传入的自定义指针。
     */
    typedef void (*FrameCallback)(const CyMedia::ImageShowInfo& info, const uint8_t* p_data, int nCount, void* pUser);

    //==================== 工具函数 ====================

    /**
     * @brief 安全地获取图像指定坐标处的像素值（带边界检查）。
     * @details 根据位深度自动将数据解释为 uint8_t*, uint16_t* 或 uint32_t*。
     *          若坐标越界，返回 0。
     * @param data 图像数据起始指针。
     * @param bit 位深度（1~31）。
     * @param w 图像宽度。
     * @param h 图像高度。
     * @param x 目标 X 坐标（0 起始）。
     * @param y 目标 Y 坐标（0 起始）。
     * @return 像素值（若为多通道，仅返回第一个通道的值）。
     */
    static inline uint32_t safeAt(const uint8_t* data, int8_t bit, int32_t w, int32_t h, int32_t x, int32_t y) {
        if (x < 0 || x >= w || y < 0 || y >= h)
            return 0;
        if (bit <= 8)
            return ((uint8_t*)data)[y * w + x];
        else if (bit <= 16)
            return ((uint16_t*)data)[y * w + x];
        else if (bit <= 31)
            return ((uint32_t*)data)[y * w + x];
        return 0;
    }
}