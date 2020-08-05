#ifndef COLLECTSERVER_H
#define COLLECTSERVER_H

#include <QObject>
#include "SCCMMSComData.pb.h"
#include <QTimer>
#include <QThread>
#include <QPointF>
#include <QDateTime>
#include <QTime>
#include "systemconfigsettingdefines.h"

class ReciveComData;

using namespace com::zhichenhaixin::proto;
typedef struct FakeData {
    double lon;
    double lat;
    double segmentCourse;          //方位角
    double alter_course;    //方位角改变
    double segmentDis;      //段距离
    double pointTotalDis;   //到这点为止的总距离
    double slack;           //余量
    double segmentCableLength;      //段落海缆长度
    double pointCableTotaLength;    //到这点的海缆的总长度
    QString comment;                //备注
    double shipspd;                 //船舶速度
    double cableSpd;                //电缆速度
    double day;                     //天数
    QDateTime datetime;                //时间
    QTime     segmentDuration;             //持续时间
    double  head;                           //船首像

    FakeData()
    {
        lon = 104.072986;
        lat = 30.624211;
        segmentCourse = 90.0;
        alter_course = 0.0;
        segmentDis = 0.0;
        pointTotalDis = 0.0;
        slack = 0.0;
        segmentCableLength = 0.0;
        pointCableTotaLength = 100.0;
        comment = "";
        shipspd = 1.0;
        cableSpd = 1.0;
        day = 0.0;
        head = 60.0;
    }
}ShipPlanData;

class CollectServer : public QObject
{
    Q_OBJECT
public:
    explicit CollectServer(void* ctx = 0, QObject *parent = 0);
    ~CollectServer();
    void* getContext();


    bool  readFile();
    double convertLonlat2DoubleWithSNWE(const QString& src);
    QPointF  getDividePoint(const QPointF& start, const QPointF& end, int totalGaps, int cur);
    double getDistanceDeg(double lat1, double lon1, double lat2, double lon2);
    double degToRad(double d);
    double radToDeg(double r);
    void distbear_to_latlon(double lat1, double lon1, double dist, double brng, double &lat_out, double &lon_out);
    double calcAzimuth(double lon1, double lat1, double lon2, double lat2);
    PMASNS  parsePMASNSData(const QString& src);
    PMAGGA  parsePMAGGAData(const QString& src);
    PMAVTG  parsePMAVTGData(const QString& src);
    PMATHA  parsePMATHAData(const QString &src);
    PMATHF  parsePMATHFData(const QString &src);
    PMAINF  parsePMAINFData(const QString &src);
    PMAALA  parsePMAALAData(const QString &src);
    PMAVDR  parsePMAVDRData(const QString &src);
    double  convertDPLonLat(const QString& src);
    bool    parseHMR3000Data(const QString& src, HMR3000& data);
    int     getHMR3000Status(const QString& str);
    void parseUSBLData(const QString &src);
    void    setMeterCounterInitLength(double length);
    double  get4017DetailValue(const QList<double>& vals, const DEVANALYPARAM& param);
    double  get4017DetailValue(const QList<double>& vals, const QString& channel_name, double offset, double coeff);
    QByteArray  makeCableDataFromFakeData(const FakeData& fake);
    QByteArray  makeNaviDataFromFakeData(const FakeData& fake);
    QStringList dlon;
    QStringList dlat;
    QStringList mlon;
    QStringList mlat;
    QStringList speedship;
    QStringList speedcable;
    QStringList datetimelist;
    QList<FakeData>     mFakeDataList;
    QByteArray  makeTensionCmd();
signals:
    void    signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void    signalSendLogMsg(const QString& msg);
    void    signalClientInout(const QString& ip, const QString& name, int port, int inout);
    void    signalStartComConnection();
    void    signalSetWorkStatus(bool sts);
public slots:
    void    slotSetWorkStatus(bool sts);
    void    start();
    void    stop();    
    void    init();
    void    setComDevParams(const QMap<QString, COMDEVPARAM>& map);
    void    setOtherDevParam(const QMap<QString, DEVANALYPARAM> params);
    void    setTowBodyParam(const QMap<QString, DEVANALYPARAM> params);
    void    setSurfaceThresholdParam(const QMap<QString, DEVANALYPARAM> params);
    void    setUnderWaterThresholdParam(const QMap<QString, DEVANALYPARAM> params);
    void    setUnderWater40172Params(const QMap<QString, DEVANALYPARAM> params);
    void    setUnderWater40171Params(const QMap<QString, DEVANALYPARAM> params);
    void    setSurface4017Params(const QMap<QString, DEVANALYPARAM> params);
    void    startFakeServer(int index = 0);
    void    stopFakeServer();
    void    pauseFakeServer();
    void    slotSetupSimulationData();
    void    slotSendComData(const QByteArray& pTopic, const QByteArray& pData);
    void    slotRecvConstructionCommand(const QByteArray& pTopic, const QByteArray& pData);
    void    slotRecvDpUploadMsg(const QByteArray& msg);
    void    slotStartComConnection();
    void    slotCloseComConnection();

private slots:
    void    slotRecvComData(const QString& comName, const QString& topic, qint64 time, const QByteArray& recv);
    void    slotUSBLReciveComData(qint64 time, QByteArray data);
    void    slotNaviReciveComData(qint64 time, QByteArray data);
    void    slotCableLayeReciveComData(qint64 time, QByteArray data);
    void    slotDpReciveComData(qint64 time, QByteArray data);
    void    slotMeterCounterReciveComData(qint64 time, QByteArray data, const QString& msgnum);
    void    slotSurface4017ReciveComData(qint64 time, QByteArray data, const QString& msgnum);
    void    slotUnderWater4017ReciveComData(qint64 time, QByteArray data, const QString& msgnum1 = "", const QString& msgnum2 = "");
    void    slotUnderHMRReciveComData(qint64 time, QByteArray data);
    void    slotGPSReciveComData(qint64 time, QByteArray data);
    void    slotSurfaceHMRReciveComData(qint64 time, QByteArray data);

