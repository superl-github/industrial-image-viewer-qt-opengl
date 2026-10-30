/**
 * @file Display.h
 * @brief CyMedia 图像显示模块的唯一主入口（对外 API）。
 * @ingroup Display
 *
 * @details
 *  `CyMedia::CyMediaDis` 是一个基于 Qt Widgets / QGraphicsView / OpenGL 的
 *  高性能图像显示控件，面向工业相机、机器视觉、医疗影像等实时图像场景。
 *
 *  ## 它提供什么
 *  - **多格式渲染**：Mono / Bayer / RGB / RGBA / BGR / YUV（Packed、Planar、
 *    Semi-Planar）/ MJPG，支持 8/10/12/16/32 位与浮点像素。
 *  - **图像增强**：ColorMap、灰度拉伸（Gray/HSV/Lab）、Bayer 去马赛克、YUV→RGB。
 *  - **交互视图**：缩放、平移、旋转、水平/垂直翻转、自适应、缩略图导航。
 *  - **图形标注**：点、矩形、线、椭圆、多边形；支持编辑、删除、ROI 统计。
 *  - **辅助面板**：灰度拉伸面板、灰度测试/直方图面板、录像计时器（可选显示）。
 *
 *  ## 最小用法
 *  @code{.cpp}
 *  // main.cpp：必须在 QApplication 创建前开启上下文共享
 *  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
 *  QApplication app(argc, argv);
 *
 *  // 业务窗口
 *  auto* display = new CyMedia::CyMediaDis(parent);
 *  layout->addWidget(display);
 *
 *  // 推送一帧（data 在函数返回后即可释放，控件内部按需拷贝）
 *  CyMedia::ImageShowInfo info(width, height, bit, CyMedia::MONO);
 *  display->upImageData(info, data);
 *
 *  // 选择交互方式
 *  display->setColorMap("jet");
 *  display->setDrawMode(CyDisDrawItem::Rectangle);
 *  display->setThumbnailAutoEnable(true);
 *  @endcode
 *
 *  ## 线程模型（重要）
 *  - `upImageData()` 可从任意线程调用；内部有有界帧队列 + 独立处理线程。
 *  - 慢速数据源会自动逐帧缓存原始帧，便于参数变化后重处理；
 *    低延迟的单帧/同步场景可用 `setDirectUpImage(true)` 强制直接处理。
 *  - 所有信号默认在控件所属线程（通常是 UI 线程）投递。
 *  - OpenGL 需要 Qt 共享上下文，务必在 `QApplication` 之前设置
 *    `Qt::AA_ShareOpenGLContexts`。
 *
 *  ## 模块边界
 *  本头文件只暴露稳定 API。图像处理管线、OpenGL 渲染器、标注实现均在
 *  `src/View` 内部；需要原始 RAW 参数对话框请单独包含
 *  `View/Dialogs/CyMediaDisRawInfoDialog.h`。
 *
 *  @see CyMediaDisView, CyMediaDisViewBckDraw, CyMediaDisViewThumbnail
 *  @see View/README.md
 */
#pragma once

#include "common/Export.h"
#include "common/Language.h"
#include "common/Log.h"
#include "DisplayTypes.h"

#include <QColor>
#include <QFrame>
#include <QList>
#include <QPainterPath>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QUuid>

namespace CyDisDrawItem {
    class BaseItem;
}

namespace CyMedia {

    /**
     * @class CyMedia::CyMediaDis
     * @brief 高性能图像显示与交互控件（Qt QFrame 集成窗口）。
     *
     * @details
     *  对外只暴露"输入图像 / 设置显示 / 操作视图 / 管理标注"四类能力。
     *  内部由 `privateData` 组合渲染层、图像管线、标注层和辅助面板。
     * @threadsafety
     *  upImageData / setDirectUpImage / registerImageCallBack：任意线程
     *  其余所有 API：必须从对象所属线程（通常 UI）调用
     *  回调 registerImageCallBack：worker 线程执行
     */
    class CYMEDIA_LIB CyMediaDis : public QFrame {
        Q_OBJECT

