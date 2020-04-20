#ifndef ZCHXRADARECHODATACHANGE_H
#define ZCHXRADARECHODATACHANGE_H

#include <QObject>
#include "zmqradarechothread.h"
#include <math.h>
//该类调用方法：1、调用接口updateRadarEchoData()；2、接收sendMsg()信号发送的雷达回波数据
class  ZCHXRadarEchoDataChange : public QObject
{
    Q_OBJECT
public:
    ZCHXRadarEchoDataChange(QObject *parent=Q_NULLPTR);
    ~ZCHXRadarEchoDataChange();
    void updateRadarEchoData();
signals:
    void sendMsg(const Map_RadarVideo &);
protected slots:
    void reciveMsg(const Map_RadarVideo &radarVideoMap);
private:
    void closeRadarEchoData();
private:

    ZMQRadarEchoThread *m_pZMQRadarEchoThread;

};

#endif // ZCHXRADARECHODATACHANGE_H
