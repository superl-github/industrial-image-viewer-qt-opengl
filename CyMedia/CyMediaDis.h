#pragma once
#include "CyMediaBaseDef.h"
#include "CyMediaDis/CyMediaRecTimeW.h"
#include "CyMediaDis/CyMediaDisGrayStretch.h"
#include "CyMediaDis/CyMediaDisGrayTest.h"
#include "CyMediaDis/drawItem/BaseItem.h"
#include "CyMediaDis/drawItem/CyDisDrawItem.h"

#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QLayout>
#include <QUrl>

namespace CyMedia {
    /**
     * @brief 高性能图像显示与交互控件（基于 Qt QFrame）。
     *
     * @details
     * `CyMediaDis` 是一个功能完整的图像显示组件，专为实时图像处理与视觉检测应用设计。
     * 它集成了以下核心能力：
     *
     * - **多格式图像渲染**：支持 Mono、Bayer（RG/GR/BG/GB）、RGB/RGBA、多种 YUV 格式（Packed/Planar/Semi-Planar），
     *   以及 8/10/12/16/32 位深度，并内置颜色映射（ColorMap）和灰度拉伸（Stretch）。
     * - **OpenGL 加速**：基于 Qt GraphicsView + OpenGL 渲染，支持多线程纹理上传，适合高帧率数据流。
     * - **交互操作**：缩放（滚轮/按钮）、平移、旋转、水平/垂直翻转、自适应窗口、缩略图导航。
     * - **图形标注**：支持绘制点、矩形、线、椭圆、多边形等标注项，可编辑、移动、删除，并支持 ROI 统计。
     * - **辅助工具**：内置灰度拉伸交互控件（`CyMediaDisGrayStretch`）和灰度测试/直方图工具（`CyMediaDisGrayTest`），
     *   可独立显示或嵌入。
     * - **信号通知**：提供丰富的信号，便于外部监听图像尺寸变化、鼠标坐标、缩放值、标注事件等。
     *
     * @note 使用前需确保 OpenGL 上下文已共享（设置 `QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts)`）。
     *
     * @see CyMediaDisView, CyMediaDisViewBckDraw, CyMediaDisGrayStretch, CyMediaDisGrayTest
     */
    class CYMEDIA_LIB CyMediaDis : public QFrame {
        Q_OBJECT

    public:
        /// 图像数据回调函数类型，用于接收处理后的图像帧。
        using CyMediaDisImageCallBack = std::function<void(CyMedia::ImageShowInfo&, uint8_t*, void*)>;

    public:
        /**
         * @brief 构造函数。
         * @param parent 父窗口（默认为 nullptr）。
         */
        CyMediaDis(QWidget* parent = nullptr);
        /**
         * @brief 析构函数。自动释放所有子控件及内部资源。
         */
        ~CyMediaDis();

    public:signals:
        /**
         * @brief 当文件或 URL 被拖拽到控件上时发出。
         * @param urls 拖拽的 URL 列表。
         */
        void urlsDrop(QList<QUrl> urls);
        /**
         * @brief 当鼠标在视图上按下时发出。
         */
        void PressOnView();
        /**
         * @brief 当鼠标在视图上双击时发出。
         */
        void DoubleClickOnView();

        /**
         * @brief 鼠标移动时发出，携带当前像素坐标及颜色值。
         * @param x 像素 X 坐标（图像坐标系）。
         * @param y 像素 Y 坐标（图像坐标系）。
         * @param r 红色分量（或灰度值，取决于 `signlR`）。
         * @param g 绿色分量。
         * @param b 蓝色分量。
         * @param signlR 若为 `true`，则 `r` 表示灰度值，`g`、`b` 无效。
         */
        void upPosPix(qint32 x, qint32 y, double r, double g, double b, bool signlR);
        /**
         * @brief 图像尺寸或位深变化时发出。
         * @param w 图像宽度（像素）。
         * @param h 图像高度（像素）。
         * @param nbit 每个通道的位深。
         */
        void imageSizeChanged(quint32 w, quint32 h, int nbit);
        /**
         * @brief 缩放比例变化时发出。
         * @param value 当前缩放倍数。
         */
        void zoomValueChange(double value);
        /**
         * @brief 视图被点击时发出（等效于 PressOnView）。
         */
        void pressOnView();

