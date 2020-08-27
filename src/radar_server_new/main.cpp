#include <QApplication>
#include "mainprocess.h"
#include "zchxmainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#if 0
    if(argc == 1)
    {
        if(!MainProc->isStart()) MainProc->start();
    }
#endif
    zchxMainWindow w;
    w.show();
    return a.exec();
}
