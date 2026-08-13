#include "CyMediaCalc_Bayer.h"
#include "CyMediaCalc.h"

#include <algorithm>
#include <cstdint>

#if defined _MSC_VER
#include <ppl.h>
#include <thread>
#elif defined _OPENMP
#   include <omp.h>
#endif

namespace CyMediaCalc_Bayer {
    // 获取自适应并行阈值
    // @param coreCount       CPU 逻辑核心数
    // @param func            转换类型，用于调整复杂度权重
    // @return                建议的最小总像素数，低于此值应使用串行
    inline int32_t get_parallel_threshold_bayer_conver(int32_t coreCount, DemosaicingMethod func) {
        // 基准：每个核心至少处理 15000 像素才值得并行
        constexpr int32_t BASE_PIXELS_PER_CORE = 15000;
        // 根据计算复杂度调整系数
        double complexityFactor = 1.0;
        switch (func) {
        case CyMedia::DEMOSAIC_NONE:
        case CyMedia::DEMOSAIC_BILINEAR:
            complexityFactor = 1.2;   // 简单计算，阈值稍高
            break;
        case CyMedia::DEMOSAIC_MALVA:
            complexityFactor = 0.9;   // 中等复杂度
        case CyMedia::DEMOSAIC_AHD:
            complexityFactor = 0.5;   // 很重，阈值降低一半
            break;
        }

        int32_t threshold = static_cast<int32_t>(coreCount * BASE_PIXELS_PER_CORE * complexityFactor);

        // 限制在合理范围内（避免极端情况）
        constexpr int32_t MIN_THRESHOLD = 20000;   // 至少 2 万像素
        constexpr int32_t MAX_THRESHOLD = 800000;  // 最多 80 万像素
        return std::clamp(threshold, MIN_THRESHOLD, MAX_THRESHOLD);
    }

    static RGBChannel getBayerColorType(ePixType fmt, int x, int y);
    RgbPixel getBayerPixelColor_Bilinear(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y);
    RgbPixel getBayerPixelColor_Malvar(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y);
    RgbPixel getBayerPixelColor_Malvar(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y);
    inline static RgbPixel demosaicPixelAHD(const std::vector<double>& R_plane, const std::vector<double>& G_plane, const std::vector<double>& B_plane, int32_t width, int32_t height, ePixType format, int32_t x, int32_t y);
    template<typename T>
    void demosaicAHD_Split(const T* data, int32_t width, int32_t height, ePixType format, std::vector<double>& R, std::vector<double>& G, std::vector<double>& B);
    bool computeHistogram_Bayer_AHD(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel);
    bool computeHistogram_Bayer_AHD_Stretch(const ImageShowInfo& info, const uint8_t* data, std::vector<double>& Rhistogram, CyMedia::StretchType type, bool bParallel);
    template<typename T>
    bool Bayer2RGB(const ImageShowInfo& info, const uint8_t* data, T* outdata, DemosaicingMethod func = DEMOSAIC_BILINEAR);
    bool Bayer2RGB_8(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, DemosaicingMethod func = DEMOSAIC_BILINEAR);
    template<typename T>
    bool Bayer2RGB_AHD(const ImageShowInfo& info, const uint8_t* data, T* outdata, bool sertial = true);
    bool Bayer2RGB_AHD_8(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, bool sertial = true);


    RgbPixel calcCoordinateColor(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y, DemosaicingMethod func/* = BILINEAR*/) {
        if (!pdata || x >= info.width || y >= info.height) {
            return {};
        }

        switch (func) {
            case CyMedia::DEMOSAIC_NONE: {
                auto color = safeAt(pdata, info.bit, info.width, info.height, x, y);
                return RgbPixel{ color, color, color };
            }
            case CyMedia::DEMOSAIC_BILINEAR:
                return getBayerPixelColor_Bilinear(info, pdata, x, y);
            case CyMedia::DEMOSAIC_MALVA:
                return getBayerPixelColor_Malvar(info, pdata, x, y);
            case CyMedia::DEMOSAIC_AHD:
                return getBayerPixelColor_Malvar(info, pdata, x, y);
        }

        return {};
    }

