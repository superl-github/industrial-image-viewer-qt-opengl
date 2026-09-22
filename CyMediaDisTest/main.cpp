#include "CyMediaDisTest.h" //mainwindow

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif
#include <iostream>

#include <QApplication>
#include <QFile>
#include <QSurfaceFormat>
#include <QTextCodec>
#include <QFileInfo>
#include <qDebug>

bool createConsole(std::string title, std::string testStr);
int main(int argc, char *argv[]) {
    //解析arg
    QString dragFile;
    QStringList argList;
#if defined(_WIN32)
    int wArgc = 0;
    LPWSTR* wArgv = CommandLineToArgvW(GetCommandLineW(), &wArgc);
    if (wArgv != nullptr) {
        for (int i = 0; i < wArgc; ++i){
            QString arg = QString::fromWCharArray(wArgv[i]);
            argList.append(arg);
        }
        HeapFree(GetProcessHeap(), 0, wArgv);
    }
#else
    for (int i = 0; i < argc; i++) {
        argList.append(QString::fromLocal8Bit(argv[i]));
    }
#endif
    for (int i = 1; i < argList.size(); i++) {
        const QString& arg = argList[i];
        if (arg == "-c") {
            createConsole("CyMediaDisTest", "init...\n");
        }
        else {
            if (QFileInfo(arg).isFile()) {
                dragFile = arg;
            }
        }
    }


    //启用全局共享上下文
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    //指定OpenGL版本
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapInterval(0);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    //format.setDepthBufferSize(24);
    //format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    //opengl检测
    bool supportOpenGL = CyMedia::CyMediaDis::supportsOpenGLForCyMedia();
    if (false == supportOpenGL) {
#if defined(_WIN32)
        MessageBoxA(
            nullptr,
            "The current device's OpenGL support does not meet the program's requirements.",
            "error",
            MB_OK
        );
#else
        std::cerr << "ERROR: The current device's OpenGL support does not meet the program's requirements." << std::endl;
#endif
        app.quit();
        return -1;
    }
    else {
        printf("OpenGL yes!!!\n");
    }
    CyMediaDisTest window;
    window.show();
    if (dragFile.size()) {
        window.openFile(dragFile);
    }
    return app.exec();
}


void qConsoleMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString typeStr;
    switch (type) {
        case QtDebugMsg:
            typeStr = "[Q_DEBUG]";
            break;
        case QtWarningMsg:
            typeStr = "[Q_WARNING]";
            break;
        case QtCriticalMsg:
            typeStr = "[Q_CRITICAL]";
            break;
        case QtFatalMsg:
            typeStr = "[VFATAL]";
            break;
        case QtInfoMsg:
            typeStr = "[Q_INFO]";
            break;
    }

    std::cout << (typeStr + msg).toUtf8().data() << std::endl;
}

bool createConsole(std::string title, std::string testStr) {
#if defined(_WIN32)
    if (!AllocConsole()) return false;

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA(title.data());

    // 重定向标准输出/错误到新控制台
    (void)freopen("CONOUT$", "w", stdout);
    (void)freopen("CONOUT$", "w", stderr);

    // 禁用快速编辑
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hInput, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    auto re = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    // 清空缓存
    std::ios::sync_with_stdio(true);
    std::cin.clear();
    std::cout.clear();
    std::cerr.clear();

    // 连接Qt输出信息
    qInstallMessageHandler(qConsoleMessageHandler);

    std::cout << testStr << std::endl;

    return true;
#else
    (void)title;
    qInstallMessageHandler(qConsoleMessageHandler);
    std::cout << testStr << std::endl;
    return true;
#endif
}