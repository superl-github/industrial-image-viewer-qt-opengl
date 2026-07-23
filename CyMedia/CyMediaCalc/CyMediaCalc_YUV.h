#pragma once
#include "../CyMediaBaseDef.h"

using namespace CyMedia;
namespace CyMediaCalc_YUV {
    //计算坐标处颜色
    RgbPixel calcCoordinateColor(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y, YUVTransMethod func = YUVTRANS_NORMAL);
    
    //计算直方图
    bool computeHistogram(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel,
        YUVTransMethod func = YUVTRANS_NORMAL);

    bool computeHistogram_Stretch(const ImageShowInfo& info, const uint8_t* data,
        std::vector<double>& Rhistogram,
        YUVTransMethod func = YUVTRANS_NORMAL,
        StretchType type = stretch_None);

    //图像转换
    bool YUV2RGBConver(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, YUVTransMethod func = YUVTRANS_NORMAL);
}