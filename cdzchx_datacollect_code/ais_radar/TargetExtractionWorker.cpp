#include "TargetExtractionWorker.h"
#include "side_car_parse/Messages/Video.h"
#include "side_car_parse/Algorithms/OSCFAR.h"
#include "side_car_parse/Algorithms/ScanCorrelator.h"
using namespace ZCHX::Messages;
using namespace ZCHX::Algorithms;

TargetExtractionWorker::TargetExtractionWorker(RadarConfig* cfg, QObject *parent) :
    QObject(parent),
    mRadarCfg(cfg),
    mExtractObj(new ZCHX::Algorithms::ExtractWithCentroiding),
    mTrackerObj(new ZCHX::Algorithms::ABTracker)
{
    moveToThread(&mThread);
    mThread.start();
}

void TargetExtractionWorker::slotRecvRawVideoDataList(const ITF_VideoFrameList &list)
{
    LOG_FUNC_DBG_START;
    if(!mExtractObj || !mTrackerObj) return;
    QMap<int, TrackPoint> pnts;
    if(list.size() > 0)
    {
        foreach (ITF_VideoFrame frame, list) {
            //使用protobuf序列化
            QByteArray *bytes = new QByteArray();
            bytes->resize(frame.ByteSize());
            frame.SerializeToArray(bytes->data(), bytes->size());
            //振幅值赋值给video
            Video::Ref video(Video::Make(QSharedPointer<QByteArray>(bytes)));
            //通过振幅和阈值设定,将video转换为BinaryVideo
            BinaryVideo::Ref binary(BinaryVideo::Make("OSCFAR", video));
            OSCFAR far;
            if(!far.process(video, binary)) continue;
            //对BinaryVideo进行抽取
            Extractions::Ref extractions;
            if(!mExtractObj->process(binary, extractions)) continue;
            ScanCorrelator corr(mRadarCfg);
            //对抽取的目标进行校正,设定correct属性
            Extractions::Ref result;
            if(!corr.process(extractions, result)) continue;
            //目标进行标号等操作
            mTrackerObj->processInput(result, pnts);
        }
    }
    emit signalSendTrackPoint(pnts.values());
    LOG_FUNC_DBG_END;
}

