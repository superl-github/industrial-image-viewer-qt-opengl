# CyMedia 更新日志

Image-Video-On-QGraphicsView

  This is the first project I uploaded.I hope it is as easy to understand, as complete, and as elegant as possible.

  It was developed to meet the need for interfacing with **camera image streams**, so it uses OpenGL rendering and image processing to ensure speed as much as possible. Using QGraphicsView eliminates a lot of external work, such as image matrices and additional attachments.

**V 1.2.3(W)**

- 现在无图像时操作镜像无效
- 优化灰度拉伸窗口布局
- 修复交替图像信息切换时的显示错误问题

**未暂存

- 

**V 1.2.2**

- void CyMediaDis::privateData::Thread_ImageData()中的“struct opeFrameThreadPara opePara;”语法会导致多个Dis拥有同一个opePara。
- 切换更新图像的线程时重建上下文
- Raw视频解析添加按给定信息解析
- 优化工具按钮
- 优化CyMediaDis.h中的包含文件
- CyMediaDisTest添加raw视频解析

**V 1.2.1**

- 修正版本信息
- 修复CyMediaCalc::YUV2RGB声明和定义不一致
- 更新翻译文件

**V 1.2.0 **

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

**V1.1**

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

**V 1.0**

- 支持MONO、RGB、BAYER、YUV(仅CPU)
- BAYER重建支持 BILINEAR/MALVA/AHD
- 支持 [Doxyfile](OutFile\Doxygen\Doxyfile) 点/线/面/多边形/圆形绘制工具
- 支持灰度拉伸(Gray/HSV/LAB)
- YUV支持只显示Y/正常
- 支持多线程更新图像，多线程更新纹理