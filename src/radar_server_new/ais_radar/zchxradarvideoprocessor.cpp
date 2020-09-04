#include "zchxradarvideoprocessor.h"
//#include <QDebug>
#include <math.h>
#include <QMutex>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QGeoCoordinate>
#include <QCoreApplication>
#include <QtMath>
#include "Log.h"
#include "profiles.h"
#include "zchxRadarRectExtraction.h"
#include "zchxradartargettrack.h"

#define cout if(1) qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
//static QMap<int,QMap<int, QList<TrackNode>>> mVideoMap;
static QMap<int, QList<TrackNode>> mVideoMap;
static QMap<int, int> mRangeMap;


ZCHXRadarVideoProcessor::ZCHXRadarVideoProcessor(int radar_id, QObject *parent)
    : QThread(parent)
    , mTracker(0)
    , mRadarSpr(60.0/24)
{
    mRadarID = radar_id;
    qRegisterMetaType<zchxRadarRectDefList>("const zchxRadarRectDefList&");
    int a1 =(Utils::Profiles::instance()->value("Color","color1_R").toInt());
    int a2 = (Utils::Profiles::instance()->value("Color","color1_G").toInt());
    int a3 = (Utils::Profiles::instance()->value("Color","color1_B").toInt());
    int b1 = (Utils::Profiles::instance()->value("Color","color2_R").toInt());
    int b2 = (Utils::Profiles::instance()->value("Color","color2_G").toInt());
    int b3 = (Utils::Profiles::instance()->value("Color","color2_B").toInt());
    m_objColor1 = QColor(a1,a2,a3);
    m_objColor2 = QColor(b1,b2,b3);

    //抽出对象初始化
    m_radarSec = QString("Radar_%1").arg(mRadarID);
    mVideoCycleCount = Utils::Profiles::instance()->value(m_radarSec, "video_cycle_or", 1).toInt();
    Utils::Profiles::instance()->setDefault(m_radarSec,  "OutputImage", 0);
    mOutputImg = Utils::Profiles::instance()->value(m_radarSec,  "OutputImage").toBool();
    m_dCentreLat = Utils::Profiles::instance()->value(m_radarSec,  "Centre_Lat").toDouble();
    m_dCentreLon = Utils::Profiles::instance()->value(m_radarSec,  "Centre_Lon").toDouble();
    mVideoExtractionWorker = new zchxRadarRectExtraction(m_dCentreLat, m_dCentreLon, mRadarID);
    double minArea = Utils::Profiles::instance()->value(m_radarSec,  "track_min_area").toDouble();
    double maxArea = Utils::Profiles::instance()->value(m_radarSec,  "track_max_area").toDouble();
    double minLen = Utils::Profiles::instance()->value(m_radarSec,  "track_min_radius").toDouble();
    double maxLen = Utils::Profiles::instance()->value(m_radarSec,  "track_radius").toDouble();
    mVideoExtractionWorker->setTargetAreaRange(minArea, maxArea);
    mVideoExtractionWorker->setTargetLenthRange(minLen, maxLen);

    //
    setRangeFactor(10.0);
    setAvgShipSpeed(5.0);

    setStackSize(64000000);
}

void ZCHXRadarVideoProcessor::setFilterAreaData(const QList<zchxMsg::filterArea> &list)
{
    if(mVideoExtractionWorker) mVideoExtractionWorker->setFilterAreaData(list);
}

void ZCHXRadarVideoProcessor::setFilterAreaEnabled(bool sts)
{
    if(mVideoExtractionWorker) mVideoExtractionWorker->setFilterAreaEnabled(sts);
}

ZCHXRadarVideoProcessor::~ZCHXRadarVideoProcessor()
{
    if(mVideoExtractionWorker)
    {
        delete mVideoExtractionWorker;
    }
}

void ZCHXRadarVideoProcessor::setRangeFactor(double factor)
{
    mRangeFactor = factor;
    updateCycleCount();
    if(mTracker) mTracker->setRangefactor(factor);
}

void ZCHXRadarVideoProcessor::setAvgShipSpeed(double speed)
{
    mAvgShipSpeed = speed;
    updateCycleCount();
}

