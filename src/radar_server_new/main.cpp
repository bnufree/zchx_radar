#include <QApplication>
#include "mainprocess.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if(argc == 1)
    {
        if(!MainProc->isStart()) MainProc->start();
    }
    return a.exec();
}
