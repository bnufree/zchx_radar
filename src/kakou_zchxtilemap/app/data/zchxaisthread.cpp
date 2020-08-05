#include <QDebug>
#include <QSettings>
#include "zchxaisthread.h"

using namespace ZCHX_RADAR_RECEIVER;

ZCHXAisThread::ZCHXAisThread(const ZCHX_Radar_Setting_Param& param, QObject *parent)
    : ZCHXReceiverThread(ZCHX_RECV_AIS, param, parent)
{
    qRegisterMetaType<QList<ZCHX::Data::ITF_AIS>>("const QList<ZCHX::Data::ITF_AIS>&");
}

void ZCHXAisThread::parseRecvData(const QByteArrayList& list)
{
    if(list.size() != 3) return;
    zchxFuntionTimer t(mRadarCommonSettings.m_sTopicList.join(","));
    ITF_AISLIST srclist;

    //结果分析
    if(!srclist.ParseFromArray(list.last().data(), list.last().size())) return;

    parseAisList(srclist);
    //检查没有更新的数据将他们删除
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QStringList keys = mDataMap.keys();
    foreach (QString key, keys) {
        if(now - mDataMap[key].UTC >= 1000 * 60* 1)
        {
            mDataMap.remove(key);
        }
    }
    emit sendMsg(mDataMap.values());

}



void ZCHXAisThread::parseAisList(const ITF_AISLIST &objRadarSurfaceTrack)
{
    int size = objRadarSurfaceTrack.ais_size();

    for (int i = 0; i < size; i++)
    {
        com::zhichenhaixin::proto::AIS point = objRadarSurfaceTrack.ais(i);
        ZCHX::Data::ITF_AIS item;
        item.type              = ZCHX::Data::ITF_AIS::Target_AIS;
        item.id                = QString::fromStdString(point.vesselinfo().id());
        item.mmsi              = point.vesselinfo().mmsi();                          // 用户识别码 长度 9
        item.shiptype          = QString::fromStdString(point.vesselinfo().shiptype());                     // 船舶种类(A类，BCS类,BSO类)	长度 3
        item.navStatus         = ZCHX::Data::NAVI_STATUS(point.vesseltrack().navstatus());                // 船舶航行状态
        item.lon               = point.vesseltrack().lon();
        item.lat                = point.vesseltrack().lat();
        item.rot                = point.vesseltrack().rot();                            // 船舶转向率 degree/min
        item.sog                = point.vesseltrack().sog();                            //对地航速
        item.cog                = point.vesseltrack().cog();                            //对地航向
        item.heading            = point.vesseltrack().heading();                       //航首向
        item.imo                = point.vesselinfo().imo();                           // IMO 号码	长度 20
        item.callSign           = QString::fromStdString(point.vesselinfo().callsign());                     // Call Sign 呼号	长度 20
        item.shipName           = QString::fromStdString(point.vesselinfo().shipname());                     // 船名	长度 30
        item.cargoType          = point.vesselinfo().cargotype();                    // 船舶类型	长度 15
        item.country            = QString::fromStdString(point.vesselinfo().country()); 	                  // 国籍	长度 3
        item.vendorID           = QString::fromStdString(point.vesselinfo().vendorid());                     // Vendor ID 制造商	长度 30
        item.shipLength         = point.vesselinfo().shiplength();                    // 船长
        item.shipWidth          = point.vesselinfo().shipwidth();
        item.eta                = QString::fromStdString(point.vesselinfo().eta());                          // 预计到达时间    (存储外推时间)
        item.draught            = point.vesselinfo().draught();
        item.markType           = 2;
        std::vector<std::pair<double, double>> path;
        path.push_back(std::pair<double, double>(item.lat, item.lon));
        item.setPath(path);
        QString key = QString("Ais_%1_%2").arg(item.mmsi).arg(item.id);
        item.id = key;
        item.UTC = QDateTime::currentMSecsSinceEpoch();
        mDataMap[key] = item;
    }
}

ZCHXAisChartThread::ZCHXAisChartThread(const ZCHX_Radar_Setting_Param& param, QObject *parent)
    : ZCHXReceiverThread(ZCHX_RECV_AIS_CHART, param, parent)
{
    qRegisterMetaType<ZCHX::Data::ITF_AIS_Chart>("const ZCHX::Data::ITF_AIS_Chart&");
}

void ZCHXAisChartThread::parseRecvData(const QByteArrayList& list)
{
    if(list.size() != 3) return;
    zchxFuntionTimer t(mRadarCommonSettings.m_sTopicList.join(","));
    com::zhichenhaixin::proto::AisChart chart;
    if(!chart.ParseFromArray(list.last().data(), list.last().size())) return;
    ZCHX::Data::ITF_AIS_Chart data;
    data.format = QString::fromUtf8(chart.format().c_str());
    data.height = chart.height();
    data.width = chart.width();
    data.id = chart.id();
    data.imageData.append( chart.imagedata().c_str());
    data.lat = chart.latitude();
    data.lon = chart.longitude();
    data.name = QString::fromUtf8(chart.name().c_str());
    data.radius = chart.radius();
    data.utc = chart.utc();
    emit sendMsg(data);
}


