#include "fusedatautil.h"
#include "util.h"
#include "profiles.h"
#include "math.h"
#include "QDebug"

#define SUM_WEIGHT 1
#define CUSTOM_TEMPTIME 30000000000l
#define MAX_DISTANCE 1000000
#define SAVE_DATA_TIME 600000
#define CLEAN_TIMER 60000

FuseDataUtil::FuseDataUtil(QObject *parent) : QObject(parent)
{
    getParameter();

    connect(&m_cleanTimer, SIGNAL(timeout()), this, SLOT(cleanData()));
    m_cleanTimer.start(CLEAN_TIMER);
}

FuseDataUtil::~FuseDataUtil()
{

}

FuseDataUtil *FuseDataUtil::getInstance()
{
    static FuseDataUtil instance;
    return &instance;
}

void FuseDataUtil::getParameter()
{
    m_distance    = Utils::Profiles::instance()->value("Fuse","distance").toDouble();
    m_cog         = Utils::Profiles::instance()->value("Fuse","cog").toDouble();
    m_sog         = Utils::Profiles::instance()->value("Fuse","sog").toDouble();
    m_belongLimit = Utils::Profiles::instance()->value("Fuse","belongLimit").toDouble();
    m_varainceDis = Utils::Profiles::instance()->value("Fuse","varaince_dis").toDouble();
    m_varainceCog = Utils::Profiles::instance()->value("Fuse","varaince_cog").toDouble();
    m_varainceSog = Utils::Profiles::instance()->value("Fuse","varaince_sog").toDouble();
    m_aisWight    = Utils::Profiles::instance()->value("Fuse","aisWight").toDouble();
    m_timeSpan    = Utils::Profiles::instance()->value("Fuse","time").toInt();
    m_fuseTimeSpan= Utils::Profiles::instance()->value("Fuse","fuseTimeSpan").toInt();
}

void FuseDataUtil::slotReceiveAisData(ITF_AISList objAisList)
{
//    qDebug() << "11111111111111111111111"
//             << AISDataEhcache.size() << RadarDataEhcache.size();
    QMutexLocker locker(&m_mutex);

    QHash<QString, ITF_AIS*> objAisMap;
    int size = objAisList.ais_size();
    for (int i = 0; i < size; i++)
    {
        ITF_AIS *obj = objAisList.mutable_ais(i);
//        qDebug() << "22222222222222222"
//                 << obj->vesseltrack().id().c_str()
//                 << "/"
//                 << obj->vesseltrack().utc();

        if (obj->vesseltrack().id().size() <= 0)
        {
            continue;
        }

        AisSimpleData* hisItem = new AisSimpleData();
        hisItem->id = obj->vesseltrack().id().c_str();
        hisItem->utc = obj->vesseltrack().utc();
        hisItem->type = 0;
        hisItem->sog = obj->vesseltrack().sog();
        hisItem->cog = obj->vesseltrack().cog();
        hisItem->lon = obj->vesseltrack().lon();
        hisItem->lat = obj->vesseltrack().lat();

        const com::zhichenhaixin::proto::VesselTrack & oldTrack = obj->vesseltrack();
        com::zhichenhaixin::proto::VesselTrack vesselTrack;
        vesselTrack.set_id(oldTrack.id());
        vesselTrack.set_mmsi(oldTrack.mmsi());
        vesselTrack.set_shiptype(oldTrack.shiptype());
        vesselTrack.set_navstatus(oldTrack.navstatus());
        vesselTrack.set_rot(oldTrack.rot());
        vesselTrack.set_sog(oldTrack.sog());
        vesselTrack.set_lon(oldTrack.lon());
        vesselTrack.set_lat(oldTrack.lat());
        vesselTrack.set_cog(oldTrack.cog());
        vesselTrack.set_heading(oldTrack.heading());
        vesselTrack.set_utc(oldTrack.utc());
        vesselTrack.set_type(TYPE_AIS);

        obj->mutable_vesseltrack()->CopyFrom(vesselTrack);

        objAisMap.insert(hisItem->id, obj);

        AisSimpleData* ais = NULL;
        if (AISDataEhcache.contains(hisItem->id))
        {
            ais = AISDataEhcache[hisItem->id];
        }
        else
        {
            ais = new AisSimpleData();
            ais->id = hisItem->id;
            AISDataEhcache.insert(ais->id, ais);
        }
        ais->utc  = hisItem->utc;
        ais->type = hisItem->type;
        ais->sog  = hisItem->sog;
        ais->cog  = hisItem->cog;
        ais->lon  = hisItem->lon;
        ais->lat  = hisItem->lat;
        ais->historyData.append(hisItem);
    }
//    qDebug() << "33333333333333333" << AISDataEhcache.size();

    // 融合雷达目标
    QList<AisSimpleData*> dataList = fuseData();
    for (int i = 0; i < dataList.size(); i++)
    {
        AisSimpleData* data = dataList.at(i);
        // 设置类型，融合雷达航迹号
        if (objAisMap.contains(data->id) && m_aisRalationMap.contains(data->id))
        {
            qDebug() << "444444444444444444444" << data->id << m_aisRalationMap.value(data->id);
            ITF_AIS * newData = objAisMap[data->id];
            const com::zhichenhaixin::proto::VesselTrack & oldTrack = newData->vesseltrack();
            com::zhichenhaixin::proto::VesselTrack vesselTrack;
            vesselTrack.set_id(oldTrack.id());
            vesselTrack.set_mmsi(oldTrack.mmsi());
            vesselTrack.set_shiptype(oldTrack.shiptype());
            vesselTrack.set_navstatus(oldTrack.navstatus());
            vesselTrack.set_rot(oldTrack.rot());
            vesselTrack.set_sog(oldTrack.sog());
            vesselTrack.set_lon(oldTrack.lon());
            vesselTrack.set_lat(oldTrack.lat());
            vesselTrack.set_cog(oldTrack.cog());
            vesselTrack.set_heading(oldTrack.heading());
            vesselTrack.set_utc(oldTrack.utc());
            vesselTrack.set_type(TYPE_MERGE);
            vesselTrack.set_tracknumber(m_aisRalationMap.value(data->id));
            newData->mutable_vesseltrack()->CopyFrom(vesselTrack);

            // test code:
//            for (int i = 0; i < size; i++)
//            {
//                const ITF_AIS & obj = objAisList.ais(i);
//                if (strcmp(obj.vesseltrack().id().c_str(),
//                           data->id.toUtf8().data()) == 0)
//                {
//                    qDebug() << "55555555555555555555555"
//                             << obj.vesseltrack().id().c_str()
//                             << obj.vesseltrack().type()
//                             << obj.vesseltrack().tracknumber();
//                    break;
//                }
//            }
        }
    }

    emit sendAisData(objAisList);
}

