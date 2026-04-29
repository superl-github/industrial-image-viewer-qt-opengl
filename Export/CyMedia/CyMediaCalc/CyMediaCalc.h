#pragma once
#include "../CyMediaBaseDef.h"

#include <Vector>

namespace CyMedia {
    //计算图像坐标处像素颜色
    void calcCoordinateColor(CyMedia::ImageShowInfo& info, uint8_t* pdata, int x, int y, double* colorR, double* colorG, double* colorB, DemosaicMethod func = BILINEAR);

    //计算图像直方图
    bool computeGrayHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& histogramVec,
        double* maxPixel, double* minPixel, double* avePixel);

    bool computeBayerHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel,
        DemosaicMethod func = BILINEAR);

    bool computeBayerHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo,
        std::vector<double>& histogram,
        StretchType type = stretch_None,
        DemosaicMethod func = BILINEAR);

    bool computeRGBHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& Rhistogram,
        std::vector<double>& Ghistogram,
        std::vector<double>& Bhistogram,
        std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel);

    bool computeRGBHistogram(
        uint8_t* pData, CyMedia::ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask,
        std::vector<double>& histogram,
        StretchType strytchType = stretch_Gray);

    //计算自动拉伸参数 RgbPixel.r->start RgbPixel.g->end
    void computeGrayStretchPara(std::vector<double>& histogram, int32_t& start, int32_t& end);

    //计算均匀度
    void computerUniformity(std::vector<double>& histogram, double& ave, double& maxColor, double* std, double* uniformity);
    void computerThreeUniformity(
        std::vector<double>& histogram_1, std::vector<double>& histogram_2, std::vector<double>& histogram_3,
        std::vector<double>& ave, std::vector<double>& maxColor, 
        std::vector<double>& std, std::vector<double>& uniformity);


    //图像转换
    bool monoUnPack(CyMedia::ImageShowInfo& info, uint8_t* data, uint8_t* out_data);

    bool bayer2RGBConvert(CyMedia::ImageShowInfo& info, uint8_t* data, uint8_t* out_data);

    void copyAlignImage(void* pSrc, void* pAlign, int srcWidth, int srcHeight, int AlignWidth, int AlignHeight, int pixelSize);
}
