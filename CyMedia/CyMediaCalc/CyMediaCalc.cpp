#include "CyMediaCalc.h"
#include "CyMdiaCalc_Mono.h"
#include "CyMdiaCalc_Bayer.h"
#include "CyMdiaCalc_Rgb.h"

#include <cstdlib>

#if defined _MSC_VER
#include <ppl.h>
#include <thread>
#endif

namespace CyMedia {
    void calcCoordinateColor(ImageShowInfo& info, uint8_t* pdata, int x, int y, double* colorR, double* colorG, double* colorB, DemosaicingMethod func/* = BILINEAR*/) {
        unsigned int imageW = info.width;
        unsigned int imageH = info.height;
        if (x < 0 || y < 0 || x >= imageW || y >= imageH) {
            return;
        }
        double r = 0.0, g = 0.0, b = 0.0;
        //计算
        if (info.special_pixel == PIXEL_VALUE_F32) {
            r = ((float*)pdata)[y * info.width + x];
            g = r;
            b = r;
        }
        else if (info.special_pixel == PIXEL_VALUE_F64) {
            r = ((double*)pdata)[y * info.width + x];
            g = r;
            b = r;
        }
        else {
            switch (info.format) {
                case CyMedia::MONO: 
                case CyMedia::MONO_OVERSIZE:{
                    r = calcCoordinateColor_Mono(info, pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::MONO10P: {
                    r = calcCoordinateColor_Mono10P(pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::MONO10P_GVSP: {
                    r = calcCoordinateColor_Mono10P_GVSP(pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::MONO12P: {
                    r = calcCoordinateColor_Mono12P(pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::MONO12P_GVSP: {
                    r = calcCoordinateColor_Mono12P_GVSP(pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::BAYERRG:
                case CyMedia::BAYERGR:
                case CyMedia::BAYERBG:
                case CyMedia::BAYERGB: {
                    auto tempRGB = calcCoordinateColor_Bayer(info, pdata, x, y, func);
                    r = tempRGB.r;
                    g = tempRGB.g;
                    b = tempRGB.b;
                }break;

                case CyMedia::RGB: {
                    auto tempRGB = calcCoordinateColor_RGB(info, pdata, x, y);
                    r = tempRGB.r;
                    g = tempRGB.g;
                    b = tempRGB.b;
                }break;
            }
        }

        //赋值
        if (colorR)
            *colorR = r;
        if (colorG)
            *colorG = g;
        if (colorB)
            *colorB = b;
    }

    bool computeGrayHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& histogramVec,
        double* maxPixel, double* minPixel, double* avePixel) {
        if (false == imageinfo.isMono())
            return false;
        return computeHistogram_Mono(imageinfo, pData, calcMask, useMask, histogramVec,
            maxPixel, minPixel, avePixel);
    }

    bool computeBayerHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel, DemosaicingMethod func /*= BILINEAR*/) {
        if (false == imageinfo.isBayer())
            return false;
        return computeHistogram_Bayer(imageinfo, pData, calcMask, useMask, Rhistogram, Ghistogram, Bhistogram, maxPixel, minPixel, avePixel, func);
    }

    bool computeBayerHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<double>& histogram, StretchType type /*= stretch_None*/, DemosaicingMethod func/* = BILINEAR*/) {
        if (false == imageinfo.isBayer())
            return false;

        // 不去马赛克直接处理原始单通道数据
        if (func == DEMOSAIC_NONE) {
            double max, min, ave;
            return computeHistogram_Mono(imageinfo, pData, 0, false, histogram, &max, &min, &ave);
        }

        // 确定每个通道的像素字节数
        int pixelSize = 0;
        if (imageinfo.bit <= 8) pixelSize = 1;
        else if (imageinfo.bit <= 16) pixelSize = 2;
        else if (imageinfo.bit <= 31) pixelSize = 4;
        else return false;

        size_t totalPixels = static_cast<size_t>(imageinfo.width) * imageinfo.height;
        size_t bufferSize = totalPixels * 3 * pixelSize;  // RGB 三通道
        std::vector<uint8_t> rgbBuffer(bufferSize);

        // 将 Bayer 转换为 RGB
        if (!Bayer2RGBConver(imageinfo, pData, rgbBuffer.data(), func)) {
            return false;
        }

        // 构造临时的 RGB 图像信息
        ImageShowInfo rgbInfo;
        rgbInfo.width = imageinfo.width;
        rgbInfo.height = imageinfo.height;
        rgbInfo.bit = imageinfo.bit;
        rgbInfo.format = RGB;
        rgbInfo.special_pixel = PIXEL_VALUE_INT;
        rgbInfo.upLenth();

        // 调用 RGB 转单通道直方图函数
        return computeHistogram_RGBTrans(rgbInfo, rgbBuffer.data(), nullptr, false, histogram, type);
    }

    bool computeRGBHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel) {
        if (imageinfo.format != RGB)
            return false;
        return computeHistogram_RGB(imageinfo, pData, calcMask, useMask, Rhistogram, Ghistogram, Bhistogram,
            maxPixel, minPixel, avePixel);
    }
    bool computeRGBHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& histogram, StretchType strytchType /*= stretch_Gray*/) {
        if (false == imageinfo.isRGB()) {
            return false;
        }

        return computeHistogram_RGBTrans(imageinfo, pData, calcMask, useMask, histogram, strytchType);
    }

    double determineYAxisMax(std::vector<double>& his, int max_clipped_bins /*= 5*/, double outlier_factor /*= 1.2*/) {
        if (his.empty()) return 1.0;

        // 统计非零柱子数
        int non_zero = 0;
        for (double v : his) {
            if (v > 0.0) ++non_zero;
        }

        // 如果所有值都是 0，返回默认上限
        if (non_zero == 0) return 1.0;

        // 拷贝并降序排序
        std::vector<double> sorted = his;
        std::sort(sorted.begin(), sorted.end(), std::greater<double>());

        double max_val = sorted.front();

        // 非零柱子数很少时，不应截断任何柱子
        if (non_zero <= max_clipped_bins) {
            return max_val * 1.05;
        }

        // 此时 non_zero > max_clipped_bins，第 (max_clipped_bins+1) 大的值一定 >0
        double candidate = sorted[max_clipped_bins]; // 索引 max_clipped_bins

        if (candidate * outlier_factor >= max_val) {
            return max_val * 1.05;
        }
        else {
            return candidate;
        }
    }

    void computeGrayStretchPara(std::vector<double>& histogram, int32_t& start, int32_t& end) {
        //直方图平均值
        double hisAve = 0.0;
        uint32_t t_StretchSFlag = 0, t_StretchEFlag = 0;
        start = 0;
        end = histogram.size() - 1;
        for (auto& oneValue : histogram) {
            hisAve += oneValue;
        }
        hisAve = hisAve / histogram.size();
        //起始值计算
        t_StretchSFlag = hisAve * 0.15 + 0.4;
        for (int hisI = 1; hisI < histogram.size(); hisI++) {
            if (histogram[hisI] > t_StretchSFlag) {
                start = hisI;
                break;
            }
        }
        //末尾值计算
        t_StretchEFlag = hisAve * 0.15 + 0.4;
        for (int hisI = histogram.size() - 2; hisI > start; hisI--) {
            if (histogram[hisI] >= t_StretchEFlag) {
                end = hisI;
                break;
            }
        }
        //边际校正
        if (start >= histogram.size())
            end = histogram.size();
        if (end <= start)
            end = start + 10;
    }

    void computerUniformity(std::vector<double>& histogram, double& ave, double& maxColor, double* std, double* uniformity) {
        return computerUniformity_Mono(histogram, ave, maxColor, std, uniformity);
    }

    void computerThreeUniformity(std::vector<double>& histogram_1, std::vector<double>& histogram_2, std::vector<double>& histogram_3, std::vector<double>& ave, std::vector<double>& maxColor, std::vector<double>& std, std::vector<double>& uniformity) {
        if (std.size() < 3)
            std.resize(3);
        if (uniformity.size() < 3)
            uniformity.resize(3);
        computerUniformity_Mono(histogram_1, ave[0], maxColor[0], &std[0], &uniformity[0]);
        computerUniformity_Mono(histogram_2, ave[1], maxColor[1], &std[1], &uniformity[1]);
        computerUniformity_Mono(histogram_3, ave[2], maxColor[2], &std[2], &uniformity[2]);
    }

    bool monoUnPack(ImageShowInfo& info, uint8_t* data, uint8_t* out_data) {
        if (info.format == MONO10P) {
            return Mono10P2MonoConver(info, data, (uint16_t*)out_data);
        }
        else if (info.format == MONO10P_GVSP) {
            return Mono10P_GVSP2MonoConver(info, data, (uint16_t*)out_data);
        }
        else if (info.format == MONO12P) {
            return Mono12P2MonoConver(info, data, (uint16_t*)out_data);
        }
        else if (info.format == MONO12P_GVSP) {
            return Mono12P_GVSP2MonoConver(info, data, (uint16_t*)out_data);
        }

        return false;
    }

    bool bayer2RGBConvert(ImageShowInfo& info, uint8_t* data, uint8_t* out_data) {
        return Bayer2RGBConver(info, data, out_data);
    }

    void copyAlignImage(void* pSrc, void* pAlign, int srcWidth, int srcHeight, int AlignWidth, int AlignHeight, int pixelSize) {
        uint32_t copySize = pixelSize * srcWidth;
        uint32_t alignSize = pixelSize * AlignWidth;
        uint8_t* pSrc_byte = (uint8_t*)pSrc;
        uint8_t* pAlign_byte = (uint8_t*)pAlign;

        for (int32_t rowIndex = 0; rowIndex < srcHeight; rowIndex++) {
            memcpy(&pAlign_byte[rowIndex * alignSize], &pSrc_byte[rowIndex * copySize], copySize);
        }
    }

}