void FuseDataUtil::slotSendComTracks(const zchxTrackPointMap& srcMap)
{
//    qDebug() << "aaaaaaaaaaaaaaaaaaaaaaaa"
//             << AISDataEhcache.size() << RadarDataEhcache.size();
    QMutexLocker locker(&m_mutex);
    zchxTrackPointMap radarMap = srcMap;
    zchxTrackPointMap::iterator it;
    for (it = radarMap.begin(); it != radarMap.end(); ++it)
    {
        zchxTrackPoint &obj = radarMap[it.key()];
        RadarSimpleData* hisItem = new RadarSimpleData();
        hisItem->trackNumber = obj.tracknumber();
        hisItem->utc = QDateTime::currentMSecsSinceEpoch();
        hisItem->wgs84PosLat = obj.current().center().latitude();
        hisItem->wgs84PosLong = obj.current().center().longitude();
        hisItem->cog = obj.current().cog();
        hisItem->sog = obj.current().sogknot();

//        obj.set_type(TYPE_AIS);

        RadarSimpleData* radar = NULL;
        if (RadarDataEhcache.contains(hisItem->trackNumber))
        {
            radar = RadarDataEhcache[hisItem->trackNumber];
        }
        else
        {
            radar = new RadarSimpleData();
            radar->trackNumber = hisItem->trackNumber;
            RadarDataEhcache.insert(radar->trackNumber, radar);
        }
        radar->utc          = hisItem->utc;
        radar->wgs84PosLat  = hisItem->wgs84PosLat;
        radar->wgs84PosLong = hisItem->wgs84PosLong;
        radar->cog          = hisItem->cog;
        radar->sog          = hisItem->sog;
        radar->historyData.append(hisItem);
    }
//    qDebug() << "ccccccccccccccccccc" << RadarDataEhcache.size();

    // 融合AIS目标
    QList<AisSimpleData*> dataList = fuseData();
    for (int i = 0; i < dataList.size(); i++)
    {
        AisSimpleData* data = dataList.at(i);
        // 设置类型，融合AIS编号
        if (m_aisRalationMap.contains(data->id))
        {
            int trackNumber = m_aisRalationMap.value(data->id);
            if (radarMap.contains(trackNumber) && m_radarRalationMap.contains(trackNumber))
            {
                qDebug() << "dddddddddddddddddddd" << data->id << trackNumber;
                zchxTrackPoint & newData = radarMap[trackNumber];
//                newData.set_type(TYPE_MERGE);
//                newData.set_aisid(data->id.toUtf8().data());
//                qDebug() << "eeeeeeeeeeeeeeeeeeee"
//                         << radarMap.value(trackNumber).type()
//                         << radarMap.value(trackNumber).aisid().c_str();
            }
        }
    }

    emit sendComTracks(radarMap);
}

