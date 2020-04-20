#ifndef ZCHXRADARDATASERVER_H
#define ZCHXRADARDATASERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "asterixformat.hxx"
#include "asterixformatdescriptor.hxx"
#include "zchxdrawradarvideo.h"
#include <QPixmap>
#include "ZCHXRadar.pb.h"
#include "zmq.h"
#include "ZCHXRadarVideo.pb.h"
#include "zchxdrawvideorunnable.h"
#include <QThreadPool>
#include <QThread>
#include "ZmqMonitorThread.h"
#include "zchxfunction.h"


typedef com::zhichenhaixin::proto::RadarVideo  ITF_RadarVideo;
typedef com::zhichenhaixin::proto::TrackPoint  ITF_RadarPoint;
class ZCHXRadarDataServer : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXRadarDataServer(int uSourceID = 1,QObject *parent = 0);
    ~ZCHXRadarDataServer();
    void readRadarLimitFormat();//读取雷达限制区域文件



signals:
    void signalClientInout(const QString& ip, const QString& name, int port, int inout);
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void startProcessSignal();
    void processAndSendSignal();
public slots:
    void openRadar();
    void closeRadar();
    void processAndSendSlot();
    void startProcessSlot();
    //track
    void displayUdpTrackError(QAbstractSocket::SocketError error);
    void updateTrackUdpProgress();

    //video
    void displayUdpVideoError(QAbstractSocket::SocketError error);
    void updateVideoUdpProgress();

    void setRadarVideoPixmap(const QPixmap &objPixmap);//接收回波图片
    void setRadarAfterglowPixmap(const int uIndex, const QPixmap &videoPixmap, const QPixmap &objPixmap,
                             const QPixmap &prePixmap);//接收余辉图片

    //4g雷达 IP 236.6.7.10  port 6680
    //6g雷达 IP 236.6.7.100  port 6132
    //开关控制 IP 236.6.101.100  port 6133
    void heartProcessSlot();//心跳通信

private:
    void init();
    void analysisRadar(const QByteArray &sRadarData,const QString &sRadarType,
                       int uLineNum = 0,int uCellNum = 0,int uHeading = 0);//解析接收到的所有AIS
    void analysisCat010Radar(DataBlock *pDB);//雷达目标
    void analysisCat240Radar( DataBlock *pDB,
                             int uLineNum, int uCellNum,int uHeading);//回波
    void analysisLowranceRadar(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading);//小雷达
    void processVideoData();

    void sendRadarTrack();//发送雷达目标

    void clearRadarTrack();//定期清理雷达目标


    //解析限制区域
    void analysisLonLatTOPolygon(const QString sFileName,QList<QPolygonF> &landPolygon,QList<QPolygonF> &seaPolygon);
    bool inLimitArea(const double dCentreLat, const double dCentreLon,
                     const double dAzimuth,const int uPosition,
                     const double dStartRange, const double dRangeFactor);
    bool inLimitAreaForTrack(const double dLat, const double dLon);
private:
    int m_uSourceID;

    QUdpSocket *m_pUdpTrackSocket;
    QUdpSocket *m_pUdpVideoSocket;
    QString m_sTrackIP;
    int  m_uTrackPort;
    QString m_sVideoIP;
    int  m_uVideoPort;

    QUdpSocket *m_pHeartSocket;//心跳
    QTimer     *m_pHeartTimer;
    int m_uHeartTime;
    QString m_sHeartIP;
    int  m_uHeartPort;
    QString m_sOptRadarIP;
    int  m_uOptRadarPort;

    CAsterixFormatDescriptor *m_pCAsterixFormatDescriptor;
    CAsterixFormat           *m_pCAsterixFormat;
    QString m_sPath;

    double m_dCentreLon;
    double m_dCentreLat;

    std::vector<std::pair<double, double>> m_latLonVec;//雷达目标经纬度集合
    QMap<int,QJsonObject> m_radarTrackMap;//key是航迹号
    QMap<int,ITF_RadarPoint> m_radarPointMap;//key是航迹号
    QMap<int,RADAR_VIDEO_DATA> m_radarVideoMap;//key是azimuth

    //回波、余辉绘制
    ZCHXDrawRadarVideo *m_DrawRadarVideo;
    //ZCHXDrawVideoRunnable *m_pRunnable;
    QString m_sRadarVideoType;//cat010-新科，Lowrance-小雷达
    int m_uLoopNum;//余辉循环值
    int  m_uCellNum;//一条线上多少个点
    int  m_uLineNum;//一圈多少条线
    int  m_uHeading;//雷达方位
    double m_dRadius;//半径
    double m_distance;
    QPixmap m_prePixmap;//生成余辉的前一张图片
    qint64 m_lastClearRadarVideoTime;
    int m_clearRadarTrackTime;//单位是分钟

    //zmq发送
    void *m_pVideoContext;
    void *m_pVideoLisher;
    int  m_uVideoSendPort;
    QString m_sVideoTopic;

    void *m_pTrackContext;
    void *m_pTrackLisher;
    int  m_uTrackSendPort;
    QString m_sTrackTopic;

    //限制区域
    bool m_bLimit;//是否设置限制区域
    QList<QPolygonF> m_landPolygon;
    QList<QPolygonF> m_seaPolygon;

    //QThreadPool objThreadPool;
    QThread m_workThread;

    ZmqMonitorThread *m_pTrackMonitorThread;
    ZmqMonitorThread *m_pVideoMonitorThread;


};

#endif // ZCHXRADARDATASERVER_H
