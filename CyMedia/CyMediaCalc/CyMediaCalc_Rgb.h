#include "../CyMediaBaseDef.h"
#include <vector>

using namespace CyMedia;
namespace CyMediaCalc_RGB {
    //计算坐标处颜色
    RgbPixel calcCoordinateColor(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y);

    //计算直方图
    bool computeHistogram(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel);

    //计算拉伸直方图
    bool computeHistogram_Stretch(const ImageShowInfo& info, const uint8_t* data,
        std::vector<double>& histogram,
        CyMedia::StretchType type = stretch_None);

    //JPG2RGB
    bool JPG2RGB(const CyMedia::ImageShowInfo& info, const void* data, void* out_data, bool& isGray);
}