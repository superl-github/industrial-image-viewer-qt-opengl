#include "CyMediaDisTest.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <iostream>

#include <QApplication>
#include <QFile>
#include <QSurfaceFormat>
#include <QTextCodec>
#include <QFileInfo>

bool createConsole(std::string title, std::string testStr);
int main(int argc, char *argv[]) {
    //获取传递参数
    QString dragFile;
    QStringList mainCommandList;
    for (int i = 0; i < argc - 1; i++) {
        mainCommandList.append(argv[i + 1]);
    }
    for (auto& oneCommand : mainCommandList) {
        if (oneCommand == "-c") {
            createConsole("CyMediaDisTest", "init...\n");
        }
        else {
            if (QFileInfo(oneCommand).isFile()) {
                dragFile = oneCommand;
            }
        }
    }

    QFile tempFile("D:\\Users\\llf\\Desktop\\RTX\\llf\\light_defect.raw");
    int index = 0;
    if (tempFile.open(QIODevice::ReadOnly)) {
        auto readCode = tempFile.readAll();
        tempFile.close();
        double* pCode = (double*)readCode.data();
        QList<QPoint> checkPosList;
        checkPosList.append({ 809, 840});
        checkPosList.append({ 801, 841 });
        checkPosList.append({ 800, 841 });
        checkPosList.append({ 800, 840 });

        for (auto onePos : checkPosList) {
            printf("(%d,%d) %f\n", onePos.x(), onePos.y(), pCode[onePos.y() * 1024 + onePos.x()]);
        }
        printf("\n\n");
        for (auto onePos : checkPosList) {
            printf("(%d,%d) => %08x\n", onePos.x(), onePos.y(), ((onePos.y() << 16) | onePos.x()));
        }
        printf("\n\n");
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
    if (!AllocConsole()) return false;

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA(title.data());

    // 重定向标准输出/错误到新控制台
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);

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
}