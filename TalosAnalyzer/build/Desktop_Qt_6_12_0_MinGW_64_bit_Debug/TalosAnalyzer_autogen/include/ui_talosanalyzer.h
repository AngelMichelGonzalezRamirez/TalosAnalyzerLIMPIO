/********************************************************************************
** Form generated from reading UI file 'talosanalyzer.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TALOSANALYZER_H
#define UI_TALOSANALYZER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TalosAnalyzer
{
public:
    QWidget *centralwidget;
    QPushButton *JBAbrir;
    QPushButton *JBLimpiar;
    QPushButton *JBGuardar;
    QPushButton *JBAnalizar;
    QPushButton *JBSalir;
    QLabel *JLCodigo;
    QPlainTextEdit *JTACodigo;
    QLabel *JLErrores;
    QPlainTextEdit *JTAErrores;
    QLabel *JLSintaxis;
    QPlainTextEdit *JTASintaxis;
    QLabel *JLTokens;
    QPlainTextEdit *JTATokens;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *TalosAnalyzer)
    {
        if (TalosAnalyzer->objectName().isEmpty())
            TalosAnalyzer->setObjectName("TalosAnalyzer");
        TalosAnalyzer->resize(1313, 724);
        centralwidget = new QWidget(TalosAnalyzer);
        centralwidget->setObjectName("centralwidget");
        JBAbrir = new QPushButton(centralwidget);
        JBAbrir->setObjectName("JBAbrir");
        JBAbrir->setGeometry(QRect(30, 630, 83, 29));
        JBLimpiar = new QPushButton(centralwidget);
        JBLimpiar->setObjectName("JBLimpiar");
        JBLimpiar->setGeometry(QRect(150, 630, 83, 29));
        JBGuardar = new QPushButton(centralwidget);
        JBGuardar->setObjectName("JBGuardar");
        JBGuardar->setGeometry(QRect(280, 630, 83, 29));
        JBAnalizar = new QPushButton(centralwidget);
        JBAnalizar->setObjectName("JBAnalizar");
        JBAnalizar->setGeometry(QRect(420, 630, 83, 29));
        JBSalir = new QPushButton(centralwidget);
        JBSalir->setObjectName("JBSalir");
        JBSalir->setGeometry(QRect(570, 630, 83, 29));
        JLCodigo = new QLabel(centralwidget);
        JLCodigo->setObjectName("JLCodigo");
        JLCodigo->setGeometry(QRect(20, 10, 721, 20));
        JTACodigo = new QPlainTextEdit(centralwidget);
        JTACodigo->setObjectName("JTACodigo");
        JTACodigo->setGeometry(QRect(20, 32, 721, 579));
        JLErrores = new QLabel(centralwidget);
        JLErrores->setObjectName("JLErrores");
        JLErrores->setGeometry(QRect(750, 460, 551, 20));
        JTAErrores = new QPlainTextEdit(centralwidget);
        JTAErrores->setObjectName("JTAErrores");
        JTAErrores->setGeometry(QRect(750, 482, 551, 169));
        JLSintaxis = new QLabel(centralwidget);
        JLSintaxis->setObjectName("JLSintaxis");
        JLSintaxis->setGeometry(QRect(750, 230, 551, 20));
        JTASintaxis = new QPlainTextEdit(centralwidget);
        JTASintaxis->setObjectName("JTASintaxis");
        JTASintaxis->setGeometry(QRect(750, 252, 551, 169));
        JLTokens = new QLabel(centralwidget);
        JLTokens->setObjectName("JLTokens");
        JLTokens->setGeometry(QRect(750, 10, 551, 20));
        JTATokens = new QPlainTextEdit(centralwidget);
        JTATokens->setObjectName("JTATokens");
        JTATokens->setGeometry(QRect(750, 32, 551, 169));
        TalosAnalyzer->setCentralWidget(centralwidget);
        menubar = new QMenuBar(TalosAnalyzer);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1313, 25));
        TalosAnalyzer->setMenuBar(menubar);
        statusbar = new QStatusBar(TalosAnalyzer);
        statusbar->setObjectName("statusbar");
        TalosAnalyzer->setStatusBar(statusbar);
#if QT_CONFIG(shortcut)
        JLCodigo->setBuddy(JTACodigo);
        JLErrores->setBuddy(JTAErrores);
        JLSintaxis->setBuddy(JTASintaxis);
        JLTokens->setBuddy(JTATokens);
#endif // QT_CONFIG(shortcut)

        retranslateUi(TalosAnalyzer);

        QMetaObject::connectSlotsByName(TalosAnalyzer);
    } // setupUi

    void retranslateUi(QMainWindow *TalosAnalyzer)
    {
        TalosAnalyzer->setWindowTitle(QCoreApplication::translate("TalosAnalyzer", "TalosAnalyzer", nullptr));
        JBAbrir->setText(QCoreApplication::translate("TalosAnalyzer", "Abrir", nullptr));
        JBLimpiar->setText(QCoreApplication::translate("TalosAnalyzer", "Limpiar", nullptr));
        JBGuardar->setText(QCoreApplication::translate("TalosAnalyzer", "Guardar", nullptr));
        JBAnalizar->setText(QCoreApplication::translate("TalosAnalyzer", "Analizar", nullptr));
        JBSalir->setText(QCoreApplication::translate("TalosAnalyzer", "Salir", nullptr));
        JLCodigo->setText(QCoreApplication::translate("TalosAnalyzer", "C\303\263digo fuente", nullptr));
        JLErrores->setText(QCoreApplication::translate("TalosAnalyzer", "Errores: l\303\255nea, columna y descripci\303\263n", nullptr));
        JLSintaxis->setText(QCoreApplication::translate("TalosAnalyzer", "Resultado del an\303\241lisis sint\303\241ctico", nullptr));
        JLTokens->setText(QCoreApplication::translate("TalosAnalyzer", "Tokens: lexema y categor\303\255a", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TalosAnalyzer: public Ui_TalosAnalyzer {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TALOSANALYZER_H