    bool computeHistogram(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask, 
        std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel, 
        DemosaicingMethod func/* = BILINEAR*/) {
        if (false == info.isBayer()) return false;
        if (!data) return false;if (info.bit >= 32) return false;
        
        int32_t bitMax = (1U << info.bit) - 1;
        int32_t pixelCount = info.width * info.height;
        int32_t validCount = 0;

        //初始化容器
        Rhistogram.assign(bitMax, 0.0);
        Ghistogram.assign(bitMax, 0.0);
        Bhistogram.assign(bitMax, 0.0);
        double* pRhi = Rhistogram.data();
        double* pGhi = Ghistogram.data();
        double* pBhi = Bhistogram.data();

        maxPixel.assign(3, 0.0);
        minPixel.assign(3, bitMax);
        avePixel.assign(3, 0.0);
        double* pMax = maxPixel.data();
        double* pMin = minPixel.data();
        double* pAve = avePixel.data();

        switch (func) {
            case CyMedia::DEMOSAIC_NONE:
            case CyMedia::DEMOSAIC_BILINEAR:
            case CyMedia::DEMOSAIC_MALVA: {
                if (useMask && mask) {
                    for (uint32_t y = 0; y < info.height; ++y) {
                        for (uint32_t x = 0; x < info.width; ++x) {
                            if (mask->at(y * info.width + x)) {
                                RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                                pRhi[px.r]++;
                                pGhi[px.g]++;
                                pBhi[px.b]++;

                                pAve[0] += px.r;
                                pAve[1] += px.g;
                                pAve[2] += px.b;

                                if (pMax[0] < px.r) {
                                    pMax[0] = px.r;
                                }
                                if (pMax[1] < px.g) {
                                    pMax[1] = px.g;
                                }
                                if (pMax[2] < px.b) {
                                    pMax[2] = px.b;
                                }

                                if (pMin[0] > px.r) {
                                    pMin[0] = px.r;
                                }
                                if (pMin[1] > px.g) {
                                    pMin[1] = px.g;
                                }
                                if (pMin[2] > px.b) {
                                    pMin[2] = px.b;
                                }
                                validCount++;
                            }
                        }
                    }
                }
                else {
                    validCount = pixelCount;
                    for (uint32_t y = 0; y < info.height; ++y) {
                        for (uint32_t x = 0; x < info.width; ++x) {
                            RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                            pRhi[px.r]++;
                            pGhi[px.g]++;
                            pBhi[px.b]++;

                            pAve[0] += px.r;
                            pAve[1] += px.g;
                            pAve[2] += px.b;

                            if (pMax[0] < px.r) {
                                pMax[0] = px.r;
                            }
                            if (pMax[1] < px.g) {
                                pMax[1] = px.g;
                            }
                            if (pMax[2] < px.b) {
                                pMax[2] = px.b;
                            }

                            if (pMin[0] > px.r) {
                                pMin[0] = px.r;
                            }
                            if (pMin[1] > px.g) {
                                pMin[1] = px.g;
                            }
                            if (pMin[2] > px.b) {
                                pMin[2] = px.b;
                            }
                        }
                    }
                }
                pAve[0] /= validCount;
                pAve[1] /= validCount;
                pAve[2] /= validCount;
            }break;

            case CyMedia::DEMOSAIC_AHD: {
                computeHistogram_Bayer_AHD(info, data, mask, useMask, Rhistogram, Ghistogram, Bhistogram, maxPixel, minPixel, avePixel);
            }break;
        }
        return true;
    }

    bool computeHistogram_Stretch(const ImageShowInfo& info, const uint8_t* data, std::vector<double>& Rhistogram, DemosaicingMethod func /*= DEMOSAIC_BILINEAR*/, CyMedia::StretchType type /*= stretch_None*/) {
        if (false == info.isBayer()) return false;
        if (!data) return false;if (info.bit >= 32) return false;

        int32_t bitMax = (1U << info.bit) - 1;
        int32_t pixelCount = info.width * info.height;
        //初始化容器
        Rhistogram.assign(bitMax, 0.0);
        double* pRhi = Rhistogram.data();
        // ---- 并行阈值判断 ----
        static int32_t cpuCores = []() {
            int32_t cores = static_cast<int32_t>(std::thread::hardware_concurrency());
            return cores > 0 ? cores : 4;
            }();
        const int32_t threshold = get_parallel_threshold_bayer_conver(cpuCores, func);
        bool bParallel = (pixelCount >= threshold);

        switch (func)  {
            case CyMedia::DEMOSAIC_NONE:
            case CyMedia::DEMOSAIC_BILINEAR:
            case CyMedia::DEMOSAIC_MALVA: {
                if (bParallel) {
#if defined _MSC_VER
                    // ---- PPL 并行 ----
                    concurrency::combinable<std::vector<double>> localHist(
                        [&]() { return std::vector<double>(bitMax, 0.0); }
                    );
                    concurrency::parallel_for(0, info.height, [&](int y) {
                        auto& local = localHist.local();
                        for (int x = 0; x < info.width; ++x) {
                            RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                            int idx = CyMediaCalc::RGBT2StretchOneFast(px, type, bitMax);
                            local[idx] += 1.0;
                        }
                        });
                    localHist.combine_each([&](std::vector<double>& local) {
                        for (int i = 0; i < bitMax; ++i)
                            pRhi[i] += local[i];
                        });
                }
#elif defined _OPENMP
                    // ---- OpenMP 并行 ----
                    int numThreads = omp_get_max_threads();
                    std::vector<std::vector<double>> localHists(
                        numThreads, std::vector<double>(bitMax, 0.0)
                    );
#pragma omp parallel for
                    for (int y = 0; y < info.height; ++y) {
                        int tid = omp_get_thread_num();
                        auto& local = localHists[tid];
                        for (int x = 0; x < info.width; ++x) {
                            RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                            int idx = CyMediaCalc::RGBT2StretchOneFast(px, type, bitMax);
                            local[idx] += 1.0;
                        }
                    }
                    for (int t = 0; t < numThreads; ++t) {
                        for (int i = 0; i < bitMax; ++i)
                            pRhi[i] += localHists[t][i];
                    }
#else
                    // 无并行支持，回退串行
                    bParallel = false;
#endif
                if (false == bParallel) {
                    for (uint32_t y = 0; y < info.height; ++y) {
                        for (uint32_t x = 0; x < info.width; ++x) {
                            RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                            // 更新直方图
                            pRhi[CyMediaCalc::RGBT2StretchOneFast(px, type, bitMax)] += 1.0;
                        }
                    }
                }
            }break;

            case CyMedia::DEMOSAIC_AHD: {
                computeHistogram_Bayer_AHD_Stretch(info, data, Rhistogram, type, bParallel);
            }break;
        }
        return true;
    }

    bool bayer2RGB(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, DemosaicingMethod func /*= BILINEAR*/) {
        if (info.bit <= 8)
            return Bayer2RGB(info, data, (uint8_t*)outdata, func);
        else if (info.bit <= 16)
            return Bayer2RGB(info, data, (uint16_t*)outdata, func);
        else if (info.bit <= 31)
            return false;// Bayer2RGB(info, data, (uint32_t*)outdata, func);

        return false;
    }

