#pragma once
#include "CyMediaBaseDef.h"
#include <QObject>

namespace CyMedia {
    class CYMEDIA_LIB CyMdiaImageParse : public QObject {
    public:
        CyMdiaImageParse(QObject* parent = nullptr);

    public:
        static QString fileTypeStr(CyMedia::ImageSuffix type);
        static CyMedia::ImageSuffix getTypeByPath(QString& filepath);
        
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
        static int openImage(QString filePath, CyMedia::ImageSuffix fileType, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data);

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
        static int openImage_NotHeaderRaw(QString filePath, int dataOffset, CyMedia::ImageShowInfo& info, std::vector<uint8_t>& data);

        
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
        static int saveImageToFile(QString filePath, const CyMedia::ImageShowInfo& info, const uint8_t* data, bool addRawHeader);
    };
};
