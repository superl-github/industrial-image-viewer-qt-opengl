#pragma once

#include "../CyMediaBaseDef.h"
#include <vector>

namespace CyMedia {
    //计算坐标处颜色
    int32_t calcCoordinateColor_Mono(ImageShowInfo& info, uint8_t* pdata, size_t idx);
    int32_t calcCoordinateColor_Mono10P(const uint8_t* data, size_t idx);
    int32_t calcCoordinateColor_Mono12P(const uint8_t* data, size_t idx);
    int32_t calcCoordinateColor_Mono10P_GVSP(const uint8_t* data, size_t idx);
    int32_t calcCoordinateColor_Mono12P_GVSP(const uint8_t* data, size_t idx);

    //计算直方图
    bool computeHistogram_Mono(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& histogram,
        double* maxPixel, double* minPixel, double* avePixel);

    void computerUniformity_Mono(std::vector<double>& histogram, double& ave, double& maxColor, double* std, double* uniformity);

    //图像转换
    bool Mono10P2MonoConver(ImageShowInfo& info, uint8_t* data, uint16_t* outdata);
    bool Mono12P2MonoConver(ImageShowInfo& info, uint8_t* data, uint16_t* outdata);
    bool Mono10P_GVSP2MonoConver(ImageShowInfo& info, uint8_t* data, uint16_t* outdata);
    bool Mono12P_GVSP2MonoConver(ImageShowInfo& info, uint8_t* data, uint16_t* outdata);
}