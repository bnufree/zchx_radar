#include "radartestwindow.h"
#include <QApplication>

//test QHash
struct TestObj{
    int id;
    QString val;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    RadarTestWindow w;
    w.showMaximized();

    return a.exec();
}