    bool bayer2RGB_8(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, DemosaicingMethod func /*= DEMOSAIC_BILINEAR*/) {
        return Bayer2RGB_8(info, data, (uint8_t*)outdata, func);
    }





    inline RgbPixel getBayerPixelColor_Bilinear(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y) {
        uint32_t maxColor = (1U << info.bit) - 1;
        auto centerColor = getBayerColorType(info.format, x, y);
        int32_t centerVal = safeAt(pdata, info.bit, info.width, info.height, x, y);

        // 否则：双线性插值
        int cx = static_cast<int>(x), cy = static_cast<int>(y);

        uint32_t R = 0, G = 0, B = 0;
        if (centerColor == Ch_R) {
            R = centerVal;
            // G: 上下左右平均
            G = (safeAt(pdata, info.bit, info.width, info.height, cx - 1, cy) + safeAt(pdata, info.bit, info.width, info.height, cx + 1, cy) + safeAt(pdata, info.bit, info.width, info.height, cx, cy - 1) + safeAt(pdata, info.bit, info.width, info.height, cx, cy + 1) + 2) / 4;
            // B: 四角平均
            B = (safeAt(pdata, info.bit, info.width, info.height, cx - 1, cy - 1) + safeAt(pdata, info.bit, info.width, info.height, cx + 1, cy - 1) + safeAt(pdata, info.bit, info.width, info.height, cx - 1, cy + 1) + safeAt(pdata, info.bit, info.width, info.height, cx + 1, cy + 1) + 2) / 4;
        }
        else if (centerColor == Ch_B) {
            B = centerVal;
            G = (safeAt(pdata, info.bit, info.width, info.height, cx - 1, cy) + safeAt(pdata, info.bit, info.width, info.height, cx + 1, cy) + safeAt(pdata, info.bit, info.width, info.height, cx, cy - 1) + safeAt(pdata, info.bit, info.width, info.height, cx, cy + 1) + 2) / 4;
            R = (safeAt(pdata, info.bit, info.width, info.height, cx - 1, cy - 1) + safeAt(pdata, info.bit, info.width, info.height, cx + 1, cy - 1) + safeAt(pdata, info.bit, info.width, info.height, cx - 1, cy + 1) + safeAt(pdata, info.bit, info.width, info.height, cx + 1, cy + 1) + 2) / 4;
        }
        else { // G
            G = centerVal;
            // 区分 G 在 R 行还是 B 行
            bool isRGRow = false;
            if (info.format == BAYERRG || info.format == BAYERGR) {
                isRGRow = (cy % 2 == 0);
            }
            else if (info.format == BAYERBG || info.format == BAYERGB) {
                isRGRow = (cy % 2 == 1);
            }

            if (isRGRow) {
                // 当前 G 在 R 行 → 左右是 R，上下是 B
                R = (safeAt(pdata, info.bit, info.width, info.height, cx - 1, cy) + safeAt(pdata, info.bit, info.width, info.height, cx + 1, cy) + 1) / 2;
                B = (safeAt(pdata, info.bit, info.width, info.height, cx, cy - 1) + safeAt(pdata, info.bit, info.width, info.height, cx, cy + 1) + 1) / 2;
            }
            else {
                // 当前 G 在 B 行 → 左右是 B，上下是 R
                B = (safeAt(pdata, info.bit, info.width, info.height, cx - 1, cy) + safeAt(pdata, info.bit, info.width, info.height, cx + 1, cy) + 1) / 2;
                R = (safeAt(pdata, info.bit, info.width, info.height, cx, cy - 1) + safeAt(pdata, info.bit, info.width, info.height, cx, cy + 1) + 1) / 2;
            }
        }

        return {
            std::min(R, maxColor),
            std::min(G, maxColor),
            std::min(B, maxColor)
        };
    }

