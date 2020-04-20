#ifndef AISTHREAD_H
#define AISTHREAD_H

#include <QThread>
#include "ZCHXAISVessel.pb.h"
#include "zchxradarutils.h"


typedef com::zhichenhaixin::proto::AISList ITF_AISLIST;
namespace ZCHX_RADAR_RECEIVER
{
class ZCHXAisThread : public ZCHXReceiverThread
{
    Q_OBJECT
public:
    explicit ZCHXAisThread(const ZCHX_Radar_Setting_Param& param, QObject *parent = 0);
    virtual void parseRecvData(const QByteArrayList&);

signals:
    void sendMsg(const QList<ZCHX::Data::ITF_AIS>&);
private:
    void parseAisList(const ITF_AISLIST &objRadarSurfaceTrack);

private:
    QMap<QString, ZCHX::Data::ITF_AIS>  mDataMap;
};

class ZCHXAisChartThread : public ZCHXReceiverThread
{
    Q_OBJECT
public:
    explicit ZCHXAisChartThread(const ZCHX_Radar_Setting_Param& param, QObject *parent = 0);
    virtual void parseRecvData(const QByteArrayList&);

signals:
    void sendMsg(const ZCHX::Data::ITF_AIS_Chart&);

private:
    QMap<QString, ZCHX::Data::ITF_AIS>  mDataMap;
};
}

#endif // AISTHREAD_H
