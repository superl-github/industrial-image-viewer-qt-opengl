#pragma once
#include <vector>

#include <QString>

namespace CyMediaTest {
    enum eAnalogImageType {
        AnalogImage_Begin = 0,
        AnalogImage_RandomColor,       // 随机彩色
        AnalogImage_CheckerBoard,      // 移动棋盘格
        AnalogImage_MovingStripes,     // 动态正弦条纹（灰度）
        AnalogImage_Plasma,            // 等离子分形效果
        AnalogImage_End,
    };

    /** RGB888 -> NV12（Y 平面 + UV 交错平面，U 在前 V 在后） */
    void rgbToNV12(const uint8_t* rgb, int width, int height, std::vector<uint8_t>& out);
    /** RGB888 -> NV21（Y 平面 + VU 交错平面，V 在前 U 在后） */
    void rgbToNV21(const uint8_t* rgb, int width, int height, std::vector<uint8_t>& out);
};
