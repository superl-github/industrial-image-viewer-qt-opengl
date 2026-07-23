#include "CyMediaCalc_Rgb.h"
#include "CyMediaCalc.h"

#include <algorithm>
#include <cmath>

#if defined _MSC_VER
#include <ppl.h>
#include <thread>
#elif defined _OPENMP
#   include <omp.h>
#endif

namespace CyMediaCalc_RGB {
    // 获取自适应并行阈值
    // @param coreCount       CPU 逻辑核心数
    // @param type            拉伸类型，用于调整复杂度权重
    // @return                建议的最小总像素数，低于此值应使用串行
    int32_t get_parallel_threshold_stretch(int32_t coreCount, StretchType type) {
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
    template<typename T>
    RgbPixel getPixelValue(const T* data, int32_t pixelI);
    RgbPixel calcCoordinateColor(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y);

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
        const int32_t threshold = get_parallel_threshold_stretch(cpuCores, stretch_None);
        bool bseria = totalPixels < threshold;
        //并行计算
        if (false == bseria) {
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
            //退回串行
            bseria = true;
#endif
        }

        //串行
        if (bseria) {
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
    }

    bool computeHistogram(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram,
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
    void computeHistogram_RGBTrans_impl(const ImageShowInfo& info, const T* pData,
        std::vector<double>& histogram, StretchType type, float maxValF) {
        const int32_t totalPixels = info.width * info.height;
        const int32_t maxBitValue = (1 << info.bit);

        // 获取 CPU 核心数
        static int32_t cpuCores = []() {
            int32_t cores = static_cast<int32_t>(std::thread::hardware_concurrency());
            return cores > 0 ? cores : 4;  // 若获取失败，默认 4 核
            }();
        //计算并行计算阈值
        const int32_t threshold = get_parallel_threshold_stretch(cpuCores, type);
        if (totalPixels < threshold) {
            for (int32_t i = 0; i < totalPixels; ++i) {
                const T* pix = pData + i * 3;
                RgbPixel rgb{ pix[0],
                              pix[1],
                              pix[2] };
                int index = CyMediaCalc::RGBT2StretchOneFast(rgb, type, maxValF);
                histogram[index] += 1.0;
            }
            return;
        }

#if defined(_MSC_VER)
        concurrency::combinable<std::vector<double>> localHists([maxBitValue]() {
            return std::vector<double>(maxBitValue, 0.0);
            });

        concurrency::parallel_for(0, totalPixels, [&](int32_t i) {
            const T* pix = pData + i * 3;
            RgbPixel rgb{ pix[0],
                          pix[1],
                          pix[2] };
            int index = CyMediaCalc::RGBT2StretchOneFast(rgb, type, maxValF);

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
                int index = CyMediaCalc::RGBT2StretchOneFast(rgb, type, maxValF);
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
            int index = CyMediaCalc::RGBT2StretchOneFast(rgb, type, maxValF);
            histogram[index] += 1.0;
        }
#endif
    }
    bool computeHistogram_Stretch(const ImageShowInfo& info, const uint8_t* data, std::vector<double>& histogram, StretchType type /*= stretch_None*/) {
        if (!data || info.format != RGB) {
            return false;
        }

        const uint32_t bitMax = (1 << info.bit);
        if (type == stretch_Lab) {
            histogram.assign(100, 0.0);
        }
        else {
            histogram.assign(bitMax, 0.0);
        }

        if (info.bit <= 8) {
            computeHistogram_RGBTrans_impl<uint8_t>(info,
                static_cast<const uint8_t*>(data),
                histogram, type, bitMax);
        }
        else if (info.bit <= 16) {
            computeHistogram_RGBTrans_impl<uint16_t>(info,
                reinterpret_cast<const uint16_t*>(data),
                histogram, type, bitMax);
        }
        else if (info.bit <= 31) {
            computeHistogram_RGBTrans_impl<uint32_t>(info,
                reinterpret_cast<const uint32_t*>(data),
                histogram, type, bitMax);
        }
        else {
            return false;
        }

        return true;
    }

    template<typename T>
    CyMedia::RgbPixel getPixelValue(const T* data, int32_t pixelI) {
        return {
            data[pixelI * 3 + 0],
            data[pixelI * 3 + 1],
            data[pixelI * 3 + 2]
        };
    }

    CyMedia::RgbPixel calcCoordinateColor(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y) {
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
}