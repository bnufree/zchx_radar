#ifndef ZCHXLOWRANCERADARDATASERVER_H
#define ZCHXLOWRANCERADARDATASERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QList>
#include <QPixmap>
#include "zmq.h"
#include "zchxdrawvideorunnable.h"
#include <QThread>
#include "zchxfunction.h"
#include "radarccontroldefines.h"
#include "updatevideoudpthread.h"
#include <QCoreApplication>
//#include <QDebug>
#include <QDataStream>
#include <QTcpServer>
#include <QTcpSocket>
#include "zchxRadarVideoRecvThread.h"

//lowrance 雷达参数
struct UDPSocketParam{
    QString     mHost;
    quint32      mPort;
};

struct LowranceRadarSocket{
    UDPSocketParam      mReportSocket;          //雷达状态信息报告
    UDPSocketParam      mVideoSocket;           //回波数据获取
    UDPSocketParam      mHeartSocket;           //雷达操作控制
    int                 mPriority;              //优先级
};


class ZCHXLowranceRadarDataServer : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXLowranceRadarDataServer(int source, QObject *parent = 0);
    ~ZCHXLowranceRadarDataServer();
    int   sourceID() const;

signals:
    void signalClientInout(const QString& ip, const QString& name, int port, int inout);
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void startProcessSignal();
    void prtVideoSignal_1(bool);
    void joinGropsignal(QString);
    void signalRadarConfigChanged(int, int, int);
    void colorSetSignal(int,int,int,int,int,int);//回波颜色设置
    void signalSendRadarType(int type);

    //发送信号到另一个线程处理

    void analysisLowranceRadar(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading);//小雷达
    void analysisCatRadar(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading,const QString &sType);//新科雷达
    void signalRadarStatusChanged(const QList<RadarStatus>& sts, int radarID);

public slots:
    void prtVideoSlot(bool);//打印回波数据

    void openRadar();
    void closeRadar();
    void startProcessSlot();
    //track
    void displayUdpTrackError(QAbstractSocket::SocketError error);
    void updateTrackUdpProgress();

    //video
    void displayUdpVideoError(QAbstractSocket::SocketError error);
    void updateVideoUdpProgress(const QByteArray& data);
    //report
    void displayUdpReportError(QAbstractSocket::SocketError error);
    void updateReportUdpProgress();
    void ProcessReport(const QByteArray& bytes, int len);
    void new_client();
    void client_dis();
    void read_client_data();
    void setPixSlot(QPixmap pix);
    //4g雷达 IP 236.6.7.10  port 6680
    //6g雷达 IP 236.6.7.100  port 6132
    //开关控制 IP 236.6.101.100  port 6133
    void heartProcessSlot();//心跳通信
    //雷达控制
    void setControlValue(INFOTYPE infotype, int value);
    void updateValue(INFOTYPE controlType, int value);
    bool getControlValueRange(INFOTYPE controlType, int& min, int& max);
    bool getControlAutoAvailable(INFOTYPE controlType);

private slots:
    void analysisRadar(const QByteArray &sRadarData,const QString &sRadarType,
                       int uLineNum = 0,int uCellNum = 0,int uHeading = 0);//解析接收到的所有AIS

private:
    void init();

    void parseRadarControlSetting(INFOTYPE infotype);

private:
    QList<LowranceRadarSocket>  mRadarParamsList;
    QStringList                 mIPV4List;
    bool prt;//控制打印回波数据标志
    int m_uSourceID;
    QUdpSocket *m_pUdpTrackSocket;
    zchxVideoDataRecvThread *m_pUdpVideoSocket;
    QUdpSocket *m_pUdpReportSocket;
    QString m_sTrackIP;
    int  m_uTrackPort;
    QString m_sVideoIP;
    int  m_uVideoPort;
    QString m_sReportIP;
    int m_uReportPort;
    bool m_bReportOpen;
    int uTcpPort;

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
    //updatevideoudpthread *u_workThread;//接收雷达数据线程类
    //updatevideoudpthread *u2_workThread;//接收雷达数据线程类
    QMap<INFOTYPE, RadarStatus>   mRadarStatusMap; //雷达状态容器
    uint8_t       mRadarPowerStatus;//雷达状态
    QString mac_ip;
    QTcpServer *mServer;
    QTcpSocket *mSocket;
    bool tcp_flag;//通过TCP发送原始回波数据标志
    int name_num = 1;//回波文件编号
    int zt_name=1;//状态文件编号

    QTcpSocket *m_pTcpSocket;//1_通信套接字
    int mTod = 0;
};

#endif // ZCHXLOWRANCERADARDATASERVER_H