// 定时清除过期数据
void FuseDataUtil::cleanData()
{
    qDebug() << "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnn";
    QMutexLocker locker(&m_mutex);

    int curUtc = QDateTime::currentMSecsSinceEpoch();

    // AIS
    QHash<QString, AisSimpleData*>::iterator aisIt;
    for (aisIt = AISDataEhcache.begin(); aisIt != AISDataEhcache.end();)
    {
        AisSimpleData* data = aisIt.value();
        while(data->historyData.size() > 0 &&
              curUtc - data->historyData[0]->utc > SAVE_DATA_TIME)
        {
            AisSimpleData* his = data->historyData.front();
            data->historyData.removeFirst();
            delete his;
        }

        if (data->historyData.size() == 0)
        {
            qDebug() << "dddddddddddddddddddddd 1111111111111111";
            AISDataEhcache.erase(aisIt++);
            delete data;
        }
        else
        {
            ++aisIt;
        }
    }

    // Radar
    QHash<int, RadarSimpleData*>::iterator radarIt;
    for (radarIt = RadarDataEhcache.begin(); radarIt != RadarDataEhcache.end(); )
    {
        RadarSimpleData* data = radarIt.value();
        while(data->historyData.size() > 0 &&
              curUtc - data->historyData[0]->utc > SAVE_DATA_TIME)
        {
            RadarSimpleData* his = data->historyData.front();
            data->historyData.removeFirst();
            delete his;
        }

        if (data->historyData.size() == 0)
        {
            qDebug() << "dddddddddddddddddddddd 2222222222222";
            RadarDataEhcache.erase(radarIt++);
            delete data;
        }
        else
        {
            ++radarIt;
        }
    }
}

/**
 * 雷达和ais数据融合
 * @return
 */
QList<AisSimpleData*> FuseDataUtil::fuseData()
{
    m_aisRalationMap.clear();
    m_radarRalationMap.clear();
    const QList<AisSimpleData*> & aisList = AISDataEhcache.values();
    const QList<RadarSimpleData*> & radarList = RadarDataEhcache.values();

    QList<AisSimpleData*> points;
    if (radarList.size() > 0 && aisList.size() > 0)
    {
        QList<AisSimpleData*> aisList2;
        QList<RadarSimpleData*> radarList2;
        int aislen = aisList.size();
        for (int i = 0; i < aislen; i++)
        {
            AisSimpleData* ais = aisList.at(i);
            int radarlen = radarList.size();
            for (int j = 0; j < radarlen; j++)
            {
                RadarSimpleData* radar = radarList.at(j);
                double shipDistance = Util::getDistanceDeg(radar->wgs84PosLong, radar->wgs84PosLat, ais->lon, ais->lat);

                // 粗关联门限判断
                if (shipDistance < m_distance /* && (gapSog < sog) && (gapCog < cog) */) {
                    radarList2.append(radar);
                    aisList2.append(ais);
                    break;
                }
            }
        }

        handleCalculate(aisList2, radarList2, points);
    }

    return points;
}

/**
 * @title: handleCalculate
 * @category: 处理满足粗关联的数据
 * @param radarList
 * @param aisList
 * @param points
 */
void FuseDataUtil::handleCalculate(const QList<AisSimpleData*> & aisList,
                                   const QList<RadarSimpleData*> & radarList,
                                   QList<AisSimpleData*> & points)
{
    float belong = 0;
    // 满足粗关联的个数
    int len = aisList.size();
    for (int i = 0; i < len; i++)
    {
        AisSimpleData * ais = aisList.at(i);
        if (m_aisRalationMap.contains(ais->id))
        {
            continue;
        }
        RadarSimpleData * radar = sortDistance(ais->lon, ais->lat, radarList);
        if (m_radarRalationMap.contains(radar->trackNumber))
        {
            continue;
        }
        belong = calculateFuseDgr(radar->trackNumber, ais->id);

        // 列表中所有轨迹点的平均隶属度大于隶属度门限，则判定为同一条船
        if (belong >= m_belongLimit)
        {
            // 将雷达和AIS融合的对应编号保存在list中
            AisSimpleData * fuseVo = covertFuseData(ais, radar);
            if (fuseVo == NULL)
            {
                continue;
            }
            points.append(fuseVo);
        }
    }
}

