#include "CyMediaImageParse.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace CyMedia {
    CyMediaImageParse::CyMediaImageParse() {

    }

    std::string_view CyMediaImageParse::imageTypeStr(CyMedia::ImageSuffix type) {
        switch (type) {
            case CyMedia::IMAGE_SUFFIX_RAW: return "raw";
            case CyMedia::IMAGE_SUFFIX_BMP: return "bmp";
            case CyMedia::IMAGE_SUFFIX_TIFF: return "tiff";
            case CyMedia::IMAGE_SUFFIX_PNG:   return "png";
            case CyMedia::IMAGE_SUFFIX_JPEG:  return "jpeg";
        }
        return "Undefined";
    }


    CyMedia::ImageSuffix CyMediaImageParse::getTypeByPath(const std::string& filepath) {
        size_t dot = filepath.find_last_of('.');
        if (dot == std::string::npos)
            return CyMedia::IMAGE_SUFFIX_INVALID;
        
        std::string ext = filepath.substr(filepath.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == "raw")       return CyMedia::IMAGE_SUFFIX_RAW;
        if (ext == "bmp")       return CyMedia::IMAGE_SUFFIX_BMP;
        if (ext == "tiff" || ext == "tif") return CyMedia::IMAGE_SUFFIX_TIFF;
        if (ext == "png")       return CyMedia::IMAGE_SUFFIX_PNG;
        if (ext == "jpg" || ext == "jpeg") return CyMedia::IMAGE_SUFFIX_JPEG;

        return CyMedia::IMAGE_SUFFIX_INVALID;
    }


    int CyMediaImageParse::openImage(const std::string& filePath, CyMedia::ImageSuffix fileType, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data) {
        if (!std::filesystem::is_regular_file(filePath)) {
            return  1;
        }
        //RAW
        if (fileType == IMAGE_SUFFIX_RAW) {
            uint32_t fileSize = std::filesystem::file_size(filePath);
            int headSize = sizeof(CyMedia::ImageShowInfo);
            if (fileSize < headSize) return 1;
            //打开文件
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) {
                return 1;
            }
            //读取头
            file.read((char*)(&info), headSize);
            //验证长度
            if (fileSize != info.length + headSize) {
                file.close();
                return 3;
            }
            data.resize(info.length);
            file.read((char*)data.data(), info.length);
            file.close();
            return 0;
        }
        //Other
        int w, h, channels;
        unsigned char* img = stbi_load(filePath.c_str(), &w, &h, &channels, 0);
        if (!img)
            return 2; // invalid file format

        info.width = w;
        info.height = h;
        info.bit = 8; // stb_image 默认返回 8 位
        info.special_pixel = PIXEL_VALUE_INT;

        switch (channels) {
        case 1: info.format = MONO; break;
        case 3: info.format = RGB;  break;
        case 4: info.format = RGBA; break;
        default:
            stbi_image_free(img);
            return 2;
        }

        info.length = w * h * channels;
        data.resize(info.length);
        memcpy(data.data(), img, info.length);
        stbi_image_free(img);
        return 0;
    }


    int CyMediaImageParse::openImage_NotHeaderRaw(const std::string& filePath, int dataOffset, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data) {//以二进制模式打开文件
        if (!std::filesystem::is_regular_file(filePath)) {
            return  1;// file error
        }
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return 1;
        }
        //判断文件大小
        auto fileSize = std::filesystem::file_size(filePath);
        if (fileSize < dataOffset + info.length) return 3;
        //读取数据
        file.seekg(dataOffset, std::ios::beg);
        if (!file)
            return 3; // header error (seek failed)
        data.resize(info.length);
        file.read((char*)data.data(), info.length);
        if (!file)
            return 1; // file read error
        file.close();
        return 0;
    }


    int CyMediaImageParse::saveImageToFile(std::string filePath, const CyMedia::ImageShowInfo& info, const uint8_t* data, bool addRawHeader) {
        auto fileType = getTypeByPath(filePath);

        //RAW
        if (fileType == CyMedia::IMAGE_SUFFIX_RAW) {
            //确保父目录存在，如果不存在则自动创建
            std::filesystem::path path(filePath);
            if (path.has_parent_path()) {
                std::filesystem::create_directories(path.parent_path());
            }
            // 2. 以二进制模式打开文件
            std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                return 1;
            }
            //写入头信息
            file.write((char*)(&info), sizeof(CyMedia::ImageShowInfo));
            if (!file)
                return 1;
            //写入数据
            file.write((char*)data, info.length);
            if (!file) {
                return 1;
            }
            return file.good() ? 0 : 1;
        }
        
        //Other
        if (info.bit != 8) {
            return 2;//stb_image只支持8位 TODO 后续压缩处理
        }
        int channels = 0;
        if (info.format == MONO)       channels = 1;
        else if (info.format == RGB)   channels = 3;
        else if (info.format == RGBA)  channels = 4;
        else
            return 2; // 不支持的像素格式 TODO 其他格式转为RGB或者Mono

        int w = info.width, h = info.height;
        int result = 0;

        if (fileType == IMAGE_SUFFIX_BMP) {
            result = stbi_write_bmp(filePath.c_str(), w, h, channels, data);
        }
        else if (fileType == IMAGE_SUFFIX_PNG) {
            result = stbi_write_png(filePath.c_str(), w, h, channels, data, 0);
        }
        else if (fileType == IMAGE_SUFFIX_JPEG) {
            result = stbi_write_jpg(filePath.c_str(), w, h, channels, data, 90);
        }
        else {
            return 2; // 不支持的保存格式
        }

        return (result != 0) ? 0 : 1;
    }
};
