#ifndef DIRECTRADARTHREAD_H
#define DIRECTRADARTHREAD_H

#include <QThread>
#include "ZCHXRadar.pb.h"
#include "ZCHXRadarVideo.pb.h"
#include "zchxradarutils.h"

typedef com::zhichenhaixin::proto::TrackPoint ITF_TrackPoint;
typedef com::zhichenhaixin::proto::RadarSurfaceTrack ITF_RadarSurfaceTrack;
typedef com::zhichenhaixin::proto::RadarRectDef      PROTOBUF_RECT_DEF;

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
    void sendMsg(const QList<ZCHX::Data::ITF_RadarRouteNode>&);
private:
    void parseRadarList(const ITF_RadarSurfaceTrack &objRadarSurfaceTrack,
                        QList<ZCHX::Data::ITF_RadarPoint>& radarPointList);
    void transferNodeRect(ZCHX::Data::ITF_RadarRectDef& out, const PROTOBUF_RECT_DEF& in);
};
}

#endif // DIRECTRADARTHREAD_H