    public:
        /// 图形标注类型的短别名，便于业务代码书写。
        using DrawItemType = CyDisDrawItem::ItemType;

        /**
         * @brief 弹出一个独立窗口显示单张图像。
         * @param info 图像信息。
         * @param data 图像数据（内部会拷贝/处理）。
         * @param titleName 窗口标题。
         * @return 窗口指针；无父窗口时关闭自动释放。
         */
        static QWidget* imgShow(CyMedia::ImageShowInfo info, void* data, const QString& title = {}, QWidget* parent = nullptr);

    public:
        explicit CyMediaDis(QWidget* parent = nullptr);
        ~CyMediaDis() override;

        CyMediaDis(const CyMediaDis&) = delete;
        CyMediaDis& operator=(const CyMediaDis&) = delete;

    signals:
        //==================== 交互信号 ====================
        /// 文件/URL 被拖入视图。
        void urlsDrop(QList<QUrl> urls);
        /// 鼠标在视图上按下。
        void pressOnView();
        /// 鼠标在视图上双击。
        void DoubleClickOnView();
        /// 直接更新模式下，需要外部重新提供一帧图像。
        void needUpImage();

        /**
         * @brief 鼠标所在像素的坐标与颜色。
         * @param signlR 为 true 时 r 表示灰度值，g/b 无效。
         */
        void upPosPix(qint32 x, qint32 y, double r, double g, double b, bool isGray);
        /// 图像尺寸或位深变化。
        void imageSizeChanged(int32_t w, int32_t h, int nbit);
        /// 缩放倍数变化。
        void zoomValueChange(double value);

        //==================== 标注信号 ====================
        /// 绘制模式变化。
        void drawModeChange(CyDisDrawItem::ItemType mode);
        /// 添加标注项（参数为 UUID）。
        void itemDrawed(QUuid id);
        /// 删除标注项。
        void itemRemoved(QUuid id);
        /// 选中标注项。
        void itemSelected(QUuid id);

    public:
        //==================== 平台与格式 ====================
        /// 查询系统 OpenGL 版本；不支持时返回 false。
        static bool supportsOpenGL(int& mainV, int& subV);
        /// 是否满足本控件的最低 OpenGL 要求（3.3）。
        static bool supportsOpenGLForCyMedia();
        /// 像素格式枚举转可读字符串（如 "MONO10P"）。
        static QString pixelFormatStr(CyMedia::ePixType format);

        //==================== 国际化与日志 ====================
        CyMedia::eLanguage currentLanguage() const;
        bool setLanguage(CyMedia::eLanguage lang);

        /// 设置日志等级（TRACE~OFF）。
        void setLogLevel(CyMedia::LogLevel level);
        /// 注册日志回调；pUser 会在回调时原样传回。
        void setLogCallback(CyMedia::LogCallback cb, void* pUser = nullptr);

        //==================== 基础属性 ====================
        /// 是否接受文件拖放。
        void setSceneAcceptDrop(bool accept);

        //==================== 图像输入 ====================
        /**
         * @brief 设置内部帧缓存队列深度（1~10，默认 3）。
         * @warning 修改会停止并重启处理线程，建议在开始推流前调用。
         */
        void setImageStackNum(uint32_t num);

        /**
         * @brief 推送一帧图像。
         * @param info 图像元数据。
         * @param data 图像数据；函数返回后调用方即可复用/释放。
         * @param force 队列满时是否覆盖最早帧。
         * @return 成功入队/处理返回 true。
         */
        bool upImageData(const CyMedia::ImageShowInfo& info, void* data, bool force = false);

        /// 当前是否处于"直接处理、不入队"模式。
        bool isDirectMode();
        /// 设置直接处理模式（低帧率/单帧场景可降低延迟）。
        void setDirectUpImage(bool flag);