void ZCHXRadarVideoProcessor::updateCycleCount()
{
//    if(mAvgShipSpeed > 0 && mRangeFactor > 0)
//    {
//        double count_sec = mRangeFactor / mAvgShipSpeed;
//        int count = qCeil(count_sec / mRadarSpr);
//        if(mVideoCycleCount != count)
//        {
//            mVideoCycleCount = count;
////            qDebug()<<"video Cycle count:"<<mVideoCycleCount;
//        }
//    }
//    mVideoCycleCount = 1;
}

void ZCHXRadarVideoProcessor::appendSrcData(const zchxRadarVideoTask &task)
{
    QMutexLocker locker(&mMutex);
    if(mTaskList.size() == 0)
    {
        ZCHXRadarVideoProcessorData data;
        data.append(task);
        mTaskList.append(data);
    } else
    {
        ZCHXRadarVideoProcessorData &data = mTaskList.last();
        qDebug()<<"last size:"<<data.size()<<mVideoCycleCount;
        if(data.size() == mVideoCycleCount)
        {
            //数据已经满足多个周期回波叠加, 构建新的回波数据,新的回波数据以以前的回波数据作为基础
            ZCHXRadarVideoProcessorData newData;
#if 1
            for(int i=1; i<mVideoCycleCount; i++)
            {
                newData.append(data[i]);
            }
#endif
            newData.append(task);
            mTaskList.append(newData);
        } else
        {
            data.append(task);
        }

        qDebug()<<"task list size:"<<mTaskList.size();
    }
}

bool ZCHXRadarVideoProcessor::getProcessData(ZCHXRadarVideoProcessorData& task)
{
    QMutexLocker locker(&mMutex);
    if(mTaskList.size() == 0) return false;
    ZCHXRadarVideoProcessorData& temp = mTaskList.last();
    if(temp.size() == mVideoCycleCount)
    {
        task = temp;
#if 1
        //移除以前的任务,保留最近的一个任务,便于下一个回波过来的时候合成新的任务
        while (mTaskList.size() > 1) {
            mTaskList.takeFirst();
        }
        //将任务的第一个回波删除
        temp.takeFirst();
#else
        int size = mTaskList.size();
        qDebug()<<"remove unprocessed video task size:"<<size-1;
        mTaskList.clear();
#endif
        return true;
    }
    return false;
}

void ZCHXRadarVideoProcessor::run()
{
    while (true) {
        //获取当前的任务
        ZCHXRadarVideoProcessorData task;
        if(!getProcessData(task))
        {
            msleep(1000);
            continue;
        }
        //开始进行处理
        zchxTimeElapsedCounter counter("process video image");
        process(task);
    }
}

QColor ZCHXRadarVideoProcessor::getColor(double dValue)
{
    //cout<<"getColor";
    double m_dMinZ = 0;
    double m_dMaxZ = 255;
    if(dValue>m_dMaxZ)
    {
        //return QColor(255,0,0);
        cout<<"颜色1";
        return QColor(252,241,1);
    }
    if(dValue<m_dMinZ)
    {
        cout<<"颜色2";
        //return QColor(0,0,255);
        return QColor(19,118,61);
    }
    if(m_dMaxZ-m_dMinZ<0.0001)
    {
        cout<<"颜色3";
        return QColor(252,241,1);
    }
    double dTemp = (dValue-m_dMinZ)/(m_dMaxZ-m_dMinZ);
    int uIndex = dTemp*100;
    if(uIndex<0)
    {
        cout<<"颜色4";
        return QColor(19,118,61);
    }
    if(uIndex>=100)
    {
        //qDebug()<<"颜色5";
        return m_objColor1;//QColor(6,144,36);
    }
    //cout<<"颜色6";
    //QColor objColor = m_colorMap[uIndex];
    return  m_objColor2;//QColor(103,236,231);
}

void ZCHXRadarVideoProcessor::slotSetColor(int a1,int a2,int a3,int b1,int b2,int b3)
{
    m_objColor1 = QColor(a1,a2,a3);
    m_objColor2 = QColor(b1,b2,b3);
}