    static RGBChannel getBayerColorType(ePixType fmt, int x, int y) {
        switch (fmt) {
        case ePixType::BAYERRG: return ((y & 1) ? (x & 1 ? Ch_B : Ch_G) : (x & 1 ? Ch_G : Ch_R));
        case ePixType::BAYERGR: return ((y & 1) ? (x & 1 ? Ch_G : Ch_B) : (x & 1 ? Ch_R : Ch_G));
        case ePixType::BAYERBG: return ((y & 1) ? (x & 1 ? Ch_R : Ch_G) : (x & 1 ? Ch_G : Ch_B));
        case ePixType::BAYERGB: return ((y & 1) ? (x & 1 ? Ch_G : Ch_R) : (x & 1 ? Ch_B : Ch_G));
        default: return Ch_G;
        }
    }
    inline RgbPixel getBayerPixelColor_Malvar(const ImageShowInfo& info, const uint8_t* data, int32_t x, int32_t y) {
        uint32_t bitMax = (1U << info.bit);
        uint32_t colorMaX = bitMax - 1;
        int cx = static_cast<int>(x), cy = static_cast<int>(y);
        auto c = getBayerColorType(info.format, cx, cy);
        auto at = [&](int dx, int dy) { return safeAt(data, info.bit, info.width, info.height, cx + dx, cy + dy); };

        uint32_t R = 0, G = 0, B = 0;
        int center = at(0, 0);

        if (c == Ch_R) {
            R = center;
            int g_at_r = (at(-1, 0) + at(1, 0) + at(0, -1) + at(0, 1) + 2) / 4;
            int dg = (-at(-2, 0) + 4 * at(-1, 0) - 4 * at(1, 0) + at(2, 0)
                - at(0, -2) + 4 * at(0, -1) - 4 * at(0, 1) + at(0, 2) + 4) / 8;
            G = (g_at_r + dg) % bitMax;

            int b_at_r = (at(-1, -1) + at(1, -1) + at(-1, 1) + at(1, 1) + 2) / 4;
            int db = (at(-2, -2) - at(0, -2) + at(2, -2)
                - at(-2, 0) - 2 * at(0, 0) - at(2, 0)
                + at(-2, 2) - at(0, 2) + at(2, 2) + 4) / 8;
            B = (b_at_r + db) % bitMax;
        }
        else if (c == Ch_B) {
            B = center;
            int g_at_b = (at(-1, 0) + at(1, 0) + at(0, -1) + at(0, 1) + 2) / 4;
            int dg = (-at(-2, 0) + 4 * at(-1, 0) - 4 * at(1, 0) + at(2, 0)
                - at(0, -2) + 4 * at(0, -1) - 4 * at(0, 1) + at(0, 2) + 4) / 8;
            G = (g_at_b + dg) % bitMax;

            int r_at_b = (at(-1, -1) + at(1, -1) + at(-1, 1) + at(1, 1) + 2) / 4;
            int dr = (at(-2, -2) - at(0, -2) + at(2, -2)
                - at(-2, 0) - 2 * at(0, 0) - at(2, 0)
                + at(-2, 2) - at(0, 2) + at(2, 2) + 4) / 8;
            R = (r_at_b + dr) % bitMax;
        }
        else { // 'G'
            G = center;
            bool isRGRow = (info.format == ePixType::BAYERRG || info.format == ePixType::BAYERGR) ? ((cy & 1) == 0) : ((cy & 1) == 1);

            if (isRGRow) {
                R = (at(-1, 0) + at(1, 0) + 1) / 2;
                B = (at(0, -1) + at(0, 1) + 1) / 2;
                int dr = (-at(-2, 0) + 3 * at(-1, 0) - 3 * at(1, 0) + at(2, 0) + 2) / 4;
                int db = (-at(0, -2) + 3 * at(0, -1) - 3 * at(0, 1) + at(0, 2) + 2) / 4;
                R = (R + dr) % bitMax;
                B = (B + db) % bitMax;
            }
            else {
                B = (at(-1, 0) + at(1, 0) + 1) / 2;
                R = (at(0, -1) + at(0, 1) + 1) / 2;
                int db = (-at(-2, 0) + 3 * at(-1, 0) - 3 * at(1, 0) + at(2, 0) + 2) / 4;
                int dr = (-at(0, -2) + 3 * at(0, -1) - 3 * at(0, 1) + at(0, 2) + 2) / 4;
                B = (B + db) % bitMax;
                R = (R + dr) % bitMax;
            }
        }

        return {
            R,
            G,
            B
        };
    }

