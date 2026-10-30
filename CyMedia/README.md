# CyMedia 库

> 版本：`V 1.3.0`  
> 平台：Windows / Qt 5.14.2 / MSVC 2022  
> 显示模块详细设计见源码树 `src/View/README.md`（发布包内为 `include/View/README.md`）。

---

## 1. 模块

CyMedia 编译为 `CyMedia.dll` + `CyMedia.lib`。

| 模块 | 职责 | Qt UI 依赖 |
| --- | --- | --- |
| `Calculate` | 直方图、拉伸参数、Bayer/YUV 转换、颜色空间转换 | 否 |
| `Parse` | 图像 Raw/BMP/PNG/JPEG；视频 Raw/AVI/MP4 | 否 |
| `common` | 像素类型、图像信息、解析类型、日志、版本、导出宏 | 否 |
| `View` | 图像显示、交互、标注、OpenGL 渲染、处理管线 | 是 |
| `CyMediaDisTest` | 示例与冒烟测试程序 | 是 |

### 1.1 解析格式

图像（`CyMediaImageParse`）：

| 操作 | 格式 | 说明 |
| --- | --- | --- |
| 读取 | Raw（带 `ImageShowInfo` 头） | `openImage()` |
| 读取 | Raw（无头） | `openImage_NotHeaderRaw()`，调用方提供宽高/位深/格式/偏移 |
| 读取 | BMP / PNG / JPEG | stb_image，8 位，Mono/RGB/RGBA |
| 保存 | Raw（可带 `ImageShowInfo` 头） | `ImageSaveOpe::rawAddHead` 控制写头 |
| 保存 | BMP / PNG / JPEG | 仅 `info.bit == 8` |

- `.tif/.tiff` 后缀可被 `getTypeByPath()` 识别，但 stb_image 不提供 TIFF 解码。
- `.raw` 同时可能表示图像或视频，按业务选择 `CyMediaImageParse` 或 `VideoParser`。

视频（`VideoParser`）：

| 后缀 | 解析器 | 输出 |
| --- | --- | --- |
| `.raw` / `.rawv` | `VideoParseRaw` | 自定义 `RawFileHeader`，固定帧长 |
| `.avi` | `VideoParseFfmpeg` | FFmpeg 解码，RGB888，8 位 |
| `.mp4` | `VideoParseFfmpeg` | FFmpeg 解码，RGB888，8 位 |

`.mkv`、`.mov`、`.wmv` 等后缀不进入当前 `VideoSuffix` 路由。Raw 视频 `format=false` 读取文件头，
`format=true` 使用调用方提供的 `VideoParseInfo`；帧索引从 1 开始。

---

## 2. 仓库目录

```text
CyMedia/                       # 仓库根
├── CyMedia/                   # 库工程
│   ├── src/
│   │   ├── Calculate/         # 纯算法
│   │   ├── Parse/             # 文件/视频解析
│   │   ├── common/            # 公共类型与日志
│   │   └── View/              # 显示模块
│   ├── resources/             # 图标、GLSL、翻译、ColorMap
│   ├── 3rdparty/              # FFmpeg / QCustomPlot / stb_image
│   ├── CyMedia.vcxproj
│   └── CMakeLists.txt
├── CyMediaDisTest/            # 示例程序
│   ├── CyMediaDisTest.vcxproj
│   └── CMakeLists.txt
├── OutFile/                   # 构建输出（gitignore）
├── CMakeLists.txt             # 顶层 CMake
├── GenRelease.bat             # 发布包生成
└── README.md
```

---

## 3. 公共头文件

发布包布局：

