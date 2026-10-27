# CyMedia / View 模块

> 本文描述 `CyMedia/src/View` 的最终结构、依赖规则和使用方式。

---

## 1. 模块定位

`View` 提供基于 Qt Widgets / QGraphicsView / OpenGL 的图像显示与交互能力，对外主入口是：

```cpp
CyMedia::CyMediaDis   // QFrame，可直接放入 Qt 布局
```

能力范围：

| 能力 | 说明 |
| --- | --- |
| 图像渲染 | OpenGL 3.3 纹理上传与着色器；Mono / Bayer / RGB / RGBA / BGR / YUV / MJPG |
| 位深 | 8 / 10 / 12 / 16 / 32 位，整型与 float/double 像素 |
| 图像增强 | ColorMap、灰度拉伸（Gray/HSV/Lab）、Bayer 去马赛克、YUV→RGB |
| 视图交互 | 缩放、平移、旋转、水平/垂直翻转、自适应、1:1 |
| 缩略图 | 右下角浮窗，显示全景与当前视口，可拖拽定位 |
| 标注 | 点、矩形、线、椭圆、多边形；选中、编辑、清除、ROI 统计 |
| 辅助面板 | 灰度拉伸、灰度测试/直方图、录像计时器 |
| 多线程 | 有界帧队列 + 独立处理线程，可从采集线程推帧 |

---

## 2. 目录与职责

```text
src/View/
├── Display.h / Display.cpp         门面 + UI 适配器
├── DisplayTypes.h                  显示模块的轻量公共类型
│
├── Pipeline/                       图像处理服务端
│   ├── ImagePipeline.h/.cpp        帧校验、缓存、线程、统计、像素采样
│   └── ImageProcessor.h/.cpp       Bayer/YUV/MJPG/打包格式的 CPU 转换
│
├── Render/                         视图与 OpenGL
│   ├── CyMediaDisView              QGraphicsView 视图、缩放/平移/缩略图协调
│   ├── CyMediaDisViewBckDraw       OpenGL 纹理、着色器、FBO、FPS
│   ├── CyMediaDisViewThumbnail     缩略图浮窗
│   └── CyDMediaDisScen             QGraphicsScene，鼠标/拖放信号
│
├── Annotation/                     图形标注
│   ├── BaseItem / Item_*           图形项
│   ├── ItemManager                 图形项集合与选中状态
│   └── DrawItemTool                鼠标绘制工具
│
├── Widgets/                        UI 面板
│   ├── CyMediaDisGrayStretch       灰度拉伸
│   ├── CyMediaDisGrayTest          灰度/直方图
│   ├── CyMediaRecTimeW             录像计时
│   └── CyQCP / cycustomwidget      QCustomPlot 封装与自定义控件
│
├── Ui/                             界面外壳
│   ├── DisplayToolBar              顶部缩放/翻转工具栏
│   └── ImageShowWindow             imgShow() 的单图预览窗口
│
└── Dialogs/
    ├── CyMediaDisRawInfoDialog     RAW/裸视频参数
    └── LicenseW                    许可证信息
```

---

## 3. 依赖规则

```text
Display（门面）
   ├── Ui / Widgets / Dialogs        QtWidgets
   ├── Render                        QtWidgets + OpenGL
   ├── Annotation                    QtWidgets / QGraphics
   └── Pipeline                      QtCore + 纯数据
             └── ImageProcessor      图像类型 + Calculate
```

约定：

1. `Pipeline` 只依赖 `ImageShowInfo`、`ImageColorOpe` 等值类型，不包含 QWidget / QGraphics / QOpenGLWidget。
2. `Display.cpp` 是同时连接管线和渲染器的唯一位置；它实现 `ImagePipeline::Sink`。
3. `Display.h` 只包含显示 API 需要的类型；标注实现、RAW 对话框、工具栏由各自的头文件提供。
4. `Calculate`、`Parse`、`common` 不反向依赖 `View`。

### 公共头文件

| 头文件 | 内容 |
| --- | --- |
| `common/ImageTypes.h` | 像素格式、`ImageShowInfo`、色彩处理参数 |
| `common/ParseTypes.h` | 文件后缀、解析结果、视频信息/回调 |
| `common/Log.h` | `LogLevel`、`LogCallback` |
| `common/Language.h` | `eLanguage` |
| `common/Version.h` | 版本号 |
| `common/Export.h` | `CYMEDIA_LIB` 导入/导出宏 |
| `common/CyMediaBaseDef.h` | 上述定义的聚合头 |

`View` 内部只引用所需的最小头文件；`Display.h` 不引入 QGraphicsItem、QMenu、QDialog。

---

## 4. 数据流

```text
采集线程 / UI 线程
      │  CyMediaDis::upImageData(info, data)
      ▼
┌──────────────────────── ImagePipeline ────────────────────────┐
│ 1. 校验尺寸、位深、长度、上限                                  │
│ 2. 直接模式：当前线程同步处理                                  │
│ 3. 队列模式：取空闲 slot → 拷贝 → ready → 唤醒工作线程          │
│ 4. 工作线程等待渲染端 OpenGL ready                             │
│ 5. 保存原始帧副本（像素采样、参数变化后的重处理）              │
│ 6. ImageProcessor：Bayer/YUV/MJPG/MonoPacked → 统一格式        │
│ 7. 更新 dataFps / dataSlow                                     │
│ 8. Sink::onFrame(PipelineFrame)                                │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                 Display::privateData::onFrame()
                   ├─ 尺寸/格式变化 → ImageInfoChange（排队到 UI）
                   ├─ 像素颜色采样 → upPosPix
                   ├─ 灰度拉伸/直方图面板计算
                   ├─ OpenGL 纹理上传（共享上下文）
                   └─ ImageDataDone（排队到 UI）→ 缩放/缩略图/刷新
```

