#pragma once
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
};
