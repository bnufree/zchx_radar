#ifndef ZCHXRADARCOMMON_H
#define ZCHXRADARCOMMON_H

#include "ZCHXRadar.pb.h"
#include "ZCHXRadarVideo.pb.h"
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QApplication>


typedef     com::zhichenhaixin::proto::RadarSurfaceTrack    zchxRadarSurfaceTrack;
typedef     com::zhichenhaixin::proto::VideoFrame           zchxVideoFrame;
typedef     QList<zchxVideoFrame>                           zchxVideoFrameList;
typedef     QMap<int, zchxVideoFrame>                       zchxVideoFrameMap;

typedef     com::zhichenhaixin::proto::TrackPoint           zchxTrackPoint;
typedef     QMap<int,zchxTrackPoint>                        zchxTrackPointMap;
typedef     QList<zchxTrackPoint>                           zchxTrackPointList;

typedef     com::zhichenhaixin::proto::RadarRect            zchxRadarRect;
typedef     QList<zchxRadarRect>                            zchxRadarRectList;
typedef     com::zhichenhaixin::proto::RadarRectDef         zchxRadarRectDef;
typedef     QList<zchxRadarRectDef>                         zchxRadarRectDefList;

typedef     QMap<int,zchxRadarRect>                         zchxRadarRectMap;
typedef     com::zhichenhaixin::proto::singleVideoBlock     zchxSingleVideoBlock;

typedef     com::zhichenhaixin::proto::RadarRects           zchxRadarRects;

typedef     QMap<int,QList<com::zhichenhaixin::proto::RadarHistoryTracks>> zchxHistoryTrackMap;

class zchxRadarUtils
{
public:
    static QByteArray    surfaceTrackToByteArray(const zchxRadarSurfaceTrack& tracks);
    static QByteArray    protoBufMsg2ByteArray(::google::protobuf::Message* msg);
};

#endif // ZCHXRADARCOMMON_H
