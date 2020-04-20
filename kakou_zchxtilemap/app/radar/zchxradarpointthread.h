#ifndef DIRECTRADARTHREAD_H
#define DIRECTRADARTHREAD_H

#include <QThread>
#include "ZCHXRadar.pb.h"
#include "zchxradarutils.h"

typedef com::zhichenhaixin::proto::TrackPoint ITF_TrackPoint;
typedef com::zhichenhaixin::proto::RadarSurfaceTrack ITF_RadarSurfaceTrack;

namespace ZCHX_RADAR_RECEIVER
{
class ZCHXRadarPointThread : public ZCHXReceiverThread
{
    Q_OBJECT
public:
    explicit ZCHXRadarPointThread(const ZCHX_Radar_Setting_Param& param, QObject *parent = 0);
    virtual void parseRecvData(const QByteArrayList& list);

signals:
    void sendMsg(int, const QList<ZCHX::Data::ITF_RadarPoint>&);
private:
    void parseRadarList(const ITF_RadarSurfaceTrack &objRadarSurfaceTrack,
                        QList<ZCHX::Data::ITF_RadarPoint>& radarPointList);
};
}

#endif // DIRECTRADARTHREAD_H
