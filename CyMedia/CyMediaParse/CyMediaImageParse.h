/**
* @file CyMediaImageParse.h
* @brief CyMediaImageParse头文件，对外API
* @ingroup FileParse
*
* @details 提供图像文件解析、保存数据到文件等功能。
* 
* @author LLF
* @version 1.0
*/
#pragma once
#include "CyMediaBaseDef.h"

#include <xstring>
#include <filesystem>

namespace CyMedia {
    struct ImageSaveOpe {
        bool rawAddHead = true;
    };

    /**
     * @brief CyMedia 图像解析类。
     * @details 
     *  支持`CyMedia::ImageSuffix`列举的格式的文件解析为与`CyMedia::ImageShowInfo`符合的图像数据。
     *  支持`CyMedia::ImageShowInfo`图像数据保存为`CyMedia::ImageSuffix`格式的文件
     */
    class CYMEDIA_LIB CyMediaImageParse {
    public:
        CyMediaImageParse();

    public:
        static std::string_view imageTypeStr(CyMedia::ImageSuffix type);
        static CyMedia::ImageSuffix getTypeByPath(const std::string& filepath);
        
        /**
         * @brief openImage
         * @details Open image file
         * 
         * @param[in] filePath :Full file path
         * @param[in] fileType :File type
         * @param[in] info :Parsed image information
         * @param[in] data :Parsed image data
         * @ref  
         * @return int 0:success 1:file error 2:Invalid file format 3:not RawHeader
        ***/
        static int openImage(std::filesystem::path filePath, CyMedia::ImageSuffix fileType, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data);

        /**
         * @brief openImage_NotHeaderRaw
         * @details Open a raw image file.
         * 
         * @param[in] filePath:Full file path
         * @param[in] dataOffset:Image data offset
         * @param[in] info:Designated Information
         * @param[out] data:Parsed image data
         * @ref  
         * @return int 0:success 1:file error 3:header error
        ***/
        static int openImage_NotHeaderRaw(std::filesystem::path filePath, int dataOffset, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data);

        
        /**
         * @brief Save image to file
         * @details Open a raw image file.
         *
         * @param[in] filePath:Full file path
         * @param[in] dataOffset:Image data
         * @param[in] info:image Information
         * @param[out] data:image data
         * @ref
         * @return int 0:success 1:file error 2:Invalid file format
        ***/
        static int saveImageToFile(std::filesystem::path filePath, const CyMedia::ImageShowInfo& info, const uint8_t* data, ImageColorOpe opePara, ImageSaveOpe saveOpe);
    };
};
