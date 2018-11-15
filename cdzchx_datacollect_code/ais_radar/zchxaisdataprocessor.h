#ifndef ZCHXAISDATAPROCESSOR_H
#define ZCHXAISDATAPROCESSOR_H

#include <QObject>
#include <QThread>
#include "protobuf/ZCHXAISVessel.pb.h"
#include "zmq.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "ZmqMonitorThread.h"

const unsigned long IMO_NUM_NA =  0UL;
const unsigned long IMO_NUM_MIN =  1UL;
const unsigned long IMO_NUM_MAX =  999999999UL;

const unsigned short  CARGO_TYPE_NA = 0U;
const unsigned short  CARGO_TYPE_MIN = 1U;
const unsigned short  CARGO_TYPE_MAX = 255U;
const float SOG_KNOTS_MIN =  0.0;
const float SOG_KNOTS_MAX =  102.2;

const float LONG_DEGREES_MIN =  -180.01;
const float LONG_DEGREES_MAX =  180.01;
const float LON_DEGREES_NA =  181.0;
const float LAT_DEGREES_NA =  91.0;

const float COG_DEGREES_MIN = 0.0;
const float COG_DEGREES_MAX = 360.0;
const float LAT_DEGREES_MIN =  -90.01;
const float LAT_DEGREES_MAX =  90.01;
const unsigned short HEADING_DEGREE_MIN =  0U;
const unsigned short HEADING_DEGREE_MAX =  359U;
const unsigned short HEADING_DEGREE_NA =  511U;
const float DRAUGHT_NA =  0.0;
const float DRAUGHT_MIN =  0.1;
const float DRAUGHT_MAX =  25.5;
typedef com::zhichenhaixin::proto::AIS ITF_AIS;
typedef com::zhichenhaixin::proto::AISList ITF_AISList;

class zchxAisDataProcessor : public QObject
{
    Q_OBJECT
public:
    explicit zchxAisDataProcessor(QObject *parent = 0);
    ~zchxAisDataProcessor();


    bool CheckXor(QByteArray data);//校验位检查
    uchar checkxor(QByteArray data);

signals:    
    void signalClientInout(const QString& ip, const QString& name, int port, int inout);
    void signalRecvAisData(const QByteArray& data);
    void signalInitZmq();
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
public slots:
    void slotProcessAisData(const QByteArray& data);
    void initZmq();
private:
    void analysisAIS(const QString sSrcData);
    bool analysisCellAIS(const QString sCellAisData,int uPad, ITF_AIS& ais);//解析数据部分
    bool buildAis(const QJsonObject &obj, ITF_AIS& ais);
    void sendAis( ITF_AISList &objAisList);
    //消息为1 2 3 的A类船舶动态信息,消息为18的B类船舶动态信息,消息为19的B类船舶静态及动态信息
    bool buildVesselTrack(const QJsonObject &obj,ITF_AIS& ais);
    //消息为4 11 的基地台报告类信息
    bool buildBasestationInfo(const QJsonObject &obj,ITF_AIS& aisK);
    //消息为5的A类船舶静态信息
    bool buildVesselInfo(const QJsonObject &obj,ITF_AIS& ais);
    //消息为21的助航信息
    bool buildNavigationInfo(const QJsonObject &obj,ITF_AIS& ais);
    //消息为24的B类船舶静态信息
    bool buildPartialVesselInfo(const QJsonObject &obj,ITF_AIS& ais);

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
private:
    QThread         mWorkThread;
    //zmq发送
    void *m_pAISContext;
    void *m_pAISLisher;
    int  m_uAISSendPort;
    QString m_sAISTopic;
    ZmqMonitorThread *m_pMonitorThread;
    QString mAisData;
};

#endif // ZCHXAISDATAPROCESSOR_H