/**
 * 计算雷达和AIS航迹的隶属度
 * @param radarId
 * @param aisId
 */
float FuseDataUtil::calculateFuseDgr(int radarId, QString aisId)
{
    float avgBelong = 0.5f;
    RadarSimpleData* radarLocus = RadarDataEhcache.value(radarId);
    AisSimpleData* aisLocus = AISDataEhcache.value(aisId);

    // 如果AIS和雷达的轨迹点都不为空
    if (radarLocus != NULL && aisLocus != NULL)
    {
        // 取同一时间段内的雷达和AIS数据
        QList<AisSimpleData*> aisList;
        QList<RadarSimpleData*> radarList;
        syncRadarAndAisData(radarLocus->historyData,
                            aisLocus->historyData,
                            aisList, radarList);

        // 如果同一时间段AIS的点少于2个，则放弃匹配
        if (aisList.size() > 0 && radarList.size() > 0)
        {
            QList<AisSimpleData*> aisInsertList; // 插值算法处理后的AIS航迹列表和雷达航迹列表数据数量相同，且在list中的位置一一对应。
            QList<RadarSimpleData*> radarInsertList;
            insertData(aisList, radarList, aisInsertList, radarInsertList);// 差值算法对雷达和AIS数据进行处理

            float belongSum = 0;
            int aisSize = aisInsertList.size();
            int radarSize = radarInsertList.size();
            int listSize = aisSize < radarSize ? aisSize : radarSize;
            for (int i = 0; i < listSize; i++)
            {
                float belong = cutSingleBln(aisInsertList.at(i), radarInsertList.at(i));
                belongSum += belong;
            }
            avgBelong = belongSum / aisInsertList.size();
        }
    }
    return avgBelong;
}

/**
 * 取同一时间段内的雷达和AIS数据
 * @param radarLocusList
 * @param aisLocusList
 * @param radarId
 * @param aisId
 * @return
 */
void FuseDataUtil::syncRadarAndAisData(const QList<RadarSimpleData*> & radarTrackList,
                                       const QList<AisSimpleData*> & aisTrackList,
                                       QList<AisSimpleData*> &aisList,
                                       QList<RadarSimpleData*> &radarList)
{
    long start = QDateTime::currentMSecsSinceEpoch() - m_fuseTimeSpan; // 结束时间,时间间隔自己定
    aisList.clear();
    radarList.clear();

    int len = radarTrackList.size();
    for (int i = 0; i < len; i++)
    {
        RadarSimpleData* radarTrack = radarTrackList.at(i);
        if (radarTrack->utc < start)
        {
            continue;
        }

        radarList.append(radarTrack);
    }

    len = aisTrackList.size();
    for (int i = 0; i < len; i++)
    {
        AisSimpleData* aisTrack = aisTrackList.at(i);
        if (aisTrack->utc < start)
        {
            continue;
        }

        aisList.append(aisTrack);
    }
}

AisSimpleData* FuseDataUtil::covertFuseData(AisSimpleData * ais, RadarSimpleData * radar)
{
    if (!AISDataEhcache.contains(ais->id))
    {
        return NULL;
    }
    AisSimpleData * aisVessel = AISDataEhcache[ais->id];
    aisVessel->lat = ais->lat * m_aisWight + radar->wgs84PosLat * (SUM_WEIGHT - m_aisWight);
    aisVessel->lon = ais->lon * m_aisWight + radar->wgs84PosLong * (SUM_WEIGHT - m_aisWight);
    aisVessel->sog = (float)(ais->sog * m_aisWight + radar->sog * (SUM_WEIGHT - m_aisWight));
    aisVessel->cog = (float)(ais->cog * m_aisWight + radar->cog * (SUM_WEIGHT - m_aisWight));
    aisVessel->utc = ais->utc;
    aisVessel->type = ((short)3);// 设置为融合

    // 添加融合数据的id便于判断雷达和AIS的原数据是否显示
    m_aisRalationMap.insert(ais->id, radar->trackNumber);
    m_radarRalationMap.insert(radar->trackNumber, ais->id);
    return aisVessel;
}

