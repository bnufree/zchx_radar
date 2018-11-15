#ifndef ZCHXDRAWVIDEORUNNABLE_H
#define ZCHXDRAWVIDEORUNNABLE_H

#include <QObject>
#include <QRunnable>
#include <QMutex>
#include <QMutexLocker>
#include "zchxfunction.h"
#include <QColor>
#include <QPolygonF>
class ZCHXDrawVideoRunnable : public QObject,public QRunnable
{
    Q_OBJECT
public:
    explicit ZCHXDrawVideoRunnable(QObject *parent = 0);
    void run();
    void setAfterglow(const Afterglow &dataAfterglow);
signals:
    void signalRadarVideoPixmap(const QPixmap &objPixmap);

private:
    void initColor();//初始化颜色组
    QColor getColor(double dValue);//通过振幅值获取对应颜色值
    //判断是否在目标附近
//    bool isInRadarPointRange(const double dLat,const double dLon,
//                             const std::vector<std::pair<double, double>> path);


    bool inLimitArea(const double dCentreLat, const double dCentreLon,
                     const double dAzimuth,const int uPosition,
                     const double dStartRange, const double dRangeFactor);
private:
    QMutex m_mutex;

    Afterglow m_dataAfterglow;

    QMap<int,QColor> m_colorMap;//颜色map
    bool m_bLimit;//是否设置限制区域
    QList<QPolygonF> m_landPolygon;
    QList<QPolygonF> m_seaPolygon;
};

#endif // ZCHXDRAWVIDEORUNNABLE_H
