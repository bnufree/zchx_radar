#ifndef ZCHXFUNTIONTIMER_H
#define ZCHXFUNTIONTIMER_H

#include <QString>
#include <QTime>

class zchxFuntionTimer
{
public:
    zchxFuntionTimer(const QString& func= "");
    virtual ~zchxFuntionTimer();
private:
    qint64  elapsed();
private:
    QString mFunc;
    QTime   mTime;
};

#endif // ZCHXFUNTIONTIMER_H