/**
 * 计算单点隶属度
 * @param radarCalcul
 * @param aisCalcul
 * @param varianceMap
 * @return
 */
float FuseDataUtil::cutSingleBln(AisSimpleData* aisCalcul, RadarSimpleData* radarCalcul)
{
    // 模糊评判矩阵
    float judgeMatrix[3][2];

    // 单模糊因素集
    float dataDistance = (float)Util::getDistanceDeg(radarCalcul->wgs84PosLat, radarCalcul->wgs84PosLong, aisCalcul->lat, aisCalcul->lon);
    float dataCog = (float)fabs(radarCalcul->cog - aisCalcul->cog);
    float dataSog = (float)fabs(radarCalcul->sog - aisCalcul->sog);
    float distanceBln = (float)exp(-t1 * (pow(dataDistance, 2) / m_varainceDis)); // 距离关联隶属度，隶属度计算
    float cogBln = (float)exp(-t2 * (pow(dataCog, 2) / m_varainceCog)); // 速度关联隶属度
    float sogBln = (float)exp(-t3 * (pow(dataSog, 2) / m_varainceSog)); // 角度关联隶属度

    judgeMatrix[0][0] = distanceBln;
    judgeMatrix[0][1] = 1 - distanceBln;
    judgeMatrix[1][0] = cogBln;
    judgeMatrix[1][1] = 1 - cogBln;
    judgeMatrix[2][0] = sogBln;
    judgeMatrix[2][1] = 1 - sogBln;
    // 加权平均
    // 综合关联隶属度
    float belong = m_varainceDis * judgeMatrix[0][0]
            + m_varainceCog * judgeMatrix[1][0]
            + m_varainceSog * judgeMatrix[2][0];
    return belong;
}

/**
 * 内插AIS数据
 * @param aisList
 * @param radarList
 * @return
 */
void FuseDataUtil::insertData(const QList<AisSimpleData*> & aisList,
                              const QList<RadarSimpleData*> & radarList,
                              QList<AisSimpleData*> & aisInsertList,
                              QList<RadarSimpleData*> & radarDAis)
{
    aisInsertList.clear();
    radarDAis.clear();

    // 根据ais点的个数，获取radar最近的点，
    long long tempTime = CUSTOM_TEMPTIME;

    // 找到每个AIS对应雷达时间最接近的点
    for (AisSimpleData* aisData : aisList)
    {
        long long timesize = 0;
        RadarSimpleData* tempRadar;
        long long aisUtc = aisData->utc;
        for (RadarSimpleData* radar : radarList)
        {
            timesize = fabs(aisUtc - radar->utc);
            if (tempTime > timesize)
            {
                tempTime = timesize;
                tempRadar = radar;
            }
        }
        tempTime = CUSTOM_TEMPTIME;
        radarDAis.append(tempRadar);
    }

    // 方式二 根据AIS的基准点 将雷达最近的点推算到和AIS的时刻相匹配
    int len = aisList.size();
    for (int i = 0; i < len; i++)
    {
        AisSimpleData* ais = aisList.at(i);
        RadarSimpleData* radar = radarDAis.at(i);
        long long utc_space = (radar->utc - ais->utc) / 1000;// 时间间隔 转化成秒
        double dst = m_sog * utc_space;// 速度 * 时间 计算距离
        if (utc_space > 0 && dst > 0)
        {
            double* lons = Util::computerThatLonLat(ais->lon, ais->lat, dst, ais->cog);
            ais->lat = (lons[0]);
            ais->lon = (lons[1]);
            ais->utc = (ais->utc + utc_space * 1000);
            aisInsertList.append(ais);
            delete[] lons;
        }
    }
}

 /**
  * @title: sortDistance
  * @category: 计算距离 并进行排序
  * @param ln
  * @param lt
  * @param list
  * @return
  */
RadarSimpleData* FuseDataUtil::sortDistance(double lon, double lat, const QList<RadarSimpleData*> & list)
{
    RadarSimpleData *result = NULL;
    double minDistance = MAX_DISTANCE;
     for (int i = 0; i < list.size(); i++)
     {
         RadarSimpleData *entity = list.at(i);
         double lon2 = entity->wgs84PosLong;
         double lat2 = entity->wgs84PosLat;
         double instance = Util::getDistanceDeg(lon, lat, lon2, lat2);
         if (minDistance > instance)
         {
             minDistance = instance;
             result = entity;
         }
     }

     return result;
}
