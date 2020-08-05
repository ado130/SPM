#include "mainwindow.h"

#include <QApplication>
#include <QLocale>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QLocale::setDefault(QLocale(QLocale::English, QLocale::Europe));

    QApplication a(argc, argv);
    a.setOrganizationDomain("SPM");
    a.setApplicationName("SPM");

    MainWindow w;
    w.show();

    return a.exec();
}
