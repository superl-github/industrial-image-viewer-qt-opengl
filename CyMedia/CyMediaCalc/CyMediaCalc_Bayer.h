#include "../CyMediaBaseDef.h"

#include <vector>

using namespace CyMedia;
namespace CyMediaCalc_Bayer {
    //计算坐标处颜色
    RgbPixel calcCoordinateColor(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y, DemosaicingMethod func = DEMOSAIC_BILINEAR);

    //计算直方图
    bool computeHistogram(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel,
        DemosaicingMethod func = DEMOSAIC_BILINEAR);

    bool computeHistogram_Stretch(const ImageShowInfo& info, const uint8_t* data,
        std::vector<double>& Rhistogram,
        DemosaicingMethod func = DEMOSAIC_BILINEAR,
        CyMedia::StretchType type = stretch_None);

    //图像转换
    bool bayer2RGB(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, DemosaicingMethod func = DEMOSAIC_BILINEAR);
}