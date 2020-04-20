#ifndef ZCHXRADARVIDEODATACHANGE_H
#define ZCHXRADARVIDEODATACHANGE_H
#include <QObject>
#include "zchxradarutils.h"


namespace ZCHX_RADAR_RECEIVER {


class ZCHXRadarDataChange : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXRadarDataChange(QObject *parent = 0);
    ~ZCHXRadarDataChange();
    void stop();
    void appendRadarRect(const ZCHX_RadarRect_Param& param);
    void appendRadarRectList(const QList<ZCHX_RadarRect_Param>& list);
    void appendRadarPoint(const ZCHX_Radar_Setting_Param& param);
    void appendRadarPointList(const QList<ZCHX_Radar_Setting_Param>& list);
    void appendRadarVideo(const ZCHX_Radar_Setting_Param &param);
    void appendAis(const ZCHX_Radar_Setting_Param& param);
    void appendLimit(const ZCHX_Radar_Setting_Param &param);
    void appendAisChart(const ZCHX_Radar_Setting_Param& param);
public slots:
    void slotSetThreadStatus(int type, bool isOn);
signals:
    void signalRecvDataNow(int type,  int length);
    void sendConnectionStatus(bool sts, const QString& msg);
    void sendRadarRect(int id, const QList<ZCHX::Data::ITF_RadarRect>&);
    void sendRadarPoint(int id, const QList<ZCHX::Data::ITF_RadarPoint>&);
    void sendAisDataList(const QList<ZCHX::Data::ITF_AIS>&);
    void sendAisChart(const ZCHX::Data::ITF_AIS_Chart& data);
    void sendLimitDataList(const QList<ZCHX::Data::ITF_IslandLine>&);
    void sendRadarVideo(int siteID, double lon, double lat, double dis, int type, int loop, int curIndex, const QByteArray& objPixmap, const QByteArray& prePixMap);


private:
    QList<ZCHXReceiverThread*> mThreadList;
    QList<ZCHX_RadarRect_Param>        mRadarRectList;
    QList<ZCHX_Radar_Setting_Param>        mRadarPointList;
    QList<ZCHX_Radar_Setting_Param>        mRadarVideoList;
    QList<ZCHX_Radar_Setting_Param>        mAisList;
    QList<ZCHX_Radar_Setting_Param>        mLimitList;
    QList<ZCHX_Radar_Setting_Param>        mAisChartList;
};
}

#endif // ZCHXRADARVIDEODATACHANGE_H