        /// 注册处理完成回调（工作线程调用，禁止耗时操作）。
        void registerImageCallBack(CyMedia::ImageFrameCallback func, void* pUser);

        /// 是否已有有效图像。
        bool hasImage(void) const;
        /// 清空图像与标注交互状态。
        void clearImage(void);

        /// 当前显示图像的信息。
        ImageShowInfo imageInfoCopy() const;
        /// 当前显示帧率。
        double displayFps(void) const;
        /// 数据处理帧率
        double  dataFps() const; 
        /// 当前是否慢速数据流
        bool isDataSlow() const;

        //==================== 显示效果 ====================
        CyMedia::StretchType stretchType();
        void setStretchType(CyMedia::StretchType type);
        /// 设置手动拉伸阈值（start/end 以当前位深为上限）。
        void setStretchPara(uint32_t start = 0, uint32_t end = 0);

        CyMedia::DemosaicingMethod Demosaic();
        void setDemosaic(CyMedia::DemosaicingMethod method);

        CyMedia::YUVTransMethod YUVMethod();
        void setYUVMethod(CyMedia::YUVTransMethod method);

        QStringList ColorMapList() const;
        quint32 colorMapIndex() const;
        bool setColorMap(quint32 index);
        bool setColorMap(const QString& mapName);

        //==================== 视图操作 ====================
        void zoomIn();
        void zoomOut();
        void zoomAuto();
        /// 1:1 显示；reset=true 时同时重置旋转/镜像。
        void zoomRaw(bool reset);

        //==================== 工具栏/缩略图 ====================
        CyDisDrawItem::ItemType drawMode();
        void setDrawMode(CyDisDrawItem::ItemType mode);
        void setThemeColor(const QColor color);

        bool toolBarVisible(void);
        void setToolBarVisible(bool show);

        bool zoomScrollBarVisible(void);
        void setZoomScrollBarVisible(bool show);

        bool thumbnailEnable();
        void setThumbnailEnable(bool enable);
        bool thumbnailAutoEnable();
        void setThumbnailAutoEnable(bool enable);
        QSize thumbnailAutoEnableSize();
        void setThumbnailAutoEnableSize(const QSize size);

        //==================== 图形标注（稳定 API：UUID） ====================
        /// 在图像中心添加默认尺寸的标注项，返回 UUID。
        QUuid addItem(CyDisDrawItem::ItemType itemType);
        /// 用场景坐标路径添加标注项。
        QUuid addItem(CyDisDrawItem::ItemType itemType, const QPainterPath& path);
        /// 删除指定 UUID 的标注项。
        void removeItem(QUuid id);
        int itemCount();
        void setItemSelected(QUuid id);
        void clearItem();

        bool isSingleItemMode();
        void setSingleItemMode(bool flag);

        QUuid getLastItem();
        bool isDrawing();

        //==================== 图形标注（高级：直接访问实现） ====================
        /**
         * @brief 获取内部标注项列表。
         */
        QList<CyDisDrawItem::BaseItem*> items() const;//不用引用，QList 隐式共享代价小
        /// 按 UUID 取标注项；不存在返回 nullptr。
        CyDisDrawItem::BaseItem* getItem(const QUuid& id);

        //==================== 录像计时器 ====================
        bool recTimeVisible();
        void setRecTimeVisible(bool visi);
        void upRecTime(uint64_t time);
        void upRecTime(float saved, float sum);
        void upRecTime_Timed(uint64_t saved, uint64_t sum);

        //==================== 辅助面板（高级） ====================
        QWidget* stretchWidget();
        void setGrayStretchVisible(bool visible);
        QWidget* grayTestWidget();
        void setGrayTestVisible(bool visible);

    private:
        class privateData; ///< 私有实现（PIMPL）。
        privateData* d = nullptr;
    };
}
