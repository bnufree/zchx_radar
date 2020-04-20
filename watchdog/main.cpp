#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString appName;
    if(argc >=2 )
    {
        appName = QString::fromStdString(argv[1]);
    }
    MainWindow w(appName);
    w.hide();

    return a.exec();
}
