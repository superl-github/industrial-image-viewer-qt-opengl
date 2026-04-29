#include "../CyMediaBaseDef.h"
#include <vector>

namespace CyMedia {
    //计算坐标处颜色
    RgbPixel calcCoordinateColor_RGB(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y);

    //计算直方图
    bool computeHistogram_RGB(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel);

    //计算拉伸直方图
    bool computeHistogram_RGBTrans(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& histogram,
        CyMedia::StretchType type = stretch_None);

    //转灰度
    RgbPixel RGBPixel2rgb2Gray(RgbPixel pixel, float maxValue);

    //转HSV
    RgbPixel RGBPixel2rgb2hsv(RgbPixel pixel, float maxValue);

    //转LAB
    RgbPixel RGBPixel2LAB(RgbPixel pixel, float maxValue);
    //lab.l:0~100
    RgbPixel LABPixel2RGB(RgbPixel lab, float maxValue);
}