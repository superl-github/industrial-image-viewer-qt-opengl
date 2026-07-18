#include "../CyMediaBaseDef.h"

#include <vector>

namespace CyMedia {
    //计算坐标处颜色
    RgbPixel calcCoordinateColor_Bayer(const ImageShowInfo& info, const uint8_t* pdata, int32_t x, int32_t y, DemosaicingMethod func = DEMOSAIC_BILINEAR);

    //计算直方图
    bool computeHistogram_Bayer(const ImageShowInfo& info, const uint8_t* data, std::vector<uint8_t>* mask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel,
        DemosaicingMethod func = DEMOSAIC_BILINEAR);

    //图像转换
    bool Bayer2RGBConver(const ImageShowInfo& info, const uint8_t* data, uint8_t* outdata, DemosaicingMethod func = DEMOSAIC_BILINEAR);
}