#Image-Video-On-QGraphicsView

  This is the first project I uploaded.I hope it is as easy to understand, as complete, and as elegant as possible.

  It was developed to meet the need for interfacing with **camera image streams**, so it uses OpenGL rendering and image processing to ensure speed as much as possible. Using QGraphicsView eliminates a lot of external work, such as image matrices and additional attachments.

# V1.1

- 修复YUV YUV2等格式重建错误的问题
- 优化直方图计算
- 优化CyMediaCalc命名
- 优化OPenGL渲染程序
- 主要文件添加doxygen风格注释

## 细节

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