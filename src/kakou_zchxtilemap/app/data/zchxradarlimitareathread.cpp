#include "zchxradarlimitareathread.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonParseError>
#include "zchxmsgcommon.h"

using namespace ZCHX_RADAR_RECEIVER;
typedef QList<ZCHX::Data::LatLon>   zchxLatlonList;

zchxRadarLimitAreaThread::zchxRadarLimitAreaThread(const ZCHX_Radar_Setting_Param& param, QObject *parent)
    : ZCHXReceiverThread(ZCHX_RECV_LIMIT_AREA, param, parent)
{
//    setIsOver(false);
    qRegisterMetaType<QList<ZCHX::Data::ITF_IslandLine>>("const QList<ZCHX::Data::ITF_IslandLine>&");
}

void parseLatlonJsonArray(const QJsonArray& array, zchxLatlonList& list )
{
    list.clear();
    for(int j = 0; j < array.size(); ++j)
    {
        QJsonArray cellAraay = array.at(j).toArray();
        double dLon = cellAraay.at(0).toDouble();
        double dLat = cellAraay.at(1).toDouble();
        list.append(ZCHX::Data::LatLon(dLat, dLon));
    }
}

void zchxRadarLimitAreaThread::parseJsonFromByteArray(const QByteArray &data, QList<ZCHX::Data::ITF_IslandLine> &result)
{
    result.clear();
    QJsonParseError err;
    QJsonDocument docRcv = QJsonDocument::fromJson(data, &err);
    if(err.error != QJsonParseError::NoError)
    {
        qDebug()<<"parse completetion list error:"<<err.error;
        return ;
    }
    if(!docRcv.isArray())
    {
        qDebug()<<" limit file  with wrong json format.";
        return ;
    }
    QJsonArray array = docRcv.array();
    foreach (QJsonValue val, array) {
        zchxMsg::filterArea filter(val.toObject());
        qDebug()<<filter.id<<filter.name<<filter.type<<filter.site;
        ZCHX::Data::ITF_IslandLine line;
        line.name = filter.name;
        if(filter.type == 1)
        {
            line.warnColor = "#00ff00";
        } else
        {
            line.warnColor = "#ff0000";
        }
        zchxLatlonList list;
        foreach (zchxMsg::Latlon ll, filter.area) {
            list.append(ZCHX::Data::LatLon(ll.lat, ll.lon));
        }
        foreach (ZCHX::Data::LatLon ll, list) {
            line.path.push_back(std::pair<double, double>(ll.lat, ll.lon));
        }
        line.id = filter.id;


        result.append(line);
    }
}

void zchxRadarLimitAreaThread::parseRecvData(const QByteArrayList& list)
{
    if(list.size() == 0) return;
//    qDebug()<<list.first()<<QDateTime::currentDateTime();
    QList<ZCHX::Data::ITF_IslandLine> result;
    parseJsonFromByteArray(list.last(), result);
    if(result.size() > 0)
    {
//        qDebug()<<result.first().path[0].first<<result.first().path[0].second;
        emit sendMsg(result);
    }


}




