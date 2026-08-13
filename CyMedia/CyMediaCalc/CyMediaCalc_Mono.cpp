#include "CyMediaCalc_Mono.h"
#if defined _MSC_VER
    #include <ppl.h>
    #include <thread>
    #include <numeric>
#elif defined _OPENMP
#   include <omp.h>
#endif
#include <cstdlib>

namespace CyMediaCalc_Mono {
    //辅助函数
    inline uint32_t getMonoPixelValue(const ImageShowInfo& info, const uint8_t* data, size_t idx);
    bool MonoConvert(const ImageShowInfo& info, const uint8_t* data, uint16_t* outdata);
    bool MonoConvert_8(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata);

    int32_t calcCoordinateColor_Mono(ImageShowInfo& info, uint8_t* pdata, size_t idx) {
        if (info.special_pixel == PIXEL_VALUE_F32) {
            return ((float*)pdata)[idx];
        }
        else if (info.special_pixel == PIXEL_VALUE_F64) {
            return ((double*)pdata)[idx];
        }

        if (info.bit <= 8) {
            return ((uint8_t*)pdata)[idx];
        }
        if (info.bit <= 16) {
            return ((uint16_t*)pdata)[idx]/* & int((1U << info.bit) - 1)*/;
        }
        if (info.bit <= 31) {
            return ((uint32_t*)pdata)[idx]/* & int((1U << info.bit) - 1)*/;
        }

        return 0;
    }

    int32_t calcCoordinateColor_Mono10P(const uint8_t* data, size_t idx) {
        size_t byteIdx = (idx * 10) / 8;
        size_t bitOffset = (idx * 10) % 8;
        uint16_t value = (data[byteIdx] | (data[byteIdx + 1] << 8) | (data[byteIdx + 2] << 16)) >> bitOffset;
        return value/* & 0x3FF*/;
    }

    int32_t calcCoordinateColor_Mono12P(const uint8_t* data, size_t idx) {
        size_t byteIdx = (idx * 12) / 8;
        size_t bitOffset = (idx * 12) % 8;
        uint32_t value = (data[byteIdx] | (data[byteIdx + 1] << 8) | (data[byteIdx + 2] << 16)) >> bitOffset;
        return value/* & 0xFFF*/;
    }

    int32_t calcCoordinateColor_Mono10P_GVSP(const uint8_t* data, size_t idx) {
        return calcCoordinateColor_Mono10P(data, idx);
    }

    int32_t calcCoordinateColor_Mono12P_GVSP(const uint8_t* data, size_t idx) {
        return calcCoordinateColor_Mono12P(data, idx);
    }

