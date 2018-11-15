#ifndef ZCHXRADARDATASERVER_H
#define ZCHXRADARDATASERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QList>
#include "zchxdrawradarvideo.h"
#include <QPixmap>
#include "protobuf/ZCHXRadar.pb.h"
#include "zmq.h"
#include "protobuf/ZCHXRadarVideo.pb.h"
#include "zchxdrawvideorunnable.h"
#include <QThread>
#include "zchxfunction.h"
#include "radarccontroldefines.h"
#include "MultiCastDataRecvThread.h"
#include "VideoDataProcessWorker.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>

typedef com::zhichenhaixin::proto::RadarVideo  ITF_RadarVideo;
typedef com::zhichenhaixin::proto::TrackPoint  ITF_RadarPoint;
class ZCHXRadarDataServer : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXRadarDataServer(int uSourceID = 1,QObject *parent = 0);
    ~ZCHXRadarDataServer();
    int   sourceID() const;

signals:
    void signalClientInout(const QString& ip, const QString& name, int port, int inout);
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void startProcessSignal();

    //发送信号到另一个线程处理

    void analysisLowranceRadar(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading);//小雷达
    void analysisCatRadar(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading,const QString &sType);//新科雷达
    void signalRadarStatusChanged(const QList<RadarStatus>& sts, int radarID);

public slots:
    QByteArray HexStringToByteArray(QString HexString); //16进制字符串转字节数组
    QString ByteArrayToHexString(QByteArray &ba); //字节数组转16进制字符串
    void openRadar();
    void closeRadar();
    void startProcessSlot();
    //track
    void displayUdpTrackError(QAbstractSocket::SocketError error);
    void updateTrackUdpProgress();

    //video
//    void displayUdpVideoError(QAbstractSocket::SocketError error);
//    void updateVideoUdpProgress();
    //report
    void displayUdpReportError(QAbstractSocket::SocketError error);
    void updateReportUdpProgress();
    void ProcessReport(const QByteArray& bytes, size_t len);


    //4g雷达 IP 236.6.7.10  port 6680
    //6g雷达 IP 236.6.7.100  port 6132
    //开关控制 IP 236.6.101.100  port 6133
    void heartProcessSlot();//心跳通信
    //雷达控制
    void setControlValue(INFOTYPE infotype, int value);
    void updateValue(INFOTYPE controlType, int value);
    void slotRecvTrackPoint(const QList<TrackPoint>& list);
private slots:
    void analysisRadar(const QByteArray &sRadarData,const QString &sRadarType,
                       int uLineNum = 0,int uCellNum = 0,int uHeading = 0);//解析接收到的所有AIS

private:
    void init();

    void parseRadarControlSetting(INFOTYPE infotype);

private:
    int m_uSourceID;

    QUdpSocket *m_pUdpTrackSocket;
    QUdpSocket *m_pUdpVideoSocket;
    QUdpSocket *m_pUdpReportSocket;
    QString m_sTrackIP;
    int  m_uTrackPort;
    QString m_sVideoIP;
    int  m_uVideoPort;
    QString m_sReportIP;
    int m_uReportPort;
    bool m_bReportOpen;

    QUdpSocket *m_pHeartSocket;//心跳
    QTimer     *m_pHeartTimer;
    int m_uHeartTime;
    QString m_sHeartIP;
    int  m_uHeartPort;
    QString m_sOptRadarIP;
    int  m_uOptRadarPort;


    QString m_sRadarVideoType;//cat010-新科，Lowrance-小雷达
    int  m_uCellNum;//一条线上多少个点
    int  m_uLineNum;//一圈多少条线
    int  m_uHeading;//雷达方位

    QThread m_workThread;
    MultiCastDataRecvThread *mDataRecvThread;//接收雷达数据线程类
    VideoDataProcessWorker  *mVideoWorker;  //回波数据生成类
    QMap<INFOTYPE, RadarStatus>   mRadarStatusMap; //雷达状态容器
    UINT8       mRadarPowerStatus;//雷达状态



};

#endif // ZCHXRADARDATASERVER_H
