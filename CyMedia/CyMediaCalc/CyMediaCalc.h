#pragma once
#include "../CyMediaBaseDef.h"

#include <Vector>

using namespace CyMedia;
namespace CyMediaCalc {
    /**
     * @brief 计算图像中指定坐标处的像素颜色（RGB 或灰度值）。
     * @details 根据图像格式（单色、Bayer、RGB、YUV 等）自动选择对应的计算方法，
     *          并返回归一化或原始数值（浮点型）的 RGB 分量。坐标越界时不做任何操作。
     * @param info       图像信息结构（包含宽、高、格式、位深等）。
     * @param pdata      图像数据指针（8 位字节流）。
     * @param x          像素 X 坐标（列索引，从 0 开始）。
     * @param y          像素 Y 坐标（行索引，从 0 开始）。
     * @param colorR     输出红色通道值（可为 nullptr）。
     * @param colorG     输出绿色通道值（可为 nullptr）。
     * @param colorB     输出蓝色通道值（可为 nullptr）。
     * @param formatope  色彩处理参数（指定去马赛克或 YUV 转换方法等）。
     */
    void CYMEDIA_LIB calcCoordinateColor(CyMedia::ImageShowInfo& info, uint8_t* pdata, int x, int y, double* colorR, double* colorG, double* colorB, CyMedia::ImageColorOpe formatope);

    /**
     * @brief 计算直方图的 Y 轴最大显示值（用于绘图自适应）。
     * @details 通过去掉前 `max_clipped_bins` 个最大柱，并依据离群因子判断是否截断，
     *          返回合适的 Y 轴上限，避免异常高峰导致图像不可读。
     * @param his              输入的直方图向量（柱状值）。
     * @param max_clipped_bins 允许忽略的最大柱数量（默认 5）。
     * @param outlier_factor   离群判定因子（默认 1.2），若第 (max_clipped_bins+1) 大值 * 因子 < 最大值，则截断。
     * @return 建议的 Y 轴最大值（double）。
     */
    double CYMEDIA_LIB determineYAxisMax(std::vector<double>& his,
        int max_clipped_bins = 5,
        double outlier_factor = 1.2);

