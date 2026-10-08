#include "CyMediaTestCommon.h"

#include <QFile>
#include <QFileInfo>

namespace CyMediaTest {
    // ----------------------------------------------------------------------------
    // RGB888 -> NV12 (YUV420SP, U 在前)
    //   out[0 .. w*h-1]     : Y 平面
    //   out[w*h .. w*h*3/2-1]: UV 交错平面 (U0 V0 U1 V1 ...)
    // 使用 BT.601 studio swing（Y:16~235, UV:16~240）系数
    // 要求 width/height 为偶数（如为奇数，内部向下取偶，等价于裁剪）
    // ----------------------------------------------------------------------------
    void rgbToNV12(const uint8_t* rgb, int width, int height, std::vector<uint8_t>& out) {
        if (!rgb || width <= 0 || height <= 0) { out.clear(); return; }

        const int w = width & ~1;   // 向下取偶
        const int h = height & ~1;

        out.resize(w * h * 3 / 2);
        uint8_t* yPlane = out.data();
        uint8_t* uvPlane = out.data() + w * h;

        // ---------- Y 平面 ----------
        for (int j = 0; j < h; ++j) {
            const uint8_t* srcRow = rgb + static_cast<size_t>(j) * width * 3;
            uint8_t* yRow = yPlane + static_cast<size_t>(j) * w;
            for (int i = 0; i < w; ++i) {
                int R = srcRow[i * 3 + 0];
                int G = srcRow[i * 3 + 1];
                int B = srcRow[i * 3 + 2];
                int Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
                yRow[i] = static_cast<uint8_t>(Y < 0 ? 0 : (Y > 255 ? 255 : Y));
            }
        }

        // ---------- UV 平面（4:2:0，2x2 求平均） ----------
        int uvIdx = 0;
        for (int j = 0; j < h; j += 2) {
            const uint8_t* row0 = rgb + static_cast<size_t>(j) * width * 3;
            const uint8_t* row1 = rgb + static_cast<size_t>(j + 1) * width * 3;
            for (int i = 0; i < w; i += 2) {
                int R = row0[i * 3 + 0] + row0[(i + 1) * 3 + 0]
                    + row1[i * 3 + 0] + row1[(i + 1) * 3 + 0];
                int G = row0[i * 3 + 1] + row0[(i + 1) * 3 + 1]
                    + row1[i * 3 + 1] + row1[(i + 1) * 3 + 1];
                int B = row0[i * 3 + 2] + row0[(i + 1) * 3 + 2]
                    + row1[i * 3 + 2] + row1[(i + 1) * 3 + 2];
                R >>= 2; G >>= 2; B >>= 2;

                int U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                int V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
                uvPlane[uvIdx++] = static_cast<uint8_t>(U < 0 ? 0 : (U > 255 ? 255 : U));
                uvPlane[uvIdx++] = static_cast<uint8_t>(V < 0 ? 0 : (V > 255 ? 255 : V));
            }
        }
    }
    // ----------------------------------------------------------------------------
    // RGB888 -> NV21 (YUV420SP, V 在前)，与 NV12 仅 UV 排列不同
    // ----------------------------------------------------------------------------
    void rgbToNV21(const uint8_t* rgb, int width, int height, std::vector<uint8_t>& out) {
        if (!rgb || width <= 0 || height <= 0) { out.clear(); return; }

        const int w = width & ~1;
        const int h = height & ~1;

        out.resize(w * h * 3 / 2);
        uint8_t* yPlane = out.data();
        uint8_t* uvPlane = out.data() + w * h;

        // Y 平面（与 NV12 相同）
        for (int j = 0; j < h; ++j) {
            const uint8_t* srcRow = rgb + static_cast<size_t>(j) * width * 3;
            uint8_t* yRow = yPlane + static_cast<size_t>(j) * w;
            for (int i = 0; i < w; ++i) {
                int R = srcRow[i * 3 + 0];
                int G = srcRow[i * 3 + 1];
                int B = srcRow[i * 3 + 2];
                int Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
                yRow[i] = static_cast<uint8_t>(Y < 0 ? 0 : (Y > 255 ? 255 : Y));
            }
        }

        // VU 平面（交换写入顺序）
        int uvIdx = 0;
        for (int j = 0; j < h; j += 2) {
            const uint8_t* row0 = rgb + static_cast<size_t>(j) * width * 3;
            const uint8_t* row1 = rgb + static_cast<size_t>(j + 1) * width * 3;
            for (int i = 0; i < w; i += 2) {
                int R = row0[i * 3 + 0] + row0[(i + 1) * 3 + 0]
                    + row1[i * 3 + 0] + row1[(i + 1) * 3 + 0];
                int G = row0[i * 3 + 1] + row0[(i + 1) * 3 + 1]
                    + row1[i * 3 + 1] + row1[(i + 1) * 3 + 1];
                int B = row0[i * 3 + 2] + row0[(i + 1) * 3 + 2]
                    + row1[i * 3 + 2] + row1[(i + 1) * 3 + 2];
                R >>= 2; G >>= 2; B >>= 2;

                int U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                int V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
                // NV21: V 先 U 后
                uvPlane[uvIdx++] = static_cast<uint8_t>(V < 0 ? 0 : (V > 255 ? 255 : V));
                uvPlane[uvIdx++] = static_cast<uint8_t>(U < 0 ? 0 : (U > 255 ? 255 : U));
            }
        }
    }

};
