#ifndef ZCHXANALYSISANDSENDRADAR_H
#define ZCHXANALYSISANDSENDRADAR_H

#include <QObject>
#include <QThread>
#include "zchxfunction.h"
#include "zmq.h"
#include "zchxradarvideoprocessor.h"
#include <QPointF>
#include <QPolygonF>
#include <QList>
#include <QPixmap>
#include "asterixformat.hxx"
#include "asterixformatdescriptor.hxx"
#include "ZmqMonitorThread.h"
#include <QFile>
#include "zchxgettrackprocess.h"
#include "../serialport.h"
#include "QtXlsx/QtXlsx"
#include <QFileDialog>
#include <QTimer>
//#include "zchxvideorects.h"
#include <QGeoCoordinate>
//#include "zchxrectextractionthread.h"
#define SPOKES (4096)


//自定义链表结构体
struct  RadarNode{
public:
    zchxRadarRectDef        mRect;
    bool                    mIsOk;
    QList<RadarNode*>       mNextNodes;
    RadarNode()
    {
        mIsOk = false;
    }
    RadarNode(const zchxRadarRectDef& other)
    {
        mRect.CopyFrom(other);
        mIsOk = false;
    }

};

enum  TARGET_DIRECTION{
    TARGET_DIRECTION_UNDEF = 0,     //点列不足的情况
    TARGET_DIRECTION_STABLE,        //目标点列方向稳定
    TARGET_DIRECTION_UNSTABLE,      //目标点列方向散乱
};

class ZCHXAnalysisAndSendRadar : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXAnalysisAndSendRadar(RadarConfig* cfg, QObject *parent = 0);
    ~ZCHXAnalysisAndSendRadar();

    closeTT();

    ZCHXGetTrackProcess* getVideoProcessor() {return m_pGetTrackProcess;}

    zchxRadarRectList findRectSameObj(zchxRadarRectDef &obj, bool self = true);

    zchxTrackPointList findSameObj(const zchxTrackPoint& src, bool self = true);

    void Process1CycleData();

    QGeoCoordinate GetCenterPointFromListOfCoordinates(QList<QGeoCoordinate> geoCoordinateList)
    {
        //以下为简化方法（400km以内）
        int total = geoCoordinateList.size();
        double lat = 0, lon = 0;
        foreach (QGeoCoordinate g,geoCoordinateList)
        {
            lat += g.latitude() * PI / 180;
            lon += g.longitude() * PI / 180;
        }
        lat /= total;
        lon /= total;
        QGeoCoordinate a((lat * 180 / PI), (lon * 180 / PI));
        return a;
    }

signals:
    void analysisLowranceRadarSignal(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading);//小雷达
    void analysisCatRadarSignal(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading,const QString &sRadarType);//新科雷达
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void signalClientInout(const QString& ip, const QString& name, int port, int inout);

    void startTrackProcessSignal(SAzmData sAzmData);
    void startTrackProcessSignal(const SAzmDataList& sAzmData);
    void startTrackProcessSignal(const zchxVideoFrameList& list);
    //void show_info(QString, double, float);//1_打印txt信号
    void show_info(QString);//1_打印txt信号
    void show_video(int, int);//1_打印目标个数
    void show_statistics(int,int,int,int,int);//1_打印统计丢包率
    void show_missing_spokes(QString);//1_打印丢失扫描线信息
    void show_received_spokes(QString);//1_打印所有扫描线信息
    void set_pen_width(int);//1_设置笔宽度
    void land_limit_signal(QPolygonF);//发送区域限制点集合信号
    void signalRadiusFactorUpdated(double radius, double factor);
    void signalRadarVideoAndTargetPixmap(const QPixmap &videoPixmap,const Afterglow &dataAfterglow);
    void new_track_count(int);//回波块识别出来的目标
    void historySignal(int);//历史缓存目标
    void showTrackNumSignal(bool);//显示目标编号
    void signalDealViedoTrack(QList<TrackNode>,int);
    void signalTrackList(QMap< int,QList<TrackNode> >);//画回波目标

    void signalSendTrackPointList(const zchxTrackPointList& list);
    void colorSetSignal(int,int,int,int,int,int);//回波颜色设置
    void signalAnalysisVideoPiece(QMap<int,RADAR_VIDEO_DATA>,double );
    void signalShowTheLastPot(QList<QPointF> ,QList<QPointF>);
    void signalVideoRects(const zchxRadarRectMap&);
    void signalCombineTrackc(const zchxTrackPointMap&);
    void signalCombineVideo(QMap<int, QList<TrackNode>>,int);
    void singalShowRadiusCoefficient(double,double);
