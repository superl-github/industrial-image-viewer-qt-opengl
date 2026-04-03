#include "CyMdiaCalc_Rgb.h"
#include <algorithm>
#include <cmath>

// 颜色空间转换常量
const float THRESHOLD_SRGB = 0.04045f;       // sRGB 阈值
const float THRESHOLD_LINEAR = 0.0031308f;   // 线性 RGB 阈值
const float THRESHOLD_F = 0.008856f;        // f(t) 函数阈值
const float THRESHOLD_F_INV = 6.0f / 29.0f; // 逆 f(t) 函数阈值

namespace CyMedia {
    //辅助函数
    template<typename T>
    RgbPixel getPixelValue(const T* data, int32_t pixelI);
    RgbPixel calcCoordinateColor_RGB(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y);
    RgbPixel getRGBTransPixel(RgbPixel pixel, StretchType type, int32_t maxValue);
    int32_t getRGBTransPixelOne(RgbPixel pixel, StretchType type, float maxValue);
    inline float step(int32_t a, int32_t b) {
        return (b >= a) ? 1.0f : 0.0f;
    }
    // sRGB -> 线性 RGB
    RgbPixelF srgb2linear(RgbPixelF& srgb) {
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

    // 线性 RGB -> sRGB
    RgbPixelF linear2srgb(RgbPixelF& linear) {
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


    bool computeHistogram_RGB(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel) {
        if (!data || info.format != RGB) {
            return false;
        }
        
        int32_t histSize = (1U << info.bit);

        Rhistogram.assign(histSize, 0);
        Ghistogram.assign(histSize, 0);
        Bhistogram.assign(histSize, 0);
        double* pRhis = Rhistogram.data();
        double* pGhis = Ghistogram.data();
        double* pBhis = Bhistogram.data();

        maxPixel.assign(3, 0.0);
        minPixel.assign(3, histSize);
        avePixel.assign(3, 0.0);

        int32_t pMaxPix_R = 0.0;
        int32_t pMaxPix_G = 0.0;
        int32_t pMaxPix_B = 0.0;

        int32_t pMinPix_R = histSize;
        int32_t pMinPix_G = histSize;
        int32_t pMinPix_B = histSize;

        double pAvePix_R = 0.0;
        double pAvePix_G = 0.0;
        double pAvePix_B = 0.0;

        int32_t totalPixels = info.width * info.height;
        int32_t maskNum = totalPixels;
        if (useMask && mask) {
            maskNum = 0;
        }

        auto addPixel = [&](RgbPixel pix) {
                pRhis[pix.r]++;
                pGhis[pix.g]++;
                pBhis[pix.b]++;

                pAvePix_R += pix.r;
                pAvePix_G += pix.g;
                pAvePix_B += pix.b;

                if (pMaxPix_R < pix.r) {
                    pMaxPix_R = pix.r;
                }
                if (pMaxPix_G < pix.g) {
                    pMaxPix_G = pix.g;
                }
                if (pMaxPix_B < pix.b) {
                    pMaxPix_B = pix.b;
                }

                if (pMinPix_R > pix.r) {
                    pMinPix_R = pix.r;
                }
                if (pMinPix_G > pix.g) {
                    pMinPix_G = pix.g;
                }
                if (pMinPix_B > pix.b) {
                    pMinPix_B = pix.b;
                }
            };

        RgbPixel tempRGB;
        if (info.bit <= 8) {
            if (useMask && mask) {
                for (int32_t i = 0; i < totalPixels; ++i) {
                    if (mask->at(i)) {
                        addPixel(getPixelValue((uint8_t*)data, i));
                        maskNum++;
                    }
                }
            }
            else {
                for (int32_t i = 0; i < totalPixels; ++i) {
                    addPixel(getPixelValue((uint8_t*)data, i));
                }
            }
        }
        else if (info.bit <= 16) {
            if (useMask && mask) {
                for (int32_t i = 0; i < totalPixels; ++i) {
                    if (mask->at(i)) {
                        addPixel(getPixelValue((uint16_t*)data, i));
                        maskNum++;
                    }
                }
            }
            else {
                for (int32_t i = 0; i < totalPixels; ++i) {
                    addPixel(getPixelValue((uint16_t*)data, i));
                }
            }
            
        }
        else if (info.bit <= 31) {
            if (useMask && mask) {
                for (int32_t i = 0; i < totalPixels; ++i) {
                    if (mask->at(i)) {
                        addPixel(getPixelValue((uint32_t*)data, i));
                        maskNum++;
                    }
                }
            }
            else {
                for (int32_t i = 0; i < totalPixels; ++i) {
                    addPixel(getPixelValue((uint32_t*)data, i));
                }
            }
        }
        else {
            return false;
        }

        maxPixel[0] = pMaxPix_R;
        maxPixel[1] = pMaxPix_G;
        maxPixel[2] = pMaxPix_B;

        minPixel[0] = pMinPix_R;
        minPixel[1] = pMinPix_G;
        minPixel[2] = pMinPix_B;

        avePixel[0] = pAvePix_R / maskNum;
        avePixel[1] = pAvePix_G / maskNum;
        avePixel[2] = pAvePix_B / maskNum;

        return true;
    }

    bool computeHistogram_RGBTrans(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask, std::vector<double>& histogram, StretchType type /*= stretch_None*/) {
        int32_t maxBitValue = (1 << info.bit);
        histogram.assign(maxBitValue, 0.0);
        double* pHis = histogram.data();
        int32_t pixelSize = info.width * info.height;
        for (int32_t h = 0; h < info.height; h++) {
            for (int32_t w = 0; w < info.width; w++) {
                int index = getRGBTransPixelOne(calcCoordinateColor_RGB(info, data, w, h), type, maxBitValue - 1);
                pHis[index] ++;
            }
        }
        return true;
    }

    CyMedia::RgbPixel RGBPixel2rgb2Gray(RgbPixel pixel, float maxValue) {
        float disValue = pixel.r * 0.299 + pixel.g * 0.587 + pixel.b * 0.114;
        int32_t Ivalue = std::clamp(disValue, 0.0f, maxValue);
        return { Ivalue, Ivalue, Ivalue };
    }

    CyMedia::RgbPixel RGBPixel2rgb2hsv(RgbPixel rgb, float maxV) {
        float r = rgb.r, g = rgb.g, b = rgb.b;

        float max_val = std::max({ r, g, b });
        float min_val = std::min({ r, g, b });
        float delta = max_val - min_val;

        float h = 0.0f, s = 0.0f, v = max_val / maxV;

        if (delta > 1e-6f) { // avoid division by zero
            s = delta / max_val;

            if (maxV == r) {
                h = (g - b) / delta;
                if (h < 0.0f) h += 6.0f;
            }
            else if (maxV == g) {
                h = (b - r) / delta + 2.0f;
            }
            else { // maxVal == b
                h = (r - g) / delta + 4.0f;
            }

            h *= (1.0f / 6.0f); // normalize to [0,1]
        }

        return {
            int32_t(std::clamp(h, 0.0f, 1.0f) * maxV),
            int32_t(std::clamp(s, 0.0f, 1.0f) * maxV),
            int32_t(std::clamp(v, 0.0f, 1.0f) * maxV) };
    }

    CyMedia::RgbPixel RGBPixel2LAB(RgbPixel rgb, float maxV) {
        // 1. sRGB → 线性 RGB
        auto normalRGB = RgbPixelF{ rgb.r / maxV, rgb.g / maxV, rgb.b / maxV };
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
            int32_t(L),
            int32_t(a * maxV),
            int32_t(b * maxV) };
    }

    CyMedia::RgbPixel LABPixel2RGB(RgbPixel lab, float maxV) {
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
            int32_t(std::clamp(sRGB.r, 0.0f, 1.0f) * maxV),
            int32_t(std::clamp(sRGB.g, 0.0f, 1.0f) * maxV),
            int32_t(std::clamp(sRGB.b, 0.0f, 1.0f)* maxV) };
    }





