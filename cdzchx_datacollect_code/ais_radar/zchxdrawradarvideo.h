#ifndef ZCHXDRAWRADARVIDEO_H
#define ZCHXDRAWRADARVIDEO_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QPointF>
#include "zchxfunction.h"

class ZCHXDrawRadarVideo : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXDrawRadarVideo(QObject *parent = 0);
    ~ZCHXDrawRadarVideo();
    bool getIsProcessing();
    void setAfterglowType(const int uIndex);
    void setLimit(bool bLimit);
    void setDistance(double dDis);
    void setLimitArea(const QList<QPolygonF> &landPolygon,const QList<QPolygonF> &seaPolygon);
signals:
    //void signalDrawRadarVideo(const Afterglow &dataAfterglow);
    void signalDrawAfterglow(const Afterglow &dataAfterglow);//绘制余辉及回波
    void signalRadarVideoPixmap(const QPixmap &objPixmap);
    void signalRadarAfterglowPixmap(const int uIndex,const QPixmap &videoPixmap,
                                    const QPixmap &objPixmap,const QPixmap &processPixmap);
public slots:

    void slotDrawAfterglow(const Afterglow &dataAfterglow);//绘制余辉及回波
    void slotSetPenwidth(int);//设置笔的宽度
private:
    QPixmap drawRadarVideoPixmap(const Afterglow &dataAfterglow);//绘制回波
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
    QThread                              m_threadWork;
    QMap<int,QColor> m_colorMap;//颜色map

    bool m_bProcessing;//是否线程在处理中

    //绘制余辉参数
    int m_uAfterglowType;//1,3,6,12
    int m_uAfterglowIndex;
    double m_distance;
    bool m_bLimit;//是否设置限制区域
    QList<QPolygonF> m_landPolygon;
    QList<QPolygonF> m_seaPolygon;
    int pen_width;
};

#endif // ZCHXDRAWRADARVIDEO_H
