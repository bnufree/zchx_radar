#ifndef ZCHXRADARVIDEOPROCESSOR_H
#define ZCHXRADARVIDEOPROCESSOR_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QPointF>
#include <QPolygonF>
#include <QMutex>
#include <QPixmap>
#include <QColor>
#include "zchxfunction.h"
#include "zchxradarcommon.h"
#include "zchxmsgcommon.h"

class zchxRadarRectExtraction;
class zchxRadarTargetTrack;

typedef zchxRadarVideoTaskList      ZCHXRadarVideoProcessorData;

class ZCHXRadarVideoProcessor : public QThread
{
    Q_OBJECT
public:
    explicit ZCHXRadarVideoProcessor(int radar_id, QObject *parent = 0);
    ~ZCHXRadarVideoProcessor();
    void appendSrcData(const zchxRadarVideoTask& task);
    bool getProcessData(ZCHXRadarVideoProcessorData& task);
    void setTracker(zchxRadarTargetTrack* track) {mTracker = track;}
    void setRangeFactor(double factor);
    void setAvgShipSpeed(double speed);
    void setRadarSpr(double spr) {mRadarSpr = spr;}
    void setFilterAreaData(const QList<zchxMsg::filterArea>& list);
    void setFilterAreaEnabled(bool sts);

protected:
    void run();


signals:
    void signalSendRects(const zchxRadarRectDefList& list);
    void signalSendVideoPixmap(const QPixmap& img);

public slots: 
    void slotSetColor(int,int,int,int,int,int);
private:
    void    process(const ZCHXRadarVideoProcessorData& task);//绘制回波
    QColor  getColor(double dValue);//通过振幅值获取对应颜色值
    void    updateCycleCount();
private:
    double                          m_dCentreLon;
    double                          m_dCentreLat;
    QColor                          m_objColor1;
    QColor                          m_objColor2;
    QString                         m_radarSec;
    int                             mRadarID;
    zchxRadarRectExtraction*        mVideoExtractionWorker;
    bool                            mOutputImg;
    QList<ZCHXRadarVideoProcessorData>          mTaskList;
    QMutex                          mMutex;
    int                             mVideoCycleCount;
    zchxRadarTargetTrack            *mTracker;
    double                          mRangeFactor;
    double                          mAvgShipSpeed;
    double                          mRadarSpr;
};

#endif // ZCHXRADARVIDEOPROCESSOR_H
