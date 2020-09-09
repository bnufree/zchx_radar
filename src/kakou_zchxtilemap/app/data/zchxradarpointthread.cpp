#include <QDebug>
#include <QSettings>
#include "zchxradarpointthread.h"

using namespace ZCHX_RADAR_RECEIVER;

ZCHXRadarPointThread::ZCHXRadarPointThread(const ZCHX_Radar_Setting_Param& param, QObject *parent)
    : ZCHXReceiverThread(ZCHX_RECV_RADAR_POINT, param, parent)
{
    qRegisterMetaType<PROTOBUF_TrackPoint>("PROTOBUF_TrackPoint");
    qRegisterMetaType<PROTOBUF_RadarSurfaceTrack>("PROTOBUF_RadarSurfaceTrack");
    qRegisterMetaType<QList<ZCHX::Data::ITF_RadarPoint>>("const QList<ZCHX::Data::ITF_RadarPoint>&");
    qRegisterMetaType<QList<ZCHX::Data::ITF_RadarRouteNode>>("const QList<ZCHX::Data::ITF_RadarRouteNode>&");
    qRegisterMetaType<ZCHX::Data::ITF_RadarRectList>("const ZCHX::Data::ITF_RadarRectList&");
}

void ZCHXRadarPointThread::transferNodeRect(ZCHX::Data::ITF_RadarRectDef &out, const PROTOBUF_Rectdef &in)
{
    out.rectNumber = in.rectnumber();
    out.center.lat = in.center().latitude();
    out.center.lon = in.center().longitude();
    out.updateTime = in.updatetime();
    out.isRealData = in.realdata();
    out.cog = in.cog();
    out.sogKnot = in.sogknot();
    out.sogMps = in.sogms();
    for(int i=0; i<in.outline_size(); i++)
    {
        out.outline.append(ZCHX::Data::LatLon(in.outline(i).latitude(), in.outline(i).longitude()));
    }
    out.boundRect.bottomRight.lat = in.boundrect().bottomright().latitude();
    out.boundRect.bottomRight.lon = in.boundrect().bottomright().longitude();
    out.boundRect.topLeft.lat = in.boundrect().topleft().latitude();
    out.boundRect.topLeft.lon = in.boundrect().topleft().longitude();
    out.boundRect.diameter = in.boundrect().diameter();
    out.maxSeg.start.lat = in.seg().start().latitude();
    out.maxSeg.start.lon = in.seg().start().longitude();
    out.maxSeg.end.lat = in.seg().end().latitude();
    out.maxSeg.end.lon = in.seg().end().longitude();
    out.referWidth = in.fixedimg().width();
    out.referHeight = in.fixedimg().height();
    for(int i=0; i<in.fixedimg().points_size(); i++)
    {
        out.pixPoints.append(QPoint(in.fixedimg().points(i).x(), in.fixedimg().points(i).y()));
    }

    //添加预推区域
    if(in.has_prediction())
    {
        com::zhichenhaixin::proto::PredictionArea area(in.prediction());
        for(int m =0; m<area.area_size(); m++)
        {
            ZCHX::Data::LatLon block;
            block.lat = area.area(m).latitude();
            block.lon = area.area(m).longitude();
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
        PROTOBUF_RadarSurfaceTrack objRadarSurfaceTrack;
        QList<ZCHX::Data::ITF_RadarPoint> radarPointList;
        if(!objRadarSurfaceTrack.ParseFromArray(list.last().data(), list.last().size())) return;
        parseRadarList(objRadarSurfaceTrack, radarPointList);
//        qDebug()<<"radar point send time:"<<QDateTime::currentDateTime();
        emit sendMsg(mRadarCommonSettings.m_sSiteID, radarPointList);
    } else
    {
        com::zhichenhaixin::proto::RouteNodes probuf_nodes;
        if(!probuf_nodes.ParseFromArray(list.last().data(), list.last().size())) return;
        QList<ZCHX::Data::ITF_RadarRouteNode> resList;
        for(int i=0; i<probuf_nodes.node_list_size(); i++)
        {
            com::zhichenhaixin::proto::RouteNode probuf_node = probuf_nodes.node_list(i);
            ZCHX::Data::ITF_RadarRouteNode itf_node;
            itf_node.mNum = probuf_node.node_num();
            transferNodeRect(itf_node.mTop, probuf_node.top_node());
            for(int k=0; k<probuf_node.path_list_size(); k++)
            {
                ZCHX::Data::ITF_RadarRectDefList resPath;
                com::zhichenhaixin::proto::RoutePath probuf_path = probuf_node.path_list(k);
                for(int m=0; m<probuf_path.path_size();m++)
                {
                    ZCHX::Data::ITF_RadarRectDef resRectDef;
                    transferNodeRect(resRectDef, probuf_path.path(m));
                    resPath.append(resRectDef);
                }
                itf_node.mPaths.append(resPath);
            }
            resList.append(itf_node);
        }
        qDebug()<<"radar route path size:"<<resList.size();

        emit sendMsg(resList);
    }
}



void ZCHXRadarPointThread::parseRadarList(const PROTOBUF_RadarSurfaceTrack &objRadarSurfaceTrack,
                                       QList<ZCHX::Data::ITF_RadarPoint>& radarPointList)
{
    int size = objRadarSurfaceTrack.trackpoints_size();

    for (int i = 0; i < size; i++)
    {
        const PROTOBUF_TrackPoint &point = objRadarSurfaceTrack.trackpoints(i);
        ZCHX::Data::ITF_RadarPoint item;
        item.warnStatus = 0;
        item.radarSiteID                = QString::fromStdString(point.radarsiteid());
        item.trackNumber                = point.tracknumber();
        transferNodeRect(item.currentRect, point.current());
        for(int k=0; k<point.tracks_size(); k++)
        {
            ZCHX::Data::ITF_RadarRectDef temp;
            transferNodeRect(temp, point.tracks(k));
            item.historyRects.append(temp);
        }
        item.directionConfirmed = point.trackconfirmed();
        if(!point.has_objtype())
        {
            item.objType = ZCHX::Data::RadarPointNormal;
        } else
        {
            item.objType = point.objtype();
        }
        if(point.has_objname()) item.objName = QString::fromStdString(point.objname());
        item.diameter = item.currentRect.boundRect.diameter;

        radarPointList.append(item);
    }
}
