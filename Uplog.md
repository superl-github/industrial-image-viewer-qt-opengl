# V 1.4.0(ing)

- 新增 `CyCellStepper`：把"每格数值"与标签融合显示（如 `X/cell 25`），右侧/上下配一对按钮，点击 = ×2 或 ÷2，到上下限自动置灰
- 灰度测量：X 轴格子的按钮左右排列（左减右加），Y 轴格子的按钮上下排列（上加下减），去掉原来的 QSpinBox
- 灰度拉伸：X 轴固定为全量程（0~最大 bit 值，不可缩放/平移），只保留 Y 轴格子按钮；重置轴按钮移回工具行
- 新增自绘直方图控件 `CyHistogramView`：柱形(竖线)/折线/半透明轮廓三种风格，按屏幕列做峰值保持降采样，数据由工作线程写入后排队交换，去掉了 QCustomPlot 的数据容器与图元开销
- `CyHistogramView` 轮廓(Area)填充改为一条闭合多边形一次绘制：去掉逐桶矩形各自取整留下的 1px 白缝，填充光栅化比逐列矩形快 6~8 倍；阶梯线端点吸附整像素，1px 轮廓不再出现点状断口；修正竖向渲染时同一桶两端点顺序反了导致的锯齿/斜线；`setStepLine(false)` + `Style_Area` 现在也会填充了
- 灰度测量与灰度拉伸面板改用 `CyHistogramView`，不再依赖 QCustomPlot（`CyQCP` 封装已从工程移除，`3rdparty/qcustomplot` 源码保留）
- 坐标轴改为"格子"模型：每格像素长度固定，每格代表的灰度级/计数可在界面上用编辑框精确设置，并支持 X/Y 轴最大范围限制（内部接口）
- 鼠标读数改为固定在绘图区左上角显示 X 值及各通道计数，不再使用跟随鼠标的 ToolTip；鼠标移出绘图区后保留最后一次读数
- 灰度拉伸：选区支持拖动整体/左右边缘（选区外压暗），Bayer 图像新增"按原始数据统计"选项（跳过去马赛克直接统计 CFA 像素）
- 灰度测量：切换测试形状/重新显示时重新开启 Y 轴自动调整；直方图桶数上限保护（>20bit 拒绝并提示，避免巨量内存分配）
- 让调整缩放比例在首帧(分辨率变化)图像更新前完成，防止尺寸改变后的第一帧图像先按照原来的缩放显示一次再跳转到新的缩放
- 图形标注时替换模式删除旧item的信号不再延迟
- 优化超过纹理最大尺寸的灰度图的处理：原始数据被当成一维字节流，每 4 个像素打包进 1 个 RGBA texel，任意宽高均可（不要求 2/4 的倍数），容量上限 4 * maxTexSize^2 像素(去掉BackDraw::textureWidthMultiplier，使用m_vertex_current_w_packed表示真实纹理尺寸)
- 增加外部接口：const uint8_t* latestCachedImage(CyMedia::ImageShowInfo& info);/// 内部缓存的最近一帧输入图像（无锁，外部调用可能读写冲突，由外部保证使用此接口时无数据输入)
- 增加外部接口：void cacheIamge(bool enable); bool isCacheIamgeEnablee();
- 外部设置拉伸参数，同步更新到内部灰度窗口
- CyMediaRecTimeW.h/Cpp 重新设计，显示内容/LED状态交给调用方，更加灵活
- 修复 zoomRaw(reset)，reset=false时缩放异常
- 添加信号void upPos(qint32 x, qint32 y);和接口void setCacheIamge(bool enable);

**未暂存**

- 

## V 1.3.1

- 代码页包含目录改为相对目录，便于导出库使用时的include路径设置和易用性
- 可渲染20000\*20000\*16bit的灰度图（不超过显卡最大纹理尺寸的2倍情况下）
- 优化日志信息
- Test添加更多图像生成、
- 优化Test中的图像生成，避免无脑并行占满CPU
- zoom文件名修复
- 修复点灰度测量没数据
- 修正翻译文件输出目录

## V 1.3.0

- 优化文件结构和代码结构，接口更加清晰易用
- 更新ReadMe文件，完善文本
- 添加CMakeList文件，GenRealse添加DemoCopy
- 修复拼写错误
- 更新READEME
- 代码规范调整
- 从Dis拆分独立翻译类CyMedia::CyMediaI18n



