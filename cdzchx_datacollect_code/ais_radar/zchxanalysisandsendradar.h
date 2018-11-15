#ifndef ZCHXANALYSISANDSENDRADAR_H
#define ZCHXANALYSISANDSENDRADAR_H

#include <QObject>
#include <QThread>
#include "zchxfunction.h"
#include "protobuf/ZCHXRadar.pb.h"
#include "protobuf/ZCHXRadarVideo.pb.h"
#include "zmq.h"
#include "zchxdrawradarvideo.h"
#include <QPointF>
#include <QPolygonF>
#include <QList>
#include <QPixmap>
#include "asterixformat.hxx"
#include "asterixformatdescriptor.hxx"
#include "ZmqMonitorThread.h"
#include <QFile>
#include "zchxgettrackprocess.h"
#include <QTimer>
#define SPOKES (4096)
class ZCHXAnalysisAndSendRadar : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXAnalysisAndSendRadar(int uSourceID = 1,QObject *parent = 0);
    ~ZCHXAnalysisAndSendRadar();

    closeTT();

signals:
    void analysisLowranceRadarSignal(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading);//小雷达
    void analysisCatRadarSignal(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading,const QString &sRadarType);//新科雷达
    void signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void signalClientInout(const QString& ip, const QString& name, int port, int inout);

    void startTrackProcessSignal(SAzmData sAzmData);
    void startTrackProcessSignal(const SAzmDataList& sAzmData);
    void show_info(QString, double, float);//1_打印txt信号
    void show_video(int, int);//1_打印目标个数
    void show_statistics(int,int,int,int,int);//1_打印统计丢包率
    void show_missing_spokes(QString);//1_打印丢失扫描线信息
    void show_received_spokes(QString);//1_打印所有扫描线信息
    void set_pen_width(int);//1_设置笔宽度
    void signalRadiusFactorUpdated(double radius, double factor);
public slots:
    void handleTimeout();  //1_超时处理函数
    void handleTimeout_1();  //1_超时处理函数
    void analysisLowranceRadarSlot(const QByteArray &sRadarData,
                               int uLineNum,int uCellNum,int uHeading);//小雷达
    void analysisCatRadarSlot(const QByteArray &sRadarData,
                               int uLineNum, int uCellNum, int uHeading, const QString &sRadarType);//新科雷达
    void setRadarAfterglowPixmap(const int uIndex, const QPixmap &videoPixmap, const QPixmap &objPixmap,
                             const QPixmap &prePixmap);//接收余辉图片
    void sendTrackSlot(int uKey, ITF_Track_point radarPoint);
    void sendTrackSlot(const TrackObjList& list);
    void setRangeFactor(double factor);
private:
    void readRadarLimitFormat();//读取雷达限制区域文件
    void processVideoData();

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
private:
    QThread m_workThread;

    int m_uSourceID;
    double m_dCentreLon;
    double m_dCentreLat;
    double m_dRadius;//半径
    double m_distance;
    int m_uLoopNum;//余辉循环值
    int d_32;
    QString m_limit_file;//读取限制区域文件

    Radar_Track_Map m_radarPointMap;//key是航迹号
    Radar_Track_Map m_radarPointMap_1;//保存最新数据
    QMap<int,RADAR_VIDEO_DATA> m_radarVideoMap;//key是azimuth
    receive_statistics m_ri;//1_记录接收数据情况
    QTimer *m_pTimer; //1_定时重置丢包统计
    QTimer *m_pTimer_1; //1_定时显示丢包统计
    int m_next_spoke = -1;     // emulator next spoke

    std::vector<std::pair<double, double>> m_latLonVec;//雷达目标经纬度集合

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
    ZmqMonitorThread *m_pTrackMonitorThread;
    ZmqMonitorThread *m_pVideoMonitorThread;

    float         mRangeFactor;

    FILE *m_pFile;
};

#endif // ZCHXANALYSISANDSENDRADAR_H
