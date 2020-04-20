#ifndef UP_AIS_PTHREAD_H
#define UP_AIS_PTHREAD_H

#include <QObject>
#include <QThread>

class up_ais_pthread : public QThread
{
    Q_OBJECT
public:
     up_ais_pthread(QString filename, int interval, QObject *parent = 0);
     void run();
     void   setInterval(int inter) {mInterval = inter;}
     void   setIsOver(bool over) {mIsOver = over;}
signals:
     void send_ais_signal(QByteArray);//导入AIS数据

public slots:

private:
     QString    mFileName;
     int        mInterval;
     bool       mIsOver;
};
#endif // UP_AIS_PTHREAD_H