    template<typename T>
    CyMedia::RgbPixel getPixelValue(const T* data, int32_t pixelI) {
        return {
            static_cast<int32_t>(data[pixelI * 3 + 0]),
            static_cast<int32_t>(data[pixelI * 3 + 1]),
            static_cast<int32_t>(data[pixelI * 3 + 2])
        };
    }

    CyMedia::RgbPixel calcCoordinateColor_RGB(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y) {
        RgbPixel pixel{ 0, 0, 0 };
        if (!pdata || info.format != RGB || x < 0 || y < 0 ||
            x >= info.width || y >= info.height) {
            return pixel;
        }

        if (info.bit <= 8) {
            return getPixelValue((uint8_t*)pdata, y * info.width + x);
        }
        else if (info.bit <= 16) {
            return getPixelValue((uint16_t*)pdata, y * info.width + x);
        }
        else if (info.bit <= 31) {
            return getPixelValue((uint32_t*)pdata, y * info.width + x);
        }


        return pixel;
    }

    CyMedia::RgbPixel getRGBTransPixel(RgbPixel pixel, StretchType type, float maxValue) {
        switch (type) {
            case CyMedia::stretch_Gray:
                return RGBPixel2rgb2Gray(pixel, maxValue);
            case CyMedia::stretch_HSV:
                return RGBPixel2rgb2hsv(pixel, maxValue);
            case CyMedia::stretch_Lab:
                return RGBPixel2LAB(pixel, maxValue);
        }

        return pixel;
    }
    int32_t getRGBTransPixelOne(RgbPixel pixel, StretchType type, float maxValue) {
        switch (type) {
        case CyMedia::stretch_Gray:
            return RGBPixel2rgb2Gray(pixel, maxValue).r;
        case CyMedia::stretch_HSV:
            return RGBPixel2rgb2hsv(pixel, maxValue).b;
        case CyMedia::stretch_Lab:
            return RGBPixel2LAB(pixel, maxValue).r;
        }

        return pixel.r;
    }
}