#ifndef ZMQRADARECHOTHREAD_H
#define ZMQRADARECHOTHREAD_H

#include <QThread>
#include <QMap>
#include <QDateTime>
#include "protobuf/ZCHXRadar.pb.h"


typedef QMap<int, ITF_VideoFrame > Map_RadarVideo;

class ZMQRadarEchoThread : public QThread
{
    Q_OBJECT
public:
    ZMQRadarEchoThread(QObject * parent = 0);
    void run();
    bool getIsOver() const;
    void setIsOver(bool value);

private:
        void dealRadarEchoData(const ITF_VideoFrame &objVideoFrame);

signals:
    void sendMsg(const Map_RadarVideo &);
private:
    bool  isOver;
    qint64 m_lastUpdateRadarEchoTime;       //最后更雷达回波数据的时间
    Map_RadarVideo m_videoFrameMap;
};

#endif // ZMQRADARECHOTHREAD_H
