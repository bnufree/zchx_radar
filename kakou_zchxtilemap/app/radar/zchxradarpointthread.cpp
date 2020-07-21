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
    qRegisterMetaType<QList<ZCHX::Data::ITF_RadarRouteNode>>("const QList<ZCHX::Data::ITF_RadarRouteNode>&");
}

void ZCHXRadarPointThread::transferNodeRect(ZCHX::Data::ITF_RadarRectDef &out, const PROTOBUF_RECT_DEF &in)
{
    out.rectNumber = in.rectnumber();
    out.topLeftlatitude = in.topleftlatitude();
    out.topLeftlongitude = in.topleftlongitude();
    out.bottomRightlatitude = in.bottomrightlatitude();
    out.bottomRightlongitude = in.bottomrightlongitude();
    out.centerlatitude = in.centerlatitude();
    out.centerlongitude = in.centerlongitude();
    out.updateTime = in.updatetime();
    out.diameter = in.diameter();
    out.startlatitude = in.startlatitude();
    out.startlongitude = in.startlongitude();
    out.endlatitude = in.endlatitude();
    out.endlongitude = in.endlongitude();
    out.angle = in.cog();
    out.isRealData = in.realdata();

    //添加预推区域
    if(in.has_predictionareas())
    {
        com::zhichenhaixin::proto::predictionArea area(in.predictionareas());
        for(int m =0; m<area.area_size(); m++)
        {
            ZCHX::Data::ITF_SingleVideoBlock block;
            block.latitude = area.area(m).latitude();
            block.longitude = area.area(m).longitude();
            out.predictionArea.append(block);
        }
    }
}

void ZCHXRadarPointThread::parseRecvData(const QByteArrayList& list)
{
    if(list.size() == 0) return;


    //结果分析
    zchxFuntionTimer t(mRadarCommonSettings.m_sTopicList.join(","));
    QString topic = QString::fromLatin1(list.first().data());
    if(topic.contains("track", Qt::CaseInsensitive))
    {
        ITF_RadarSurfaceTrack objRadarSurfaceTrack;
        QList<ZCHX::Data::ITF_RadarPoint> radarPointList;
        if(!objRadarSurfaceTrack.ParseFromArray(list.last().data(), list.last().size())) return;
        parseRadarList(objRadarSurfaceTrack, radarPointList);
        qDebug()<<"radar point send time:"<<QDateTime::currentDateTime();
        emit sendMsg(mRadarCommonSettings.m_sSiteID, radarPointList);
    } else
    {
        com::zhichenhaixin::proto::RouteNodes probuf_nodes;
        if(!probuf_nodes.ParseFromArray(list.last().data(), list.last().size())) return;
        QList<ZCHX::Data::ITF_RadarRouteNode> resList;
        for(int i=0; i<probuf_nodes.nodes_size(); i++)
        {
            com::zhichenhaixin::proto::RouteNode probuf_node = probuf_nodes.nodes(i);
            ZCHX::Data::ITF_RadarRouteNode itf_node;
            transferNodeRect(itf_node.mTopNode, probuf_node.topnode());
            for(int k=0; k<probuf_node.pathlist_size(); k++)
            {
                ZCHX::Data::ITF_RadarRectDefList resPath;
                com::zhichenhaixin::proto::RoutePath probuf_path = probuf_node.pathlist(k);
                for(int m=0; m<probuf_path.path_size();m++)
                {
                    ZCHX::Data::ITF_RadarRectDef resRectDef;
                    transferNodeRect(resRectDef, probuf_path.path(m));
                    resPath.append(resRectDef);
                }
                itf_node.mPathList.append(resPath);
            }
            resList.append(itf_node);
        }
        qDebug()<<"radar route path size:"<<resList.size();

        emit sendMsg(resList);
    }
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
