#ifndef ZXHCPROCESSECHODATA_H
#define ZXHCPROCESSECHODATA_H

#include <QObject>
#include <QThread>
#include "zmqradarechothread.h"
#include "zchxfunction.h"
#include "zmq.h"
class ZXHCProcessEchoData : public QObject
{
    Q_OBJECT
public:
    explicit ZXHCProcessEchoData(QObject *parent = 0);

signals:
    void signalProcess(const Map_RadarVideo &radarVideoMap);
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
public slots:
    void slotProcess(const Map_RadarVideo &radarVideoMap);
private:
    void sendRadarTrack();//发送雷达目标

    void clearRadarTrack();//定期清理雷达目标
private:
    QThread m_workThread;

    void *m_pTrackContext;
    void *m_pTrackLisher;
    int  m_uTrackSendPort;
    QString m_sTrackTopic;

    int  m_uCellNum;//一条线上多少个点
    int  m_uLineNum;//一圈多少条线
    int  m_uHeading;//雷达方位
    double m_dCentreLon;
    double m_dCentreLat;
    int m_clearRadarTrackTime;//单位是分钟

    QMap<int,com::zhichenhaixin::proto::TrackPoint> m_radarPointMap;//key是航迹号


};

#endif // ZXHCPROCESSECHODATA_H
