#ifndef ZMQRADARECHOTHREAD_H
#define ZMQRADARECHOTHREAD_H

#include <QThread>
#include <QMap>
#include <QDateTime>
#include "ZCHXRadarVideo.pb.h"
#include "zchxradarutils.h"

typedef com::zhichenhaixin::proto::RadarVideo  ITF_VideoFrame;
typedef QMap<int, ITF_VideoFrame> Map_RadarVideo;

namespace ZCHX_RADAR_RECEIVER{

class ZCHXRadarEchoThread : public ZCHXReceiverThread
{
    Q_OBJECT
public:
    ZCHXRadarEchoThread(const ZCHX_Radar_Setting_Param& param, QObject * parent = 0);
    virtual void parseRecvData(const QByteArrayList&);

private:
    void dealRadarEchoData(const ITF_VideoFrame &objVideoFrame);

signals:
    void sendMsg(int siteID, double lon, double lat, double dis, int type, int loop, int curIndex, const QByteArray& objPixmap, const QByteArray& prePixMap);
private:
    qint64 m_lastUpdateRadarEchoTime;       //最后更雷达回波数据的时间
    Map_RadarVideo m_videoFrameMap;
};
}

#endif // ZMQRADARECHOTHREAD_H
