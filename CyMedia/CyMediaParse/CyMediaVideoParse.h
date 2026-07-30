/**
 * @file CyMediaVideoParse.h
 * @brief CyMedia 视频解析器统一入口（对外接口）。
 * @details 提供视频文件解析、帧获取、播放控制等完整功能。
 * @author LLF
 * @version 1.0
 */

#pragma once
#include "CyMediaBaseDef.h"

#include <memory>
#include <string>
#include <vector>

namespace CyMedia {
    /**
     * @brief CyMedia 视频解析器前端类。
     * @details 单一入口，自动识别文件格式（根据后缀 .raw/.avi/.mp4）。
     *          - 支持主动拉取（getFrame）和被动推送（回调 + 异步线程）两种模式。
     *          - 切换文件时调用 open()，内部自动释放旧资源并创建新解析器。
     * @note 所有方法均为线程安全。
     */
    class CYMEDIA_LIB VideoParser {

    public:
        /**
         * @brief 构造函数。
         */
        VideoParser();

        /**
         * @brief 析构函数，自动关闭文件并释放资源。
         */
        ~VideoParser();

    public:
        static std::string videoTypeStr(CyMedia::VideoSuffix type);
        static CyMedia::VideoSuffix getvideoTypeByPath(const std::string& filepath);

    public:
        //==================== 文件控制 ====================

        /**
         * @brief 打开视频文件（自动识别格式）。
         * @param filePath 文件路径（UTF-8 编码）。
         * @return 成功返回 true，失败（文件不存在/格式不支持/数据损坏）返回 false。
         * @note 若当前已打开文件，将自动关闭并释放资源。
         */
        bool open(const std::string& filePath);

        /**
         * @brief 关闭当前文件，释放所有资源（线程、缓存、句柄）。
         * @details 关闭后对象可复用，再次调用 open() 即可打开新文件。
         */
        void close();

        /**
         * @brief 检查当前是否已成功打开文件。
         * @return 已打开且有效返回 true，否则 false。
         */
        bool isOpen() const;

        //==================== 元数据查询 ====================

        /**
         * @brief 获取当前视频的图像属性信息。
         * @return CyMedia::ImageShowInfo 结构体（宽、高、像素格式等）。
         *         若未打开文件，返回默认空结构（width == 0）。
         * @see CyMedia::ImageShowInfo
         */
        ImageShowInfo getImageInfo() const;

        /**
         * @brief 获取视频总帧数。
         * @return 总帧数（索引从 1 开始），未打开时返回 0。
         */
        uint32_t getFrameCount() const;

        /**
         * @brief 获取视频原始帧率。
         * @return 帧率（帧/秒），未打开时返回 0.0f。
         */
        float getFramerate() const;

        //==================== 主动获取帧（同步） ====================

        /**
         * @brief 获取指定索引的帧数据。
         * @param index 帧索引（从 1 开始，范围 1 ~ getFrameCount()）。
         * @param outData 输出缓冲区（若为空 vector 则自动分配内存）。
         * @return 成功返回 true，失败（索引越界或读取错误）返回 false。
         */
        bool getFrame(uint32_t index, std::vector<uint8_t>& outData);

        //==================== 异步推送（回调模式） ====================

        /**
         * @brief 注册帧数据回调函数。
         * @param callback 回调函数（可为空以取消注册）。
         * @param userData 用户自定义指针，回调时原样透传。
         * @note 注册后需调用 play() 启动推送线程。
         * @see play(), setPause()
         */
        void registerFrameCallback(FrameCallback callback, void* userData = nullptr);

        /**
         * @brief 启动或恢复异步播放（非阻塞）。
         * @details 内部线程按帧率持续推送帧至回调函数。
         *          若已处于播放状态，调用无副作用。
         */
        void play();

        /**
         * @brief 暂停或恢复异步播放。
         * @param pause true 暂停，false 恢复。
         */
        void setPause(bool pause);

        /**
         * @brief 查询暂停状态。
         * @return 暂停返回 true，播放中或未打开返回 false。
         */
        bool isPaused() const;

        /**
         * @brief 跳转到指定帧（异步模式）。
         * @param pos 目标帧索引（从 1 开始）。
         * @return 成功返回 true，索引超限返回 false。
         * @note 跳转后若当前为播放状态，会从新位置继续推送。
         */
        bool seek(uint32_t pos);

        /**
         * @brief 获取当前异步播放进度（最近推送的帧索引）。
         * @return 当前帧索引（从 1 开始），未推送时返回 0。
         */
        uint32_t getCurrentPosition() const;

        /**
         * @brief 设置播放速度倍率。
         * @param speed 倍率（必须 > 0）。1.0f 为原速，2.0f 为两倍速，0.5f 为半速。
         */
        void setSpeed(float speed);

    private:
        class Private;
        Private* d = nullptr;
    };
}
