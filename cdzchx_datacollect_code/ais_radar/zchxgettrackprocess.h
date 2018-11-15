#ifndef ZCHXGETTRACKPROCESS_H
#define ZCHXGETTRACKPROCESS_H

#include <QObject>
#include <QThread>
#include "zchxfunction.h"
#include "protobuf/ZCHXRadar.pb.h"
extern "C"

{

#include "ctrl.h"

}
struct Mercator{
public:
    Mercator(double x, double y){mX = x; mY= y;}
    bool operator ==(const Mercator& other)
    {
        return fabs(this->mX- other.mX) <= 0.000000001  && \
               fabs(this->mY - other.mY) <= 0.000000001 ;
    }
    double mX;
    double mY;
};

struct Wgs84LonLat{
public:
    Wgs84LonLat() {mLon = 0.0; mLat = 0.0;}
    Wgs84LonLat(double x, double y){mLon = x; mLat= y;}
    bool operator ==(const Wgs84LonLat& other) const
    {
        return fabs(this->mLon - other.mLon) <= 0.000000001  && \
               fabs(this->mLat - other.mLat) <= 0.000000001 ;
    }

    double mLon;
    double mLat;
};

typedef com::zhichenhaixin::proto::TrackPoint ITF_Track_point;
typedef QMap<int,com::zhichenhaixin::proto::TrackPoint> Radar_Track_Map;
typedef QList<SAzmData>             SAzmDataList;
struct  TrackObj{
    int uKey;
    ITF_Track_point radarPoint;
};

typedef QList<TrackObj>             TrackObjList;
class ZCHXGetTrackProcess : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXGetTrackProcess(double dLat,double dLon,QObject *parent = 0);
    ~ZCHXGetTrackProcess();
    Wgs84LonLat mercatorToWgs84LonLat(const Mercator& mercator)
    {
        double x = mercator.mX/20037508.34*180.0;
        double y = mercator.mY/20037508.34*180.0;
        y= 180/3.1415926*(2*atan(exp(y* 3.1415926535897932384626/180.0))-3.1415926/2.0);
        return Wgs84LonLat(x, y);
    }

    Mercator wgs84LonlatToMercator(const Wgs84LonLat& wgs84 )
    {
        double x = wgs84.mLon * 20037508.34 / 180;
        double y = log(tan((90 + wgs84.mLat) *  3.1415926535897932384626 / 360)) / (3.1415926 / 180);
        y = y * 20037508.34 / 180;

        return Mercator(x, y);
    }



signals:
    void getTrackProcessSignal(SAzmData sAzmData);
    void sendTrack(int uKey,ITF_Track_point radarPoint);
    void getTrackProcessSignal(const SAzmDataList& sAzmDataList);
    void sendTrack(const TrackObjList& radarPoints);

public slots:
    void getTrackProcessSlot(SAzmData sAzmData);
    void getTrackProcessSlot(const SAzmDataList& sAzmDataList);

private:
    double m_dCentreLon;
    double m_dCentreLat;
    QThread m_workThread;


};

#endif // ZCHXGETTRACKPROCESS_H
