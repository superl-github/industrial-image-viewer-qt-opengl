#include "CyMediaImageParse.h"
#include "../CyMediaCalc/CyMediaCalc.h"

#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#include <cstdio>  // for _wfopen
#endif

FILE* openFileForReading(const std::filesystem::path& path) {
#ifdef _WIN32
    // Windows 下使用宽字符版本 _wfopen
    return _wfopen(path.wstring().c_str(), L"rb");
#else
    // Linux/macOS 下，path.string() 返回 UTF-8 字符串
    return fopen(path.string().c_str(), "rb");
#endif
}
FILE* openFileForWriting(const std::filesystem::path& path) {
#ifdef _WIN32
    return _wfopen(path.wstring().c_str(), L"wb");
#else
    return fopen(path.string().c_str(), "wb");
#endif
}

void my_stbi_write_func(void* context, void* data, int size) {
    // 将 context 指针恢复为 FILE*
    FILE* fp = static_cast<FILE*>(context);
    // 将数据写入文件
    fwrite(data, 1, size, fp);
}

namespace CyMedia {
    CyMediaImageParse::CyMediaImageParse() {

    }

    std::string_view CyMediaImageParse::imageTypeStr(CyMedia::ImageSuffix type) {
        switch (type) {
            case ImageSuffix::RAW: return "raw";
            case ImageSuffix::BMP: return "bmp";
            case ImageSuffix::TIFF: return "tiff";
            case ImageSuffix::PNG:   return "png";
            case ImageSuffix::JPEG:  return "jpeg";
        }
        return "Undefined";
    }


    CyMedia::ImageSuffix CyMediaImageParse::getTypeByPath(const std::string& filepath) {
        size_t dot = filepath.find_last_of('.');
        if (dot == std::string::npos)
            return ImageSuffix::INVALID;
        
        std::string ext = filepath.substr(filepath.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == "raw")       return ImageSuffix::RAW;
        if (ext == "bmp")       return ImageSuffix::BMP;
        if (ext == "tiff" || ext == "tif") return ImageSuffix::TIFF;
        if (ext == "png")       return ImageSuffix::PNG;
        if (ext == "jpg" || ext == "jpeg") return ImageSuffix::JPEG;

        return ImageSuffix::INVALID;
    }