//画回波255位黄色,1-244振幅为蓝色
void ZCHXRadarVideoProcessor::process(const ZCHXRadarVideoProcessorData& task)
{
    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(task.size() == 0) return;
    //首先将所有任务的回波都合成一个回波图形
    QMap<int,RADAR_VIDEO_DATA> RadarVideo = task[0].m_RadarVideo;
    int video_index = task[0].m_IndexT;
#if 1
    for(int i=0; i<task.size(); i++)
    {
        qDebug()<<"task time:"<<task[i].m_IndexT;
    }
    qDebug()<<"data time:"<<task[0].m_IndexT<<task.size();
#endif
    for(int i=1; i<task.size(); i++)
    {
        QMap<int,RADAR_VIDEO_DATA> tempVideo = task[i].m_RadarVideo;
        for (QMap<int,RADAR_VIDEO_DATA>::iterator it = tempVideo.begin(); it != tempVideo.end(); it++)
        {
            int key = it.key();
            if(RadarVideo.contains(key))
            {
                QList<int> lineData = it->mLineData;
                //将两个的振幅值进行合并
                RADAR_VIDEO_DATA &old_data = RadarVideo[key];
                if(old_data.mLineData.size() == lineData.size())
                {
                    for(int i=0; i<lineData.size(); i++)
                    {
                        if(old_data.mLineData[i] < lineData[i])
                        {
                            old_data.mLineData[i] = lineData[i];
                        }

                    }
                }

            } else
            {
                RadarVideo[key] = it.value();
            }
        }
    }


    if(RadarVideo.size() == 0) return;

    int ratio = 1;
    double uMultibeamPixmapWidth = (RadarVideo.first().m_uTotalCellNum)*2 * ratio - 1;
    double uMultibeamPixmapHeight = (RadarVideo.first().m_uTotalCellNum)*2 * ratio - 1;
//    qDebug()<<"image width:"<<uMultibeamPixmapWidth<<uMultibeamPixmapHeight;
    QPixmap objPixmap(uMultibeamPixmapWidth,uMultibeamPixmapHeight);
    objPixmap.fill(Qt::transparent);//用透明色填充
    QPainter objPainter(&objPixmap);
    objPainter.setRenderHint(QPainter::Antialiasing,true);
    objPainter.translate(QPoint(uMultibeamPixmapWidth / 2, uMultibeamPixmapHeight / 2));    // 中心点

    //设置颜色,像素点
    int arc_span = qCeil(2.0 * 360 * 16 / 4096) ;
    for (QMap<int,RADAR_VIDEO_DATA>::iterator it = RadarVideo.begin(); it != RadarVideo.end(); it++)
    {
        //cout<<"it.key"<<it.key();
        RADAR_VIDEO_DATA data = it.value();

        double dAzimuth = data.m_uAzimuth * (360.0 / data.m_uLineNum) + data.m_uHeading;
        //这里方位角是相对于正北方向,将他转换到画图的坐标系
        double angle_paint = -270 - dAzimuth;
        int arc_start = qCeil(angle_paint * 16) - arc_span;
//        QList<int> amplitudeList = data.m_pAmplitude;
//        QList<int> indexList = data.m_pIndex;

        QColor preColor(Qt::transparent);
        for (int i = 0; i < data.mLineData.size(); i++)
        {
            int position = i;
            int value = data.mLineData[i];
            if(value == 0) continue;
            int min_amplitude = Utils::Profiles::instance()->value(m_radarSec,"min_amplitude").toInt();
            int max_amplitude = Utils::Profiles::instance()->value(m_radarSec,"max_amplitude").toInt();

            if(value<min_amplitude || value > max_amplitude)continue;
#if 1
            //开始画扫描点对应的圆弧轨迹
            QRect rect(0, 0, 2*position * ratio, 2*position*ratio);
            rect.moveCenter(QPoint(0, 0));
            QColor objColor = this->getColor(value);
            objPainter.setPen(QPen(QColor(objColor), ratio));//改像素点
            objPainter.drawArc(rect, arc_start, arc_span);
#else


#endif
        }
    }
    //通过生成的回波图形,识别各个回波图形
    QImage img = objPixmap.toImage();
    double range_factor = RadarVideo.first().m_dRangeFactor/ ratio;    
    QImage result;
    zchxRadarRectDefList list;
    if(mVideoExtractionWorker)
    {
        mVideoExtractionWorker->parseVideoPieceFromImage(result, list, img, range_factor, video_index, mOutputImg);
    }
    //发送回波矩形集合
    qDebug()<<"parse rect list size:"<<list.size();
    if(list.size() > 0)
    {
        if(!mTracker)
        {
            signalSendRects(list);
        } else
        {
            mTracker->process(list);
        }
    }

    emit signalSendVideoPixmap(QPixmap::fromImage(result));
    return;
}
