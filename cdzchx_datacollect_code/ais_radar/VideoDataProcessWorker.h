#ifndef VIDEODATAPROCESSWORKER_H
#define VIDEODATAPROCESSWORKER_H

#include <QObject>
#include <QThread>
#include "common.h"
#include "protobuf/ZCHXRadar.pb.h"
#include "side_car_parse/Messages/RadarConfig.h"
#include "TargetExtractionWorker.h"

using namespace ZCHX::Messages;
using namespace com::zhichenhaixin::proto;

//transfer raw data to video frame data
class VideoDataProcessWorker : public QObject
{
    Q_OBJECT
public:
    explicit VideoDataProcessWorker(RadarConfig* cfg, QObject *parent = 0);

signals:
    void    signalSendVideoFrameData(const ITF_VideoFrame& data);
    void    signalSendVideoFrameDataList(const ITF_VideoFrameList& dataList);
    void    signalSendTrackPoint(const QList<TrackPoint>& list);

public slots:
    void    slotRecvVideoRawData(const QByteArray& raw);
private:
    QThread     mThread;
    RadarConfig *mRadarCfg;
    TargetExtractionWorker  *mExtract;
    QMap<int, ITF_VideoFrame> mVideoMap;
    
};

#endif // VIDEODATAPROCESSWORKER_H
