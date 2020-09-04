#include <QApplication>
#include "mainprocess.h"
#include "zchxmainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if(!MainProc->isStart()) MainProc->start();
    zchxMainWindow w;
    w.show();
    return a.exec();
}
