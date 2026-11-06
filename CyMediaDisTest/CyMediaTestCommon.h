#pragma once
#include <vector>

#include <QString>
#include <QImage>

namespace CyMediaTest {
    // ====================== 通用配色轮播 ======================

    // HSL 颜色：h ∈ [0,360)，s ∈ [0,1]，l ∈ [0,1]
    struct HSL {
        double h = 0.0;
        double s = 0.0;
        double l = 0.0;
    };

    // 轮播配置
    struct PaletteCycleConfig {
        int      cycleFrames = 240;   // 一个完整周期（@60fps = 4s）
        int      holdFrames = 170;   // 停留时间
        int      transFrames = 70;    // 过渡时间（默认 = cycleFrames - holdFrames）
        bool     randomNext = false; // 下一配色随机跳转（避免固定循环规律）
        uint32_t seed = 0;     // randomNext 的种子，0 表示用 cycleIdx 本身
    };

    // 完整调色板：主色 + 底色（两组等长，按索引配对）
    struct Palette {
        std::vector<HSL> main;       // 主色：亮臂/高光
        std::vector<HSL> bg;         // 底色：暗部/背景
        PaletteCycleConfig cfg;

        // 主函数：采样当前帧的主色和底色（RGB，[0,1] 范围）
        void sample(int frameIdx,
            double& cR, double& cG, double& cB,
            double& bgR, double& bgG, double& bgB) const;

        // 只取主色（适合直接替换旧的 iqPalette 用法）
        void sampleMain(int frameIdx,
            double& cR, double& cG, double& cB) const;

        // 只取底色
        void sampleBg(int frameIdx,
            double& bgR, double& bgG, double& bgB) const;

        // 带 blend 输出的版本：blend=0 表示停留，>0 表示过渡进度 [0,1]
        // 用于需要感知"正在换色"的场景（例如给高光提亮）
        double sampleWithBlend(int frameIdx,
            double& cR, double& cG, double& cB,
            double& bgR, double& bgG, double& bgB) const;

        // 直接取主色 / 底色的 HSL（Plasma 这类需要色相摆动的时候用）
        void sampleHsl(int frameIdx, HSL& mainOut, HSL& bgOut) const;
    };

    // ====================== 独立工具函数 ======================

    // HSL → RGB，rgb ∈ [0,1]
    void hsl2rgb(const HSL& c, double& r, double& g, double& b);

    // HSL 插值：色相走短路径、S/L 线性
    HSL lerpHsl(const HSL& a, const HSL& b, double t);

    // 色阶映射工具：把标量 v ∈ [0,1] 映射到 [bg, fg] 上的 RGB 三通道
    // 常用替代旧代码的 iqPalette(v, r, g, b)
    // whiteMix: 高光往白推的强度（0~1），0 表示不推白
    void mapToRamp(double v,
        double fgR, double fgG, double fgB,
        double bgR, double bgG, double bgB,
        uint8_t& outR, uint8_t& outG, uint8_t& outB,
        double whiteMix = 0.0);

    // ====================== 预置配色方案 ======================
    // 均为 const 引用，避免重复构造；调用方直接用即可

    // 深海青蓝 → 翡翠 → 琥珀 → 熔岩 → 紫罗兰（默认，最通用）
    const Palette& paletteDeepSea();

    // 琥珀 → 熔岩（暖调，岩浆/篝火感）
    const Palette& paletteAmber();

    // 单色模式：hue 固定，不做轮播（想快速"纯色"时使用）
    const Palette& paletteMono(double hue, double sat = 0.75);

    enum eAnalogImageType {
        AnalogImage_Begin = 0,
        AnalogImage_RandomColor,       // 随机彩色
        AnalogImage_CheckerBoard,      // 移动棋盘格
        AnalogImage_MovingStripes,     // 动态正弦条纹（灰度）
        AnalogImage_Plasma,            // 等离子分形效果
        AnalogImage_Julia,             // Julia 集合（参数动画）
        AnalogImage_FractalNoise,      // fBm 分形噪声
        AnalogImage_Kaleidoscope,      // 万花筒
        AnalogImage_Twirl,             // 漩涡扭曲棋盘格
        AnalogImage_Tunnel,            // 3D 透视隧道
        AnalogImage_Cellular,          // Worley 细胞噪声
        AnalogImage_RadialRipple,      // 多中心动态涟漪
        AnalogImage_Spiral,            // 螺旋漩涡
        AnalogImage_Moire,             // 莫尔（Moire）干涉条纹
        AnalogImage_End,
    };

    QString getAnalogImageTypeName(CyMediaTest::eAnalogImageType type);

    /** RGB888 -> NV12（Y 平面 + UV 交错平面，U 在前 V 在后） */
    void rgbToNV12(const uint8_t* rgb, int width, int height, std::vector<uint8_t>& out);
    /** RGB888 -> NV21（Y 平面 + VU 交错平面，V 在前 U 在后） */
    void rgbToNV21(const uint8_t* rgb, int width, int height, std::vector<uint8_t>& out);

    void genAnalogImage(CyMediaTest::eAnalogImageType type, QImage& img, int frameIdx);
};
