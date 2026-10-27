// CalcColor.h
#pragma once
#include "CyMediaBaseDef.h"

using namespace CyMedia;

namespace CyMediaCalc {
    /**
     * @brief 将 sRGB 像素转换为线性 RGB（去 Gamma 校正）。
     * @param srgb 输入 sRGB 像素（各分量归一化到 [0,1]）。
     * @return 线性 RGB 像素。
     */
    RgbPixelF srgb2linear(RgbPixelF& srgb);
    /**
     * @brief 将线性 RGB 像素转换为 sRGB（应用 Gamma 校正）。
     * @param linear 输入线性 RGB 像素。
     * @return sRGB 像素。
     */
    RgbPixelF linear2srgb(RgbPixelF& linear);
    /**
     * @brief 将 RGB 像素转换为灰度值（RgbPixel 三个通道均设为灰度值）。
     * @param pixel   输入 RGB 像素（0~bitMax-1）。
     * @param bitMax  通道最大值（通常为 2^bit - 1）。
     * @return 灰度像素（RGB 相等）。
     */
    RgbPixel  RGBPixel2rgb2Gray(RgbPixel pixel, uint32_t bitMax);
    /**
     * @brief 将 RGB 像素转换为灰度值（RgbPixel 三个通道均设为灰度值）。
     * @param pixel   输入 RGB 像素（0~bitMax-1）。
     * @param bitMax  通道最大值（通常为 2^bit - 1）。
     * @return 灰度像素（RGB 相等）。
     */
    RgbPixel  RGBPixel2rgb2hsv(RgbPixel pixel, uint32_t bitMax);
    /**
     * @brief 将 RGB 像素转换为 CIELAB 颜色空间（L, a, b 分量存储为 uint32_t）。
     * @param pixel   输入 RGB 像素。
     * @param bitMax  量化最大值（用于归一化 a, b 分量）。
     * @return Lab 像素（r=L, g=a, b=b，L 范围 0~100，a,b 被缩放到 0~bitMax-1）。
     */
    RgbPixel  RGBPixel2LAB(RgbPixel pixel, uint32_t bitMax);
    /**
     * @brief 根据拉伸类型将 RGB 像素转换为相应的单通道值（并存储为 RgbPixel 三通道相同）。
     * @param pixel   输入 RGB 像素。
     * @param type    拉伸类型（灰度/HSV/Lab）。
     * @param bitMax  位最大值。
     * @return 转换后的像素（三通道相同，值为拉伸后的值）。
     */
    RgbPixel  RGBPixel2Strtch(RgbPixel pixel, StretchType type, uint32_t bitMax);
    /**
     * @brief 根据拉伸类型将 RGB 像素转换为单个整数值（仅返回拉伸后的通道值）。
     * @param pixel   输入 RGB 像素。
     * @param type    拉伸类型。
     * @param bitMax  位最大值。
     * @return 拉伸后的单通道整数值（如灰度值、V 值或 L 值）。
     */
    int32_t   RGBPixel2StrtchOne(RgbPixel pixel, StretchType type, uint32_t bitMax);
    
    //==================== 快速版本（供内部高性能调用，无安全判断和边界修复） ====================

    /**
     * @brief 快速获取 HSV 的 V（亮度）分量。
     * @param rgb 输入 RGB 像素。
     * @return V = max(R,G,B)。
     */
    uint32_t fastGray(const RgbPixel& rgb);
    /**
     * @brief 快速获取 HSV 的 V（亮度）分量。
     * @param rgb 输入 RGB 像素。
     * @return V = max(R,G,B)。
     */
    uint32_t fastHSV_V(const RgbPixel& rgb);
    /**
     * @brief 快速计算 CIELAB 的 L* 分量（近似）。
     * @param rgb    输入 RGB 像素。
     * @param bitMax 位最大值（用于归一化）。
     * @return L* 值（范围 0~100）。
     */
    uint32_t fastLab_L(const RgbPixel& rgb, uint32_t bitMax);
    /**
     * @brief 根据拉伸类型快速计算单通道拉伸值（高效无分支版本）。
     * @param rgb     输入 RGB 像素。
     * @param type    拉伸类型。
     * @param bitMax  位最大值。
     * @return 拉伸后的单通道值（0~bitMax）。
     */
    uint32_t RGBT2StretchOneFast(const RgbPixel& rgb, StretchType type, uint32_t bitMax);
    /**
     * @brief 将 CIELAB 像素转换回 sRGB 像素。
     * @param lab    Lab 像素（r=L, g=a, b=b，a,b 已量化为 0~maxV-1）。
     * @param maxV   量化最大值（用于还原 a,b 的归一化范围）。
     * @return 转换后的 sRGB 像素。
     */
    RgbPixel  LABPixel2RGB(RgbPixel lab, uint32_t maxValue);
}