        /**
         * @brief 绘制模式变更时发出。
         * @param mode 新的绘制模式（见 `CyDisDrawItem::ItemType`）。
         */
        void drawModeChange(CyDisDrawItem::ItemType mode);
        /**
         * @brief 新增图形标注项时发出。
         * @param id 新项的 UUID。
         */
        void itemDrawed(QUuid id);
        /**
         * @brief 删除图形标注项时发出。
         * @param id 被删除项的 UUID。
         */
        void itemRemoved(QUuid id);
        /**
         * @brief 选中某个图形标注项时发出。
         * @param id 被选中项的 UUID。
         */
        void itemSelected(QUuid id);

    public:
        //==================== 系统支持检测 ====================

        /**
         * @brief 检测当前系统是否支持 OpenGL，并返回主/次版本号。
         * @param[out] mainV 主版本号。
         * @param[out] subV 次版本号。
         * @return 支持 OpenGL 返回 `true`，否则 `false`。
         */
        static bool supportsOpenGL(int& mainV, int& subV);
        /**
         * @brief 检测当前系统是否满足本控件的 OpenGL 最低要求（3.3）。
         * @return 满足返回 `true`，否则 `false`。
         */
        static bool supportsOpenGLForCyMedia();
        /**
         * @brief 将像素格式枚举转换为可读字符串。
         * @param format 像素格式枚举值。
         * @return 格式名称字符串（如 "MONO10P"）。
         */
        static QString pixelFormatStr(CyMedia::ePixType format);

        //==================== 国际化/日志 ====================

        /**
         * @brief 获取当前界面语言。
         * @return 当前语言枚举（`ENGLISH` 或 `CHINESE`）。
         */
        CyMedia::eLanguage currentLanguage();
        /**
         * @brief 设置界面语言（支持中英文切换）。
         * @param lang 目标语言。
         * @return 加载翻译文件成功返回 `true`，否则 `false`。
         */
        bool setLanguage(CyMedia::eLanguage lang);

        /**
         * @brief 启用/禁用调试日志输出。
         * @param flag `true` 开启，`false` 关闭。
         */
        void setPrintLog(bool flag);
        /**
         * @brief 设置日志回调函数，用于自定义日志处理。
         * @param cb 回调函数对象（接收字符串和用户指针）。
         * @param pUser 用户自定义数据（默认为 nullptr）。
         */
        void setLogCallback(CyMedia::LogCallback cb, void* pUser = nullptr);

        //==================== 基本控件属性 ====================
        
        /**
         * @brief 设置场景是否接受文件拖放。
         * @param accept 接受拖放为 `true`，否则 `false`。
         */
        void setSceneAcceptDrop(bool accept);

        //==================== 图像数据接口 ====================
    
        /**
         * @brief 设置图像数据缓存队列的最大深度。
         * @details 当数据输入速度大于处理速度时，队列用于缓存未处理的帧。增大缓存可减少丢帧，
         *          但会增加内存占用。
         * @param num 缓存帧数（1~10，默认 3）。
         */
        void setImageStackNum(uint32_t num);
        /**
         * @brief 上传一帧图像数据并触发显示更新（非原始数据）。
         * @param info 图像信息结构（宽、高、位深、格式等）。
         * @param data 图像数据指针（由调用方管理，控件内部会拷贝）。
         * @param force 若为 `true`，即使队列满也会覆盖最旧帧；否则丢弃新帧。
         * @return 数据成功入队返回 `true`，否则 `false`。
         */
        bool upImageData(CyMedia::ImageShowInfo info, uint8_t* data, bool force = false);
        /**
         * @brief 注册图像处理完成后的回调函数。
         * @details 当一帧图像被处理（包括格式转换、拉伸等）后，会调用此回调，
         *          可用于外部保存图像或进一步分析。
         * @param func 回调函数。
         * @param pUser 用户自定义数据，回调时传回。
         */
        void registerImageCallBack(CyMediaDisImageCallBack func, void*pUser);
        
        /**
         * @brief 查询是否已有有效图像数据。
         * @return 有数据返回 `true`，否则 `false`。
         */
        bool haveDate(void);
        /**
         * @brief 清除当前显示的图像，恢复空白状态。
         */
        void clearImage(void);

