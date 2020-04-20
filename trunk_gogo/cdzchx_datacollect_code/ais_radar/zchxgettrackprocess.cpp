#include "zchxgettrackprocess.h"
#include <QDebug>
#include <QLibrary>
#include <QDateTime>
#include <QMutex>
#include "zchxradarextractthread.h"
#include "Algorithms/Threshold.h"
#include "Algorithms/OSCFAR.h"
#include "QtXlsx/QtXlsx"
#include "../profiles.h"
#include <QList>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
////////////////////////////////////////////////////////////////////////////////

typedef int(*FUN2)(struct SAzmData* psScan, int* pSit);
extern  FUN2 Tracking_Fun = NULL;

////////////////////////////////////////////////////////////////////////////////////

ZCHXGetTrackProcess::ZCHXGetTrackProcess(RadarConfig* cfg, QObject *parent):
    QObject(parent),
    mRadarConfig(cfg),
    mExtractObj(0),
    mTrackerObj(0),
    mCorrector(0),
    mExtractThread(0),
    mParseParamUpdated(false)
//    mVideoThresholdMode(1),
//    mConstantThresholdVal(100),
//    mOscfadAlpha(1.5),
//    mOscfarThresholdIndex(50),
//    mOscfarWindowSize(64)
{
//    qDebug()<<__FUNCTION__<<mVideoThresholdMode<<mConstantThresholdVal;
    if(mRadarConfig && mRadarConfig->getParseMode() == 2)
    {
#if 1
        //mExtractObj = new ExtractWithCentroiding;
        mTrackerObj = new ABTracker(cfg);
        mCorrector = new ScanCorrelator(cfg);
#else
        mExtractThread = new zchxRadarExtractionThread(mRadarConfig);
        connect(mExtractThread, SIGNAL(sendTrackPoint(zchxTrackPointList)), this, SIGNAL(sendTrack(zchxTrackPointList)));
#endif
    }
    id = cfg->getID();
    finish = 1;
    m_dCentreLat = mRadarConfig->getSiteLat();
    m_dCentreLon = mRadarConfig->getSiteLong();
    qRegisterMetaType<SAzmData>("SAzmData");
    qRegisterMetaType<zchxTrackPoint>("const zchxTrackPoint&");
    qRegisterMetaType<SAzmDataList>("const SAzmDataList&");
    qRegisterMetaType<zchxTrackPointList>("const zchxTrackPointList&");
    qRegisterMetaType<zchxVideoFrameList>("const zchxVideoFrameList&");
    qRegisterMetaType<QList<TrackNode>>("QList<TrackNode>");
    qRegisterMetaType<QMap<int,RADAR_VIDEO_DATA>>("QMap<int,RADAR_VIDEO_DATA>");

    //调用小雷达目标库
    QLibrary lib("Record.dll");
    if (lib.load()) {
        qDebug() << "load ok!";

        Tracking_Fun = (FUN2)lib.resolve("?Tracking@@YAHPEAUSAzmData@@PEAH@Z");

        if (Tracking_Fun) {
            qDebug() << "load Tracking ok!";
        } else {
            qDebug() << "resolve Tracking error!";
        }
    } else {
        qDebug() << "load error!";
    }

    connect(this, SIGNAL(getTrackProcessSignal(zchxVideoFrameList)),
            this, SLOT(getTrackProcessSlot(zchxVideoFrameList)));
    moveToThread(&m_workThread);
    m_workThread.start();


}

ZCHXGetTrackProcess::~ZCHXGetTrackProcess()
{
#ifdef OUT_BINARY_FILE
    if(mLogFile.isOpen()) mLogFile.close();
#endif
    if(m_workThread.isRunning()) {
        m_workThread.quit();
    }

    m_workThread.terminate();
}

bool ZCHXGetTrackProcess::getFinish()
{
    return finish;
}

//第三方解析函数
void ZCHXGetTrackProcess::getTrackProcessSlot(const zchxVideoFrameList &list)
{
    finish = 0;
    //cout<<"第三方解析函数"<<id<<list.size();
    if(list.size() == 0) return;

    //cout<<"跟踪:"<<id;
    if(!mTrackerObj || !mCorrector) return;//有一个为空就return
    //重新更新一下校正的参数
    mCorrector->updateCorrtor();//比较新的参数和旧的参数有什么不同
    ExtractWithCentroiding extract(mRadarConfig);
    Extractions::Ref extractions = 0;
    zchxTrackPointList radarPoints;
    foreach (zchxVideoFrame frame, list)
    {
        Video::Ref video(Video::Make(frame));
        //通过振幅和阈值设定,将video转换为BinaryVideo
        BinaryVideo::Ref binary(BinaryVideo::Make("ZCHXGetTrackProcess", video));
        if(mRadarConfig->getThresholdMode() == 0)
        {
            cout<<"1";
            Threshold process(mRadarConfig->getConstantHoldValue());
            process.process(video, binary);
        } else
        {
            //cout<<"2";
            ZCHX::Algorithms::OSCFAR os;
            os.setWindowSize(mRadarConfig->getOscfarWidowSize());
            os.setAlpha(mRadarConfig->getOscfarAlphaCoeff());
            os.setThresholdIndex(mRadarConfig->getOscfarThresHoldIndex());
            os.process(video, binary);
        }
        binary->setRadarConfig(mRadarConfig);
        //对BinaryVideo进行抽取
        extract.Binary2Extraction(binary, extractions);
    }
    if(extractions)
    {
        int ext_size = 0, corr_size =0, track_size = 0;
        ext_size = extractions->size();
        if(extractions->size() > 0)
        {
#if 0
            //ScanCorrelator corr(mRadarConfig);
            //对抽取的目标进行校正,设定correct属性
            Extractions::Ref result;
            mCorrector->process(extractions, result);
            corr_size = result->size();
            //目标进行标号等操作
            if(corr_size > 0)mTrackerObj->Extraction2Track(result);
#else
            corr_size = ext_size;
            mTrackerObj->Extraction2Track(extractions);
#endif
        }
        radarPoints = mTrackerObj->getTrackPnts();
        track_size = radarPoints.size();
        //qDebug()<<"track point size:"<<track_size;
        emit sendProcessTrackInfo(ext_size, corr_size, track_size);
    }
    // qDebug()<<"对BinaryVideo进行抽取";
    //emit sendTrack(mTrackerObj->getTrackPnts());
    emit sendTrack(radarPoints);
    finish = 1;
}
