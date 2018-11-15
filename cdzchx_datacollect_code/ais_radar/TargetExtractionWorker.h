#ifndef TARGETEXTRACTIONWORKER_H
#define TARGETEXTRACTIONWORKER_H

#include <QObject>
#include <QThread>
#include "common.h"
#include "protobuf/ZCHXRadar.pb.h"
#include "side_car_parse/Messages/RadarConfig.h"
#include "side_car_parse/Algorithms/ExtractWithCentroiding.h"
#include "side_car_parse/Algorithms/ABTracker.h"

using namespace ZCHX::Algorithms;
using namespace ZCHX::Messages;

class TargetExtractionWorker : public QObject
{
    Q_OBJECT
public:
    explicit TargetExtractionWorker(RadarConfig* cfg, QObject *parent = 0);

signals:
    void    signalSendTrackPoint(const QList<TrackPoint>& list);
public slots:
    void    slotRecvRawVideoDataList(const ITF_VideoFrameList& list);
private:
    QThread mThread;
    RadarConfig *mRadarCfg;
    ExtractWithCentroiding  *mExtractObj;
    ABTracker               *mTrackerObj;
};

#endif // TARGETEXTRACTIONWORKER_H
