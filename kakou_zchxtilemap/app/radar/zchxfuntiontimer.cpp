#include "zchxfuntiontimer.h"
#include <QDebug>

zchxFuntionTimer::zchxFuntionTimer(const QString& func)
{
    mFunc = func;
    mTime.start();
//    qDebug()<<QString("%1 start").arg(mFunc);
}

qint64 zchxFuntionTimer::elapsed()
{
    qint64 t = mTime.elapsed();
    mTime.restart();
    return t;
}

zchxFuntionTimer::~zchxFuntionTimer()
{
#if 0
    qDebug()<<QString("%1 end with elpsed:%2 ms").arg(mFunc).arg(elapsed());
#endif
}
