#include "testmapwatchdogthread.h"
#include <QDir>
#include <QApplication>

testMapWatchDogThread::testMapWatchDogThread(QObject *parent) : QThread(parent)
{

}

void testMapWatchDogThread::run()
{
    QDir dir(QApplication::applicationDirPath() + QString("/watchdog"));
    if(!dir.exists())
    {
        dir.mkpath(dir.path());
    }
    QString fileName = QString("%1/zchxMapTest.txt").arg(dir.path());
    QFile file(fileName);
    while (true) {
        if(file.open(QIODevice::WriteOnly))
        {
            file.write("ddddd");
            file.close();
        }
        sleep(30);
    }
}
