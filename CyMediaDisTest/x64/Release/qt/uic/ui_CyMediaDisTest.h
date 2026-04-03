/********************************************************************************
** Form generated from reading UI file 'CyMediaDisTest.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CYMEDIADISTEST_H
#define UI_CYMEDIADISTEST_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CyMediaDisTestClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *CyMediaDisTestClass)
    {
        if (CyMediaDisTestClass->objectName().isEmpty())
            CyMediaDisTestClass->setObjectName(QString::fromUtf8("CyMediaDisTestClass"));
        CyMediaDisTestClass->resize(600, 400);
        menuBar = new QMenuBar(CyMediaDisTestClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        CyMediaDisTestClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(CyMediaDisTestClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        CyMediaDisTestClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(CyMediaDisTestClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        CyMediaDisTestClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(CyMediaDisTestClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        CyMediaDisTestClass->setStatusBar(statusBar);

        retranslateUi(CyMediaDisTestClass);

        QMetaObject::connectSlotsByName(CyMediaDisTestClass);
    } // setupUi

    void retranslateUi(QMainWindow *CyMediaDisTestClass)
    {
        CyMediaDisTestClass->setWindowTitle(QCoreApplication::translate("CyMediaDisTestClass", "CyMediaDisTest", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CyMediaDisTestClass: public Ui_CyMediaDisTestClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CYMEDIADISTEST_H