public slots:
    void slotDrawCombinVideo(QList<TrackNode>);
    void sendVideoRects(const zchxRadarRectDefList&);

    void showTrackNumSlot(bool);//显示目标编号
    void handleTimeout();  //1_超时处理函数
    void handleTimeout_1();  //1_超时处理函数
    void analysisLowranceRadarSlot(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading);//小雷达
    void analysisCatRadarSlot(const QByteArray &sRadarData,
                               int uLineNum, int uCellNum, int uHeading, const QString &sRadarType);//新科雷达
    void setRadarAfterglowPixmap(const int uIndex, const QPixmap &videoPixmap, const QPixmap &objPixmap,
                             const QPixmap &prePixmap);//接收余辉图片
    void sendTrackSlot(const zchxTrackPoint& radarPoint);
    void sendTrackSlot2(const zchxTrackPointList &list);
    void setRangeFactor(double factor);
    //void send_new_track_slot(QPolygonF,QPolygonF);//发送给web端
    void slotGetGpsData(double, double);//实时更新从GPS传入的经纬度坐标
    void updateFloatSlot();//更新浮标配置
    void sendRadarRectPixmap();
    void slotSendComTracks(const zchxTrackPointMap&);
    float getAngle(float,float,float,float);
    void slotTrackMap(QMap<int,QList<TrackNode>>);
private:
    Latlon getMergeTargetLL(const zchxRadarRectDefList& list);
    void changeTargetLL(const Latlon& ll, zchxRadarRectDef& target);
    int isTargetDirectStable(const zchxRadarRect& rect, int check_point_num, double *avg_cog = 0 );
    bool isTargetJumping(const zchxRadarRect& rect, double merge_dis, int jump_target_num = 3);
    void readRadarLimitFormat();//读取雷达限制区域文件
    void processVideoData(bool rotate = true);
    bool isDirectionChange(double src, double target);
    double calAvgCog(const zchxRadarRectDefList& list);

    void analysisCat253Radar(QByteArray ba);//解析新雷达控制数据
    void analysisCat020Radar(QByteArray ba);//解析新雷达目标数据
    void analysisCat010Radar(DataBlock *pDB);//雷达目标
    void analysisCat240Radar( DataBlock *pDB,
                             int uLineNum, int uCellNum,int uHeading);//回波

    void sendRadarTrack();//发送雷达目标

    void clearRadarTrack();//定期清理雷达目标
    //解析限制区域
    void analysisLonLatTOPolygon(const QString sFileName,
                                 QList<QPolygonF> &landPolygon,QList<QPolygonF> &seaPolygon);
    bool inLimitAreaForTrack(const double dLat, const double dLon);
    bool inLimitArea(const double dCentreLat, const double dCentreLon,
                     const double dAzimuth,const int uPosition,
                     const double dStartRange, const double dRangeFactor);
    void dumpTargetDistance(const QString& tag, double merge_dis);
    void checkTargetRectAfterUpdate(double merge_dis);
    void mergeRectTargetInDistance(zchxRadarRectDefList& list, int distance);
private:
    QThread m_workThread;
    QString str_radar;
    int     m_uSourceID;
    int cell_num;
    double m_dCentreLon;
    double m_dCentreLat;
    double m_dRadius;//半径
    int m_dDiameter;//直径
    double m_distance;
    int m_jupmdis;
    int m_uLoopNum;//余辉循环值
    int d_32;
    QString sRadarType;//读取接入雷达型号 4G/6G
    QString m_limit_file;//读取限制区域文件
    QPolygonF land_limit;//区域限制点集合
    QPolygonF sea_limit;//区域限制点集合

    zchxRadarRectMap  m_radarRectMap;//用于发送的回波矩形MAP
