#include "CalcColor.h"

namespace CyMediaCalc {

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
RgbPixel  RGBPixel2rgb2Gray(RgbPixel pixel, uint32_t bitMax) {
    float disValue = pixel.r * 0.299 + pixel.g * 0.587 + pixel.b * 0.114;
    uint32_t gary = uint32_t(disValue) % bitMax;
    return { gary, gary, gary };
}
RgbPixel  RGBPixel2rgb2hsv(RgbPixel pixel, uint32_t bitMax) {
    float r = pixel.r, g = pixel.g, b = pixel.b;

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
RgbPixel  RGBPixel2LAB(RgbPixel pixel, uint32_t bitMax) {
    // 1. sRGB → 线性 RGB
    float pixelMax = bitMax - 1.0;
    auto normalRGB = RgbPixelF{ pixel.r / pixelMax, pixel.g / pixelMax, pixel.b / pixelMax };
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
RgbPixel  RGBPixel2Strtch(RgbPixel pixel, StretchType type, uint32_t bitMax) {
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
int32_t   RGBPixel2StrtchOne(RgbPixel pixel, StretchType type, uint32_t bitMax) {
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

// 快速版（供内部高性能调用）
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

RgbPixel  LABPixel2RGB(RgbPixel lab, uint32_t maxValue) {
    float L = lab.r;
    float a = lab.g / maxValue;
    float b = lab.b / maxValue;

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
        uint32_t(sRGB.r) % maxValue,
        uint32_t(sRGB.g) % maxValue,
        uint32_t(sRGB.b) % maxValue };
}

}
