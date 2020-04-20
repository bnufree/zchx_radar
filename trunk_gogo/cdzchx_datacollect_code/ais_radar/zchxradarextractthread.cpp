#include "zchxradarextractthread.h"
#include "Messages/Video.h"
#include "Messages/BinaryVideo.h"
#include "Algorithms/ExtractWithCentroiding.h"
#include "Algorithms/Threshold.h"
#include "Algorithms/OSCFAR.h"
#include "Algorithms/ScanCorrelator.h"
#include "Algorithms/ABTracker.h"
#include <QMutexLocker>

using namespace ZCHX;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Messages;

zchxRadarExtractionThread::zchxRadarExtractionThread(RadarConfig* cfg, QObject* parent) :
    QThread(parent),
    mCfg(cfg)
{
    ;
}

void zchxRadarExtractionThread::appendTask(const zchxVideoFrameList &list)
{
    QMutexLocker lock(&mTaskMutex);
    mFrameList.append(list);
}

bool zchxRadarExtractionThread::getTask(zchxVideoFrameList& list)
{
    list.clear();
    QMutexLocker lock(&mTaskMutex);
    if(mFrameList.size() > 0)
    {
        list = mFrameList.front();
        mFrameList.pop_front();
        return true;
    }
    return false;
}

void zchxRadarExtractionThread::run()
{
    zchxVideoFrameList wklist;
    //track
    ABTracker tracker(mCfg, false, 0);
    ScanCorrelator corr(mCfg);
    while (1) {
        if(getTask(wklist))
        {
            ExtractWithCentroiding extract_worker(mCfg);
            Extractions::Ref result;

            foreach (zchxVideoFrame frame, wklist) {
                if(frame.msgindex() == 0) tracker.setNewLoop(true);
                else if(frame.msgindex() == 4095) tracker.setNewLoop(false);
                Video::Ref video(Video::Make(frame));
                //通过振幅和阈值设定,将video转换为BinaryVideo
                BinaryVideo::Ref binary(BinaryVideo::Make("ExtractWithCentroiding", video));
        #if 1
                Threshold process(150);
                process.process(video, binary);
        #else
                ZCHX::Algorithms::OSCFAR os;
                os.process(video, binary);
        #endif
                binary->setRadarConfig(mCfg);

                //对BinaryVideo进行抽取
                Extractions::Ref extractions(Extractions::Make("Extract", binary));
                extract_worker.Binary2Extraction(binary, extractions);
                if(extractions->size() > 0)
                {
#if 1
                    //对抽取的目标进行校正,设定correct属性
                    corr.process(extractions, result);
                    //qDebug()<<"corrlation size:"<<result->getSize();
                    //tracker.slotRecvExtractions(result);
                    //result->clear();
#else
                    tracker.slotRecvExtractions(extractions);
#endif
                }
            }
            tracker.setNewLoop(false);
            if((!result) && result->getSize() > 0)
            {
                //track
                tracker.slotRecvExtractions(result);
            }

        } else
        {
            QThread::msleep(500);
        }

        //update trackpoint
        emit sendTrackPoint(tracker.getTrackPnts());
    }


}