# V 1.2.7

- 优化画点时对已有框的边框的判断逻辑
- 优化View代码可能的内存问题
- 修复渲染NV12/21格式崩溃问题
- 调整qrc资源文件结构
- 解决关于Qt窗口无法翻译的问题



## V 1.2.6

- 加强rawframe内存管理

- 增强Qtlincense窗口 CyMedia::AboutQTDialog

- CyMediaDis性能优化

- 优化图形绘制，点可以画在其它图形上方

- disItem可以附带文本显示

- 点画在其他区域上需要ctrl键按下时触发



## V 1.2.5

- 优化图像分析返回结果
- YUV转RGB如果只取Y分量则返回灰度图
- 优化视频解析，取帧准确度



## V 1.2.4 

- 优化自动拉伸范围计算
- 增加BGR格式支持
- calc增加JPG2RGB
- 修复RGB保存图像未给予图像数据的问题



## V 1.2.3

- 现在无图像时操作镜像无效
- 优化灰度拉伸窗口布局
- 修复交替图像信息切换时的显示错误问题
- 修复窗口未显示时调用CyMediaDis::setThumbnailEnable程序崩溃
- 修复灰度统计的显示范围计算和平均值，均匀度计算错误(并行运算竞争导致)
- 优化videoParse



## V 1.2.2

- void CyMediaDis::privateData::Thread_ImageData()中的“struct opeFrameThreadPara opePara;”语法会导致多个Dis拥有同一个opePara。
- 切换更新图像的线程时重建上下文
- Raw视频解析添加按给定信息解析
- 优化工具按钮
- 优化CyMediaDis.h中的包含文件
- CyMediaDisTest添加raw视频解析



## V 1.2.1

- 修正版本信息
- 修复CyMediaCalc::YUV2RGB声明和定义不一致
- 更新翻译文件



## V 1.2.0

- 添加版本信息
- 完善注释
- 修复设置显示参数图像更新不及时的问题
- 修复着色器切换导致的图像渲染问题
- 完善图像分析类
- 修复，优化缩略图
- 实现缩略图共享CyMediaDisViewBckDraw上下文绘制
- 添加常用格式的图片打开/保存(仅支持8位)
- 添加直接更新图像接口

**细节**

- 添加CyMediaVideoParse以及相关依赖类
- 设置YUV/Bayer/Colormap...后更新viewport
- CyMediaDisViewBckDraw添加函数：QString colorMapName() const;
- 重新编译着色器后更新所有uniform，避免产生影响
- 优化拉伸直方图更新机制，CyMediaDis::privateData::addOneGrayData添加"bool upstretch"参数
- 修复imageinfo.width/height初始值为0导致缩略图尺寸为0，无法显示
- 修复缩略图的size只在View尺寸变化时更新，图像变化时没有更新。(在CyMediaDisView::sceneRectUp里添加处理)
- 缩略图改为QOpenGLWidget，共享viewport上下文，建立自己的VAO，通过CyMediaDisViewBckDraw::renderTexture绘制背景，继承了主引擎的灰度拉伸，格式转换等图像处理。
- 在主动更新主背景的地方添加缩略图更新
- 缩略图选框适应主视图镜像/旋转(背景保持1:1)
- 添加直接更新图像，不copy进环形buffer的结构支持

- 处理非8位以及需要转换的图像保存

# V1.1

- 修复YUV YUV2等格式重建错误的问题
- 优化直方图计算
- 优化CyMediaCalc命名
- 优化OPenGL渲染程序
- 主要文件添加doxygen风格注释

**细节:**

- 拉伸直方图转并行
- 分隔着色器，运行时切换编译，减少GPU分支
- YUV着色器实现(packed/planar/semi planar)
- 优化uniform更新，优化位深处理(减少glsl if 分支)
- 添加文件头注释，拆分CyMediaDisGrayTest::upImage函数，避免过于繁琐

# V 1.0

- 支持MONO、RGB、BAYER、YUV(仅CPU)
- BAYER重建支持 BILINEAR/MALVA/AHD
- 支持 [Doxyfile](OutFile\Doxygen\Doxyfile) 点/线/面/多边形/圆形绘制工具
- 支持灰度拉伸(Gray/HSV/LAB)
- YUV支持只显示Y/正常
- 支持多线程更新图像，多线程更新纹理