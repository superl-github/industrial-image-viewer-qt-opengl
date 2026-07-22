#include "CyMdiaCalc_YUV.h"
#include <vector>
#include <algorithm>
#include <cmath>

#if defined _MSC_VER
#include <ppl.h>
#include <thread>
#elif defined _OPENMP
#include <omp.h>
#endif

namespace CyMedia {

    // ---------- 内部辅助函数 ----------
    // YUV 转 RGB (BT.601 全范围，Y/U/V 均为 0~255)
    static inline void YUVtoRGB(uint8_t Y, uint8_t U, uint8_t V,
        uint8_t& R, uint8_t& G, uint8_t& B) {
        int y = Y;
        int u = U - 128;
        int v = V - 128;

        int r = y + static_cast<int>(1.402 * v + 0.5);
        int g = y - static_cast<int>(0.344 * u + 0.714 * v + 0.5);
        int b = y + static_cast<int>(1.772 * u + 0.5);

        R = static_cast<uint8_t>(std::clamp(r, 0, 255));
        G = static_cast<uint8_t>(std::clamp(g, 0, 255));
        B = static_cast<uint8_t>(std::clamp(b, 0, 255));
    }

    // 从 YUV 数据中提取指定坐标的 Y、U、V 分量
    static inline bool getYUV(const ImageShowInfo& info, const uint8_t* data,
        int x, int y, uint8_t& Y, uint8_t& U, uint8_t& V) {
        int w = info.width;
        int h = info.height;
        if (x < 0 || x >= w || y < 0 || y >= h)
            return false;

        switch (info.format) {
        case FOURCC_YUY2: {
            int pixel_index = y * w + x;
            int macro_index = y * w + (x / 2);
            Y = data[pixel_index * 2];
            U = data[macro_index * 4 + 1];
            V = data[macro_index * 4 + 3];
            break;
        }
        case FOURCC_YVYU: {
            int pixel_index = y * w + x;
            int macro_index = y * w + (x / 2);
            Y = data[pixel_index * 2];
            V = data[macro_index * 4 + 1];
            U = data[macro_index * 4 + 3];
            break;
        }
        case FOURCC_I422: {
            int y_idx = y * w + x;
            int uv_x = x / 2;
            int uv_y = y;
            int uv_off = uv_y * (w / 2) + uv_x;
            Y = data[y_idx];
            U = data[w * h + uv_off];
            V = data[w * h + (w / 2) * h + uv_off];
            break;
        }
        case FOURCC_YV16: {
            int y_idx = y * w + x;
            int uv_x = x / 2;
            int uv_y = y;
            int uv_off = uv_y * (w / 2) + uv_x;
            Y = data[y_idx];
            V = data[w * h + uv_off];
            U = data[w * h + (w / 2) * h + uv_off];
            break;
        }
        case FOURCC_I420: {
            int y_idx = y * w + x;
            int uv_x = x / 2;
            int uv_y = y / 2;
            int uv_w = w / 2;
            int uv_h = h / 2;
            int uv_off = uv_y * uv_w + uv_x;
            Y = data[y_idx];
            U = data[w * h + uv_off];
            V = data[w * h + uv_w * uv_h + uv_off];
            break;
        }
        case FOURCC_YV12: {
            int y_idx = y * w + x;
            int uv_x = x / 2;
            int uv_y = y / 2;
            int uv_w = w / 2;
            int uv_h = h / 2;
            int uv_off = uv_y * uv_w + uv_x;
            Y = data[y_idx];
            V = data[w * h + uv_off];
            U = data[w * h + uv_w * uv_h + uv_off];
            break;
        }
        case FOURCC_NV12: {
            int y_idx = y * w + x;
            int uv_x = x / 2;
            int uv_y = y / 2;
            int uv_w = w / 2;
            int uv_off = uv_y * uv_w + uv_x;
            Y = data[y_idx];
            int uv_start = w * h;
            U = data[uv_start + uv_off * 2];
            V = data[uv_start + uv_off * 2 + 1];
            break;
        }
        case FOURCC_NV21: {
            int y_idx = y * w + x;
            int uv_x = x / 2;
            int uv_y = y / 2;
            int uv_w = w / 2;
            int uv_off = uv_y * uv_w + uv_x;
            Y = data[y_idx];
            int uv_start = w * h;
            V = data[uv_start + uv_off * 2];
            U = data[uv_start + uv_off * 2 + 1];
            break;
        }
        default:
            return false;
        }
        return true;
    }

    // ---------- 公开函数实现 ----------

    RgbPixel calcCoordinateColor_YUV(const ImageShowInfo& info, const uint8_t* pdata,
        int32_t x, int32_t y, YUVTransMethod func) {
        RgbPixel result{ 0, 0, 0 };
        uint8_t Y, U, V;
        if (!getYUV(info, pdata, x, y, Y, U, V)) {
            return result;   // 格式不支持或坐标越界返回黑色
        }

        if (func == YUVTRANS_Y) {
            result.r = result.g = result.b = static_cast<int32_t>(Y);
        }
        else {
            uint8_t R, G, B;
            YUVtoRGB(Y, U, V, R, G, B);
            result.r = static_cast<int32_t>(R);
            result.g = static_cast<int32_t>(G);
            result.b = static_cast<int32_t>(B);
        }
        return result;
    }

