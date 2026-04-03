#include <QtWidgets/QApplication>

#include <iostream>
#include <windows.h>

#include "CyMediaDisTest.h"

void createConsole(std::string title, std::string testStr);
int main(int argc, char *argv[])
{
    createConsole("CyMediaDisTest", "init...\n");
    QApplication app(argc, argv);
    CyMediaDisTest window;
    window.show();
    return app.exec();
}


void qConsoleMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
}
void createConsole(std::string title, std::string testStr) {
}