---

## 5. 线程模型

| 场景 | 行为 |
| --- | --- |
| 普通推流 | `upImageData()` 校验并入队后返回；处理在 `ImagePipeline` 工作线程 |
| 低帧率（< 2.1 FPS） | `dataSlow = true`，逐帧保存原始副本，面板与采样按低帧率策略更新 |
| 直接模式 | `setDirectUpImage(true)`：不入队，在调用线程同步处理 |
| 参数重处理 | 拉伸/去马赛克/YUV 参数变化时调用 `requestReprocess()`，使用原始帧副本 |
| 清空 | `clearImage()` 停止线程、清队列、清原始帧；短窗口内拒绝新帧 |

启动前必须设置：

```cpp
QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
```

### Sink 约定

`ImagePipeline::Sink::onFrame()` 在处理线程执行：

- `PipelineFrame::data` 只在回调期间有效，不保存指针。
- 跨线程 UI 更新通过信号排队（`ImageInfoChange`、`ImageDataDone`）。
- 灰度面板接口只做计算并发信号，绘制发生在 UI 线程。

---

## 6. 公开 API

头文件：

```cpp
#include "View/Display.h"
```

### 6.1 创建与推帧

```cpp
QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
QApplication app(argc, argv);

auto* display = new CyMedia::CyMediaDis(parent);
layout->addWidget(display);

CyMedia::ImageShowInfo info(width, height, bit, CyMedia::MONO);
display->upImageData(info, data);          // data 返回后可释放
```

低延迟/单帧：

```cpp
display->setDirectUpImage(true);
display->upImageData(info, data);
```

### 6.2 图像增强

```cpp
display->setColorMap("jet");
display->setStretchType(CyMedia::stretch_HSV);
display->setStreaChPara(20, 235);
display->setDemosaic(CyMedia::DEMOSAIC_MALVA);
display->setYUVMethod(CyMedia::BT601);
```

### 6.3 视图

```cpp
display->zoomIn();
display->zoomOut();
display->zoomAuto();
display->zoomraw(true);
display->setThumbnailAutoEnable(true);
display->setToolBarVisible(true);
```

### 6.4 标注

```cpp
connect(display, &CyMedia::CyMediaDis::itemDrawed,
        [](QUuid id) { /* 保存 UUID */ });

display->setDrawMode(CyDisDrawItem::Rectangle);
QUuid id = display->addItem(CyDisDrawItem::Rectangle);
display->setItemSelected(id);
display->removeItme(id);
display->clearItem();
```

`items()` / `getItem()` 返回 `CyDisDrawItem::BaseItem*`，属于标注高级接口；
解引用时包含 `View/Annotation/BaseItem.h`。常规业务使用 UUID API。

### 6.5 辅助面板

```cpp
display->setGrayStretchVisible(true);
display->setGrayTestVisible(true);

QWidget* stretch = display->stretchWidget();
QWidget* hist    = display->grayTestWidget();
```

### 6.6 RAW 参数对话框

```cpp
#include "View/Dialogs/CyMediaDisRawInfoDialog.h"

CyMedia::CyMediaDis_GetRawInfoDialog dlg(parent);
dlg.setSpecialPixeVisible(true);
if (dlg.exec() == QDialog::Accepted) {
    auto w = dlg.imageWidth();
    auto h = dlg.imageHeight();
    auto bit = dlg.imagenBit();
    auto fmt = dlg.imagePixelType();
}
```

### 6.7 日志与语言

```cpp
display->setLogCallback([](CyMedia::LogLevel lv, const std::string& msg, void*) {
    // 自定义输出
}, nullptr);
display->setlogLevel(CyMedia::INFO);
display->setLanguage(CyMedia::CHINESE);
```

---

## 7. 扩展约定

**渲染器**：`CyMediaDisViewBckDraw` 分为纹理/着色器/绘制三个职责，扩展着色器变体时在
`ShaderVariant` 与 `determineShaderVariant()` 中登记。

**图像格式**：像素格式在 `common/ImageTypes.h` 的 `ePixType` 中定义；CPU 转换在
`ImageProcessor::process()` 中分支；GPU 转换在渲染器着色器中实现。

**标注类型**：在 `CyDisDrawItem::ItemType` 中增加枚举，在 `ItemFactory` 中注册创建函数，
并在 `Annotation/` 下实现对应 `BaseItem` 子类。

**面板**：面板类位于 `Widgets/`，只依赖公共图像类型；面板参数通过 `ImagePipeline::Options`
传入处理管线。

---

## 8. 构建

工程文件：`CyMedia/CyMedia.vcxproj`、`CyMedia/CyMedia.vcxproj.filters`。

CMake：仓库根 `CMakeLists.txt`、`CyMedia/CMakeLists.txt`、`CyMediaDisTest/CMakeLists.txt`。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/Qt5.14.2/5.14.2/msvc2017_64"
cmake --build build --config Release --parallel
```

两套构建共用同一份源文件列表，增删源文件时同步更新 VS 工程、filter 与 CMake。