        /**
         * @brief 获取当前显示的图像信息（引用）。
         * @return `ImageShowInfo` 引用。
         */
        CyMedia::ImageShowInfo& imageinfo();
        /**
         * @brief 获取当前的显示帧率（每秒帧数）。
         * @return 帧率值（可能为 0）。
         */
        double displayFps(void);

        //==================== 图像处理参数 ====================

        /**
         * @brief 获取当前灰度拉伸类型。
         * @return 拉伸类型枚举（`stretch_None`, `stretch_Gray`, `stretch_HSV`, `stretch_Lab`）。
         */
        CyMedia::StretchType stretchType();
        /**
         * @brief 设置灰度拉伸类型。
         * @param type 拉伸类型。
         */
        void setStretchType(CyMedia::StretchType type);
        /**
         * @brief 设置手动拉伸的起始和结束阈值。
         * @param start 起始值（0~max）。
         * @param end 结束值（start~max）。
         * @note 仅在 `stretchType` 非 `None` 且 `setStretchType` 未启用自动拉伸时生效。
         */
        void setStreaChPara(uint32_t start = 0, uint32_t end = 0);

        /**
         * @brief 获取当前 Bayer 去马赛克算法。
         * @return 去马赛克方法枚举。
         */
        CyMedia::DemosaicingMethod Demosaic();
        /**
         * @brief 设置 Bayer 去马赛克算法。
         * @param method 算法枚举（`DEMOSAIC_NONE`/`BILINEAR`/`MALVA`/`AHD`）。
         */
        void setDemosaic(CyMedia::DemosaicingMethod method);

        /**
         * @brief 获取当前 YUV 转 RGB 的转换方法。
         * @return 转换方法枚举。
         */
        CyMedia::YUVTransMethod YUVMethod();
        /**
         * @brief 设置 YUV 转 RGB 的转换方法。
         * @param method 转换方法（`YUVTRANS_NORMAL` 或 `YUVTRANS_Y`）。
         */
        void setYUVMethod(CyMedia::YUVTransMethod method);

        /**
         * @brief 获取当前可用的颜色映射（ColorMap）名称列表。
         * @return QStringList 颜色映射名称。
         */
        QStringList ColorMapList() const;
        /**
         * @brief 获取当前激活的颜色映射索引。
         * @return 索引值（0 起始）。
         */
        quint32 colorMapIndex() const;
        /**
         * @brief 通过索引设置颜色映射（仅对单色图像有效）。
         * @param index 颜色映射索引。
         * @return 设置成功返回 `true`，否则 `false`。
         */
        bool setColorMap(quint32 index);
        /**
         * @brief 通过名称设置颜色映射。
         * @param mapName 颜色映射名称（须在 `ColorMapList()` 中）。
         * @return 设置成功返回 `true`，否则 `false`。
         */
        bool setColorMap(const QString& mapName);

        //==================== 视图操作 ====================

        /** @brief 放大视图（以鼠标为中心）。 */
        void zoomIn();
        /** @brief 缩小视图（以鼠标为中心）。 */
        void zoomOut();
        /** @brief 自适应缩放，使图像完整显示在视口中。 */
        void zoomAuto();
        /**
         * @brief 恢复为 1:1 原始像素比例。
         * @param reset 若为 `true`，同时重置旋转和镜像状态。
         */
        void zoomraw(bool reset);

        //==================== 工具/UI 控件 ====================

        /**
         * @brief 获取当前绘制模式。
         * @return `ItemType` 枚举（`Invalid` 表示未激活）。
         */
        CyDisDrawItem::ItemType drawMode();
        /**
         * @brief 设置绘制模式（激活对应的图形绘制工具）。
         * @param mode 目标模式（`Invalid` 禁用绘制）。
         */
        void setDrawMode(CyDisDrawItem::ItemType mode);

        /**
         * @brief 设置主题颜色（影响部分控件边框、手柄、选择高亮等）。
         * @param color 主题色。
         */
        void setThemeColor(QColor color);

        /**
         * @brief 查询工具栏是否可见。
         * @return 可见返回 `true`。
         */
        bool toolBarVisible(void);
        /**
         * @brief 设置工具栏可见性。
         * @param show `true` 显示，`false` 隐藏。
         */
        void setToolBarVisible(bool show);

