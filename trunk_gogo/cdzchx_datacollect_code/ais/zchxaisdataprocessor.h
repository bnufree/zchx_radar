#ifndef ZCHXAISDATAPROCESSOR_H
#define ZCHXAISDATAPROCESSOR_H

#include <QObject>
#include <QThread>
#include "ZCHXAISVessel.pb.h"
#include "zmq.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "ZmqMonitorThread.h"
#include <bitset>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include "ais.h"
#include "beidoudata.h"

using std::bitset;
using std::ostream;
using std::string;
using std::vector;
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

class zchxDataOutputServerMgr;
class zchxAisChartWorker;

struct AisSimpleData
{
    QString              id;                         //唯一识别码
    long long            utc;                        //时间标记 13位时间戳
    short                type;                       //类型 0：ais 1：北斗 2：CMDA 3:融合  4:船讯网数据(船舶档案)
    float                sog;                        //对地航速单位：节
    float                cog;                        //对地航向
    double               lon;                        //经度
    double               lat;                        //纬度
    QList<AisSimpleData*>   historyData;
};

struct zchxAisDataProcessParam{
    int         mStationId;
    int         mDataSendPort;
    QString     mAisDataTopic;
    QString     mChartTopic;
    QString     mStationName;
    bool        mIsCenterInit;
    double      mCenterLat;
    double      mCenterLon;
    bool        mAisLimitChk;
    QRectF      mAisLimitRect;
    bool        mAis2Beidou;
    bool        mAisFixRadius;
    int         mRadius;

    zchxAisDataProcessParam()
    {
        mIsCenterInit = false;
        mAisLimitChk = false;
        mAis2Beidou = false;
    }
};

//template<size_t T, size_t T2>
//int backward_bits(bitset<T> &bits, const size_t start, const size_t len, bitset<T2> &bs_tmp) {
//  assert(len <= 32);
//  assert(start + len <= T);
//  for (size_t i = 0; i < len; i++)
//    bits[start+i] = bs_tmp[len - i - 1];
//  return bs_tmp.to_ulong();
//}
class zchxAisDataProcessor : public QObject
{
    Q_OBJECT
public:
    explicit zchxAisDataProcessor(const zchxAisDataProcessParam& param,QObject *parent = 0);
    ~zchxAisDataProcessor();

    bool CheckXor(QByteArray data);//校验位检查
    uchar checkxor(QByteArray data);

signals:    
    void signalClientInout(const QString& ip, const QString& name, int port, int inout);
    void signalRecvAisData(const QByteArray& data);
    void signalInitZmq();
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void signalAisTrackData(int, double, double);
    void signalAisToBeidou(double, double);
    void signalSendAisData(const ITF_AISList & objAisList);
public slots:
    void slotProcessAisData(const QByteArray& data);
    void initZmq();
    void prtAisSlot(bool);//打印AIS数据
    void slotSendAis(ITF_AISList objAisList);//发送自定义模拟数据
    void showBeidou();
    void sendAisAfterMerge(ITF_AISList objAisList);
    void slotSendPixMap(const QByteArray& img, int width, int height, double range, const QString& format);
private:
    void initChartWorker();
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
    bool                            mPrint; //AIS打印标志
    QThread                         mWorkThread;
    zchxDataOutputServerMgr*        mDataOutputMgr;
    zchxAisChartWorker*             mChartWorker;
    QString                         mAisData;
    QString                         mSource;
    zchxAisDataProcessParam         mParam;
    //beidouData *beidou;
};

#endif // ZCHXAISDATAPROCESSOR_H