    bool computeHistogram(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask, std::vector<double>& histogram,
        double* maxPixel, double* minPixel, double* avePixel) {
        if (info.special_pixel != PIXEL_VALUE_INT) {
            return false;
        }

        if (!data || info.width == 0 || info.height == 0) {
            return false;
        }

        // 判断是否为 Mono 类型
        bool isMono = (info.format == MONO) || (info.format == MONO_OVERSIZE);
        bool isMono10P = (info.format == MONO10P || info.format == MONO10P_GVSP);
        bool isMono12P = (info.format == MONO12P || info.format == MONO12P_GVSP);

        if (!(isMono || isMono10P || isMono12P)) {
            return false; // 不支持非 Mono 格式
        }

        size_t pixelCount = static_cast<size_t>(info.width) * info.height;
        size_t calcCount = pixelCount;
        if (useMask) {
            calcCount = 0;
            for (auto& onePixel : *mask) {
                calcCount += onePixel;
            }
            calcCount /= 255;
        }

        // 确定最大灰度值（决定直方图 bins 数）
        uint32_t maxVal = 0;
        if (info.bit <= 8) {
            maxVal = 255;
        }
        else if (info.bit <= 31) {
            maxVal = (1U << info.bit) - 1;
        }
        else {
            return false;
        }
        uint32_t tMax = 0, tMin = maxVal;
        double tAve = 0.0;

        size_t numBins = maxVal + 1;
        histogram.assign(numBins, 0.0);

        // 获取硬件并发线程数，用于分块
        unsigned int nThreads = std::thread::hardware_concurrency();
        if (nThreads == 0) nThreads = 4;
        size_t chunkSize = std::max(pixelCount / (nThreads * 4), size_t(1024)); // 每块至少 1k 像素
        // 并行处理像素块
#ifdef _MSC_VER
        concurrency::combinable<std::vector<size_t>> localHist(
            [&] { return std::vector<size_t>(numBins, 0); }
        );
        concurrency::combinable<uint64_t> localCount([] { return uint64_t(0); });
        concurrency::combinable<double>   localSum([] { return 0.0; });
        concurrency::combinable<uint32_t> localMax([] { return uint32_t(0); });
        concurrency::combinable<uint32_t> localMin([maxVal] { return maxVal; });

        concurrency::parallel_for(size_t(0), pixelCount, chunkSize, [&](size_t start) {
            size_t end = std::min(start + chunkSize, pixelCount);

            auto& hist = localHist.local();
            uint64_t& cnt = localCount.local();
            double& sum = localSum.local();
            uint32_t& lmax = localMax.local();
            uint32_t& lmin = localMin.local();

            if (useMask && mask) {
                for (size_t i = start; i < end; ++i) {
                    if ((*mask)[i]) {
                        uint32_t gray = getMonoPixelValue(info, data, i);
                        if (gray > maxVal) gray = maxVal;

                        hist[gray]++;
                        cnt++;
                        sum += gray;
                        if (lmax < gray) lmax = gray;
                        if (lmin > gray) lmin = gray;
                    }
                }
            }
            else {
                for (size_t i = start; i < end; ++i) {
                    uint32_t gray = getMonoPixelValue(info, data, i);
                    if (gray > maxVal) gray = maxVal;

                    hist[gray]++;
                    cnt++;
                    sum += gray;
                    if (lmax < gray) lmax = gray;
                    if (lmin > gray) lmin = gray;
                }
            }
            });

        // 合并直方图（使用 combine_each）
        histogram.assign(numBins, 0.0);
        localHist.combine_each([&](const std::vector<size_t>& h) {
            for (size_t i = 0; i < numBins; ++i) {
                histogram[i] += static_cast<double>(h[i]);
            }
            });

        // 合并统计值（使用 combine_each）
        uint64_t validCount = 0;
        double totalSum = 0.0;

        localCount.combine_each([&](uint64_t c) { validCount += c; });
        localSum.combine_each([&](double s) { totalSum += s; });
        localMax.combine_each([&](uint32_t m) { if (m > tMax) tMax = m; });
        localMin.combine_each([&](uint32_t m) { if (m < tMin) tMin = m; });

        tAve = validCount ? totalSum / validCount : 0.0;
#else
        // 遍历所有像素
        if (useMask && mask) {
            for (size_t i = 0; i < pixelCount; ++i) {
                if ((*mask)[i]) {
                    uint32_t gray = getMonoPixelValue(info, data, i);
                    if (gray > maxVal) gray = maxVal;

                    histogram[gray]++;

                    tAve += gray;
                    if (tMax < gray) {
                        tMax = gray;
                    }
                    if (tMin > gray) {
                        tMin = gray;
                    }
                }
            }
        }
        else {
            for (size_t i = 0; i < pixelCount; ++i) {
                uint32_t gray = getMonoPixelValue(info, data, i);
                if (gray > maxVal) gray = maxVal;

                histogram[gray]++;

                tAve += gray;
                if (tMax < gray) {
                    tMax = gray;
                }
                if (tMin > gray) {
                    tMin = gray;
                }
            }
        }
        tAve /= calcCount;
#endif
        if (avePixel) *avePixel = tAve;
        if (maxPixel) *maxPixel = tMax;
        if (minPixel) *minPixel = tMin;

        return true;
    }