```text
include/
├── CyMediaBaseDef.h           # 聚合头
├── common/
│   ├── ImageTypes.h           # 像素格式、ImageShowInfo、色彩参数
│   ├── ParseTypes.h           # 文件后缀、ParseResult、视频信息/回调
│   ├── Log.h                  # LogLevel / LogCallback
│   ├── Language.h             # eLanguage
│   ├── Version.h              # VERSION
│   ├── Export.h               # CYMEDIA_LIB
│   └── DisLog.h               # 日志单例
├── View/
│   ├── Display.h              # 主入口
│   ├── DisplayTypes.h         # ItemType / StretchRange / 回调
│   ├── Annotation/            # BaseItem / DrawItem
│   ├── Dialogs/               # RAW 参数、License
│   ├── Widgets/               # 拉伸/统计/计时器头文件
│   └── README.md
├── Parse/
└── Calculate/
```

使用：

```cpp
#include "View/Display.h"                     // 显示主入口
#include "View/Dialogs/CyMediaDisRawInfoDialog.h"
#include "Parse/ImageParse.h"
#include "Parse/VideoParse.h"
#include "Calculate/Calculate.h"
```

`Display.h` 只引入显示 API 所需的公共类型；QGraphics 标注、RAW 对话框、工具栏由独立头文件提供。

### 最小示例

```cpp
#include "View/Display.h"

QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
QApplication app(argc, argv);

auto* display = new CyMedia::CyMediaDis(parent);
layout->addWidget(display);

CyMedia::ImageShowInfo info(width, height, bit, CyMedia::MONO);
display->upImageData(info, data);

display->setColorMap("jet");
display->setDrawMode(CyDisDrawItem::Rectangle);
```

线程模型、信号、标注 UUID API、辅助面板见 `src/View/README.md`。

---

## 4. 构建

### 4.1 前置条件

- Windows 10/11 x64 或 Win32
- Visual Studio 2022（v143）
- Qt 5.14.2（MSVC 构建）
- Windows SDK 10
- FFmpeg 库位于 `CyMedia/3rdparty/ffmpeg`（仓库包含 x64/Win32 的 include、lib、dll）

### 4.2 Visual Studio

打开根目录 `CyMedia.sln`，选择 `Debug|x64` 或 `Release|x64`：

```text
OutFile/x64/Release/CyMedia.dll
OutFile/x64/Release/CyMedia.lib
OutFile/CyMediaDisTest/x64/Release/CyMediaDisTest.exe
```

VS 工程由 Qt VS Tools（`QtMsBuild`）驱动。

### 4.3 CMake

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/Qt5.14.2/5.14.2/msvc2017_64"
cmake --build build --config Release --parallel
```

Win32：

```powershell
cmake -S . -B build-win32 -G "Visual Studio 17 2022" -A Win32 `
      -DCMAKE_PREFIX_PATH="C:/Qt/Qt5.14.2/5.14.2/msvc2017"
cmake --build build-win32 --config Release --parallel
```

选项：

| 选项 | 默认 | 说明 |
| --- | --- | --- |
| `CYMEDIA_BUILD_DEMO` | `ON` | 构建 `CyMediaDisTest` |
| `CYMEDIA_DEPLOY_QT` | `OFF` | Demo 构建后执行 `windeployqt` |
| `CMAKE_PREFIX_PATH` | — | Qt 安装目录 |

只构建库：追加 `-DCYMEDIA_BUILD_DEMO=OFF`。

CMake 处理：

- `.qrc`、`.ui` 与 `.cpp` 内的 `Q_OBJECT` / `.moc`；
- Demo 的 `App_*.qm` 翻译文件；
- x64 / Win32 的 FFmpeg include/lib；
- `CyMedia.dll`、FFmpeg DLL、Demo 翻译文件到输出目录。

> 维护提示：源文件列表变化时，同步更新 `CyMedia.vcxproj`、`CyMedia.vcxproj.filters` 与对应 `CMakeLists.txt`，保持两套构建一致。

### 4.4 CMake 安装

```powershell
cmake --install build --config Release --prefix install
```

```text
install/
├── bin/CyMedia.dll
├── bin/avcodec-*.dll ...          # FFmpeg runtime
├── bin/colorMap/                  # ColorMap 数据
├── lib/CyMedia.lib
└── include/
    ├── CyMediaBaseDef.h
    ├── common/
    ├── View/
    ├── Parse/
    └── Calculate/
```

