#include "Calculate.h"
#include "CalcMono.h"
#include "CalcBayer.h"
#include "CalcRgb.h"
#include "CalcYUV.h"

#include <cstdlib>
#include <algorithm>

#if defined _MSC_VER
#include <ppl.h>
#include <thread>
#endif

namespace CyMediaCalc {

void calcCoordinateColor(ImageShowInfo& info, uint8_t* pdata, int x, int y, double* colorR, double* colorG, double* colorB, CyMedia::ImageColorOpe formatope) {
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
    else if (info.isRGB()) {
        auto tempRGB = CyMediaCalc_RGB::calcCoordinateColor(info, pdata, x, y);
        r = tempRGB.r;
        g = tempRGB.g;
        b = tempRGB.b;
    }
    else if (info.isBayer()) {
        auto tempRGB = CyMediaCalc_Bayer::calcCoordinateColor(info, pdata, x, y, formatope.bayerFunc);
        r = tempRGB.r;
        g = tempRGB.g;
        b = tempRGB.b;
    }
    else if (info.isYUV()) {
        auto tempRGB = CyMediaCalc_YUV::calcCoordinateColor(info, pdata, x, y, formatope.YUVFunc);
        r = tempRGB.r;
        g = tempRGB.g;
        b = tempRGB.b;
    }
    else {
        switch (info.format) {
            case CyMedia::MONO: 
            case CyMedia::MONO_OVERSIZE:{
                r = CyMediaCalc_Mono::calcCoordinateColor_Mono(info, pdata, y * info.width + x);
                g = r;
                b = r;
            }break;

            case CyMedia::MONO10P: {
                r = CyMediaCalc_Mono::calcCoordinateColor_Mono10P(pdata, y * info.width + x);
                g = r;
                b = r;
            }break;

            case CyMedia::MONO10P_GVSP: {
                r = CyMediaCalc_Mono::calcCoordinateColor_Mono10P_GVSP(pdata, y * info.width + x);
                g = r;
                b = r;
            }break;

            case CyMedia::MONO12P: {
                r = CyMediaCalc_Mono::calcCoordinateColor_Mono12P(pdata, y * info.width + x);
                g = r;
                b = r;
            }break;

            case CyMedia::MONO12P_GVSP: {
                r = CyMediaCalc_Mono::calcCoordinateColor_Mono12P_GVSP(pdata, y * info.width + x);
                g = r;
                b = r;
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

bool computeGrayHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& histogramVec,
    double* maxPixel, double* minPixel, double* avePixel) {
    if (false == imageinfo.isMono() && false == imageinfo.isMono())
        return false;
    return CyMediaCalc_Mono::computeHistogram(imageinfo, pData, calcMask, useMask, histogramVec,
        maxPixel, minPixel, avePixel);
}

bool computeRGBHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel) {
    if (imageinfo.format != RGB && imageinfo.format != BGR)
        return false;
    return CyMediaCalc_RGB::computeHistogram(imageinfo, pData, calcMask, useMask, Rhistogram, Ghistogram, Bhistogram,
        maxPixel, minPixel, avePixel);
}
bool computeRGBHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<double>& histogram, StretchType strytchType /*= stretch_Gray*/) {
    if (false == imageinfo.isRGB()) {
        return false;
    }

    return CyMediaCalc_RGB::computeHistogram_Stretch(imageinfo, pData, histogram, strytchType);
}

bool computeBayerHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel, DemosaicingMethod func /*= BILINEAR*/) {
    if (false == imageinfo.isBayer()) return false;
    return CyMediaCalc_Bayer::computeHistogram(imageinfo, pData, calcMask, useMask, Rhistogram, Ghistogram, Bhistogram, maxPixel, minPixel, avePixel, func);
}

bool computeBayerHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<double>& histogram, StretchType type /*= stretch_None*/, DemosaicingMethod func/* = BILINEAR*/) {
    if (false == imageinfo.isBayer()) return false;

    // 不去马赛克直接处理原始单通道数据
    if (func == DEMOSAIC_NONE) {
        double max, min, ave;
        return CyMediaCalc_Mono::computeHistogram(imageinfo, pData, 0, false, histogram, &max, &min, &ave);
    }
    return CyMediaCalc_Bayer::computeHistogram_Stretch(imageinfo, pData, histogram, func, type);
}

bool computerYUVHistogram(uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel, YUVTransMethod func /*= YUVTRANS_NORMAL */) {
    if (false == imageinfo.isYUV()) return false;
    return CyMediaCalc_YUV::computeHistogram(imageinfo, pData, calcMask, useMask, Rhistogram, Ghistogram, Bhistogram, maxPixel, minPixel, avePixel, func);
}

bool computerYUVHistogram(uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<double>& histogram, StretchType type /*= stretch_None*/, YUVTransMethod func /*= YUVTRANS_NORMAL */) {
    if (false == imageinfo.isYUV()) return false;

    return CyMediaCalc_YUV::computeHistogram_Stretch(imageinfo, pData, histogram, func, type);
}

