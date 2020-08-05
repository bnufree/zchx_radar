#ifndef ZCHXDRAWRADARVIDEO_H
#define ZCHXDRAWRADARVIDEO_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QPointF>
#include "zchxfunction.h"
#include <QPolygonF>
//#include "QtXlsx/QtXlsx"
#include "zchxradarcommon.h"

class zchxRadarRectExtraction;
class ZCHXDrawRadarVideo : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXDrawRadarVideo(int radar_id, QObject *parent = 0);
    ~ZCHXDrawRadarVideo();
    bool getIsProcessing();
    void setAfterglowType(const int uIndex);
    void setLimit(bool bLimit);
    void setDistance(double dDis);
    void setLimitArea(const QList<QPolygonF> &landPolygon,const QList<QPolygonF> &seaPolygon);


signals:
    //void signalDrawRadarVideo(const Afterglow &dataAfterglow);
    void signalDrawAfterglow(double nRange, const Afterglow &dataAfterglow,  zchxTrackPointMap map,bool rotate = true);//绘制余辉及回波
    void signalRadarVideoPixmap(const QPixmap &objPixmap);
    void signalRadarAfterglowPixmap(const int uIndex,const QPixmap &videoPixmap,
                                    const QPixmap &objPixmap,const QPixmap &processPixmap);
    void signalRadarVideoAndTargetPixmap(const QPixmap &videoPixmap,const Afterglow &dataAfterglow);
    void SignalShowTrackNum(bool);
    void signalSendLatVec(std::vector<std::pair<double, double>> ,QList<QPointF>);
    void signalSendRects(const zchxRadarRectDefList& list);

public slots:
    //void land_limit_slot(QPolygonF);//在采集器上画限制区域
    void slotDrawAfterglow(double nRange, const Afterglow &dataAfterglow,zchxTrackPointMap map, bool rotate = true);//绘制余辉及回波
    void slotSetPenwidth(int);//设置笔的宽度
    void showTrackNumSlot(bool);
    void slotTrackMap(QMap<int,QList<TrackNode> >,int);
    void slotSetColor(int,int,int,int,int,int);
    void showTheLastPot(QList<QPointF>,QList<QPointF>);
    void getRects(zchxRadarRectMap);
    bool inLimitAreaForTrack(const double dLat, const double dLon);
    void drawCombineVideo(QList<TrackNode>);
    void setStradar(QString str);
private:
    void gpsPoint2DescartesPoint(const double latitude, const double longitude, const double altitude, double &x, double &y, double &z);//GPS 转笛卡尔
    QPixmap drawRadarVideoPixmap(const Afterglow &dataAfterglow, bool roate,double nRange);//绘制回波
    QPixmap drawRadarVideoPixmap2(const Afterglow &dataAfterglow, bool roate,double nRange);//绘制回波
    QPixmap drawRadarVideoAndTargetPixmap(zchxTrackPointMap map,double nRange, const Afterglow &dataAfterglow, QPixmap pixmap, bool rotate);//绘制回波(带目标)
    void initColor();//初始化颜色组
    QColor getColor(double dValue);//通过振幅值获取对应颜色值
    QColor getColor_1(double dValue);//通过振幅值获取pre对应颜色值
    //判断是否在目标附近
    bool isInRadarPointRange(const double dLat,const double dLon,
                             const std::vector<std::pair<double, double>> path);
    QPixmap processPixmap(const QPixmap &objPixmap);


    bool inLimitArea(const double dCentreLat, const double dCentreLon,
                     const double dAzimuth,const int uPosition,
                     const double dStartRange, const double dRangeFactor);
private:
    QThread m_threadWork;
    QMap<int,QColor> m_colorMap;//颜色map

    bool m_bProcessing;//是否线程在处理中

    //绘制余辉参数
    int m_uAfterglowType;//1,3,6,12
    int m_uAfterglowIndex;
    double m_distance;
    bool m_bLimit;//是否设置限制区域
    QList<QPolygonF> m_landPolygon;
    QList<QPolygonF> m_seaPolygon;
    int pen_width;//画笔宽度
    QPolygonF land_limit;//区域限制点集合
    QPolygonF sea_limit;//区域限制点集合
    double m_dCentreLon;
    double m_dCentreLat;
    bool m_showNum;//是否显示编号
    QPolygonF videoQPolygonF;
    //QMap<int,QList<QPointF> > mVideoTrackmap;
    QMap<int,QList<TrackNode>> mVideoTrackmap;
    int videoNum;
    QColor objColor1;
    QColor objColor2;
    QList<QPointF> LastPotList1;
    QList<QPointF> LastPotList2;
    zchxRadarRectMap  m_radarRectMap;//用于发送的回波矩形MAP
    //回波矩形块集合
    zchxRadarRectList mRadar_Rect_List_1;
    bool sendVideoFlag = 0;
    //int track_radius;
    QPixmap videoPixmap_1,videoPixmap_2,videoPixmap_3;//用于记录前2次的回波图像
    QList<TrackNode> combineVideoList;
    int id;
	QString str_radar;
    int                             mRadarID;
    zchxRadarRectExtraction*        mVideoExtractionWorker;
    bool                            mOutputImg;
};

#endif // ZCHXDRAWRADARVIDEO_H
