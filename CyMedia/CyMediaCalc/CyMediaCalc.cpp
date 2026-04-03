#include "CyMediaCalc.h"
#include "CyMdiaCalc_Mono.h"
#include "CyMdiaCalc_Bayer.h"
#include "CyMdiaCalc_Rgb.h"

#include <cstdlib>

#if defined _MSC_VER
#include <ppl.h>
#include <thread>
#endif

namespace CyMedia {
    void calcCoordinateColor(ImageShowInfo& info, uint8_t* pdata, int x, int y, double* colorR, double* colorG, double* colorB, DemosaicMethod func/* = BILINEAR*/) {
        unsigned int imageW = info.width;
        unsigned int imageH = info.height;
        if (x < 0 || y < 0 || x >= imageW || y >= imageH) {
            return;
        }
        double r = 0.0, g = 0.0, b = 0.0;
        //计算
        if (info.special_value == IMGVALUE_F32) {
            r = ((float*)pdata)[y * info.width + x];
            g = r;
            b = r;
        }
        else if (info.special_value == IMGVALUE_F64) {
            r = ((double*)pdata)[y * info.width + x];
            g = r;
            b = r;
        }
        else {
            switch (info.format) {
                case CyMedia::MONO: 
                case CyMedia::MONO_OVERSIZE:{
                    r = calcCoordinateColor_Mono(info, pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::MONO10P: {
                    r = calcCoordinateColor_Mono10P(pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::MONO10P_GVSP: {
                    r = calcCoordinateColor_Mono10P_GVSP(pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::MONO12P: {
                    r = calcCoordinateColor_Mono12P(pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::MONO12P_GVSP: {
                    r = calcCoordinateColor_Mono12P_GVSP(pdata, y * info.width + x);
                    g = r;
                    b = r;
                }break;

                case CyMedia::BAYERRG:
                case CyMedia::BAYERGR:
                case CyMedia::BAYERBG:
                case CyMedia::BAYERGB: {
                    auto tempRGB = calcCoordinateColor_Bayer(info, pdata, x, y, func);
                    r = tempRGB.r;
                    g = tempRGB.g;
                    b = tempRGB.b;
                }break;

                case CyMedia::RGB: {
                    auto tempRGB = calcCoordinateColor_RGB(info, pdata, x, y);
                    r = tempRGB.r;
                    g = tempRGB.g;
                    b = tempRGB.b;
                }break;
            }
        }

        //赋值
        if (colorR)
            *colorR = r;
        if (colorG)
            *colorG = g;
        if (colorB)
            *colorB = b;
    }

    bool computeGrayHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& histogramVec,
        double* maxPixel, double* minPixel, double* avePixel) {
        if (false == imageinfo.isMono())
            return false;
        return computeHistogram_Mono(imageinfo, pData, calcMask, useMask, histogramVec,
            maxPixel, minPixel, avePixel);
    }

    bool computeBayerHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel, DemosaicMethod func /*= BILINEAR*/) {
        if (false == imageinfo.isBayer())
            return false;
        return computeHistogram_Bayer(imageinfo, pData, calcMask, useMask, Rhistogram, Ghistogram, Bhistogram, maxPixel, minPixel, avePixel, func);
    }

    bool computeBayerHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<double>& histogram, StretchType type /*= stretch_None*/, DemosaicMethod func/* = BILINEAR*/) {
        if (false == imageinfo.isBayer())
            return false;

        //源数据
        if (func == BAYERSOUCE) {
            double max, min, ave;
            return computeHistogram_Mono(imageinfo, pData, 0, false, histogram, &max, &min, &ave);
        }
        //转RGB
        
        //统计直方图
        
        return false;
    }

    bool computeRGBHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& Rhistogram, std::vector<double>& Ghistogram, std::vector<double>& Bhistogram, std::vector<double>& maxPixel, std::vector<double>& minPixel, std::vector<double>& avePixel) {
        if (imageinfo.format != RGB)
            return false;
        return computeHistogram_RGB(imageinfo, pData, calcMask, useMask, Rhistogram, Ghistogram, Bhistogram,
            maxPixel, minPixel, avePixel);
    }
    bool computeRGBHistogram(uint8_t* pData, ImageShowInfo& imageinfo, std::vector<uint8_t>* calcMask, bool useMask, std::vector<double>& histogram, StretchType strytchType /*= stretch_Gray*/) {
        if (false == imageinfo.isRGB()) {
            return false;
        }

        return computeHistogram_RGBTrans(imageinfo, pData, calcMask, useMask, histogram, strytchType);
    }

    void computeGrayStretchPara(std::vector<double>& histogram, int32_t& start, int32_t& end) {
        //直方图平均值
        double hisAve = 0.0;
        uint32_t t_StretchSFlag = 0, t_StretchEFlag = 0;
        start = 0;
        end = 0;
        for (auto& oneValue : histogram) {
            hisAve += oneValue;
        }
        hisAve = hisAve / histogram.size();
        //起始值计算
        t_StretchSFlag = hisAve * 0.15 + 0.4;
        for (int hisI = 1; hisI < histogram.size(); hisI++) {
            if (histogram[hisI] > t_StretchSFlag) {
                start = hisI;
                break;
            }
        }
        //末尾值计算
        t_StretchEFlag = hisAve * 0.15 + 0.4;
        for (int hisI = histogram.size() - 2; hisI > start; hisI--) {
            if (histogram[hisI] >= t_StretchEFlag) {
                end = hisI;
                break;
            }
        }
        //边际校正
        if (start >= histogram.size())
            end = histogram.size();
        if (end <= start)
            end = start + 1;
    }

    void computerUniformity(std::vector<double>& histogram, double& ave, double& maxColor, double* std, double* uniformity) {
        return computerUniformity_Mono(histogram, ave, maxColor, std, uniformity);
    }

    void computerThreeUniformity(std::vector<double>& histogram_1, std::vector<double>& histogram_2, std::vector<double>& histogram_3, std::vector<double>& ave, std::vector<double>& maxColor, std::vector<double>& std, std::vector<double>& uniformity) {
        if (std.size() < 3)
            std.resize(3);
        if (uniformity.size() < 3)
            uniformity.resize(3);
        computerUniformity_Mono(histogram_1, ave[0], maxColor[0], &std[0], &uniformity[0]);
        computerUniformity_Mono(histogram_2, ave[1], maxColor[1], &std[1], &uniformity[1]);
        computerUniformity_Mono(histogram_3, ave[2], maxColor[2], &std[2], &uniformity[2]);
    }

    bool monoUnPack(ImageShowInfo& info, uint8_t* data, uint8_t* out_data) {
        if (info.format == MONO10P) {
            return Mono10P2MonoConver(info, data, (uint16_t*)out_data);
        }
        else if (info.format == MONO10P_GVSP) {
            return Mono10P_GVSP2MonoConver(info, data, (uint16_t*)out_data);
        }
        else if (info.format == MONO12P) {
            return Mono12P2MonoConver(info, data, (uint16_t*)out_data);
        }
        else if (info.format == MONO12P_GVSP) {
            return Mono12P_GVSP2MonoConver(info, data, (uint16_t*)out_data);
        }

        return false;
    }

    bool bayer2RGBConvert(ImageShowInfo& info, uint8_t* data, uint8_t* out_data) {
        return Bayer2RGBConver(info, data, out_data);
    }

    void copyAlignImage(void* pSrc, void* pAlign, int srcWidth, int srcHeight, int AlignWidth, int AlignHeight, int pixelSize) {
        uint32_t copySize = pixelSize * srcWidth;
        uint32_t alignSize = pixelSize * AlignWidth;
        uint8_t* pSrc_byte = (uint8_t*)pSrc;
        uint8_t* pAlign_byte = (uint8_t*)pAlign;

        for (int32_t rowIndex = 0; rowIndex < srcHeight; rowIndex++) {
            memcpy(&pAlign_byte[rowIndex * alignSize], &pSrc_byte[rowIndex * copySize], copySize);
        }
    }

}
