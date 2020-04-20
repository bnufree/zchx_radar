#include <QDebug>
#include <QSettings>
#include "zchxradarpointthread.h"

using namespace ZCHX_RADAR_RECEIVER;

ZCHXRadarPointThread::ZCHXRadarPointThread(const ZCHX_Radar_Setting_Param& param, QObject *parent)
    : ZCHXReceiverThread(ZCHX_RECV_RADAR_POINT, param, parent)
{
    qRegisterMetaType<ITF_TrackPoint>("ITF_TrackPoint");
    qRegisterMetaType<ITF_RadarSurfaceTrack>("ITF_RadarSurfaceTrack");
    qRegisterMetaType<QList<ZCHX::Data::ITF_RadarPoint>>("const QList<ZCHX::Data::ITF_RadarPoint>&");
}

void ZCHXRadarPointThread::parseRecvData(const QByteArrayList& list)
{
    if(list.size() == 0) return;

    ITF_RadarSurfaceTrack objRadarSurfaceTrack;
    QList<ZCHX::Data::ITF_RadarPoint> radarPointList;

    //结果分析
    zchxFuntionTimer t(mRadarCommonSettings.m_sTopicList.join(","));
    if(!objRadarSurfaceTrack.ParseFromArray(list.last().data(), list.last().size())) return;
    parseRadarList(objRadarSurfaceTrack, radarPointList);
    emit sendMsg(mRadarCommonSettings.m_sSiteID, radarPointList);
}



void ZCHXRadarPointThread::parseRadarList(const ITF_RadarSurfaceTrack &objRadarSurfaceTrack,
                                       QList<ZCHX::Data::ITF_RadarPoint>& radarPointList)
{
    int size = objRadarSurfaceTrack.trackpoints_size();

    for (int i = 0; i < size; i++)
    {
        const ITF_TrackPoint & point = objRadarSurfaceTrack.trackpoints(i);
        ZCHX::Data::ITF_RadarPoint item;
        item.trackNumber              = point.tracknumber();
        item.timeOfDay                      = point.timeofday();
        item.systemAreaCode           = point.systemareacode();
        item.systemIdentificationCode = point.systemidentificationcode();
        item.cartesianPosX            = point.cartesianposx();
        item.cartesianPosY            = point.cartesianposy();
        item.wgs84PosLat              = point.wgs84poslat();
        item.wgs84PosLon             = point.wgs84poslong();
        item.timeOfDay                = point.timeofday();
        item.trackLastReport          = point.tracklastreport();
        item.sigmaX                   = point.sigmax();
        item.sigmaY                   = point.sigmay();
        item.sigmaXY                  = point.sigmaxy();
        item.ampOfPriPlot             = point.ampofpriplot();
        item.cartesianTrkVel_vx        = point.cartesiantrkvel_vx();
        item.cartesianTrkVel_vy        = point.cartesiantrkvel_vy();
        item.cog                      = point.cog();
        item.sog                      = point.sog();
        item.trackType                = ZCHX::Data::CNF(point.tracktype());
        item.fllow              = point.fleetnumber();
        item.status                   = point.status();
        item.extrapolation            = ZCHX::Data::CST(point.extrapolation());
        item.trackPositionCode        = ZCHX::Data::STH(point.trackpositioncode());
        item.diameter                 = point.diameter();

        radarPointList.append(item);
    }
}
