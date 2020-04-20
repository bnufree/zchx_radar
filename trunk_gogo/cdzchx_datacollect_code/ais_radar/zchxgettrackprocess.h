#ifndef ZCHXGETTRACKPROCESS_H
#define ZCHXGETTRACKPROCESS_H

#include <QObject>
#include <QThread>
#include "zchxfunction.h"
#include "zchxradarcommon.h"
#include "Messages/RadarConfig.h"
#include "Algorithms/ABTracker.h"
#include "Algorithms/ExtractWithCentroiding.h"
#include "Algorithms/ScanCorrelator.h"
#include <QPolygonF>

using namespace ZCHX::Algorithms;
using namespace ZCHX::Messages;

extern "C"

{

#include "ctrl.h"

}

typedef QList<SAzmData>             SAzmDataList;

//typedef QList<TrackObj>             TrackObjList;
class zchxRadarExtractionThread;

//#define         OUT_BINARY_FILE
class ZCHXGetTrackProcess : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXGetTrackProcess(RadarConfig* cfg,QObject *parent = 0);
    ~ZCHXGetTrackProcess();

    void setParseParamsUpdated(bool sts) {mParseParamUpdated = sts;}
    bool getFinish();
//    int  setOscfarParams(int window_size, int refer_index, int alpha){mOscfadAlpha = alpha; mOscfarThresholdIndex = refer_index; mOscfarWindowSize = window_size;}

signals:
    void sendTrack(const zchxTrackPoint& radarPoint);

    void sendTrack(const zchxTrackPointList& radarPoints);
    void getTrackProcessSignal(const zchxVideoFrameList& list);
    void sendProcessTrackInfo(int ext, int cor, int track);
    void signalShowTheLastPot(QList<QPointF>,QList<QPointF> );

public slots:
    void getTrackProcessSlot(const zchxVideoFrameList& list);
private:
    double m_dCentreLon;
    double m_dCentreLat;
    QThread m_workThread;
    int id;
    RadarConfig* mRadarConfig;
    ExtractWithCentroiding  *mExtractObj;
    ABTracker               *mTrackerObj;
    ScanCorrelator          *mCorrector;
    zchxTrackPointMap         mPnts;
    QHash<int, zchxVideoFrame>   mFrameHash;
    zchxRadarExtractionThread *mExtractThread;
//    int                     mVideoThresholdMode; //0:固定阈值;1:OSCFAR模式
//    int                     mConstantThresholdVal;
//    int                     mOscfarWindowSize;
//    int                     mOscfarThresholdIndex;
//    int                     mOscfadAlpha;
    bool                    mParseParamUpdated;      //界面参数是否操作了
    bool finish;


#ifdef OUT_BINARY_FILE
    QFile                   mLogFile;
#endif
};

#endif // ZCHXGETTRACKPROCESS_H