void computeGrayStretchPara(const std::vector<double>& histogram, int32_t& start, int32_t& end) {
    const size_t n = histogram.size();
    if (n <= 1) { start = 0; end = 0; return; }
    // 计算总像素数
    double total = 0.0;
    for (double v : histogram) {
        if (v > 0.0) total += v;
    }
    if (total <= 0.0) { start = 0; end = static_cast<int32_t>(n) - 1; return; }
    // 两端各裁剪 1% 像素。
    const double lowCut = 0.01;
    const double highCut = 0.01;
    const double lowLimit = total * lowCut;
    const double highLimit = total * highCut;
    // 从低灰度端累计，找到起始灰度
    size_t s = 0;
    double acc = 0.0;
    for (size_t i = 0; i < n; ++i) {
        acc += histogram[i];
        if (acc >= lowLimit) {
            s = i;
            break;
        }
    }
    // 从高灰度端累计，找到结束灰度
    size_t e = n - 1;
    acc = 0.0;
    for (size_t i = n; i > 0; --i) {
        const size_t idx = i - 1;
        acc += histogram[idx];
        if (acc >= highLimit) {
            e = idx;
            break;
        }
    }
    // 最小范围保护：
    // 避免 start/end 过于接近，导致线性拉伸后噪声被过度放大
    const size_t minSpan = std::max<size_t>(2, static_cast<size_t>(n * 0.02));
    if (e <= s) {
        s = 0;
        e = n - 1;
    }
    if (e - s < minSpan) {
        const size_t center = (s + e) / 2;
        const size_t half = minSpan / 2;

        s = (center > half) ? center - half : 0;
        e = std::min<size_t>(s + minSpan, n - 1);

        if (e - s < minSpan) {
            s = 0;
            e = n - 1;
        }
    }
    start = static_cast<int32_t>(s);
    end = static_cast<int32_t>(e);
}

void computerUniformity(const std::vector<double>& histogram, const double& ave, double maxBitColor, double* std, double* uniformity, int* hisXRangeMax) {
    return CyMediaCalc_Mono::computerUniformity(histogram, ave, maxBitColor, std, uniformity, hisXRangeMax);
}

void computerThreeUniformity(const std::vector<double>& histogram_1, const std::vector<double>& histogram_2, const std::vector<double>& histogram_3, 
    const std::vector<double>& ave, double maxBitColor, 
    std::vector<double>& std, std::vector<double>& uniformity, std::vector<int>& hisXRangeMax) {
    if (std.size() < 3)
        std.resize(3);
    if (uniformity.size() < 3)
        uniformity.resize(3);
    CyMediaCalc_Mono::computerUniformity(histogram_1, ave[0], maxBitColor, &std[0], &uniformity[0], &hisXRangeMax[0]);
    CyMediaCalc_Mono::computerUniformity(histogram_2, ave[1], maxBitColor, &std[1], &uniformity[1], &hisXRangeMax[1]);
    CyMediaCalc_Mono::computerUniformity(histogram_3, ave[2], maxBitColor, &std[2], &uniformity[2], &hisXRangeMax[2]);
}

bool monoUnPack(const ImageShowInfo& info, const uint8_t* data, uint8_t* out_data) {
    if (info.format == MONO10P) {
        return CyMediaCalc_Mono::Mono10P2MonoConver(info, data, (uint16_t*)out_data);
    }
    else if (info.format == MONO10P_GVSP) {
        return CyMediaCalc_Mono::Mono10P_GVSP2MonoConver(info, data, (uint16_t*)out_data);
    }
    else if (info.format == MONO12P) {
        return CyMediaCalc_Mono::Mono12P2MonoConver(info, data, (uint16_t*)out_data);
    }
    else if (info.format == MONO12P_GVSP) {
        return CyMediaCalc_Mono::Mono12P_GVSP2MonoConver(info, data, (uint16_t*)out_data);
    }

    return false;
}

bool monoUnPack_8(const CyMedia::ImageShowInfo& info, const uint8_t* data, uint8_t* out_data) {
    return CyMediaCalc_Mono::MonoUmPack_8(info, data, out_data);
}

bool bayer2RGB(const ImageShowInfo& info, const uint8_t* data, uint8_t* out_data, DemosaicingMethod func/* = DEMOSAIC_BILINEAR*/) {
    return CyMediaCalc_Bayer::bayer2RGB(info, data, out_data, func);
}

bool bayer2RGB_8(const CyMedia::ImageShowInfo& info, const uint8_t* data, uint8_t* out_data, DemosaicingMethod func /*= DEMOSAIC_BILINEAR*/) {
    return CyMediaCalc_Bayer::bayer2RGB_8(info, data, out_data, func);
}

bool YUV2RGB(const ImageShowInfo& info, const uint8_t* data, uint8_t* out_data, YUVTransMethod func /*= YUVTRANS_NORMAL*/) {
    return CyMediaCalc_YUV::YUV2RGBConver(info, data, out_data, func);
}

bool CYMEDIA_LIB JPG2RGB(const CyMedia::ImageShowInfo& info, const void* data, void* out_data, bool& isGray) {
    return CyMediaCalc_RGB::JPG2RGB(info, data, out_data, isGray);
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
