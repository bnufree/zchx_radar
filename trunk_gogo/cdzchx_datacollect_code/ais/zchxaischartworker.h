#ifndef ZCHXAISCHARTWORKER_H
#define ZCHXAISCHARTWORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QSvgGenerator>
#include "ais_radar/zchxfunction.h"

class QTimer;

class zchxAisChartWorker : public QObject
{
    Q_OBJECT
public:
    explicit zchxAisChartWorker(double lat, double lon, bool fixed_radius, int radius, QObject *parent = 0);
    ~zchxAisChartWorker();

signals:
    void signalSendPixmap(const QByteArray& img, int width, int height, double range, const QString& format);
public slots:
    void    slotAppendAisLatLon(double lat, double lon);
    void    slotUpdateWithList();
    void    slotMakePixMap();
    void    slotUpdateCenter(double lat, double lon);
private:
    QList<Latlon>  getWorkLL();
    void           paintImg(QPaintDevice* dev = 0);
private:
    double      mCenterLat;
    double      mCenterLon;
    int         mImgWidth;
    int         mImgHeight;
    double      mRangeFactor;
    double      mCurRange;
    double      mMaxRange;
    QMap<QString, QList<Latlon>>        mPixPosMap;             //每一个像素点对应的经纬度
    QList<Latlon>                       mWorkLL;
    QThread                             mWorkThread;
    QTimer*                             mHeartTimer;
    QMutex                              mMutex;
    bool                                mRadiusFixed;
    int                                 mFixedRadius;

};

#endif // ZCHXAISCHARTWORKER_H
