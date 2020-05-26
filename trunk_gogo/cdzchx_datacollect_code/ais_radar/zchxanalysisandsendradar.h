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
#include "ZmqMonitorThread.h"
#include <QFile>
#include "../serialport.h"
//#include "QtXlsx/QtXlsx"
#include <QFileDialog>
#include <QTimer>
//#include "zchxvideorects.h"
#include <QGeoCoordinate>
//#include "zchxrectextractionthread.h"
#include "zchxradartargettrack.h"
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

class zchxRadarDataOutputMgr;

class ZCHXAnalysisAndSendRadar : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXAnalysisAndSendRadar(int id, QObject *parent = 0);
    ~ZCHXAnalysisAndSendRadar();


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
    void slotReadLimitData();
    void slotDrawCombinVideo(QList<TrackNode>);

    void showTrackNumSlot(bool);//显示目标编号
    void handleTimeout();  //1_超时处理函数
    void handleTimeout_1();  //1_超时处理函数
    void analysisLowranceRadarSlot(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading);//小雷达
    void slotRecvVideoImg(const QPixmap& img) {setRadarAfterglowPixmap(1, img);}
    void setRadarAfterglowPixmap(const int uIndex,
                                 const QPixmap &videoPixmap,
                                 const QPixmap &objPixmap = QPixmap(),
                                 const QPixmap &prePixmap = QPixmap());//接收余辉图片
    void setRangeFactor(double factor);
    //void send_new_track_slot(QPolygonF,QPolygonF);//发送给web端
    void slotGetGpsData(double, double);//实时更新从GPS传入的经纬度坐标
    void updateFloatSlot();//更新浮标配置
    void sendRadarRectPixmap(const zchxRadarRectMap& map);
    void slotSendComTracks(const zchxRadarSurfaceTrack&);
    float getAngle(float,float,float,float);
    void slotTrackMap(QMap<int,QList<TrackNode>>);
    void slotSetRadarType(int type);
private:
    void processVideoData(bool rotate = true);
    void InitializeLookupData();
private:
    QThread             m_workThread;
    QString             str_radar;
    int                 m_uSourceID;
    int                 cell_num;
    double              m_dCentreLon;
    double              m_dCentreLat;
    double              m_dRadius;//半径
    int                 m_uLoopNum;//余辉循环值
    QString             sRadarType;//读取接入雷达型号 4G/6G


    QMap<int,RADAR_VIDEO_DATA> m_radarVideoMap;//key是azimuth,这里需要注意统一成中间点的角度,因为有时接收到的值有可能是奇数有时是偶数
    receive_statistics m_ri;//1_记录接收数据情况
    QTimer *m_pTimer; //1_定时重置丢包统计
    QTimer *m_pTimer_1; //1_定时显示丢包统计
    int m_next_spoke = -1;     // emulator next spoke



    //zmq发送
    zchxRadarDataOutputMgr     *mRadarOutMgr;
    QString                     mRadarRectTopic;
    int                         mRadarRectPort;
    QString                     mRadarVideoTopic;
    int                         mRadarVideoPort;
    QString                     mRadarTrackTopic;
    int                         mRadarTrackPort;


    ZCHXRadarVideoProcessor     *m_VideoProcessor;                  //回波处理成矩形目标点
    zchxRadarTargetTrack        *m_targetTrack;                     //矩形目标点进行目标跟踪
    QPixmap m_prePixmap;//生成余辉的前一张图片

    float         mRangeFactor;
    QMap<int,QStringList> fMap;//保存浮标数据
    int floatRange;
    int radar_num;
    int hIniNum;

    int mStartAzimuth  = -1;
    int upRad = 0;
    int mRadarType;
    bool mUseNativeRadius;
    int mDopplerVal;
};

#endif // ZCHXANALYSISANDSENDRADAR_H
