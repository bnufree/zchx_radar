#ifndef ZMQRADARVIDEOTHREAD_H
#define ZMQRADARVIDEOTHREAD_H

#include <QThread>
#include "ZCHXRadarVideo.pb.h"
#include "zchxradarutils.h"
#include <QRunnable>

typedef com::zhichenhaixin::proto::singleVideoBlock  PROTOBUF_SingleVideoBlock;
typedef com::zhichenhaixin::proto::RadarRectDef  PROTOBUF_RadarRectDef;

typedef com::zhichenhaixin::proto::RadarRect  PROTOBUF_RadarRect;
typedef com::zhichenhaixin::proto::RadarRects PROTOBUF_RadarRectList;


namespace ZCHX_RADAR_RECEIVER
{

class zchxRectVideoFunc : public QRunnable
{
public:
    zchxRectVideoFunc(ZCHX::Data::ITF_RadarRect* rect = 0) : mRect(rect) {}
    void run();
private:
    ZCHX::Data::ITF_RadarRect* mRect;
};

class ZCHXRadarRectThread : public ZCHXReceiverThread
{
    Q_OBJECT
public:
    ZCHXRadarRectThread(const ZCHX_RadarRect_Param& param, QObject *parent = 0);
    virtual void parseRecvData(const QByteArrayList &data);

private:
    void convertZMQ2ZCHX(QList<ZCHX::Data::ITF_RadarRect>& res, const PROTOBUF_RadarRectList& src);    
signals:
    void sendVideoMsg(int id, const QList<ZCHX::Data::ITF_RadarRect>&);
private:
    ZCHX_RadarRect_Param     mRectParam;

};
}

#endif // ZMQRADARVIDEOTHREAD_H
