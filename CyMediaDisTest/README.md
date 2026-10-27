# CyMediaDisTest

这是 CyMedia 的示例与冒烟测试程序，包含：

- 图像/视频打开与回放
- 模拟图像采集
- ColorMap / Bayer / YUV / 灰度拉伸 / 直方图
- 图形标注与 ROI 统计
- 中英文切换

## 在源码树中构建

推荐使用仓库根目录：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/Qt5.14.2/5.14.2/msvc2017_64"
cmake --build build --config Release --parallel
```

也可以打开根目录 `CyMedia.sln`，选择 `CyMediaDisTest` 工程。

## 在发布包中构建

发布包的目录结构是：

```text
CyMedia/
├── include/
├── Bin/<arch>/<config>/
├── Lib/<arch>/<config>/
└── Demo/CyMediaDisTest/      # 本目录
```

本目录的 `CMakeLists.txt` 会自动在 `../..` 查找发布包中的 `include/`、`Lib/`、`Bin/`：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/Qt5.14.2/5.14.2/msvc2017_64"
cmake --build build --config Release --parallel
```

如果包被移动或拆开，可显式指定：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCYMEDIA_PACKAGE_ROOT="D:/sdk/CyMedia" `
      -DCMAKE_PREFIX_PATH="C:/Qt/Qt5.14.2/5.14.2/msvc2017_64"
```

构建完成后，`CyMedia.dll`、FFmpeg DLL、翻译文件会复制到可执行文件旁边。Qt 运行库可用 `windeployqt` 部署，或把 Qt `bin` 加入 `PATH`。

> 发布包中的 Demo 只提供源码 + CMake，不包含预编译 exe。VS 工程文件保留在源码仓库根目录的 `CyMedia.sln` 中。
