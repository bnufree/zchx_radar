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
#include "VideoDataRecvThread.h"
#include "VideoDataProcessWorker.h"
#include "zchxRadarHeartWorker.h"
#include "zchxRadarCtrlWorker.h"
#include "zchxRadarReportWorker.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include "side_car_parse/Messages/RadarConfig.h"

typedef com::zhichenhaixin::proto::RadarVideo  ITF_RadarVideo;
typedef com::zhichenhaixin::proto::TrackPoint  ITF_RadarPoint;
class ZCHXRadarDataServer : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXRadarDataServer(ZCHX::Messages::RadarConfig* cfg,QObject *parent = 0);
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
    QUdpSocket *m_pUdpTrackSocket;
    QUdpSocket *m_pUdpVideoSocket;
    QUdpSocket *m_pUdpReportSocket;
    QThread m_workThread;
    VideoDataRecvThread *mDataRecvThread;//接收雷达数据线程类
    VideoDataProcessWorker  *mVideoWorker;  //回波数据生成类
    zchxRadarHeartWorker    *mHeartObj;     //心跳工作对象
    zchxRadarCtrlWorker     *mCtrlObj;      //雷达控制对象
    zchxRadarReportWorker   *mReportObj;    //雷达参数报告    
    UINT8       mRadarPowerStatus;//雷达状态
    ZCHX::Messages::RadarConfig*        mRadarConfig;



};

#endif // ZCHXRADARDATASERVER_H
