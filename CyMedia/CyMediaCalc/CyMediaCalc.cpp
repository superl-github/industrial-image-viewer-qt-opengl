#include "CyMediaCalc.h"
#include "CyMediaCalc_Mono.h"
#include "CyMediaCalc_Bayer.h"
#include "CyMediaCalc_Rgb.h"
#include "CyMediaCalc_YUV.h"

#include <cstdlib>

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
        if (false == imageinfo.isMono())
            return false;
        return CyMediaCalc_Mono::computeHistogram(imageinfo, pData, calcMask, useMask, histogramVec,
            maxPixel, minPixel, avePixel);
    }

    bool computeRGBHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel) {
        if (imageinfo.format != RGB)
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
        return CyMediaCalc_Mono::computerUniformity(histogram, ave, maxColor, std, uniformity);
    }

    void computerThreeUniformity(std::vector<double>& histogram_1, std::vector<double>& histogram_2, std::vector<double>& histogram_3, std::vector<double>& ave, std::vector<double>& maxColor, std::vector<double>& std, std::vector<double>& uniformity) {
        if (std.size() < 3)
            std.resize(3);
        if (uniformity.size() < 3)
            uniformity.resize(3);
        CyMediaCalc_Mono::computerUniformity(histogram_1, ave[0], maxColor[0], &std[0], &uniformity[0]);
        CyMediaCalc_Mono::computerUniformity(histogram_2, ave[1], maxColor[1], &std[1], &uniformity[1]);
        CyMediaCalc_Mono::computerUniformity(histogram_3, ave[2], maxColor[2], &std[2], &uniformity[2]);
    }

    bool monoUnPack(ImageShowInfo& info, uint8_t* data, uint8_t* out_data) {
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

    bool bayer2RGB(ImageShowInfo& info, uint8_t* data, uint8_t* out_data, DemosaicingMethod func/* = DEMOSAIC_BILINEAR*/) {
        return CyMediaCalc_Bayer::bayer2RGB(info, data, out_data, func);
    }

    bool CYMEDIA_LIB YUV2RGB(ImageShowInfo& info, uint8_t* data, uint8_t* out_data, YUVTransMethod func /*= YUVTRANS_NORMAL*/) {
        return CyMediaCalc_YUV::YUV2RGBConver(info, data, out_data, func);
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





    CyMedia::RgbPixelF srgb2linear(RgbPixelF& srgb) {
        RgbPixelF linear;
        linear.r = (srgb.r > THRESHOLD_SRGB) ?
            std::pow((srgb.r + 0.055f) / 1.055f, 2.4f) :
            srgb.r / 12.92f;
        linear.g = (srgb.g > THRESHOLD_SRGB) ?
            std::pow((srgb.g + 0.055f) / 1.055f, 2.4f) :
            srgb.g / 12.92f;
        linear.b = (srgb.b > THRESHOLD_SRGB) ?
            std::pow((srgb.b + 0.055f) / 1.055f, 2.4f) :
            srgb.b / 12.92f;
        return linear;
    }

    CyMedia::RgbPixelF linear2srgb(RgbPixelF& linear) {
        RgbPixelF srgb;
        srgb.r = (linear.r > THRESHOLD_LINEAR) ?
            1.055f * std::pow(linear.r, 1.0f / 2.4f) - 0.055f :
            12.92f * linear.r;
        srgb.g = (linear.g > THRESHOLD_LINEAR) ?
            1.055f * std::pow(linear.g, 1.0f / 2.4f) - 0.055f :
            12.92f * linear.g;
        srgb.b = (linear.b > THRESHOLD_LINEAR) ?
            1.055f * std::pow(linear.b, 1.0f / 2.4f) - 0.055f :
            12.92f * linear.b;
        return srgb;
    }

    CyMedia::RgbPixel RGBPixel2rgb2Gray(RgbPixel pixel, uint32_t bitMax) {
        float disValue = pixel.r * 0.299 + pixel.g * 0.587 + pixel.b * 0.114;
        uint32_t gary = uint32_t(disValue) % bitMax;
        return { gary, gary, gary };
    }

    CyMedia::RgbPixel RGBPixel2rgb2hsv(RgbPixel rgb, uint32_t bitMax) {
        float r = rgb.r, g = rgb.g, b = rgb.b;

        float max_val = std::max({ r, g, b });
        float min_val = std::min({ r, g, b });
        float delta = max_val - min_val;

        float h = 0.0f, s = 0.0f, v = max_val / (bitMax - 1);

        if (delta > 1e-6f) { // avoid division by zero
            s = delta / max_val;

            if (bitMax == r) {
                h = (g - b) / delta;
                if (h < 0.0f) h += 6.0f;
            }
            else if (bitMax == g) {
                h = (b - r) / delta + 2.0f;
            }
            else { // bitMax == b
                h = (r - g) / delta + 4.0f;
            }

            h *= (1.0f / 6.0f); // normalize to [0,1]
        }

        return {
            uint32_t(h) % bitMax,
            uint32_t(s) % bitMax,
            uint32_t(v) % bitMax };
    }

    CyMedia::RgbPixel RGBPixel2LAB(RgbPixel rgb, uint32_t bitMax) {
        // 1. sRGB → 线性 RGB
        float pixelMax = bitMax - 1.0;
        auto normalRGB = RgbPixelF{ rgb.r / pixelMax, rgb.g / pixelMax, rgb.b / pixelMax };
        auto lin = srgb2linear(normalRGB);

        // 2. 线性 RGB → XYZ
        float X = lin.r * 0.4124564f + lin.g * 0.3575761f + lin.b * 0.1804375f;
        float Y = lin.r * 0.2126729f + lin.g * 0.7151522f + lin.b * 0.0721750f;
        float Z = lin.r * 0.0193339f + lin.g * 0.1191920f + lin.b * 0.9503041f;

        // 3. XYZ 归一化到 D65 白点
        X /= 0.95047f;
        Z /= 1.08883f;

        // 4. 应用 f(t) 函数
        auto f = [](float t) {
            return (t > THRESHOLD_F) ?
                std::pow(t, 1.0f / 3.0f) :
                7.787f * t + 16.0f / 116.0f;
            };
        float fx = f(X);
        float fy = f(Y);
        float fz = f(Z);

        // 5. 计算 Lab
        float L = 116.0f * fy - 16.0f;
        float a = 500.0f * (fx - fy);
        float b = 200.0f * (fy - fz);

        // Normalize L to [0, 1] as in your GLSL
        return {
            uint32_t(L) % bitMax,
            uint32_t(a * pixelMax) % bitMax,
            uint32_t(b * pixelMax) % bitMax };
    }

    CyMedia::RgbPixel RGBPixel2Strtch(RgbPixel pixel, StretchType type, uint32_t bitMax) {
        switch (type) {
        case CyMedia::stretch_Gray:
            return RGBPixel2rgb2Gray(pixel, bitMax);
        case CyMedia::stretch_HSV:
            return RGBPixel2rgb2hsv(pixel, bitMax);
        case CyMedia::stretch_Lab:
            return RGBPixel2LAB(pixel, bitMax);
        }

        return pixel;
    }

    int32_t RGBPixel2StrtchOne(RgbPixel pixel, StretchType type, uint32_t bitMax) {
        switch (type) {
        case CyMedia::stretch_Gray:
            return RGBPixel2rgb2Gray(pixel, bitMax).r;
        case CyMedia::stretch_HSV:
            return RGBPixel2rgb2hsv(pixel, bitMax).b;
        case CyMedia::stretch_Lab:
            return RGBPixel2LAB(pixel, bitMax).r;
        }

        return pixel.r;
    }

    uint32_t fastGray(const RgbPixel& rgb) {
        return static_cast<uint32_t>(rgb.r * 0.299f + rgb.g * 0.587f + rgb.b * 0.114f);
    }

    uint32_t fastHSV_V(const RgbPixel& rgb) {
        return std::max({ rgb.r, rgb.g, rgb.b });
    }

    uint32_t fastLab_L(const RgbPixel& rgb, uint32_t bitMax) {
        float r = rgb.r / (bitMax - 1.0);
        float g = rgb.g / (bitMax - 1.0);
        float b = rgb.b / (bitMax - 1.0);

        auto linear = [](float c) {
            return (c > 0.04045f) ? std::pow((c + 0.055f) / 1.055f, 2.4f) : (c / 12.92f);
            };
        float R = linear(r);
        float G = linear(g);
        float B = linear(b);

        float Y = R * 0.2126729f + G * 0.7151522f + B * 0.0721750f;

        float fy = (Y > 0.008856f) ? std::pow(Y, 1.0f / 3.0f) : (7.787f * Y + 16.0f / 116.0f);
        float L = 116.0f * fy - 16.0f;

        return static_cast<int32_t>(L);
    }

    uint32_t RGBT2StretchOneFast(const RgbPixel& rgb, StretchType type, uint32_t bitMax) {
        uint32_t value;
        switch (type) {
            case stretch_Gray: {
                value = fastGray(rgb);
            }break;
            case stretch_HSV: {
                value = fastHSV_V(rgb);
            }break;
            case stretch_Lab: {
                value = fastLab_L(rgb, bitMax);
            }break;

            default:
                value = rgb.r;
        }

        return value % uint32_t(bitMax + 1);
    }

    CyMedia::RgbPixel LABPixel2RGB(RgbPixel lab, uint32_t maxV) {
        float L = lab.r;
        float a = lab.g / maxV;
        float b = lab.b / maxV;

        // 1. Lab → f(X/Xn), f(Y/Yn), f(Z/Zn)
        float fy = (L + 16.0f) / 116.0f;
        float fx = fy + a / 500.0f;
        float fz = fy - b / 200.0f;

        // 2. 逆 f(t) 函数
        auto f_inv = [](float t) {
            return (t > THRESHOLD_F_INV) ?
                t * t * t :
                (t - 16.0f / 116.0f) / 7.787f;
            };

        float fx_inv = f_inv(fx);
        float fy_inv = f_inv(fy);
        float fz_inv = f_inv(fz);

        // 3. 乘回 D65 白点
        float X = fx_inv * 0.95047f;
        float Y = fy_inv;
        float Z = fz_inv * 1.08883f;

        // 4. XYZ → 线性 RGB
        float R = X * 3.2404542f + Y * -1.5371385f + Z * -0.4985314f;
        float G = X * -0.9692660f + Y * 1.8760108f + Z * 0.0415560f;
        float B = X * 0.0556434f + Y * -0.2040259f + Z * 1.0572252f;

        // 5. 线性 RGB → sRGB
        auto linear_rgb = RgbPixelF{ R, G, B };
        auto sRGB = linear2srgb(linear_rgb);

        return {
            uint32_t(sRGB.r) % maxV,
            uint32_t(sRGB.g) % maxV,
            uint32_t(sRGB.b) % maxV };
    }
}
