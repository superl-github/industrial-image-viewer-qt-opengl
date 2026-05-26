#include <QtWidgets/QApplication>

#include "CyMediaDisTest.h"

#include <iostream>
#include <windows.h>

#include <QFile>

void createConsole(std::string title, std::string testStr);
int main(int argc, char *argv[])
{
    if (argc >= 2 && QString(argv[1]) == "-c")
        createConsole("CyMediaDisTest", "init...\n");

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