    int CyMediaImageParse::openImage(std::filesystem::path filePath, CyMedia::ImageSuffix fileType, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data) {
        if (!std::filesystem::is_regular_file(filePath)) {
            return  1;
        }
        //RAW
        if (fileType == ImageSuffix::RAW) {
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
        FILE* fp = openFileForReading(filePath);
        if (!fp) return 1;

        int w, h, channels;
        unsigned char* img = stbi_load_from_file(fp, &w, &h, &channels, 0);
        fclose(fp);

        if (!img) return 2; // invalid file format

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


    int CyMediaImageParse::openImage_NotHeaderRaw(std::filesystem::path filePath, int dataOffset, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data) {//以二进制模式打开文件
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


    int CyMediaImageParse::saveImageToFile(std::filesystem::path filePath, const CyMedia::ImageShowInfo& info, const uint8_t* data, ImageColorOpe opePara, ImageSaveOpe saveOpe) {
        auto fileType = getTypeByPath(filePath.string());

        //RAW
        if (fileType == ImageSuffix::RAW) {
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
            if (saveOpe.rawAddHead) {
                file.write((char*)(&info), sizeof(CyMedia::ImageShowInfo));
                if (!file) return 1;
            }
            //写入数据
            file.write((char*)data, info.length);
            if (!file) return 1;
            return file.good() ? 0 : 1;
        }
        
        //Other
        if (info.bit != 8) {
            return 2;//stb_image只支持8位 TODO 后续压缩处理
        }
        // 不支持的保存格式
        if (fileType != ImageSuffix::BMP &&
            fileType != ImageSuffix::PNG &&
            fileType != ImageSuffix::JPEG) {
            return 2;
        }

        int channels = 0;
        void* writeData = (void*)data;
        uint8_t* opeImageData = nullptr;
        switch (info.format) {
            case CyMedia::MONO: {
                channels = 1;
                if (info.bit <= 8) {
                    
                }
                else if (info.bit <= 16){
                    float scale = 256.0 / (1 << info.bit);
                    uint16_t* psrc = (uint16_t*)data;
                    opeImageData = new uint8_t[info.width * info.height];
                    for (int h = 0; h < info.height; h++) {
                        for (int w = 0; w < info.width; w++) {
                            opeImageData[h * info.width + w] = psrc[h * info.width + w] * scale;
                        }
                    }
                    writeData = opeImageData;
                }
                else if (info.bit <= 31) {
                    float scale = 256.0 / (1 << info.bit);
                    uint32_t* psrc = (uint32_t*)data;
                    opeImageData = new uint8_t[info.width * info.height];
                    for (int h = 0; h < info.height; h++) {
                        for (int w = 0; w < info.width; w++) {
                            opeImageData[h * info.width + w] = psrc[h * info.width + w] * scale;
                        }
                    }
                    writeData = opeImageData;
                }
                else {
                    return 2;
                }
            }break;

            case CyMedia::MONO10P:
            case CyMedia::MONO10P_GVSP:
            case CyMedia::MONO12P:
            case CyMedia::MONO12P_GVSP: {
                opeImageData = new uint8_t[info.width * info.height];
                CyMediaCalc::monoUnPack_8(info, data, opeImageData);
                writeData = opeImageData;

            }break;

            case CyMedia::MONO_OVERSIZE: return 2;

            case CyMedia::RGB: {
                channels = 3;
            }break;

            case CyMedia::RGBA: {
                channels = 4;
            }break;

            case CyMedia::BAYERRG:
            case CyMedia::BAYERGR:
            case CyMedia::BAYERBG:
            case CyMedia::BAYERGB: {
                if (opePara.bayerFunc == DEMOSAIC_NONE) {
                    channels = 1;
                }
                else {
                    channels = 3;
                    opeImageData = new uint8_t[info.width * info.height * 3];
                    CyMediaCalc::bayer2RGB_8(info, data, opeImageData, opePara.bayerFunc);
                    writeData = opeImageData;
                }
            }break;

            case CyMedia::FOURCC_YUY2:
            case CyMedia::FOURCC_YVYU:
            case CyMedia::FOURCC_I422:
            case CyMedia::FOURCC_YV16:
            case CyMedia::FOURCC_I420:
            case CyMedia::FOURCC_YV12:
            case CyMedia::FOURCC_NV12:
            case CyMedia::FOURCC_NV21: {
                if (opePara.YUVFunc == YUVTRANS_Y) {
                    channels = 1;
                }
                else {
                    channels = 3;
                    opeImageData = new uint8_t[info.width * info.height * 3];
                    CyMediaCalc::YUV2RGB(info, data, opeImageData, opePara.YUVFunc);
                    writeData = opeImageData;
                }
            }break;
        }

        FILE* fp = openFileForWriting(filePath);
        if (!fp) return 1;

        int w = info.width, h = info.height;
        int result = 0;

        if (fileType == ImageSuffix::BMP) {
            result = stbi_write_bmp_to_func(my_stbi_write_func, fp, w, h, channels, writeData);
        }
        else if (fileType == ImageSuffix::PNG) {
            result = stbi_write_png_to_func(my_stbi_write_func, fp, w, h, channels, writeData, 0);
        }
        else if (fileType == ImageSuffix::JPEG) {
            result = stbi_write_jpg_to_func(my_stbi_write_func, fp, w, h, channels, writeData, 90);
        }

        if (opeImageData) {
            delete[] opeImageData;
        }

        return (result != 0) ? 0 : 1;
    }
};
