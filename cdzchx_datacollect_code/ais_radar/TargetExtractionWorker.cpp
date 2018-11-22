#include "TargetExtractionWorker.h"
#include "side_car_parse/Messages/Video.h"
#include "side_car_parse/Algorithms/OSCFAR.h"
#include "side_car_parse/Algorithms/ScanCorrelator.h"
#include "side_car_parse/Algorithms/Threshold.h"
using namespace ZCHX::Messages;
using namespace ZCHX::Algorithms;

TargetExtractionWorker::TargetExtractionWorker(RadarConfig* cfg, QObject *parent) :
    QObject(parent),
    mRadarCfg(cfg),
    mExtractObj(new ZCHX::Algorithms::ExtractWithCentroiding),
    mTrackerObj(new ZCHX::Algorithms::ABTracker(cfg))
{
    mPnts.clear();
    moveToThread(&mThread);
    mThread.start();
}

void TargetExtractionWorker::slotRecvRawVideoDataList(const ITF_VideoFrameList &list)
{
    LOG_FUNC_DBG_START;
    if(!mExtractObj || !mTrackerObj) return;
    if(list.size() > 0)
    {
        foreach (ITF_VideoFrame frame, list) {
            //使用protobuf序列化
            QByteArray *bytes = new QByteArray();
            bytes->resize(frame.ByteSize());
            frame.SerializeToArray(bytes->data(), bytes->size());
            LOG_FUNC_DBG<<"index:"<<frame.msgindex();
            //振幅值赋值给video
            Video::Ref video(Video::Make(QSharedPointer<QByteArray>(bytes)));
//            for(int i=0; i<video->size();i++)
//            {
//                LOG_FUNC_DBG<<QString("video[%1] = %2").arg(i).arg(video[i]);
//            }
            //通过振幅和阈值设定,将video转换为BinaryVideo
            BinaryVideo::Ref binary(BinaryVideo::Make("OSCFAR", video));
//            OSCFAR far;
//            if(!far.process(video, binary))
//            {
//                LOG_FUNC_DBG<<"init bianry video failed.";
//                continue;
//            }

            Threshold process;
            process.process(video, binary);
//            for(int i=0; i<binary->size();i++)
//            {
//                LOG_FUNC_DBG<<QString("binary[%1] = %2").arg(i).arg(binary[i]);
//            }
            binary->setRadarConfig(mRadarCfg);

            //对BinaryVideo进行抽取
            Extractions::Ref extractions(Extractions::Make("Extract", binary));

            mExtractObj->process(binary, extractions);
            //LOG_FUNC_DBG<<"extraction size:"<<extractions->size();
            if(extractions->size() > 0)
            {
#if 0
                ScanCorrelator corr(mRadarCfg);
                //对抽取的目标进行校正,设定correct属性
                Extractions::Ref result;
                if(!corr.process(extractions, result)) continue;
                //目标进行标号等操作
                mTrackerObj->processInput(result, pnts);
#endif
                mTrackerObj->processInput(extractions, mPnts);
            }

        }
    }
    qDebug()<<" pnts size:"<<mPnts.size();
    if(mPnts.size() > 0)
    {
        emit signalSendTrackPoint(mPnts.values());
    }
    LOG_FUNC_DBG_END;
}

