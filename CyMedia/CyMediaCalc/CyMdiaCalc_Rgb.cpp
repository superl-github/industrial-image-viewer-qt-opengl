#include "CyMdiaCalc_Rgb.h"
#include <algorithm>
#include <cmath>

#if defined _MSC_VER
#include <ppl.h>
#include <thread>
#elif defined _OPENMP
#   include <omp.h>
#endif

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

    // 获取自适应并行阈值
    // @param coreCount       CPU 逻辑核心数
    // @param type            拉伸类型，用于调整复杂度权重
    // @return                建议的最小总像素数，低于此值应使用串行
    inline int32_t getAdaptiveParallelThreshold(int32_t coreCount, StretchType type) {
        // 基准：每个核心至少处理 15000 像素才值得并行
        constexpr int32_t BASE_PIXELS_PER_CORE = 15000;

        // 根据计算复杂度调整系数
        double complexityFactor = 1.0;
        switch (type) {
            case stretch_None:
            case stretch_Gray:
                complexityFactor = 1.2;   // 简单计算，阈值稍高
                break;
            case stretch_HSV:
                complexityFactor = 0.9;   // 中等复杂度
                break;
            case stretch_Lab:
                complexityFactor = 0.5;   // 很重，阈值降低一半
                break;
        }

        int32_t threshold = static_cast<int32_t>(coreCount * BASE_PIXELS_PER_CORE * complexityFactor);

        // 限制在合理范围内（避免极端情况）
        constexpr int32_t MIN_THRESHOLD = 20000;   // 至少 2 万像素
        constexpr int32_t MAX_THRESHOLD = 800000;  // 最多 80 万像素
        return std::clamp(threshold, MIN_THRESHOLD, MAX_THRESHOLD);
    }
    inline float step(int32_t a, int32_t b) {
        return (b >= a) ? 1.0f : 0.0f;
    }
    inline int32_t fastGray(const RgbPixel& rgb, float /*maxV*/) {
        return static_cast<int32_t>(rgb.r * 0.299f + rgb.g * 0.587f + rgb.b * 0.114f);
    }
    inline int32_t fastHSV_V(const RgbPixel& rgb, float maxV) {
        float r = rgb.r, g = rgb.g, b = rgb.b;
        float max_val = std::max({ r, g, b });
        return static_cast<int32_t>((max_val / maxV) * maxV);
    }
    inline int32_t fastLab_L(const RgbPixel& rgb, float maxV) {
        float r = rgb.r / maxV;
        float g = rgb.g / maxV;
        float b = rgb.b / maxV;

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
    inline int32_t getRGBTransPixelOneFast(const RgbPixel& rgb, StretchType type, float maxV) {
        switch (type) {
        case stretch_Gray:  return fastGray(rgb, maxV);
        case stretch_HSV:   return fastHSV_V(rgb, maxV);
        case stretch_Lab:   return fastLab_L(rgb, maxV);
        default:            return rgb.r;
        }
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


    template<typename T>
    void computeHistogram_RGB_impl(const ImageShowInfo& info,const T* pData, std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel,
        double& finalSumR, double& finalSumG, double& finalSumB, int32_t& maskNum) {
        const int32_t totalPixels = info.width * info.height;
        const int32_t histSize = (1U << info.bit);
        const int32_t maxVal = (1 << info.bit) - 1;   // 用于 16/32 位，8 位时也可统一用 255

        // 获取 CPU 核心数
        static int32_t cpuCores = []() {
            int32_t cores = static_cast<int32_t>(std::thread::hardware_concurrency());
            return cores > 0 ? cores : 4;  // 若获取失败，默认 4 核
            }();
        //计算并行计算阈值
        const int32_t threshold = getAdaptiveParallelThreshold(cpuCores, stretch_None);
        if (totalPixels < threshold) {
            int32_t maxR = 0, maxG = 0, maxB = 0;
            int32_t minR = maxVal, minG = maxVal, minB = maxVal;

            for (int32_t i = 0; i < totalPixels; ++i) {
                if (useMask && mask && !(*mask)[i]) continue;

                const T* pix = pData + i * 3;
                int32_t r = static_cast<int32_t>(pix[0]);
                int32_t g = static_cast<int32_t>(pix[1]);
                int32_t b = static_cast<int32_t>(pix[2]);

                Rhistogram[r] += 1.0;
                Ghistogram[g] += 1.0;
                Bhistogram[b] += 1.0;

                if (r > maxR) maxR = r;
                if (g > maxG) maxG = g;
                if (b > maxB) maxB = b;

                if (r < minR) minR = r;
                if (g < minG) minG = g;
                if (b < minB) minB = b;

                finalSumR += r;
                finalSumG += g;
                finalSumB += b;
                maskNum++;
            }

            maxPixel[0] = maxR;
            maxPixel[1] = maxG;
            maxPixel[2] = maxB;
            minPixel[0] = minR;
            minPixel[1] = minG;
            minPixel[2] = minB;
            return;
        }


#if defined(_MSC_VER)
        // ----- PPL 并行版本 -----
        struct LocalStat {
            std::vector<double> rHist, gHist, bHist;
            int32_t maxR = 0, maxG = 0, maxB = 0;
            int32_t minR, minG, minB;
            double sumR = 0, sumG = 0, sumB = 0;
            int32_t count = 0;

            LocalStat(int32_t size, int32_t initMin)
                : rHist(size, 0), gHist(size, 0), bHist(size, 0),
                minR(initMin), minG(initMin), minB(initMin) {
            }
        };

        concurrency::combinable<LocalStat> localStats([histSize, maxVal]() {
            return LocalStat(histSize, maxVal);
            });

        concurrency::parallel_for(0, totalPixels, [&](int32_t i) {
            if (useMask && mask && !(*mask)[i]) return;

            const T* pix = pData + i * 3;
            int32_t r = static_cast<int32_t>(pix[0]);
            int32_t g = static_cast<int32_t>(pix[1]);
            int32_t b = static_cast<int32_t>(pix[2]);

            auto& local = localStats.local();
            local.rHist[r] += 1.0;
            local.gHist[g] += 1.0;
            local.bHist[b] += 1.0;

            if (r > local.maxR) local.maxR = r;
            if (g > local.maxG) local.maxG = g;
            if (b > local.maxB) local.maxB = b;

            if (r < local.minR) local.minR = r;
            if (g < local.minG) local.minG = g;
            if (b < local.minB) local.minB = b;

            local.sumR += r;
            local.sumG += g;
            local.sumB += b;
            local.count++;
            });

        int32_t finalMaxR = 0, finalMaxG = 0, finalMaxB = 0;
        int32_t finalMinR = maxVal, finalMinG = maxVal, finalMinB = maxVal;
        int32_t finalCount = 0;

        localStats.combine_each([&](const LocalStat& local) {
            for (size_t k = 0; k < Rhistogram.size(); ++k) {
                Rhistogram[k] += local.rHist[k];
                Ghistogram[k] += local.gHist[k];
                Bhistogram[k] += local.bHist[k];
            }
            if (local.maxR > finalMaxR) finalMaxR = local.maxR;
            if (local.maxG > finalMaxG) finalMaxG = local.maxG;
            if (local.maxB > finalMaxB) finalMaxB = local.maxB;

            if (local.minR < finalMinR) finalMinR = local.minR;
            if (local.minG < finalMinG) finalMinG = local.minG;
            if (local.minB < finalMinB) finalMinB = local.minB;

            finalSumR += local.sumR;
            finalSumG += local.sumG;
            finalSumB += local.sumB;
            finalCount += local.count;
            });

        maxPixel[0] = finalMaxR;
        maxPixel[1] = finalMaxG;
        maxPixel[2] = finalMaxB;
        minPixel[0] = finalMinR;
        minPixel[1] = finalMinG;
        minPixel[2] = finalMinB;
        maskNum = finalCount;

#elif defined(_OPENMP)
        // ----- OpenMP 并行版本 -----
        int32_t finalMaxR = 0, finalMaxG = 0, finalMaxB = 0;
        int32_t finalMinR = maxVal, finalMinG = maxVal, finalMinB = maxVal;
        int32_t finalCount = 0;

#pragma omp parallel
        {
            std::vector<double> localRHist(histSize, 0);
            std::vector<double> localGHist(histSize, 0);
            std::vector<double> localBHist(histSize, 0);
            int32_t localMaxR = 0, localMaxG = 0, localMaxB = 0;
            int32_t localMinR = maxVal, localMinG = maxVal, localMinB = maxVal;
            double localSumR = 0, localSumG = 0, localSumB = 0;
            int32_t localCount = 0;

#pragma omp for nowait
            for (int32_t i = 0; i < totalPixels; ++i) {
                if (useMask && mask && !(*mask)[i]) continue;

                const T* pix = pData + i * 3;
                int32_t r = static_cast<int32_t>(pix[0]);
                int32_t g = static_cast<int32_t>(pix[1]);
                int32_t b = static_cast<int32_t>(pix[2]);

                localRHist[r] += 1.0;
                localGHist[g] += 1.0;
                localBHist[b] += 1.0;

                if (r > localMaxR) localMaxR = r;
                if (g > localMaxG) localMaxG = g;
                if (b > localMaxB) localMaxB = b;

                if (r < localMinR) localMinR = r;
                if (g < localMinG) localMinG = g;
                if (b < localMinB) localMinB = b;

                localSumR += r;
                localSumG += g;
                localSumB += b;
                localCount++;
            }

#pragma omp critical
            {
                for (int32_t k = 0; k < histSize; ++k) {
                    Rhistogram[k] += localRHist[k];
                    Ghistogram[k] += localGHist[k];
                    Bhistogram[k] += localBHist[k];
                }
                if (localMaxR > finalMaxR) finalMaxR = localMaxR;
                if (localMaxG > finalMaxG) finalMaxG = localMaxG;
                if (localMaxB > finalMaxB) finalMaxB = localMaxB;

                if (localMinR < finalMinR) finalMinR = localMinR;
                if (localMinG < finalMinG) finalMinG = localMinG;
                if (localMinB < finalMinB) finalMinB = localMinB;

                finalSumR += localSumR;
                finalSumG += localSumG;
                finalSumB += localSumB;
                finalCount += localCount;
            }
        }

        maxPixel[0] = finalMaxR;
        maxPixel[1] = finalMaxG;
        maxPixel[2] = finalMaxB;
        minPixel[0] = finalMinR;
        minPixel[1] = finalMinG;
        minPixel[2] = finalMinB;
        maskNum = finalCount;

#else
        // ----- 串行版本 -----
        int32_t maxR = 0, maxG = 0, maxB = 0;
        int32_t minR = maxVal, minG = maxVal, minB = maxVal;

        for (int32_t i = 0; i < totalPixels; ++i) {
            if (useMask && mask && !(*mask)[i]) continue;

            const T* pix = pData + i * 3;
            int32_t r = static_cast<int32_t>(pix[0]);
            int32_t g = static_cast<int32_t>(pix[1]);
            int32_t b = static_cast<int32_t>(pix[2]);

            Rhistogram[r] += 1.0;
            Ghistogram[g] += 1.0;
            Bhistogram[b] += 1.0;

            if (r > maxR) maxR = r;
            if (g > maxG) maxG = g;
            if (b > maxB) maxB = b;

            if (r < minR) minR = r;
            if (g < minG) minG = g;
            if (b < minB) minB = b;

            finalSumR += r;
            finalSumG += g;
            finalSumB += b;
            maskNum++;
        }

        maxPixel[0] = maxR;
        maxPixel[1] = maxG;
        maxPixel[2] = maxB;
        minPixel[0] = minR;
        minPixel[1] = minG;
        minPixel[2] = minB;
#endif
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

        maxPixel.assign(3, 0.0);
        minPixel.assign(3, static_cast<double>(histSize - 1));
        avePixel.assign(3, 0.0);

        double finalSumR = 0, finalSumG = 0, finalSumB = 0;
        int32_t maskNum = 0;

        // 根据位深选择模板实例
        if (info.bit <= 8) {
            computeHistogram_RGB_impl<uint8_t>(info,
                static_cast<const uint8_t*>(data), mask, useMask,
                Rhistogram, Ghistogram, Bhistogram,
                maxPixel, minPixel, avePixel,
                finalSumR, finalSumG, finalSumB, maskNum);
        }
        else if (info.bit <= 16) {
            computeHistogram_RGB_impl<uint16_t>(info,
                reinterpret_cast<const uint16_t*>(data), mask, useMask,
                Rhistogram, Ghistogram, Bhistogram,
                maxPixel, minPixel, avePixel,
                finalSumR, finalSumG, finalSumB, maskNum);
        }
        else if (info.bit <= 31) {
            computeHistogram_RGB_impl<uint32_t>(info,
                reinterpret_cast<const uint32_t*>(data), mask, useMask,
                Rhistogram, Ghistogram, Bhistogram,
                maxPixel, minPixel, avePixel,
                finalSumR, finalSumG, finalSumB, maskNum);
        }
        else {
            return false;
        }

        // 计算平均值
        if (maskNum > 0) {
            avePixel[0] = finalSumR / maskNum;
            avePixel[1] = finalSumG / maskNum;
            avePixel[2] = finalSumB / maskNum;
        }

        return true;
    }

    template<typename T>
    void computeHistogram_RGBTrans_impl(const ImageShowInfo& info, const T* pData, std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& histogram, StretchType type, float maxValF) {
        const int32_t totalPixels = info.width * info.height;
        const int32_t maxBitValue = (1 << info.bit);

        // 获取 CPU 核心数
        static int32_t cpuCores = []() {
            int32_t cores = static_cast<int32_t>(std::thread::hardware_concurrency());
            return cores > 0 ? cores : 4;  // 若获取失败，默认 4 核
            }();
        //计算并行计算阈值
        const int32_t threshold = getAdaptiveParallelThreshold(cpuCores, type);
        if (totalPixels < threshold) {
            for (int32_t i = 0; i < totalPixels; ++i) {
                if (useMask && mask && !(*mask)[i]) continue;

                const T* pix = pData + i * 3;
                RgbPixel rgb{ static_cast<int32_t>(pix[0]),
                              static_cast<int32_t>(pix[1]),
                              static_cast<int32_t>(pix[2]) };
                int index = getRGBTransPixelOneFast(rgb, type, maxValF);
                histogram[index] += 1.0;
            }
        }

#if defined(_MSC_VER)
        concurrency::combinable<std::vector<double>> localHists([maxBitValue]() {
            return std::vector<double>(maxBitValue, 0.0);
            });

        concurrency::parallel_for(0, totalPixels, [&](int32_t i) {
            if (useMask && mask && !(*mask)[i]) return;

            const T* pix = pData + i * 3;
            RgbPixel rgb{ static_cast<int32_t>(pix[0]),
                          static_cast<int32_t>(pix[1]),
                          static_cast<int32_t>(pix[2]) };
            int index = getRGBTransPixelOneFast(rgb, type, maxValF);

            auto& local = localHists.local();
            local[index] += 1.0;
            });

        localHists.combine_each([&](const std::vector<double>& local) {
            for (size_t k = 0; k < histogram.size(); ++k) {
                histogram[k] += local[k];
            }
            });

#elif defined(_OPENMP)
#pragma omp parallel
        {
            std::vector<double> localHist(maxBitValue, 0.0);
#pragma omp for nowait
            for (int32_t i = 0; i < totalPixels; ++i) {
                if (useMask && mask && !(*mask)[i]) continue;

                const T* pix = pData + i * 3;
                RgbPixel rgb{ static_cast<int32_t>(pix[0]),
                              static_cast<int32_t>(pix[1]),
                              static_cast<int32_t>(pix[2]) };
                int index = getRGBTransPixelOneFast(rgb, type, maxValF);
                localHist[index] += 1.0;
            }
#pragma omp critical
            {
                for (int32_t k = 0; k < maxBitValue; ++k) {
                    histogram[k] += localHist[k];
                }
            }
        }
#else
        for (int32_t i = 0; i < totalPixels; ++i) {
            if (useMask && mask && !(*mask)[i]) continue;

            const T* pix = pData + i * 3;
            RgbPixel rgb{ static_cast<int32_t>(pix[0]),
                          static_cast<int32_t>(pix[1]),
                          static_cast<int32_t>(pix[2]) };
            int index = getRGBTransPixelOneFast(rgb, type, maxValF);
            histogram[index] += 1.0;
        }
#endif
    }
    bool computeHistogram_RGBTrans(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask, std::vector<double>& histogram, StretchType type /*= stretch_None*/) {
        if (!data || info.format != RGB) {
            return false;
        }

        const int32_t maxBitValue = (1 << info.bit);
        const float maxValF = static_cast<float>(maxBitValue - 1);
        histogram.assign(maxBitValue, 0.0);

        if (info.bit <= 8) {
            computeHistogram_RGBTrans_impl<uint8_t>(info,
                static_cast<const uint8_t*>(data), mask, useMask,
                histogram, type, maxValF);
        }
        else if (info.bit <= 16) {
            computeHistogram_RGBTrans_impl<uint16_t>(info,
                reinterpret_cast<const uint16_t*>(data), mask, useMask,
                histogram, type, maxValF);
        }
        else if (info.bit <= 31) {
            computeHistogram_RGBTrans_impl<uint32_t>(info,
                reinterpret_cast<const uint32_t*>(data), mask, useMask,
                histogram, type, maxValF);
        }
        else {
            return false;
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