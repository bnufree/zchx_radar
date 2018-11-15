#ifndef ZCHXAISDATASERVER_H
#define ZCHXAISDATASERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QThread>


class ZCHXAisDataServer : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXAisDataServer(QObject *parent = 0);
    ~ZCHXAisDataServer();
signals:
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void startProcessSignal();
    void signalSocketMsg(const QString& error);
    void signalSendAisData(const QByteArray& data);
public slots:
    void startProcessSlot();
protected slots:
    void stateChanged(QAbstractSocket::SocketState state);
    void displayError(QAbstractSocket::SocketError error);
    void updateServerProgress();
    void acceptConnection();
    void slotCheckAisRecv();
private:
    void init();

#if 0
    void analysisAIS(const QString sAisData);//解析接收到的所有AIS
    bool analysisCellAIS(const QString sCellAisData,int uPad);//解析数据部分

    void buildAis(const QJsonObject &obj, ITF_AISList &aisList);

    void sendAis( ITF_AISList &objAisList);
    //消息为1 2 3 的A类船舶动态信息,消息为18的B类船舶动态信息,消息为19的B类船舶静态及动态信息
    ITF_AIS buildVesselTrack(const QJsonObject &obj,bool &bOK);
    //消息为4 11 的基地台报告类信息
    ITF_AIS buildBasestationInfo(const QJsonObject &obj,bool &bOK);
    //消息为5的A类船舶静态信息
    ITF_AIS buildVesselInfo(const QJsonObject &obj,bool &bOK);
    //消息为21的助航信息
    ITF_AIS buildNavigationInfo(const QJsonObject &obj,bool &bOK);
    //消息为24的B类船舶静态信息
    ITF_AIS buildPartialVesselInfo(const QJsonObject &obj,bool &bOK);

    QJsonObject ais1_2_3_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais4_11_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais5_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais6_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais7_13_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais8_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais9_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais10_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais12_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais14_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais15_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais16_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais17_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais18_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais19_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais20_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais21_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais22_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais23_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais24_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais25_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais26_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
    QJsonObject ais27_to_json(const QString &sAisBody, const int uPad, qint64  recivetime);
#endif
private:
    bool m_bServer;//1_标志位 IsServer=false
    QTcpServer *m_pTcpServer;//1_监听套接字
    QTcpSocket *m_pTcpSocket;//1_通信套接字
    //QTimer     *m_pTimer;
    int  m_uServerPort;
    QString m_sIP;
    int  m_uPort;
    //bool m_bNeedConnect;
    //QMap<QString,QJsonObject> m_aisJsonMap;//存放接收到是有效ais数据，key是ais的唯一标识符mmsi+type



    qint64 m_lastClearAISTime;

    QThread m_workThread;
    qint64   mLastRecvAisDataTime;
    QTimer*     mAisHeartTimer;
    int         mDataTimeOut;
};

#endif // ZCHXAISDATASERVER_H