Qt 运行库（`Qt5Core.dll`、`platforms/qwindows.dll` 等）由使用方用 `windeployqt` 部署。

---

## 5. 运行依赖

```text
CyMedia.dll
Qt5Core.dll / Qt5Gui.dll / Qt5Widgets.dll / Qt5OpenGL.dll
platforms/qwindows.dll
avcodec-*.dll / avformat-*.dll / avutil-*.dll / swscale-*.dll ...
colorMap/
```

图标、GLSL、翻译、QCustomPlot 通过 `.qrc` 编译进 DLL；ColorMap 从运行目录 `colorMap/` 读取。

---

## 6. 线程与 OpenGL

- 在 `QApplication` 创建前设置 `QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts)`。
- `CyMediaDis::upImageData()` 可从采集线程调用；`ImagePipeline` 负责队列与处理线程。
- 单帧/同步场景使用 `setDirectUpImage(true)`。
- 工作线程的 OpenGL 上传依赖共享上下文，不跨线程直接操作渲染器。
- `ImagePipeline::Sink::onFrame()` 在工作线程执行；渲染上传在该回调内完成，尺寸/渲染完成信号排队到 UI 线程。

---

## 7. 发布包

```bat
GenRelease.bat          :: 增量生成
GenRelease.bat clean    :: 清空输出目录后生成
```

```text
OutFile/CyMedia_Release/
├── README.md
├── include/
│   ├── CyMediaBaseDef.h
│   ├── View/
│   ├── Parse/
│   ├── Calculate/
│   └── common/
├── Bin/
│   ├── x64/{Debug,Release}/CyMedia.dll + FFmpeg DLL
│   ├── Win32/{Debug,Release}/
│   └── colorMap/
├── Lib/
│   ├── x64/{Debug,Release}/CyMedia.lib + .pdb
│   └── Win32/{Debug,Release}/
└── Demo/
    └── CyMediaDisTest/        # 源码 + CMakeLists
```

`Demo/CyMediaDisTest` 提供源码、CMakeLists、UI/qrc、翻译与资源，不包含预编译 exe 或 DLL。包内构建：

```powershell
cmake -S OutFile/CyMedia_Release/Demo/CyMediaDisTest -B build-demo `
      -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/Qt5.14.2/5.14.2/msvc2017_64"
cmake --build build-demo --config Release --parallel
```

Demo 的 CMake 自动从包根目录查找 `include/`、`Lib/`、`Bin/`；包被移动或拆开时可用
`-DCYMEDIA_PACKAGE_ROOT=<package root>` 指定。

---

## 8. 设计约束

1. `Calculate`、`Parse`、`common` 不依赖 `View`；`View` 单向依赖公共类型。
2. `common/ImageTypes.h`、`ParseTypes.h`、`Log.h`、`Language.h`、`Version.h`、`Export.h` 分别承载独立职责；`CyMediaBaseDef.h` 是聚合入口。
3. `View/Display.h` 只引入显示 API 所需类型；标注实现通过 `View/Annotation/BaseItem.h` 单独引入。
4. `Display` 是唯一同时连接 `ImagePipeline` 与渲染器的位置。
5. `ImagePipeline` 不包含 QWidget / QGraphics / QOpenGLWidget。
6. 标注、面板、对话框位于独立子目录，业务层可按需引入。

---

## 9. 文档索引

| 文档 | 内容 |
| --- | --- |
| 源码树根目录 `README.md` | 仓库总览、模块与示例 |
| `src/View/README.md`（发布包 `include/View/README.md`） | View 分层、数据流、线程、API |
| `CyMediaDisTest/README.md` | Demo 源码树/发布包构建方式 |
| `GenRelease.bat` | 发布包生成 |
| `CMakeLists.txt` / `CyMedia/CMakeLists.txt` | CMake 构建入口 |

## 10. 第三方

- Qt 5.14.2（LGPL/商业许可）
- FFmpeg（LGPL/GPL，按构建配置）
- QCustomPlot（GPL/商业许可）
- stb_image（public domain / MIT）
