#include <QtWidgets/QApplication>

#include <iostream>
#include <windows.h>

#include "CyMediaDisTest.h"

void createConsole(std::string title, std::string testStr);
int main(int argc, char *argv[])
{
    if (argc >= 2 && QString(argv[1]) == "-c")
        createConsole("CyMediaDisTest", "init...\n");
    QApplication app(argc, argv);
    CyMediaDisTest window;
    window.show();
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
void createConsole(std::string title, std::string testStr) {
    if (!AllocConsole())
        return;

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA(title.data());

    // 重定向标准输出/错误到新控制台
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hInput, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    auto re = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    std::ios::sync_with_stdio(true);
    std::cin.clear();
    std::cout.clear();
    std::cerr.clear();

    qInstallMessageHandler(qConsoleMessageHandler);

    std::cout << testStr << std::endl;
}