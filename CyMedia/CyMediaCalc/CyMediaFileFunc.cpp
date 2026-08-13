#include "CyMediaFileFunc.h"

#include <QFile>
#include <QFileInfo>

namespace CyMedia {
    CyMdiaFileFunc::CyMdiaFileFunc(QObject* parent /*= nullptr*/) 
    : QObject(parent){

    }

    QString CyMdiaFileFunc::fileTypeStr(CyMedia::ImageSuffix type) {
        switch (type) {
        case ImageSuffix::RAW: return "raw";
        case ImageSuffix::BMP: return "bmp";
        case ImageSuffix::TIFF: return "tiff";
        }
        return "Undefined";
    }


    CyMedia::ImageSuffix CyMdiaFileFunc::getTypeByPath(QString& filepath) {
        QFileInfo fileInfo(filepath);
        QString suffix = fileInfo.suffix().toLower();
        if (suffix == "raw") {
            return ImageSuffix::RAW;
        }
        else if (suffix == "bmp") {
            return ImageSuffix::BMP;
        }
        else if (suffix == "tiff" || suffix == "tif") {
            return ImageSuffix::TIFF;
        }

        return ImageSuffix::INVALID;
    }


    int CyMdiaFileFunc::openImage(QString filePath, CyMedia::ImageSuffix fileType, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data) {
        QFile tFile(filePath);
        if (false == tFile.exists()) return 1;
        switch (fileType) {
            case ImageSuffix::RAW: {
                if (false == tFile.open(QIODevice::ReadOnly)) {
                    return 1;
                }
                uint32_t fileSize = tFile.size();
                //读取头
                int headSize = sizeof(CyMedia::ImageShowInfo);
                if (fileSize < headSize) return 1;
                auto headCode = tFile.read(headSize);
                if (headCode.size() != headSize) {
                    tFile.close();
                    return 1;
                }
                memcpy(&info, headCode.data(), headSize);
                //验证长度
                if (fileSize != info.length + headSize) {
                    tFile.close();
                    return 3;
                }
                data.resize(info.length);
                auto imgCode = tFile.readAll();
                tFile.close();
                if (imgCode.size() != info.length) {
                    return 1;
                }
                memcpy(data.data(), imgCode.data(), info.length);
                return 1;
            }break;

            case ImageSuffix::BMP: {
                return 2;
            }break;

            case ImageSuffix::TIFF: {
                return 2;
            }break;

            case ImageSuffix::PNG: {
                return 2;
            }break;

            case ImageSuffix::JPEG: {
                return 2;
            }break;
        }

        return 2;
    }


    int CyMdiaFileFunc::openImage_NotHeaderRaw(QString filePath, int dataOffset, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data) {
        QFile rawFile(filePath);
        if (false == rawFile.exists()) return 1;
        uint32_t fileSize = rawFile.size();
        if (fileSize < dataOffset + info.length) return 1;
        if (false == rawFile.open(QIODevice::ReadOnly)) return 1;
        rawFile.seek(dataOffset);
        auto imgCode = rawFile.read(info.length);
        rawFile.close();
        if (imgCode.size() != info.length) return 1;
        data.resize(info.length);
        memcpy(data.data(), imgCode.data(), info.length);
        return 0;
    }


    int CyMdiaFileFunc::saveImageToFile(QString filePath, const CyMedia::ImageShowInfo& info, const uint8_t* data, bool addRawHeader) {
        auto fileType = getTypeByPath(filePath);
        switch (fileType) {
            case ImageSuffix::RAW: {
                QFile tFile(filePath);
                if (false == tFile.open(QIODevice::ReadOnly)) return 1;
                tFile.write((char*)(&info));
                tFile.write((char*)data, info.length);
                tFile.waitForBytesWritten(1000);
                return 0;
            }break;

            case ImageSuffix::BMP: {
                return 3;
            }break;

            case ImageSuffix::TIFF: {
                return 3;
            }break;

            case ImageSuffix::PNG: {
                return 3;
            }break;

            case ImageSuffix::JPEG: {
                return 3;
            }break;
        }
        return 3;
    }

};