        /**
         * @brief 查询缩放滚动条是否可见。
         * @return 可见返回 `true`。
         */
        bool zoomScrollBarVisible(void);
        
        /**
        * @brief 设置缩放滚动条可见性。
        * @param show `true` 显示，`false` 隐藏。
        */
        void setZoomScrollBarVisible(bool show);

        /**
         * @brief 查询缩略图是否启用（固定大小）。
         * @return 启用返回 `true`。
         */
        bool thumbnailEnable();
        /**
        * @brief 设置缩略图启用状态。
        * @note 当 `thumbnailAutoEnable()` 为 `true` 时，该设置无效。
        * @param enable `true` 启用，`false` 禁用。
        */
        void setThumbnailEnable(bool enable);
        /**
         * @brief 查询是否开启自动缩略图（根据图像尺寸自动决定是否启用）。
         * @return 自动模式返回 `true`。
         */
        bool thumbnailAutoEnable();
        /**
         * @brief 开启/关闭自动缩略图模式。
         * @param enable `true` 启用，`false` 禁用。
         */
        void setThumbnailAutoEnable(bool enable);
        /**
         * @brief 获取自动缩略图的触发尺寸阈值。
         * @return QSize 宽高阈值。
         */
        QSize thumbnailAutoEnableSize();
        /**
         * @brief 设置自动缩略图的触发尺寸阈值（图像宽度或高度超过该值即启用缩略图）。
         * @param size 宽高阈值。
         */
        void setThumbnailAutoEnableSize(QSize size);

        //==================== 图形标注项管理 ====================

        /**
         * @brief 添加一个默认尺寸的图形标注项（位于图像中心）。
         * @param itemType 图形类型（如 `Rectangle`, `Line` 等）。
         * @return 新项的 UUID。
         */
        QUuid addItem(CyDisDrawItem::ItemType itemType);
        /**
         * @brief 使用指定的 QPainterPath 添加图形标注项。
         * @param itemType 图形类型。
         * @param path 场景坐标系下的路径。
         * @return 新项的 UUID。
         */
        QUuid addItem(CyDisDrawItem::ItemType itemType, QPainterPath path);
        /**
         * @brief 删除指定 UUID 的图形标注项。
         * @param id 要删除的项 UUID。
         */
        void removeItme(QUuid id);
        /**
         * @brief 获取当前图形标注项总数。
         * @return 数量。
         */
        int itemCount();
        /**
         * @brief 获取所有图形标注项的引用列表。
         * @return 项目列表引用。
         */
        QList<CyDisDrawItem::BaseItem*>& items();
        /**
         * @brief 根据 UUID 获取图形标注项指针。
         * @param id 项 UUID。
         * @return 指针，若不存在则返回 `nullptr`。
         */
        CyDisDrawItem::BaseItem* getItem(QUuid& id);
        /**
         * @brief 设置指定项为选中状态。
         * @param id 项 UUID。
         */
        void setItemSelected(QUuid id);
        /**
         * @brief 清除所有图形标注项。
         */
        void clearItem();

        /**
         * @brief 查询是否处于单条目模式（即绘制新项时自动删除前一项）。
         * @return 单条目模式返回 `true`。
         */
        bool isSingleItemMode();
        /**
        * @brief 设置单条目模式。
        * @param flag `true` 启用，`false` 禁用。
        */
        void setSingleItemMode(bool flag);

        /**
         * @brief 获取最后添加（或当前绘制）的图形标注项的 UUID。
         * @return UUID（无效表示无项）。
         */
        QUuid getLaseItem();
        /**
         * @brief 查询当前是否正在绘制图形（鼠标拖拽中）。
         * @return 正在绘制返回 `true`。
         */
        bool isDrawing();

        //==================== 计时/录像指示器 ====================

        /**
         * @brief 查询录像时间指示器是否可见。
         * @return 可见返回 `true`。
         */
        bool recTimeVisible();
        /**
         * @brief 设置录像时间指示器可见性。
         * @param visi `true` 显示，`false` 隐藏。
         */
        void setRecTimeVisible(bool visi);