    void computerUniformity(const std::vector<double>& histogram, const double& ave, double& maxBitColor, double* std, double* uniformity, int* hisXRangeMax) {
        double pixcelCount = 0.0;
        double d = 0.0;
        double dstd = 0.0;
        double dSn = 0.0;
        int maxAvailableHisI = 0;
        for (int hisI = 0; hisI < histogram.size(); hisI++) {
            if (histogram[hisI] <= 0.0) {
                continue;
            }
            maxAvailableHisI = hisI;
            d = (hisI - ave);
            dSn += histogram[hisI] * (d * d);
            pixcelCount += histogram[hisI];
        }

        dSn = sqrt(dSn / pixcelCount);
        dstd = dSn;
        if (dSn != 0.0) {
            dSn = maxBitColor / dSn;
            dSn = 20 * log10(dSn);
        }

        if (std) *std = dstd;
        if (uniformity) *uniformity = dSn;
        if (hisXRangeMax) *hisXRangeMax = maxAvailableHisI;
    }

    bool Mono10P2MonoConver(const ImageShowInfo& info, const uint8_t* data, uint16_t* outdata) {
        if (info.format != MONO10P)
            return false;
        return MonoConvert(info, data, outdata);
    }

    bool Mono12P2MonoConver(const ImageShowInfo& info, const uint8_t* data, uint16_t* outdata) {
        if (info.format != MONO12P)
            return false;
        return MonoConvert(info, data, outdata);
    }

    bool Mono10P_GVSP2MonoConver(const ImageShowInfo& info, const uint8_t* data, uint16_t* outdata) {
        if (info.format != MONO10P_GVSP)
            return false;
        return MonoConvert(info, data, outdata);
    }

    bool Mono12P_GVSP2MonoConver(const ImageShowInfo& info, const uint8_t* data, uint16_t* outdata) {
        if (info.format != MONO12P_GVSP)
            return false;
        return MonoConvert(info, data, outdata);
    }

    bool MonoUmPack_8(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata) {
        if (info.format >= MONO10P &&
            info.format <= MONO12P_GVSP) {
            return MonoConvert_8(info, data, outdata);
        }
        return false;
    }






