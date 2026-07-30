// CyMediaVideoParseBase.h
#pragma once
#include "CyMediaBaseDef.h"
#include <functional>
#include <string>
#include <vector>

namespace CyMedia {
    /**
     * @brief 视频解析器抽象基类。
     * @details 定义了视频文件解析的标准接口，支持主动获取（同步）和
     *          被动回调（异步推流）两种工作模式。派生类需实现具体容器（RAW/AVI/MP4）的解析逻辑。
     * @note 所有接口均为线程安全设计，派生类内部需自行加锁保护共享资源。
     */
    class VideoParseBase {
    public:
        virtual ~VideoParseBase() = default;

        //==================== 文件控制 ====================
        /**
         * @brief 打开视频文件。
         * @param filePath 文件绝对路径或相对路径（UTF-8编码）。
         * @return 成功返回 true，失败返回 false（如文件不存在、格式不匹配）。
         */
        virtual bool open(const std::string& filePath) = 0;

        /**
         * @brief 关闭当前打开的文件，释放所有资源（包括线程和缓存）。
         * @details 关闭后对象可复用，再次调用 open() 即可解析新文件。
         */
        virtual void close() = 0;

        /**
         * @brief 检查文件是否已成功打开且处于有效状态。
         * @return 已打开且有效返回 true，否则 false。
         */
        virtual bool isOpen() const = 0;

        //==================== 元数据查询 ====================
        /**
         * @brief 获取当前视频帧的图像属性。
         * @return ImageShowInfo 结构体，包含宽、高、位深、像素格式等。
         *         若文件未打开，返回默认空结构（宽度为0）。
         * @see CyMedia::ImageShowInfo
         */
        virtual ImageShowInfo getImageInfo() const = 0;

        /**
         * @brief 获取视频文件总帧数。
         * @return 总帧数（索引从 1 开始）。
         */
        virtual uint32_t getFrameCount() const = 0;

        /**
         * @brief 获取视频原始帧率。
         * @return 帧率（帧/秒），若无法获取则返回 0.0f。
         */
        virtual float getFramerate() const = 0;

        //==================== 主动获取帧（同步模式） ====================
        /**
         * @brief 获取指定索引位置的帧数据。
         * @details 该操作直接从文件中读取，不影响异步播放进度（currentPos 不变）。
         *          调用者需保证 outData 已预分配足够内存，或传入空 vector 由内部自动分配。
         * @param index 帧索引（从 1 开始，范围 1 ~ getFrameCount()）。
         * @param outData 输出缓冲区（引用），函数执行后包含完整的帧数据。
         * @return 成功返回 true，失败（索引越界或文件错误）返回 false。
         */
        virtual bool getFrame(uint32_t index, std::vector<uint8_t>& outData) = 0;

        //==================== 被动回调推流（异步模式） ====================
        /**
         * @brief 注册帧数据回调函数。
         * @details 注册后，当异步播放线程推送一帧时，该回调将被触发。
         *          回调执行期间，数据指针有效，但回调返回后数据可能被覆盖，外部需及时拷贝。
         * @param callback 回调函数对象（可为 nullptr 以取消回调）。
         * @param userData 用户自定义数据，会在回调时原样透传。
         */
        virtual void registerFrameCallback(FrameCallback callback, void* userData = nullptr) = 0;

        /**
         * @brief 开始或恢复异步播放（非阻塞）。
         * @details 调用后内部线程开始按帧率推送数据。若当前处于暂停状态，将自动恢复。
         */
        virtual void play() = 0;

        /**
         * @brief 暂停或恢复异步播放。
         * @param pause true 暂停推流，false 恢复推流（从当前帧继续）。
         */
        virtual void setPause(bool pause) = 0;

        /**
         * @brief 获取当前暂停状态。
         * @return 暂停中返回 true，播放中返回 false。
         */
        virtual bool isPaused() const = 0;

        /**
         * @brief 跳转到指定帧（异步模式下）。
         * @details 跳转后，下一次 push 将从该帧开始。若当前处于播放状态，会丢弃积压的帧。
         * @param pos 目标帧索引（从 1 开始）。
         * @return 成功返回 true，索引超限返回 false。
         */
        virtual bool seek(uint32_t pos) = 0;

        /**
         * @brief 获取当前异步播放进度（最近一次推送的帧索引）。
         * @return 当前帧索引（从 1 开始），若从未推送则返回 0。
         */
        virtual uint32_t getCurrentPosition() const = 0;

        /**
         * @brief 设置播放速度倍率。
         * @param speed 倍率（必须 > 0）。1.0f 为原速，2.0f 为两倍速，0.5f 为半速。
         * @note 速度改变会影响帧间隔时间，内部计时器将自动重校准。
         */
        virtual void setSpeed(float speed) = 0;
    };

} // namespace CyMedia