//    Radar_Rect_Map  m_radarRectMap_1;//用于发送的回波矩形MAP 点迹目标集合
//    Radar_Rect_Map  m_radarRectMap_2;//用于发送的回波矩形MAP 缓存回波


    zchxTrackPointMap m_radarPointMap;//key是航迹号
    zchxTrackPointMap m_radarPointMap_1;//保存最新数据
    zchxTrackPointMap m_radarPointMap_2;//保存最新数据
    zchxTrackPointMap m_radarPointMap_3;//保存最新数据
    zchxTrackPointMap m_radarPointMap_draw;//用于画编号
    QMap<int,RADAR_VIDEO_DATA> m_radarVideoMap;//key是azimuth,这里需要注意统一成中间点的角度,因为有时接收到的值有可能是奇数有时是偶数
    receive_statistics m_ri;//1_记录接收数据情况
    QTimer *m_pTimer; //1_定时重置丢包统计
    QTimer *m_pTimer_1; //1_定时显示丢包统计
    int m_next_spoke = -1;     // emulator next spoke

    std::vector<std::pair<double, double>> m_latLonVec;//雷达目标经纬度集合
    std::vector<std::pair<float, float>> m_latLonVecCartesian;//雷达目标经纬度集合(笛卡尔坐标)

    int m_clearRadarTrackTime;//单位是秒

    //zmq发送
    void *m_pVideoContext;
    void *m_pVideoLisher;
    int  m_uVideoSendPort;
    QString m_sVideoTopic;

    void *m_pTrackContext;
    void *m_pTrackLisher;
    int  m_uTrackSendPort;
    QString m_sTrackTopic;

    void *m_pRectContext;
    void *m_pRectLisher;
    int  m_uRectSendPort;
    QString m_sRectTopic;

    //
    ZCHXGetTrackProcess *m_pGetTrackProcess;

    //新科雷达解析
    CAsterixFormatDescriptor *m_pCAsterixFormatDescriptor;
    CAsterixFormat           *m_pCAsterixFormat;
    QString m_sPath;

    //限制区域
    bool m_bLimit;//是否设置限制区域
    QList<QPolygonF> m_landPolygon;
    QList<QPolygonF> m_seaPolygon;


    ZCHXDrawRadarVideo *m_DrawRadarVideo;
    QPixmap m_prePixmap;//生成余辉的前一张图片

    //监听
    ZmqMonitorThread *m_pRectMonitorThread;
    ZmqMonitorThread *m_pTrackMonitorThread;
    ZmqMonitorThread *m_pVideoMonitorThread;

    float         mRangeFactor;

    FILE *m_pFile;
    zchxTrackPointMap m_radarPointMap_a;//发送给web端的新目标
    zchxTrackPointMap m_radarPointMap_b;
    bool first;
    serialport *m_serialport;
    bool send_finish; //回波图片是否发送完成
    bool track_finish; //目标是否发送完成
    bool rect_finish; //矩形块是否发送完成
    RadarConfig* mRadarConfig;


    QList<int> mList;//距离需要移除的目标编号
//    QMap<int,int> mPushNumMap;//记录目标推送次数
    int mTargetMergeRadius;//距离判断
    QMap<int,QStringList> fMap;//保存浮标数据
    int floatRange;
    int radar_num;
    int hIniNum;

    //bool flag = 1;

    //目标集合
    QMap<int, QList<TrackNode>> mVideoTrack;
    bool finish_flag;

    //回波块识别
//    zchxVideoRects *mVideoRects;
//    zchxRectExtractionThread*  mRectExtractionThread;

//    bool finishiProcess = 1;
    int rectNum = 1; //重新赋值编号
    int objNum,maxNum;
    int mStartAzimuth  = -1;
    int nplan = 1;
    int upRad = 0;
    double mDirectionInvertHoldValue;
    bool   mAdjustCogEnabled;
};

#endif // ZCHXANALYSISANDSENDRADAR_H