    /**
     * @brief 计算单色（灰度）图像的直方图及统计信息。
     * @param pData         图像数据指针。
     * @param imageinfo     图像信息（必须为 Mono 格式）。
     * @param calcMask      可选掩码图像（与图像同尺寸，非零像素参与计算）。
     * @param useMask       是否启用掩码。
     * @param histogramVec  输出直方图向量（大小为 2^bit 或 256 等，取决于位深）。
     * @param maxPixel      输出最大像素值（可为 nullptr）。
     * @param minPixel      输出最小像素值（可为 nullptr）。
     * @param avePixel      输出平均像素值（可为 nullptr）。
     * @return 成功返回 true，否则 false（如格式不是单色）。
     */
    bool CYMEDIA_LIB computeGrayHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& histogramVec,
        double* maxPixel, double* minPixel, double* avePixel);

    /**
     * @brief 计算 RGB 图像的三通道直方图及统计信息。
     * @param pData         图像数据指针（必须是 RGB 格式）。
     * @param imageinfo     图像信息。
     * @param calcMask      可选掩码。
     * @param useMask       是否启用掩码。
     * @param Rhistogram    输出红色通道直方图。
     * @param Ghistogram    输出绿色通道直方图。
     * @param Bhistogram    输出蓝色通道直方图。
     * @param maxPixel      输出各通道最大值（向量大小为 3）。
     * @param minPixel      输出各通道最小值（向量大小为 3）。
     * @param avePixel      输出各通道平均值（向量大小为 3）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB computeRGBHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel);

    /**
     * @brief 计算 RGB 图像经拉伸变换后的单通道直方图（用于自动对比度）。
     * @param pData       图像数据（RGB 格式）。
     * @param imageinfo   图像信息。
     * @param histogram   输出拉伸后的直方图（单通道，值域取决于 `strytchType`）。
     * @param strytchType 拉伸类型（灰度、HSV 亮度、Lab 亮度等），默认 `stretch_Gray`。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB computeRGBHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo,
        std::vector<double>& histogram,
        StretchType strytchType = stretch_Gray);

    /**
     * @brief 计算 Bayer 格式图像经去马赛克后的三通道直方图及统计信息。
     * @param pData         原始 Bayer 数据。
     * @param imageinfo     图像信息（须为 Bayer 格式）。
     * @param calcMask      可选掩码。
     * @param useMask       是否启用掩码。
     * @param Rhistogram    输出红色通道直方图。
     * @param Ghistogram    输出绿色通道直方图。
     * @param Bhistogram    输出蓝色通道直方图。
     * @param maxPixel      输出各通道最大值。
     * @param minPixel      输出各通道最小值。
     * @param avePixel      输出各通道平均值。
     * @param func          去马赛克算法（默认双线性）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB computeBayerHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel,
        DemosaicingMethod func = DEMOSAIC_BILINEAR);

    /**
     * @brief 计算 Bayer 图像经去马赛克和拉伸后的单通道直方图。
     * @details 若 `func` 为 `DEMOSAIC_NONE`，则直接使用原始单通道数据（不进行去马赛克）。
     * @param pData       原始 Bayer 数据。
     * @param imageinfo   图像信息（须为 Bayer 格式）。
     * @param histogram   输出拉伸后的直方图。
     * @param type        拉伸类型（默认不拉伸）。
     * @param func        去马赛克算法（默认双线性）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB computeBayerHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo,
        std::vector<double>& histogram,
        StretchType type = stretch_None,
        DemosaicingMethod func = DEMOSAIC_BILINEAR);

    /**
     * @brief 计算 YUV 格式图像转换到 RGB 后的三通道直方图及统计信息。
     * @param pData         YUV 数据。
     * @param imageinfo     图像信息（须为 YUV 格式）。
     * @param calcMask      可选掩码。
     * @param useMask       是否启用掩码。
     * @param Rhistogram    输出红色通道直方图。
     * @param Ghistogram    输出绿色通道直方图。
     * @param Bhistogram    输出蓝色通道直方图。
     * @param maxPixel      输出各通道最大值。
     * @param minPixel      输出各通道最小值。
     * @param avePixel      输出各通道平均值。
     * @param func          YUV 转 RGB 方法（默认 BT.601）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB computerYUVHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel,
        YUVTransMethod func = BT601
    );

    /**
     * @brief 计算 YUV 格式图像转换到 RGB 后经拉伸的单通道直方图。
     * @param pData       YUV 数据。
     * @param imageinfo   图像信息（须为 YUV 格式）。
     * @param histogram   输出拉伸后的直方图。
     * @param type        拉伸类型（默认不拉伸）。
     * @param func        YUV 转 RGB 方法（默认 BT.601）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB computerYUVHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo,
        std::vector<double>& histogram,
        StretchType type = stretch_None,
        YUVTransMethod func = BT601
    );

    /**
     * @brief 根据灰度直方图计算自动拉伸的起始和结束灰度级。
     * @param histogram 输入的直方图向量。
     * @param start     输出拉伸起始灰度级（索引）。
     * @param end       输出拉伸结束灰度级（索引）。
     */
    void CYMEDIA_LIB computeGrayStretchPara(std::vector<double>& histogram, int32_t& start, int32_t& end);

    /**
     * @brief 计算单通道直方图的均匀度指标。
     * @param[in] histogram   输入直方图。
     * @param[in] ave         输出平均值（所有柱值的均值）。
     * @param[in] maxBitColor   对应bit的最大像素值。
     * @param[out] std         输出标准差（可为 nullptr）。
     * @param[out] uniformity  输出均匀度（可为 nullptr）。
     */
    void CYMEDIA_LIB computerUniformity(const std::vector<double>& histogram, const double& ave, double maxBitColor, double* std, double* uniformity, int* hisXRangeMax);
    /**
     * @brief 计算三个通道直方图的均匀度指标（向量化输出）。
     * @param[in] histogram_1  第一通道直方图（如 R 或 Gray）。
     * @param[in] histogram_2  第二通道直方图（如 G）。
     * @param[in] histogram_3  第三通道直方图（如 B）。
     * @param[in] ave          输出各通道平均值（向量大小需 >=3）。
     * @param[in] maxBitColor  对应bit的最大像素值。
     * @param[out] std          输出各通道标准差。
     * @param[out] uniformity   输出各通道均匀度。
     */
    void CYMEDIA_LIB computerThreeUniformity(
        const std::vector<double>& histogram_1, const std::vector<double>& histogram_2, const std::vector<double>& histogram_3,
        const std::vector<double>& ave, double maxBitColor,
        std::vector<double>& std, std::vector<double>& uniformity, std::vector<int>& hisXRangeMax);


    //==================== 图像转换 ====================
    /**
     * @brief 将 10/12 位打包的单色格式解包为 16 位数据（高位对齐）。
     * @details 支持 MONO10P、MONO10P_GVSP、MONO12P、MONO12P_GVSP 四种打包格式。
     *          输出数据为 16 位（uint16_t）数组，每个像素占 2 字节。
     * @param info     图像信息（必须为 MONO10P/12P 等打包格式）。
     * @param data     输入打包数据。
     * @param out_data 输出 16 位解包数据（调用者需分配足够内存：width*height*2 字节）。
     * @return 成功返回 true，否则 false（如格式不支持）。
     */
    bool CYMEDIA_LIB monoUnPack(const CyMedia::ImageShowInfo& info, const uint8_t* data, uint8_t* out_data);
    /**
     * @brief 将 10/12 位打包的单色格式解包并缩放为 8 位数据。
     * @details 类似于 `monoUnPack`，但输出为 8 位（uint8_t）数据，内部进行线性缩放至 [0,255]。
     * @param info     图像信息（打包格式）。
     * @param data     输入打包数据。
     * @param out_data 输出 8 位解包数据（调用者分配 width*height 字节）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB monoUnPack_8(const CyMedia::ImageShowInfo& info, const uint8_t* data, uint8_t* out_data);

    /**
     * @brief 将 Bayer 格式图像转换为 RGB (颜色位宽遵循原数据)图像。
     * @param info     图像信息（须为 Bayer 格式）。
     * @param data     输入 Bayer 数据。
     * @param out_data 输出 RGB 数据（调用者分配 info.length*3 字节）。
     * @param func     去马赛克算法（默认双线性）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB bayer2RGB(const CyMedia::ImageShowInfo& info, const uint8_t* data, uint8_t* out_data, DemosaicingMethod func = DEMOSAIC_BILINEAR);
    /**
     * @brief 将 Bayer 格式图像转换为 RGB 24 位（每通道 8 位）图像。
     * @param info     图像信息（须为 Bayer 格式）。
     * @param data     输入 Bayer 数据。
     * @param out_data 输出 RGB 数据（调用者分配 width*height*3 字节）。
     * @param func     去马赛克算法（默认双线性）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB bayer2RGB_8(const CyMedia::ImageShowInfo& info, const uint8_t* data, uint8_t* out_data, DemosaicingMethod func = DEMOSAIC_BILINEAR);

    /**
     * @brief 将 YUV 格式图像转换为 RGB 24 位图像。
     * @param info     图像信息（须为 YUV 格式）。
     * @param data     输入 YUV 数据。
     * @param out_data 输出 RGB 数据（width*height*3 字节）。
     * @param func     YUV 转 RGB 方法（默认 BT.601）。
     * @return 成功返回 true，否则 false。
     */
    bool CYMEDIA_LIB YUV2RGB(const CyMedia::ImageShowInfo& info, const uint8_t* data, uint8_t* out_data, YUVTransMethod func = BT601);

    /**
     * @brief 将源图像数据按行列对齐复制到目标缓冲区。
     * @details 用于将宽度/高度可能小于对齐尺寸的图像拷贝到对齐后的内存块中，
     *          剩余区域（右侧和下方）不处理。
     * @param pSrc        源图像数据指针。
     * @param pAlign      目标对齐缓冲区指针。
     * @param srcWidth    源图像宽度（像素）。
     * @param srcHeight   源图像高度（像素）。
     * @param AlignWidth  目标对齐宽度（像素）。
     * @param AlignHeight 目标对齐高度（像素）。
     * @param pixelSize   每个像素的字节数。
     */
    void CYMEDIA_LIB copyAlignImage(void* pSrc, void* pAlign, int srcWidth, int srcHeight, int AlignWidth, int AlignHeight, int pixelSize);


    //==================== 色彩空间转换（像素级） ====================
    
    /**
     * @brief 将 sRGB 像素转换为线性 RGB（去 Gamma 校正）。
     * @param srgb 输入 sRGB 像素（各分量归一化到 [0,1]）。
     * @return 线性 RGB 像素。
     */
    RgbPixelF srgb2linear(RgbPixelF& srgb);

    /**
     * @brief 将线性 RGB 像素转换为 sRGB（应用 Gamma 校正）。
     * @param linear 输入线性 RGB 像素。
     * @return sRGB 像素。
     */
    RgbPixelF linear2srgb(RgbPixelF& linear);

    /**
     * @brief 将 RGB 像素转换为灰度值（RgbPixel 三个通道均设为灰度值）。
     * @param pixel   输入 RGB 像素（0~bitMax-1）。
     * @param bitMax  通道最大值（通常为 2^bit - 1）。
     * @return 灰度像素（RGB 相等）。
     */
    RgbPixel RGBPixel2rgb2Gray(RgbPixel pixel, uint32_t bitMax);
    /**
     * @brief 将 RGB 像素转换为 HSV 表示（返回的 RgbPixel 中 r=H, g=S, b=V，范围被量化为 [0, bitMax-1]）。
     * @param pixel   输入 RGB 像素。
     * @param bitMax  量化最大值。
     * @return 包含 HSV 分量的 RgbPixel 结构。
     */
    RgbPixel RGBPixel2rgb2hsv(RgbPixel pixel, uint32_t bitMax);
    /**
     * @brief 将 RGB 像素转换为 CIELAB 颜色空间（L, a, b 分量存储为 uint32_t）。
     * @param pixel   输入 RGB 像素。
     * @param bitMax  量化最大值（用于归一化 a, b 分量）。
     * @return Lab 像素（r=L, g=a, b=b，L 范围 0~100，a,b 被缩放到 0~bitMax-1）。
     */
    RgbPixel RGBPixel2LAB(RgbPixel pixel, uint32_t bitMax);
    /**
     * @brief 根据拉伸类型将 RGB 像素转换为相应的单通道值（并存储为 RgbPixel 三通道相同）。
     * @param pixel   输入 RGB 像素。
     * @param type    拉伸类型（灰度/HSV/Lab）。
     * @param bitMax  位最大值。
     * @return 转换后的像素（三通道相同，值为拉伸后的值）。
     */
    RgbPixel RGBPixel2Strtch(RgbPixel pixel, StretchType type, uint32_t bitMax);
    /**
     * @brief 根据拉伸类型将 RGB 像素转换为单个整数值（仅返回拉伸后的通道值）。
     * @param pixel   输入 RGB 像素。
     * @param type    拉伸类型。
     * @param bitMax  位最大值。
     * @return 拉伸后的单通道整数值（如灰度值、V 值或 L 值）。
     */
    int32_t RGBPixel2StrtchOne(RgbPixel pixel, StretchType type, uint32_t bitMax);

    //==================== 快速版本（供内部高性能调用，无安全判断和边界修复） ====================
    
    /**
     * @brief 快速灰度计算（整数加权）。
     * @param rgb 输入 RGB 像素。
     * @return 灰度值（0~max）。
     */
    uint32_t fastGray(const RgbPixel& rgb);
    /**
     * @brief 快速获取 HSV 的 V（亮度）分量。
     * @param rgb 输入 RGB 像素。
     * @return V = max(R,G,B)。
     */
    uint32_t fastHSV_V(const RgbPixel& rgb);
    /**
     * @brief 快速计算 CIELAB 的 L* 分量（近似）。
     * @param rgb    输入 RGB 像素。
     * @param bitMax 位最大值（用于归一化）。
     * @return L* 值（范围 0~100）。
     */
    uint32_t fastLab_L(const RgbPixel& rgb, uint32_t bitMax);
    /**
     * @brief 根据拉伸类型快速计算单通道拉伸值（高效无分支版本）。
     * @param rgb     输入 RGB 像素。
     * @param type    拉伸类型。
     * @param bitMax  位最大值。
     * @return 拉伸后的单通道值（0~bitMax）。
     */
    uint32_t RGBT2StretchOneFast(const RgbPixel& rgb, StretchType type, uint32_t bitMax);
    /**
     * @brief 将 CIELAB 像素转换回 sRGB 像素。
     * @param lab    Lab 像素（r=L, g=a, b=b，a,b 已量化为 0~maxV-1）。
     * @param maxV   量化最大值（用于还原 a,b 的归一化范围）。
     * @return 转换后的 sRGB 像素。
     */
    RgbPixel LABPixel2RGB(RgbPixel lab, uint32_t maxValue);
}