    bool    parseNaviDevData4Com(const QByteArray & data);
    bool    parseCableLayerData4Com(const QByteArray & data);
    qint64  bytesToInt(QByteArray bytes);
    void    slotTimeOut();
    void    slotHeartMsg();

    double  convertLonlatFromddmmdotmmmm(const QString& ValueStr, const QString& NSEWFlgStr);
    QString  convertLonlatToddmmdotmmmm(double val);
    qint64  convertTimeFromhhmmssdotsss(const QString& valstr);
    QString convertTimeTohhmmssdotsss(qint64 time);
    void     stopComWork(ReciveComData *data);

private:
//    ReciveComData *m_pGpsComData; //使用QT的串口管理
//    ReciveComData *m_pCableLayComData; //使用QT的串口管理
//    ReciveComData  *m_pDpComData;
//    ReciveComData  *m_pUSBLComData;     //
    //ComDataApi  mApiData;
    //GPSData     mGpsData;
    //CableData   mCableData;
    QByteArray  mGpsBuf;                //导航数据，必须满足85字节才能解析
    QByteArray  mCableBuf;              //线缆数据。必须满足63字节才能解析
    QByteArray  mDpBuf;
    //DPPMAData   mDpData;

    void*       mCtx;
    void*       mSocket;                //ZMQ
    int         portNum;
    QThread     mWorkThread;
    bool        mRunning;
    bool        mFakeflag;
    int         mFakeInterval;      //秒
    QTimer      *mDisplayTimer;
    QTimer      *mHeartTimer;       //用来测试保持客户端检测服务器是否在线
    int         mFakeIndex;
    int         mFakeConstructionMode;  //0:停止模拟，1，开始模拟， 2，暂停模拟
    FakeData    mFakeData;
    QMap<QString, ReciveComData*>  mStartedComList;

