#Image-Video-On-QGraphicsView

  This is the first project I uploaded.I hope it is as easy to understand, as complete, and as elegant as possible.

  It was developed to meet the need for interfacing with **camera image streams**, so it uses OpenGL rendering and image processing to ensure speed as much as possible. Using QGraphicsView eliminates a lot of external work, such as image matrices and additional attachments.



# V 1.0

- 支持MONO、RGB、BAYER、YUV(仅CPU)
- BAYER重建支持 BILINEAR/MALVA/AHD
- 支持点/线/面/多边形/圆形绘制工具
- 支持灰度拉伸(Gray/HSV/LAB)
- YUV支持只显示Y/正常
- 支持多线程更新图像，多线程更新纹理