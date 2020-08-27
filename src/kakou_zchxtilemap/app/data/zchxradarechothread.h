#ifndef ZMQRADARECHOTHREAD_H
#define ZMQRADARECHOTHREAD_H

#include <QThread>
#include <QMap>
#include <QDateTime>
#include "ZCHXRadarDataDef.pb.h"
#include "zchxradarutils.h"

typedef com::zhichenhaixin::proto::RadarVideoImage  zchxRadarVideoImg;

namespace ZCHX_RADAR_RECEIVER{

class ZCHXRadarEchoThread : public ZCHXReceiverThread
{
    Q_OBJECT
public:
    ZCHXRadarEchoThread(const ZCHX_Radar_Setting_Param& param, QObject * parent = 0);
    virtual void parseRecvData(const QByteArrayList&);

private:
    void dealRadarEchoData(const zchxRadarVideoImg &objVideoFrame);

signals:
    void sendMsg(int siteID, double lon, double lat, double dis, const QByteArray& objPixmap);
private:
    qint64 m_lastUpdateRadarEchoTime;       //最后更雷达回波数据的时间
};
}

#endif // ZMQRADARECHOTHREAD_H