        /**
         * @brief 更新录像指示器显示的总时长（单时间模式）。
         * @param time 总时长（毫秒）。
         */
		void upRecTime(uint64_t time);
        /**
        * @brief 更新录像指示器显示的已存储时长和总时长（双时间模式）。
        * @param saved 已存储时长（毫秒）。
        * @param sum 总时长（毫秒）。
        */
		void upRecTime(uint64_t saved, uint64_t sum);
        /**
         * @brief 更新录像指示器显示的已存储时长和总时长（定时模式，带格式）。
         * @param saved 已存储时长（毫秒）。
         * @param sum 总时长（毫秒）。
         */
		void upRecTime_Timed(uint64_t saved, uint64_t sum);

        //==================== 灰度拉伸交互控件 ====================

        /**
         * @brief 获取灰度拉伸交互控件指针。
         * @return `CyMediaDisGrayStretch*` 指针。
         */
        CyMediaDisGrayStretch* stretchWidget();
        /**
         * @brief 设置灰度拉伸交互控件的可见性。
         * @param visible `true` 显示，`false` 隐藏。
         */
        void setGrayStretchVisible(bool visible);

        /**
         * @brief 获取灰度测试/直方图控件指针。
         * @return `CyMediaDisGrayTest*` 指针。
         */
        CyMediaDisGrayTest* grayTestWidget();
        /**
         * @brief 设置灰度测试/直方图控件的可见性。
         * @param visible `true` 显示，`false` 隐藏。
         */
        void setGrayTestVisible(bool visible);

    private:
        class privateData; ///< 私有数据封装（PIMPL）。
        privateData* d = nullptr;
    };


    /**
     * @brief 用于打开 RAW 图像时获取参数的对话框。
     * @details 当用户需要加载自定义尺寸、位深、像素格式的原始图像数据时，
     *          通过此对话框输入必要的解析参数。
     */
	class CYMEDIA_LIB CyMediaDis_GetRawInfoDialog : public QDialog {
		Q_OBJECT
	public:
        /**
         * @brief 构造函数。
         * @param parent 父窗口（默认为 nullptr）。
         */
		explicit CyMediaDis_GetRawInfoDialog(QWidget* parent = nullptr);

    public:
        /**
         * @brief 刷新界面翻译（多语言切换时调用）。
         */
        void flushTrans();

        /**
         * @brief 获取当前选择的文件名（仅显示用）。
         * @return 文件名字符串。
         */
		QString openFileName();
        /**
         * @brief 获取用户设置的图像宽度。
         * @return 宽度（像素）。
         */
		quint32 imageWidth();
        /**
         * @brief 获取用户设置的图像高度。
         * @return 高度（像素）。
         */
		quint32 imageHeight();
        /**
         * @brief 获取用户设置的图像位深。
         * @return 位深（1~31）。
         */
		quint32 imagenBit();
        /**
         * @brief 获取用户选择的像素格式。
         * @return `ePixType` 枚举值。
         */
        CyMedia::ePixType imagePixelType();
        /**
         * @brief 获取用户选择的像素值数据类型（整型/浮点）。
         * @return `ePixelValueType` 枚举值。
         */
        CyMedia::ePixelValueType specialPixe();
        /**
         * @brief 获取用户设置的数据偏移量（文件头部字节数）。
         * @return 偏移量（字节）。
         */
		quint32 imageOffset();

        /**
         * @brief 设置文件名显示。
         * @param name 文件名字符串。
         */
		void setOpenFileName(QString name);
        /**
         * @brief 预设所有参数值（用于编辑已有信息）。
         * @param w 宽度。
         * @param h 高度。
         * @param bit 位深。
         * @param pixelType 像素格式。
         * @param spv 像素值数据类型。
         * @param offset 偏移量。
         */
		void setOpenInfo(quint32 w, quint32 h, quint32 bit, CyMedia::ePixType pixelType, CyMedia::ePixelValueType spv, quint32 offset);

	protected:
        /**
         * @brief 初始化对话框界面（在构造函数中调用）。
         */
		void initGUI();

	private slots:
        /** @brief 点击“确定”按钮的槽函数（调用 accept()）。 */
		void onOkClicked();
        /** @brief 点击“取消”按钮的槽函数（调用 reject()）。 */
		void onCanCelClicked();

	private:
		class PrivateData;///< 私有数据封装。
		PrivateData* d;
	};
};