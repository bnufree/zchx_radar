#ifndef FUSEDATAUTIL_H
#define FUSEDATAUTIL_H

#include <QObject>
#include "ais/zchxaisdataprocessor.h"
#include "ais_radar/zchxradardataserver.h"

class FuseDataUtil : public QObject
{
    Q_OBJECT
private:
    // 类型 0：ais，1：北斗，2：CMDA 3:融合  4:融合 船讯网数据(船舶档案)
    enum TYPE
    {
        TYPE_AIS = 0,
        TYPE_BEIDOU,
        TYPE_CMDA,
        TYPE_MERGE,
        TYPE_SHIPXY,
    };

    explicit FuseDataUtil(QObject *parent = 0);
    ~FuseDataUtil();

public:
    static FuseDataUtil* getInstance();
    QList<AisSimpleData*> fuseData();

signals:
    void sendAisData(const ITF_AISList & objAisList);
    void sendComTracks(const zchxTrackPointMap& radarMap);

public slots:
    void slotReceiveAisData(ITF_AISList objAisList);
    void slotSendComTracks(const zchxTrackPointMap& radarMap);

private slots:
    void cleanData();

private:
    void getParameter();
    void handleCalculate(const QList<AisSimpleData*> & aisList,
                         const QList<RadarSimpleData*> & radarList,
                         QList<AisSimpleData*> & points);
    float calculateFuseDgr(int radarId, QString aisId);
    void syncRadarAndAisData(const QList<RadarSimpleData*> & radarTrackList,
                           const QList<AisSimpleData*> & aisTrackList,
                           QList<AisSimpleData*> &aisList,
                           QList<RadarSimpleData*> &radarList);
    void insertData(const QList<AisSimpleData*> & aisList,
                  const QList<RadarSimpleData*> & radarList,
                  QList<AisSimpleData*> & aisInsertList,
                  QList<RadarSimpleData*> & radarDAis);
    AisSimpleData* covertFuseData(AisSimpleData * ais, RadarSimpleData * radar);
    RadarSimpleData* sortDistance(double lon, double lat,
                                const QList<RadarSimpleData*> & list);
    float cutSingleBln(AisSimpleData* aisCalcul,
                       RadarSimpleData* radarCalcul);

    QMutex m_mutex;

    QHash<QString, AisSimpleData*> AISDataEhcache;
    QHash<int, RadarSimpleData*> RadarDataEhcache;

    QTimer m_cleanTimer;

    double m_distance;
    double m_cog;
    double m_sog;
    double m_belongLimit;
    double m_varainceDis;
    double m_varainceCog;
    double m_varainceSog;
    double m_aisWight;
    long   m_timeSpan;
    long   m_fuseTimeSpan;

    // 正态分布函数的参数选择
    float t1 = 0.001f;
    float t2 = 0.1f;
    float t3 = 0.1f;

    QMap<QString, int> m_aisRalationMap;
    QMap<int, QString> m_radarRalationMap;
};

#endif // FUSEDATAUTIL_H