    //设备的接收信息
    DevInfo     mDevInfo;
    ShipInfo    mShipInfo;
    PlowInfo    mPlowInfo;
    LayInfo     mLayInfo;
    Surface4017Data mSurface4017;
    UnderWater4017Data  mUnder4017;
    ConstructionInfo    mConstructInfo;
    //设备传感器等的参数配置
    //1)水面设备传感器4017的参数
    int                 mSurfaceChannelNum;
    DEVANALYPARAM       mSurfacePull1Param;
    DEVANALYPARAM       mSurfacePull2Param;
    DEVANALYPARAM       mSurfacePull3Param;
    DEVANALYPARAM       mSurfaceLeftPumpParam;
    DEVANALYPARAM       mSurfaceRightPumpParam;
    //2)水下40171的设备传感器的参数
    int                 m40171ChannelNum;
    DEVANALYPARAM       m40171WaterDepthParam;
    DEVANALYPARAM       m40171BootsAngleParam;
    DEVANALYPARAM       m40171TouchDown1Param;
    DEVANALYPARAM       m40171TouchDown2Param;
    DEVANALYPARAM       m40171TouchDown3Param;
    DEVANALYPARAM       m40171TouchDown4Param;
    //3)水下设备传感器40172的参数
    int                 m40172ChannelNum;
    DEVANALYPARAM       m40172Pull1Param;
    DEVANALYPARAM       m40172Pull2Param;
    DEVANALYPARAM       m40172Pull3Param;
    DEVANALYPARAM       m40172LeftPumpParam;
    DEVANALYPARAM       m40172RightPumpParam;
    //4)其他参数设定
    double              mReportStep;
    double              mStorageTime;
    double              mGpsLonDeviation;
    double              mGpsLatDeviation;
    double              mGpsShipLength;
    bool                mShipHeadAhead;
    double              mDisplayTime;
    double              mSurfaceCompassDeviation;
    double              mMeterCounterStepMeter;
    double              mMeterCounterInitLength;
    double              mUnderWaterCompassDeviation;
    double              mTensionCoeff;
    //5)水面传感器的阈值设定
    double              mSurfaceMaxPitch;          //最大纵倾
    double              mSurfaceMaxRoll;           //最大横倾
    double              mSurfaceMaxDepth;          //最大深度
    double              mSurfaceMaxForce1;  //最大水面力
    double              mSurfaceMaxForce2;  //最大水面力
    double              mSurfaceMaxForce3;  //最大水面力
    //6)水下传感器的阈值设定
    double              mUnderWaterMaxPitch;          //最大纵倾
    double              mUnderWaterMaxRoll;           //最大横倾
    double              mUnderWaterMaxDepth;          //最大深度
    double              mUnderWaterMinHeight;             //最小高度
    double              mUnderWaterMaxForce1;  //最大水面力
    double              mUnderWaterMaxForce2;  //最大水面力
    double              mUnderWaterMaxForce3;  //最大水面力
    //7)拖体参数
    double              mTowForwardLength;         //前方长度
    double              mTowBackwardLength;         //后方长度
    double              mTowBootsLength;            //靴体长度
    double              mTowRange;                  //拖体范围
    double              mTowBootsHeight;            //拖体轴心高度
    //8)Com通讯接口的配置
    QMap<QString, COMDEVPARAM>      mDevComParamsMap;

    qint64              mLastSendTime;
    QString             mGpsCmdStr;
    bool                mCalCourseFlag;
    bool                mLastPosFlag;
    double              mLastLon;
    double              mLastLat;
    QString             mMeterCounterCmdStr;
    bool                mFirstRecvMeterCounterFlag;
    qint64              mLastMeterCounterNum;
    double              mSavedCableLength;
    bool                mWorkNow;
    int                 mMeterConterMax;
    double                 mSpeedTimeGap;
    //计算缆速
    double              mLastCalCableLength;
    //计算船速
    double              mLastCalShipMoveLength;
    qint64              mLastCalShipSpeedTime;
    double              mLastSpeedLon;
    double              mLastSpeedLat;


};

#endif // COLLECTSERVER_H