    inline uint32_t getMonoPixelValue(const ImageShowInfo& info, const uint8_t* data, size_t idx) {
        if (info.format == MONO10P) {
            return calcCoordinateColor_Mono10P(data, idx);
        }
        if (info.format == MONO10P_GVSP) {
            return calcCoordinateColor_Mono10P_GVSP(data, idx);
        }
        if (info.format == MONO12P) {
            return calcCoordinateColor_Mono12P(data, idx);
        }
        if (info.format == MONO12P_GVSP) {
            return calcCoordinateColor_Mono12P_GVSP(data, idx);
        }
        // MONO
        if (info.bit <= 8) {
            return data[idx];
        }
        else if (info.bit <= 16) {
            uint16_t val;
            std::memcpy(&val, data + idx * 2, sizeof(val));
            return val;
        }
        else if (info.bit <= 31) {
            uint32_t val;
            std::memcpy(&val, data + idx * 4, sizeof(val));
            return val;
        }
        return 0; // fallback
    }
    bool MonoConvert(const ImageShowInfo& info, const uint8_t* data, uint16_t* outdata) {
        if (!data || !outdata || info.width <= 0 || info.height <= 0) {
            return false;
        }

        // 判断是否为 Mono 类型
        bool isMono = (info.format == MONO) || (info.format == MONO_OVERSIZE);
        bool isMono10P = (info.format == MONO10P || info.format == MONO10P_GVSP);
        bool isMono12P = (info.format == MONO12P || info.format == MONO12P_GVSP);

        if (!(isMono || isMono10P || isMono12P)) {
            return false; // 不支持非 Mono 格式
        }

        size_t pixelCount = static_cast<size_t>(info.width) * info.height;

        // 确定最大灰度值（决定直方图 bins 数）
        uint32_t maxVal = 0;
        if (isMono10P || (isMono && info.bit == 10)) {
            maxVal = 1023; // 2^10 - 1
        }
        else if (isMono12P || (isMono && info.bit == 12)) {
            maxVal = 4095; // 2^12 - 1
        }
        else if (isMono && info.bit <= 16) {
            maxVal = (1U << info.bit) - 1;
            if (maxVal > 65535) maxVal = 65535; // 安全上限
        }
        else {
            return false; // 不支持的 bit depth
        }

#ifdef _OPENMP
#pragma omp for schedule(dynamic)
            for (size_t i = 0; i < pixelCount; ++i) {
                uint32_t gray = getMonoPixelValue(info, data, i);
                if (gray > maxVal) gray = maxVal;
                outdata[i] = gray;
            }
#elif defined (_MSC_VER)
        // 获取硬件并发线程数，用于分块
        unsigned int nThreads = std::thread::hardware_concurrency();
        if (nThreads == 0) nThreads = 4;
        size_t chunkSize = std::max(pixelCount / (nThreads * 4), size_t(1024)); // 每块至少 1k 像素

        // 并行处理像素块
        concurrency::parallel_for(size_t(0), pixelCount, chunkSize, [&](size_t start) {
            size_t end = std::min(start + chunkSize, pixelCount);
            unsigned int tid = concurrency::Context::CurrentContext()->GetVirtualProcessorId() % nThreads;

            for (int32_t i = start; i < end; ++i) {
                int32_t gray = getMonoPixelValue(info, data, i);
                if (gray > maxVal) gray = maxVal; // 安全 clamp
                outdata[i] = gray;
            }
            });
#else
        // 遍历所有像素
        for (int32_t i = 0; i < pixelCount; ++i) {
            uint32_t gray = getMonoPixelValue(info, data, i);
            if (gray > maxVal) gray = maxVal;
            outdata[i] = gray;
        }
#endif

        return true;
    }
    bool MonoConvert_8(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata) {
        if (!data || !outdata || info.width <= 0 || info.height <= 0) {
            return false;
        }

        // 判断是否为 Mono 类型
        bool isMono = (info.format == MONO) || (info.format == MONO_OVERSIZE);
        bool isMono10P = (info.format == MONO10P || info.format == MONO10P_GVSP);
        bool isMono12P = (info.format == MONO12P || info.format == MONO12P_GVSP);

        if (!(isMono || isMono10P || isMono12P)) {
            return false; // 不支持非 Mono 格式
        }

        size_t pixelCount = static_cast<size_t>(info.width) * info.height;

        // 确定最大灰度值（决定直方图 bins 数）
        float bitMax = 0;
        uint32_t maxVal = 0;
        uint32_t outMaxVal = 255;
        float scale = 0;
        if (isMono10P || (isMono && info.bit == 10)) {
            bitMax = 1024;
        }
        else if (isMono12P || (isMono && info.bit == 12)) {
            bitMax = 4096;
        }
        else if (isMono && info.bit <= 16) {
            bitMax = (1U << info.bit);
            if (maxVal > 65536) maxVal = 65536; // 安全上限
        }
        else {
            return false; // 不支持的 bit depth
        }
        maxVal = bitMax - 1.0;
        scale = 256 * 1.0 / bitMax;

#ifdef _OPENMP
#pragma omp for schedule(dynamic)
        for (size_t i = 0; i < pixelCount; ++i) {
            uint32_t gray = getMonoPixelValue(info, data, i) * scale;
            if (gray > outMaxVal) gray = outMaxVal;
            outdata[i] = gray;
        }
#elif defined (_MSC_VER)
        // 获取硬件并发线程数，用于分块
        unsigned int nThreads = std::thread::hardware_concurrency();
        if (nThreads == 0) nThreads = 4;
        size_t chunkSize = std::max(pixelCount / (nThreads * 4), size_t(1024)); // 每块至少 1k 像素

        // 并行处理像素块
        concurrency::parallel_for(size_t(0), pixelCount, chunkSize, [&](size_t start) {
            size_t end = std::min(start + chunkSize, pixelCount);
            unsigned int tid = concurrency::Context::CurrentContext()->GetVirtualProcessorId() % nThreads;

            for (int32_t i = start; i < end; ++i) {
                int32_t gray = getMonoPixelValue(info, data, i) * scale;
                if (gray > outMaxVal) gray = maxVal; // 安全 clamp
                outdata[i] = gray;
            }
            });
#else
        // 遍历所有像素
        for (int32_t i = 0; i < pixelCount; ++i) {
            uint32_t gray = getMonoPixelValue(info, data, i) * scale;
            if (gray > outMaxVal) gray = outMaxVal;
            outdata[i] = gray;
        }
#endif

        return true;
    }
}