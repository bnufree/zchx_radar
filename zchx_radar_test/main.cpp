#include "radartestwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RadarTestWindow w;
    w.showMaximized();

    return a.exec();
}