    inline RgbPixel getBayerPixelColor_AHD(const ImageShowInfo& info, const uint8_t* data, int32_t x, int32_t y, const std::vector<double>& R_plane, const std::vector<double>& G_plane, const std::vector<double>& B_plane) {
        if (!data || info.width == 0 || info.height == 0) return {};

        // 分离 R, G, B 平面（未采样位置为 0）
        std::vector<double> R(info.width * info.height, 0), G(info.width * info.height, 0), B(info.width * info.height, 0);
        if (info.bit <= 8) {
            demosaicAHD_Split((uint8_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 16) {
            demosaicAHD_Split((uint16_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 31) {
            demosaicAHD_Split((uint32_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else {
            return {};
        }

        return demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
    }

    //AHD插值辅助函数
    inline static RgbPixel demosaicPixelAHD(const std::vector<double>& R_plane, const std::vector<double>& G_plane, const std::vector<double>& B_plane, int32_t width, int32_t height, ePixType format, int32_t x, int32_t y) {
        auto idx = [&](int32_t i, int32_t j) { return j * width + i; };
        auto safe = [&](const std::vector<double>& plane, int32_t i, int32_t j) {
            if (i < 0 || i >= width || j < 0 || j >= height) 
                return 0.0;
            return plane[idx(i, j)];
            };

        char c = getBayerColorType(format, x, y);
        int R = R_plane[idx(x, y)];
        int G = G_plane[idx(x, y)];
        int B = B_plane[idx(x, y)];

        // 如果是 G 像素，缺失 R 和 B
        if (c == Ch_G) {
            // 计算四个方向的梯度代价（越小越平滑）
            auto gradH = std::abs(safe(R_plane, x - 2, y) - safe(R_plane, x + 2, y)) +
                std::abs(safe(R_plane, x - 1, y) - safe(R_plane, x + 1, y));
            auto gradV = std::abs(safe(R_plane, x, y - 2) - safe(R_plane, x, y + 2)) +
                std::abs(safe(R_plane, x, y - 1) - safe(R_plane, x, y + 1));
            auto gradD1 = std::abs(safe(R_plane, x - 2, y - 2) - safe(R_plane, x + 2, y + 2)) +
                std::abs(safe(R_plane, x - 1, y - 1) - safe(R_plane, x + 1, y + 1));
            auto gradD2 = std::abs(safe(R_plane, x - 2, y + 2) - safe(R_plane, x + 2, y - 2)) +
                std::abs(safe(R_plane, x - 1, y + 1) - safe(R_plane, x + 1, y - 1));

            // 选最小梯度方向插值 R
            if (gradH <= gradV && gradH <= gradD1 && gradH <= gradD2) {
                R = (safe(R_plane, x - 1, y) + safe(R_plane, x + 1, y) + 1) / 2;
            }
            else if (gradV <= gradD1 && gradV <= gradD2) {
                R = (safe(R_plane, x, y - 1) + safe(R_plane, x, y + 1) + 1) / 2;
            }
            else if (gradD1 <= gradD2) {
                R = (safe(R_plane, x - 1, y - 1) + safe(R_plane, x + 1, y + 1) + 1) / 2;
            }
            else {
                R = (safe(R_plane, x - 1, y + 1) + safe(R_plane, x + 1, y - 1) + 1) / 2;
            }

            // 同理插值 B（用 B_plane 梯度）
            gradH = std::abs(safe(B_plane, x - 2, y) - safe(B_plane, x + 2, y)) +
                std::abs(safe(B_plane, x - 1, y) - safe(B_plane, x + 1, y));
            gradV = std::abs(safe(B_plane, x, y - 2) - safe(B_plane, x, y + 2)) +
                std::abs(safe(B_plane, x, y - 1) - safe(B_plane, x, y + 1));
            gradD1 = std::abs(safe(B_plane, x - 2, y - 2) - safe(B_plane, x + 2, y + 2)) +
                std::abs(safe(B_plane, x - 1, y - 1) - safe(B_plane, x + 1, y + 1));
            gradD2 = std::abs(safe(B_plane, x - 2, y + 2) - safe(B_plane, x + 2, y - 2)) +
                std::abs(safe(B_plane, x - 1, y + 1) - safe(B_plane, x + 1, y - 1));

            if (gradH <= gradV && gradH <= gradD1 && gradH <= gradD2) {
                B = (safe(B_plane, x - 1, y) + safe(B_plane, x + 1, y) + 1) / 2;
            }
            else if (gradV <= gradD1 && gradV <= gradD2) {
                B = (safe(B_plane, x, y - 1) + safe(B_plane, x, y + 1) + 1) / 2;
            }
            else if (gradD1 <= gradD2) {
                B = (safe(B_plane, x - 1, y - 1) + safe(B_plane, x + 1, y + 1) + 1) / 2;
            }
            else {
                B = (safe(B_plane, x - 1, y + 1) + safe(B_plane, x + 1, y - 1) + 1) / 2;
            }
        }
        else if (c == Ch_R) {
            // 插值 G 和 B
            // 使用色差恒定：G = R - (R - G)_neighbor
            int g1 = safe(G_plane, x - 1, y) + (R - safe(R_plane, x - 1, y));
            int g2 = safe(G_plane, x + 1, y) + (R - safe(R_plane, x + 1, y));
            int g3 = safe(G_plane, x, y - 1) + (R - safe(R_plane, x, y - 1));
            int g4 = safe(G_plane, x, y + 1) + (R - safe(R_plane, x, y + 1));
            G = std::clamp((g1 + g2 + g3 + g4 + 2) / 4, 0, 255);

            // B 插值用方向选择（类似上面）
            auto gradH = std::abs(safe(B_plane, x - 2, y - 1) - safe(B_plane, x + 2, y - 1)) +
                std::abs(safe(B_plane, x - 1, y - 1) - safe(B_plane, x + 1, y - 1)) +
                std::abs(safe(B_plane, x - 2, y + 1) - safe(B_plane, x + 2, y + 1)) +
                std::abs(safe(B_plane, x - 1, y + 1) - safe(B_plane, x + 1, y + 1));
            auto gradV = std::abs(safe(B_plane, x - 1, y - 2) - safe(B_plane, x - 1, y + 2)) +
                std::abs(safe(B_plane, x - 1, y - 1) - safe(B_plane, x - 1, y + 1)) +
                std::abs(safe(B_plane, x + 1, y - 2) - safe(B_plane, x + 1, y + 2)) +
                std::abs(safe(B_plane, x + 1, y - 1) - safe(B_plane, x + 1, y + 1));

            if (gradH < gradV) {
                B = (safe(B_plane, x - 1, y - 1) + safe(B_plane, x + 1, y - 1) +
                    safe(B_plane, x - 1, y + 1) + safe(B_plane, x + 1, y + 1) + 2) / 4;
            }
            else {
                B = (safe(B_plane, x - 1, y - 1) + safe(B_plane, x - 1, y + 1) +
                    safe(B_plane, x + 1, y - 1) + safe(B_plane, x + 1, y + 1) + 2) / 4;
            }
        }
        else { // 'B'
            int b1 = safe(G_plane, x - 1, y) + (B - safe(B_plane, x - 1, y));
            int b2 = safe(G_plane, x + 1, y) + (B - safe(B_plane, x + 1, y));
            int b3 = safe(G_plane, x, y - 1) + (B - safe(B_plane, x, y - 1));
            int b4 = safe(G_plane, x, y + 1) + (B - safe(B_plane, x, y + 1));
            G = std::clamp((b1 + b2 + b3 + b4 + 2) / 4, 0, 255);

            auto gradH = std::abs(safe(R_plane, x - 2, y - 1) - safe(R_plane, x + 2, y - 1)) +
                std::abs(safe(R_plane, x - 1, y - 1) - safe(R_plane, x + 1, y - 1)) +
                std::abs(safe(R_plane, x - 2, y + 1) - safe(R_plane, x + 2, y + 1)) +
                std::abs(safe(R_plane, x - 1, y + 1) - safe(R_plane, x + 1, y + 1));
            auto gradV = std::abs(safe(R_plane, x - 1, y - 2) - safe(R_plane, x - 1, y + 2)) +
                std::abs(safe(R_plane, x - 1, y - 1) - safe(R_plane, x - 1, y + 1)) +
                std::abs(safe(R_plane, x + 1, y - 2) - safe(R_plane, x + 1, y + 2)) +
                std::abs(safe(R_plane, x + 1, y - 1) - safe(R_plane, x + 1, y + 1));

            if (gradH < gradV) {
                R = (safe(R_plane, x - 1, y - 1) + safe(R_plane, x + 1, y - 1) +
                    safe(R_plane, x - 1, y + 1) + safe(R_plane, x + 1, y + 1) + 2) / 4;
            }
            else {
                R = (safe(R_plane, x - 1, y - 1) + safe(R_plane, x - 1, y + 1) +
                    safe(R_plane, x + 1, y - 1) + safe(R_plane, x + 1, y + 1) + 2) / 4;
            }
        }

        return {
            static_cast<uint8_t>(std::clamp(R, 0, 255)),
            static_cast<uint8_t>(std::clamp(G, 0, 255)),
            static_cast<uint8_t>(std::clamp(B, 0, 255))
        };
    }
    template<typename T>
    //分离 R, G, B 平面（未采样位置为 0）
    void demosaicAHD_Split(const T* data, int32_t width, int32_t height, ePixType format, std::vector<double>& R, std::vector<double>& G, std::vector<double>& B) {
        if (!data || width == 0 || height == 0) return;
        for (int32_t y = 0; y < height; ++y) {
            for (int32_t x = 0; x < width; ++x) {
                int32_t i = y * width + x;
                auto val = data[i];
                auto c = getBayerColorType(format, x, y);
                if (c == Ch_R) R[i] = val;
                else if (c == Ch_G) G[i] = val;
                else B[i] = val;
            }
        }
    }

    bool computeHistogram_Bayer_AHD(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask, 
        std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel) {
        int32_t maxColor = (1U << info.bit) - 1;

        // 分离 R, G, B 平面（未采样位置为 0）
        std::vector<double> R(info.width * info.height, 0), G(info.width * info.height, 0), B(info.width * info.height, 0);
        if (info.bit <= 8) {
            demosaicAHD_Split((uint8_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 16) {
            demosaicAHD_Split((uint16_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 31) {
            demosaicAHD_Split((uint32_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else {
            return false;
        }

        int32_t pixelCount = info.width * info.height;
        int32_t validCount = 0;

        double* pRhi = Rhistogram.data();
        double* pGhi = Ghistogram.data();
        double* pBhi = Bhistogram.data();

        double* pMax = maxPixel.data();
        double* pMin = minPixel.data();
        double* pAve = avePixel.data();

        RgbPixel px;
        if (useMask && mask) {
            for (int32_t y = 0; y < info.height; ++y) {
                for (int32_t x = 0; x < info.width; ++x) {
                    if (mask->at(y * info.width + x)) {
                        px = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);

                        pRhi[px.r]++;
                        pGhi[px.g]++;
                        pBhi[px.b]++;

                        pAve[0] += px.r;
                        pAve[1] += px.g;
                        pAve[2] += px.b;

                        if (pMax[0] < px.r) {
                            pMax[0] = px.r;
                        }
                        if (pMax[1] < px.g) {
                            pMax[1] = px.g;
                        }
                        if (pMax[2] < px.b) {
                            pMax[2] = px.b;
                        }

                        if (pMin[0] > px.r) {
                            pMin[0] = px.r;
                        }
                        if (pMin[1] > px.g) {
                            pMin[1] = px.g;
                        }
                        if (pMin[2] > px.b) {
                            pMin[2] = px.b;
                        }

                        validCount++;
                    }
                }
            }
        }
        else {
            validCount = pixelCount;
            for (int32_t y = 0; y < info.height; ++y) {
                for (int32_t x = 0; x < info.width; ++x) {
                    px = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
                    pRhi[px.r]++;
                    pGhi[px.g]++;
                    pBhi[px.b]++;

                    pAve[0] += px.r;
                    pAve[1] += px.g;
                    pAve[2] += px.b;

                    if (pMax[0] < px.r) {
                        pMax[0] = px.r;
                    }
                    if (pMax[1] < px.g) {
                        pMax[1] = px.g;
                    }
                    if (pMax[2] < px.b) {
                        pMax[2] = px.b;
                    }

                    if (pMin[0] > px.r) {
                        pMin[0] = px.r;
                    }
                    if (pMin[1] > px.g) {
                        pMin[1] = px.g;
                    }
                    if (pMin[2] > px.b) {
                        pMin[2] = px.b;
                    }
                }
            }
        }
        
        pAve[0] /= validCount;
        pAve[1] /= validCount;
        pAve[2] /= validCount;
        return true;
    }

    bool computeHistogram_Bayer_AHD_Stretch(const ImageShowInfo& info, const uint8_t* data,
        std::vector<double>& Rhistogram, CyMedia::StretchType type, bool bParallel) {
        // 分离 R, G, B 平面（未采样位置为 0）
        std::vector<double> R(info.width * info.height, 0), G(info.width * info.height, 0), B(info.width * info.height, 0);
        if (info.bit <= 8) {
            demosaicAHD_Split((uint8_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 16) {
            demosaicAHD_Split((uint16_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 31) {
            demosaicAHD_Split((uint32_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else {
            return false;
        }

        uint32_t bitMax = 1 << info.bit;
        int32_t pixelCount = info.width * info.height;
        double* pRhi = Rhistogram.data();
        // 定义一个 lambda 用于计算单像素的拉伸索引（避免重复代码）
        auto calcStretchIdx = [&](int32_t x, int32_t y) -> int32_t {
            RgbPixel px = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
            return CyMediaCalc::RGBT2StretchOneFast(px, type, bitMax);
            };
        if (bParallel) {
#if defined _MSC_VER
            // ---------- PPL 并行 ----------
            concurrency::combinable<std::vector<double>> localHist(
                [&]() { return std::vector<double>(bitMax, 0.0); }
            );

            concurrency::parallel_for(0, info.height, [&](int y) {
                auto& local = localHist.local();
                for (int x = 0; x < info.width; ++x) {
                    int idx = calcStretchIdx(x, y);
                    local[idx] += 1.0;
                }
                });

            // 合并各线程局部直方图
            localHist.combine_each([&](std::vector<double>& local) {
                for (uint32_t i = 0; i < bitMax; ++i) {
                    pRhi[i] += local[i];
                }
                });

#elif defined _OPENMP
            // ---------- OpenMP 并行 ----------
            int numThreads = omp_get_max_threads();
            std::vector<std::vector<double>> localHists(
                numThreads, std::vector<double>(bitMax, 0.0)
            );

#pragma omp parallel for
            for (int y = 0; y < info.height; ++y) {
                int tid = omp_get_thread_num();
                auto& local = localHists[tid];
                for (int x = 0; x < info.width; ++x) {
                    int idx = calcStretchIdx(x, y);
                    local[idx] += 1.0;
                }
            }

            // 合并结果
            for (int t = 0; t < numThreads; ++t) {
                auto& local = localHists[t];
                for (uint32_t i = 0; i < bitMax; ++i) {
                    pRhi[i] += local[i];
                }
            }

#else
            // ---------- 无并行库支持，强制回退串行 ----------
            bParallel = false;
#endif
        }
        if (false == bParallel) {
            for (int32_t y = 0; y < info.height; ++y) {
                for (int32_t x = 0; x < info.width; ++x) {
                    int idx = calcStretchIdx(x, y);
                    pRhi[idx] += 1.0;
                }
            }
        }
        return true;
    }

    template<typename T>
    bool Bayer2RGB(const ImageShowInfo& info, const uint8_t* data, T* outdata, DemosaicingMethod func/* = BILINEAR*/) {
        if (!data || !outdata) return false;
        if (info.width <= 0 || info.height <= 0 || info.length <= 0) return false;

        int32_t totalPixels = info.width * info.height;
        // 获取 CPU 核心数
        static int32_t cpuCores = []() {
            int32_t cores = static_cast<int32_t>(std::thread::hardware_concurrency());
            return cores > 0 ? cores : 4;  // 若获取失败，默认 4 核
            }();
        //计算并行计算阈值
        const int32_t threshold = get_parallel_threshold_bayer_conver(cpuCores, func);
        bool bseria = totalPixels < threshold;

        switch (func) {
            case CyMedia::DEMOSAIC_NONE:
            case CyMedia::DEMOSAIC_BILINEAR:
            case CyMedia::DEMOSAIC_MALVA: {
                //并行计算
                if (false == bseria) {
#if defined _MSC_VER
                    // 使用 PPL parallel_for 并行外层循环 (y 维度)
                    concurrency::parallel_for(0, static_cast<int>(info.height), [&](int y) {
                        for (uint32_t x = 0; x < static_cast<uint32_t>(info.width); ++x) {
                            int32_t idx = y * info.width + x;
                            RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                            outdata[idx * 3 + 0] = static_cast<T>(px.r);
                            outdata[idx * 3 + 1] = static_cast<T>(px.g);
                            outdata[idx * 3 + 2] = static_cast<T>(px.b);
                        }
                        });
#elif defined _OPENMP
#pragma omp parallel for
                    for (int y = 0; y < info.height; ++y) {
                        for (uint32_t x = 0; x < static_cast<uint32_t>(info.width); ++x) {
                            int32_t idx = y * info.width + x;
                            RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                            outdata[idx * 3 + 0] = static_cast<T>(px.r);
                            outdata[idx * 3 + 1] = static_cast<T>(px.g);
                            outdata[idx * 3 + 2] = static_cast<T>(px.b);
                        }
                    }
#else
                    // 退回串行
                    bseria = true;
#endif
                }
                //串行
                if (true == bseria) {
                    for (uint32_t y = 0; y < static_cast<uint32_t>(info.height); ++y) {
                        for (uint32_t x = 0; x < static_cast<uint32_t>(info.width); ++x) {
                            int32_t idx = y * info.width + x;
                            RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                            outdata[idx * 3 + 0] = static_cast<T>(px.r);
                            outdata[idx * 3 + 1] = static_cast<T>(px.g);
                            outdata[idx * 3 + 2] = static_cast<T>(px.b);
                        }
                    }
                }
                return true;
            }break;

            case CyMedia::DEMOSAIC_AHD: {
                return Bayer2RGB_AHD(info, data, outdata);
            }break;
        }

        return false;
    }
    bool Bayer2RGB_8(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, DemosaicingMethod func/* = BILINEAR*/) {
        if (!data || !outdata) return false;
        if (info.width <= 0 || info.height <= 0 || info.length <= 0) return false;

        int32_t totalPixels = info.width * info.height;
        // 获取 CPU 核心数
        static int32_t cpuCores = []() {
            int32_t cores = static_cast<int32_t>(std::thread::hardware_concurrency());
            return cores > 0 ? cores : 4;  // 若获取失败，默认 4 核
            }();
        //计算并行计算阈值
        const int32_t threshold = get_parallel_threshold_bayer_conver(cpuCores, func);
        bool bseria = totalPixels < threshold;

        float scale = 256.0 / (1 << info.bit);
        switch (func) {
        case CyMedia::DEMOSAIC_NONE:
        case CyMedia::DEMOSAIC_BILINEAR:
        case CyMedia::DEMOSAIC_MALVA: {
            //并行计算
            if (false == bseria) {
#if defined _MSC_VER
                // 使用 PPL parallel_for 并行外层循环 (y 维度)
                concurrency::parallel_for(0, static_cast<int>(info.height), [&](int y) {
                    for (uint32_t x = 0; x < static_cast<uint32_t>(info.width); ++x) {
                        int32_t idx = y * info.width + x;
                        RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                        outdata[idx * 3 + 0] = (px.r) * scale;
                        outdata[idx * 3 + 1] = (px.g) * scale;
                        outdata[idx * 3 + 2] = (px.b) * scale;
                    }
                    });
#elif defined _OPENMP
#pragma omp parallel for
                for (int y = 0; y < info.height; ++y) {
                    for (uint32_t x = 0; x < static_cast<uint32_t>(info.width); ++x) {
                        int32_t idx = y * info.width + x;
                        RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                        outdata[idx * 3 + 0] = (px.r) * scale;
                        outdata[idx * 3 + 1] = (px.g) * scale;
                        outdata[idx * 3 + 2] = (px.b) * scale;
                    }
                }
#else
                // 退回串行
                bseria = true;
#endif
            }
            //串行
            if (true == bseria) {
                for (uint32_t y = 0; y < static_cast<uint32_t>(info.height); ++y) {
                    for (uint32_t x = 0; x < static_cast<uint32_t>(info.width); ++x) {
                        int32_t idx = y * info.width + x;
                        RgbPixel px = calcCoordinateColor(info, data, x, y, func);
                        outdata[idx * 3 + 0] = (px.r) * scale;
                        outdata[idx * 3 + 1] = (px.g) * scale;
                        outdata[idx * 3 + 2] = (px.b) * scale;
                    }
                }
            }
            return true;
        }break;

        case CyMedia::DEMOSAIC_AHD: {
            return Bayer2RGB_AHD(info, data, outdata);
        }break;
        }

        return false;
    }

    template<typename T>
    bool Bayer2RGB_AHD(const ImageShowInfo& info, const uint8_t* data, T* outdata, bool seria/* = true*/) {
        // 分离 R, G, B 平面（未采样位置为 0）
        std::vector<double> R(info.width * info.height, 0), G(info.width * info.height, 0), B(info.width * info.height, 0);
        if (info.bit <= 8) {
            demosaicAHD_Split((uint8_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 16) {
            demosaicAHD_Split((uint16_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 31) {
            demosaicAHD_Split((uint32_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else {
            return false;
        }
        bool tSerial = seria;
        // 并行插值并输出
        if (false == tSerial) {
#if defined _MSC_VER
            concurrency::parallel_for(0, static_cast<int>(info.height), [&](int y) {
                for (int x = 0; x < info.width; ++x) {
                    int32_t idx = y * info.width + x;
                    RgbPixel tempColor = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
                    outdata[idx * 3 + 0] = static_cast<T>(tempColor.r);
                    outdata[idx * 3 + 1] = static_cast<T>(tempColor.g);
                    outdata[idx * 3 + 2] = static_cast<T>(tempColor.b);
                }
                });
#elif defined _OPENMP
#pragma omp parallel for
            for (int y = 0; y < info.height; ++y) {
                for (int x = 0; x < info.width; ++x) {
                    int32_t idx = y * info.width + x;
                    RgbPixel tempColor = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
                    outdata[idx * 3 + 0] = static_cast<T>(tempColor.r);
                    outdata[idx * 3 + 1] = static_cast<T>(tempColor.g);
                    outdata[idx * 3 + 2] = static_cast<T>(tempColor.b);
                }
            }
#else
            tSerial = true;
#endif
        }
        if (true == tSerial) {
            for (int y = 0; y < info.height; ++y) {
                for (int x = 0; x < info.width; ++x) {
                    int32_t idx = y * info.width + x;
                    RgbPixel tempColor = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
                    outdata[idx * 3 + 0] = static_cast<T>(tempColor.r);
                    outdata[idx * 3 + 1] = static_cast<T>(tempColor.g);
                    outdata[idx * 3 + 2] = static_cast<T>(tempColor.b);
                }
            }
        }
        
        return true;
    }
    bool Bayer2RGB_AHD_8(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, bool seria/* = true*/) {
        // 分离 R, G, B 平面（未采样位置为 0）
        std::vector<double> R(info.width * info.height, 0), G(info.width * info.height, 0), B(info.width * info.height, 0);
        if (info.bit <= 8) {
            demosaicAHD_Split((uint8_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 16) {
            demosaicAHD_Split((uint16_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else if (info.bit <= 31) {
            demosaicAHD_Split((uint32_t*)data, info.width, info.height, info.format, R, G, B);
        }
        else {
            return false;
        }
        bool tSerial = seria;
        float scale = 256.0 / (1 << info.bit);
        // 并行插值并输出
        if (false == tSerial) {
#if defined _MSC_VER
            concurrency::parallel_for(0, static_cast<int>(info.height), [&](int y) {
                for (int x = 0; x < info.width; ++x) {
                    int32_t idx = y * info.width + x;
                    RgbPixel tempColor = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
                    outdata[idx * 3 + 0] = (tempColor.r) * scale;
                    outdata[idx * 3 + 1] = (tempColor.g) * scale;
                    outdata[idx * 3 + 2] = (tempColor.b) * scale;
                }
                });
#elif defined _OPENMP
#pragma omp parallel for
            for (int y = 0; y < info.height; ++y) {
                for (int x = 0; x < info.width; ++x) {
                    int32_t idx = y * info.width + x;
                    RgbPixel tempColor = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
                    outdata[idx * 3 + 0] = (tempColor.r) * scale;
                    outdata[idx * 3 + 1] = (tempColor.g) * scale;
                    outdata[idx * 3 + 2] = (tempColor.b) * scale;
                }
            }
#else
            tSerial = true;
#endif
        }
        if (true == tSerial) {
            for (int y = 0; y < info.height; ++y) {
                for (int x = 0; x < info.width; ++x) {
                    int32_t idx = y * info.width + x;
                    RgbPixel tempColor = demosaicPixelAHD(R, G, B, info.width, info.height, info.format, x, y);
                    outdata[idx * 3 + 0] = (tempColor.r) * scale;
                    outdata[idx * 3 + 1] = (tempColor.g) * scale;
                    outdata[idx * 3 + 2] = (tempColor.b) * scale;
                }
            }
        }

        return true;
    }
}