    bool computeHistogram_YUV(const ImageShowInfo& info, const uint8_t* data,
        std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel,
        std::vector<double>& minPixel,
        std::vector<double>& avePixel,
        YUVTransMethod func) {
        if (!info.isYUV() || data == nullptr)
            return false;

        int w = info.width;
        int h = info.height;
        int total = w * h;

        // 初始化直方图 (0~255)
        Rhistogram.assign(256, 0.0);
        Ghistogram.assign(256, 0.0);
        Bhistogram.assign(256, 0.0);

        // 统计变量
        double sumR = 0.0, sumG = 0.0, sumB = 0.0;
        int32_t minR = 255, maxR = 0;
        int32_t minG = 255, maxG = 0;
        int32_t minB = 255, maxB = 0;
        int64_t count = 0;

        // 遍历所有像素
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // 掩码过滤
                if (useMask && mask != nullptr) {
                    if ((*mask)[y * w + x] == 0)
                        continue;
                }

                uint8_t Y, U, V;
                if (!getYUV(info, data, x, y, Y, U, V))
                    continue;

                int32_t r, g, b;
                if (func == YUVTRANS_Y) {
                    r = g = b = static_cast<int32_t>(Y);
                }
                else {
                    uint8_t R8, G8, B8;
                    YUVtoRGB(Y, U, V, R8, G8, B8);
                    r = static_cast<int32_t>(R8);
                    g = static_cast<int32_t>(G8);
                    b = static_cast<int32_t>(B8);
                }

                // 更新直方图
                Rhistogram[r] += 1.0;
                Ghistogram[g] += 1.0;
                Bhistogram[b] += 1.0;

                // 累计统计
                sumR += r;
                sumG += g;
                sumB += b;
                if (r < minR) minR = r;
                if (r > maxR) maxR = r;
                if (g < minG) minG = g;
                if (g > maxG) maxG = g;
                if (b < minB) minB = b;
                if (b > maxB) maxB = b;
                ++count;
            }
        }

        // 计算最大、最小、平均
        maxPixel.clear();
        minPixel.clear();
        avePixel.clear();
        if (count > 0) {
            maxPixel.push_back(static_cast<double>(maxR));
            maxPixel.push_back(static_cast<double>(maxG));
            maxPixel.push_back(static_cast<double>(maxB));
            minPixel.push_back(static_cast<double>(minR));
            minPixel.push_back(static_cast<double>(minG));
            minPixel.push_back(static_cast<double>(minB));
            avePixel.push_back(sumR / count);
            avePixel.push_back(sumG / count);
            avePixel.push_back(sumB / count);
        }
        else {
            // 无有效像素，填充 0
            maxPixel.assign(3, 0.0);
            minPixel.assign(3, 0.0);
            avePixel.assign(3, 0.0);
        }

        return true;
    }

    bool YUV2RGBConver(const ImageShowInfo& info, const uint8_t* data,
        uint8_t* outdata, YUVTransMethod func) {
        if (!info.isYUV() || data == nullptr || outdata == nullptr)
            return false;

        int w = info.width;
        int h = info.height;
        int total = w * h;

        // 定义并行任务（每个像素独立转换）
        auto convert_pixel = [&](int idx) {
            int x = idx % w;
            int y = idx / w;
            uint8_t Y, U, V;
            if (!getYUV(info, data, x, y, Y, U, V)) {
                // 出错时输出黑色
                outdata[idx * 3 + 0] = 0;
                outdata[idx * 3 + 1] = 0;
                outdata[idx * 3 + 2] = 0;
                return;
            }

            if (func == YUVTRANS_Y) {
                outdata[idx * 3 + 0] = Y;
                outdata[idx * 3 + 1] = Y;
                outdata[idx * 3 + 2] = Y;
            }
            else {
                uint8_t R, G, B;
                YUVtoRGB(Y, U, V, R, G, B);
                outdata[idx * 3 + 0] = R;
                outdata[idx * 3 + 1] = G;
                outdata[idx * 3 + 2] = B;
            }
            };

        // 并行选择
#if defined _MSC_VER
        // Windows 平台使用 PPL
        concurrency::parallel_for(0, total, convert_pixel);
#elif defined _OPENMP
        // 支持 OpenMP 时使用
#pragma omp parallel for
        for (int idx = 0; idx < total; ++idx) {
            convert_pixel(idx);
        }
#else
        // 串行回退
        for (int idx = 0; idx < total; ++idx) {
            convert_pixel(idx);
        }
#endif

        return true;
    }

} // namespace CyMedia