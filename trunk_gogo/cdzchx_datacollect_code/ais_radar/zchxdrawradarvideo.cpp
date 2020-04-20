#include "zchxdrawradarvideo.h"
#include <QDebug>
#include <math.h>
#include <QMutex>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QGeoCoordinate>
#include <QCoreApplication>
#include "Log.h"
#include "profiles.h"
#include "zchxRadarRectExtraction.h"

#define cout if(1) qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
//static QMap<int,QMap<int, QList<TrackNode>>> mVideoMap;
static QMap<int, QList<TrackNode>> mVideoMap;
static QMap<int, int> mRangeMap;

ZCHXDrawRadarVideo::ZCHXDrawRadarVideo(int radar_id, QObject *parent) : QObject(parent)
{
    mRadarID = radar_id;
    qRegisterMetaType<std::vector<std::pair<double, double>> >("std::vector<std::pair<double, double>> ");
    id = 0;
    combineVideoList.clear();
    videoQPolygonF.clear();
    mVideoTrackmap.clear();
    LastPotList1.clear();
    LastPotList2.clear();
    m_radarRectMap.clear();
    m_showNum = 0;
    pen_width = 1;
    initColor();
    m_uAfterglowIndex = 0;
    m_uAfterglowType = 3;
    m_bLimit = false;
    m_distance = 200;
    moveToThread(&m_threadWork);
    qRegisterMetaType<zchxTrackPointMap>("const zchxTrackPointMap&");
    m_bProcessing = false;
    qRegisterMetaType<Afterglow>("Afterglow");
    qRegisterMetaType<Afterglow>("&Afterglow");
    qRegisterMetaType<QList<QPointF>>("QList<QPointF>");
    qRegisterMetaType<QMap<int,QList<TrackNode> >>("QMap<int,QList<TrackNode> >");
    qRegisterMetaType<QMap< int,QList<QPointF> >>("QMap< int,QList<QPointF> >");
    qRegisterMetaType<zchxRadarRectList>("const zchxRadarRectList&");
    qRegisterMetaType<zchxRadarRectDefList>("const zchxRadarRectDefList&");
    qRegisterMetaType<zchxRadarRectMap>("const zchxRadarRectMap&");

    int a1 =(Utils::Profiles::instance()->value("Color","color1_R").toInt());
    int a2 = (Utils::Profiles::instance()->value("Color","color1_G").toInt());
    int a3 = (Utils::Profiles::instance()->value("Color","color1_B").toInt());
    int b1 = (Utils::Profiles::instance()->value("Color","color2_R").toInt());
    int b2 = (Utils::Profiles::instance()->value("Color","color2_G").toInt());
    int b3 = (Utils::Profiles::instance()->value("Color","color2_B").toInt());
    objColor1 = QColor(a1,a2,a3);
    objColor2 = QColor(b1,b2,b3);

    videoPixmap_1=videoPixmap_2=videoPixmap_3=QPixmap(1024,1024);

    //QObject::connect(this,SIGNAL(signalDrawRadarVideo(Afterglow)),this,SLOT(slotDrawRadarVideo(Afterglow)));
    connect(this,SIGNAL(signalDrawAfterglow(double, Afterglow, zchxTrackPointMap, bool)),
            this,SLOT(slotDrawAfterglow(double, Afterglow, zchxTrackPointMap, bool)));
    connect(&m_threadWork,&QThread::finished,this,&QObject::deleteLater);
    connect(this,SIGNAL(SignalShowTrackNum(bool)),this,SLOT(showTrackNumSlot(bool)));

    //抽出对象初始化
    QString section = QString("Radar_%1").arg(mRadarID);
    Utils::Profiles::instance()->setDefault(section,  "OutputImage", 0);
    mOutputImg = Utils::Profiles::instance()->value(section,  "OutputImage").toBool();
    m_dCentreLat = Utils::Profiles::instance()->value(section,  "Centre_Lat").toDouble();
    m_dCentreLon = Utils::Profiles::instance()->value(section,  "Centre_Lon").toDouble();
    QString sLimit_File = Utils::Profiles::instance()->value(section,"Limit_File").toString();
    mVideoExtractionWorker = new zchxRadarRectExtraction(m_dCentreLat, m_dCentreLon, sLimit_File, mRadarID);
    double minArea = Utils::Profiles::instance()->value(section,  "track_min_area").toDouble();
    double maxArea = Utils::Profiles::instance()->value(section,  "track_max_area").toDouble();
    double minLen = Utils::Profiles::instance()->value(section,  "track_min_radius").toDouble();
    double maxLen = Utils::Profiles::instance()->value(section,  "track_radius").toDouble();
    mVideoExtractionWorker->setTargetAreaRange(minArea, maxArea);
    mVideoExtractionWorker->setTargetLenthRange(minLen, maxLen);
    mVideoExtractionWorker->setLimitAvailable(Utils::Profiles::instance()->value(section, "Limit").toBool());

    m_threadWork.setStackSize(64000000);
    m_threadWork.start();
}

ZCHXDrawRadarVideo::~ZCHXDrawRadarVideo()
{
    if(m_threadWork.isRunning()) {
        m_threadWork.quit();
    }

    m_threadWork.terminate();
}

bool ZCHXDrawRadarVideo::getIsProcessing()
{
    return m_bProcessing;
}

void ZCHXDrawRadarVideo::setAfterglowType(const int uIndex)
{
    m_uAfterglowType = uIndex;
}

void ZCHXDrawRadarVideo::setLimit(bool bLimit)
{
    m_bLimit = bLimit;
}

void ZCHXDrawRadarVideo::setDistance(double dDis)
{
    m_distance = dDis;
}

void ZCHXDrawRadarVideo::setLimitArea(const QList<QPolygonF> &landPolygon, const QList<QPolygonF> &seaPolygon)
{
    m_landPolygon = landPolygon;
    m_seaPolygon = seaPolygon;
}
//画回波255位黄色,1-244振幅为蓝色
QPixmap ZCHXDrawRadarVideo::drawRadarVideoPixmap(const Afterglow &dataAfterglow, bool rotate, double nRange)
{
    //cout<<"画回波255位黄色,1-244振幅为蓝色";
//    std::vector<std::pair<double, double>> latLonVec;//经纬度点集
//    QList<QPointF> mTrackList;
    QPixmap videoPixmap;
    QMap<int,RADAR_VIDEO_DATA> RadarVideo = dataAfterglow.m_RadarVideo;

    double uMultibeamPixmapWidth = (RadarVideo.first().m_uTotalNum)*2 - 1;
    double uMultibeamPixmapHeight = (RadarVideo.first().m_uTotalNum)*2 - 1;
    QPixmap objPixmap(uMultibeamPixmapWidth,uMultibeamPixmapHeight);
    objPixmap.fill(Qt::transparent);//用透明色填充
    QPainter objPainter(&objPixmap);
    objPainter.setRenderHint(QPainter::Antialiasing,true);
    objPainter.translate(QPoint(uMultibeamPixmapWidth / 2, uMultibeamPixmapHeight / 2));    // 中心点
    QList<int> amplitudeList;
    QList<int> indexList;
    QPointF objPoint;
    QMap<int,RADAR_VIDEO_DATA>::iterator it;
    //回波矩形块集合
    zchxRadarRectDefList mRadar_Rect_List;
    mRadar_Rect_List.clear();
    //设置颜色,像素点
    objPainter.setPen(QPen((objColor1),1));

    QList<TrackNode> nodeList;
    if(1)//画回波图
    {
        for (it = RadarVideo.begin(); it != RadarVideo.end(); it++)
        {
            //cout<<"it.key"<<it.key();
            RADAR_VIDEO_DATA data = it.value();

            double dAzimuth = data.m_uAzimuth * (360.0 / data.m_uLineNum) + data.m_uHeading;
            double dArc = dAzimuth * PI / 180.0;//dAzimuth * 2 * PI / 180.0
            //cout<<"dAzimuth"<<dAzimuth <<data.m_uAzimuth * (360.0 / data.m_uLineNum)<<data.m_uHeading;
            //qDebug()<<"dAzimuth"<<dAzimuth;
            amplitudeList = data.m_pAmplitude;
            indexList = data.m_pIndex;

            for (int i = 0; i < amplitudeList.size(); i++)
            {
                int position = indexList[i];
                int value = amplitudeList[i];
                int min_amplitude = Utils::Profiles::instance()->value(str_radar,"min_amplitude").toInt();
                int max_amplitude = Utils::Profiles::instance()->value(str_radar,"max_amplitude").toInt();
                if(value<min_amplitude || value > max_amplitude)continue;
                //直接计算出所有黄色回波块点的坐标
                float x = 0;
                float y = 0;
#if 0
                if(dAzimuth >= 0 && dAzimuth <90)//第一象限
                {
                    x = - (position * sin(dArc));
                    y = - (position * cos(dArc));
                }
                if(dAzimuth >= 90 && dAzimuth <180)//第二象限
                {
                    x = -(position * sin((180-dAzimuth)*PI / 180.0));
                    y = (position * cos((180-dAzimuth)*PI / 180.0));
                }
                if(dAzimuth >= 180 && dAzimuth <270)//第三象限
                {
                    x = (position * sin((dAzimuth - 180)*PI / 180.0));
                    y = (position * cos((dAzimuth - 180)*PI / 180.0));
                }
                if(dAzimuth >= 270 && dAzimuth <360)//第四象限
                {
                    x = (position * sin((360-dAzimuth)*PI / 180.0));
                    y = -(position * cos((360-dAzimuth)*PI / 180.0));
                }
                objPoint.setX(-x);
                objPoint.setY(y);
#else
                x = position * sin(dArc);
                y = - (position * cos(dArc));
                objPoint.setX(x);
                objPoint.setY(y);
#endif


                QColor objColor = this->getColor(value);

//                qDebug()<<"value"<<value<<"objColor"<<objColor.red();
                int pen_width = 1;
                //cout<<"position"<<position;
//                if(position>=100&&position<200)
//                {
//                    pen_width = 2;
//                }
//                if(position>=200&&position<300)
//                {
//                    pen_width = 2;
//                }
//                if(position>=300&&position<400)
//                {
//                    pen_width = 3;
//                }
//                else if(position>=400&&position<600)
//                {
//                    pen_width = 4;
//                }
//                else if(position>=1000)
//                {
//                    pen_width = 4;
//                }
                objPainter.setPen(QPen(QColor(objColor),pen_width));//改像素点
                //转笛卡尔坐标,经纬度
                double dky = (nRange / (uMultibeamPixmapWidth / 2)) * (-objPoint.y());
                double dkx = (nRange / (uMultibeamPixmapHeight / 2)) * (objPoint.x());
                //cout<<"笛卡转屏幕实际坐标"<<dkx<<dky;
    //            double mx = dkx / (nRange / (uMultibeamPixmapWidth / 2));
    //            double my = dky / (nRange / (uMultibeamPixmapHeight / 2));
                //笛卡尔转经纬度
                LatLong startLatLong(m_dCentreLon,m_dCentreLat);
                double lat ;
                double lon ;
                getNewLatLong(startLatLong,lat, lon, dkx, dky);
                //先过滤
                //cout<<"m_bLimit"<<m_bLimit;
                if(m_bLimit) {
                    if(!inLimitAreaForTrack(lat, lon)) {
                        continue;
                    }
                }
                std::pair<double, double> latLonPair(lat, lon);
                TrackNode mTrackNode;
                mTrackNode.have_value = 0;
                mTrackNode.latlonPair = latLonPair;
                //mTrackNode.trackP = trackP;
                mTrackNode.amplitude = value;
                mTrackNode.index = position;
                nodeList.append(mTrackNode);
                QString sRadarType = Utils::Profiles::instance()->value(str_radar,"Radar_Type").toString();
                if(mVideoMap.size() > 0 && mVideoMap.firstKey() != id && sRadarType == "6G")
                    objPainter.drawPoint(objPoint);
                else
                     objPainter.drawPoint(objPoint);
            }
        }

    }
#if 0
    QString sRadarType = Utils::Profiles::instance()->value(str_radar,"Radar_Type").toString();
    if(id && sRadarType == "6G")//
    {
        mVideoMap[id] = nodeList;
        mRangeMap[id] = nRange;
        if(mVideoMap.size() == 2)
        {
            QList<TrackNode> combinVideoList;
            QMap<int, QList<TrackNode>>::iterator mIterator;
            for(mIterator = mVideoMap.begin(); mIterator != mVideoMap.end(); ++mIterator)
            {
                ++mIterator;
                QList<TrackNode> mList = mIterator.value();
                for(int j = 0; j < mList.size(); j++)
                {
                    TrackNode mTrackNode = mList[j];
                    combinVideoList.append(mTrackNode);
                }
            }
            combineVideoList = combinVideoList;
            if(mVideoMap.firstKey() == id)//combineVideoList.size()
            {
                for(int i = 0; i < nodeList.size(); i++)
                {
                    double lat;
                    double lon;
                    double dkx;
                    double dky;
                    lat = nodeList[i].latlonPair.first;
                    lon = nodeList[i].latlonPair.second;
                    LatLong startLatLong(m_dCentreLon,m_dCentreLat);
                    //去除掉通道二半径内的回波块
                    double distance = getDisDeg(lat, lon, m_dCentreLat,m_dCentreLon);
                    if(distance < mRangeMap.value(mRangeMap.lastKey()))
                    {
                        //cout<<"跳过"<<distance<<mRangeMap.value(mRangeMap.lastKey());
                        //continue;
                    }
                    getDxDy(startLatLong,lat, lon, dkx, dky);
                    double mx = dkx / (nRange / (uMultibeamPixmapWidth / 2));
                    double my = dky / (nRange / (uMultibeamPixmapHeight / 2));
                    QPointF objPoint(my,-mx);
                    int value = nodeList[i].amplitude;
                    int min_amplitude = Utils::Profiles::instance()->value(str_radar,"min_amplitude").toInt();
                    int max_amplitude = Utils::Profiles::instance()->value(str_radar,"max_amplitude").toInt();
                    if(value<min_amplitude || value > max_amplitude)continue;
                    QColor objColor = this->getColor(value);
                    int pen_width = 1;
//                    int position = nodeList[i].index;

//                    if(position>=100&&position<200)
//                    {
//                        pen_width = 2;
//                    }
//                    if(position>=200&&position<300)
//                    {
//                        pen_width = 2;
//                    }
//                    if(position>=300&&position<400)
//                    {
//                        pen_width = 3;
//                    }
//                    else if(position>=400&&position<600)
//                    {
//                        pen_width = 4;
//                    }
                    objPainter.setPen(QPen(QColor(objColor),pen_width));//改像素点
                    //objPainter.setPen(QPen(QColor(255,255,255),pen_width));
                    objPainter.drawPoint(objPoint);
                }

                for(int i = 0; i < combineVideoList.size(); i++)
                {
                    double lat;
                    double lon;
                    double dkx;
                    double dky;
                    lat = combineVideoList[i].latlonPair.first;
                    lon = combineVideoList[i].latlonPair.second;
                    LatLong startLatLong(m_dCentreLon,m_dCentreLat);
                    getDxDy(startLatLong,lat, lon, dkx, dky);
                    double mx = dkx / ((nRange) / (uMultibeamPixmapWidth / 2));
                    double my = dky / ((nRange) / (uMultibeamPixmapHeight / 2));
//                    double mx = dkx / (mRangeMap.value(mRangeMap.lastKey()) / (uMultibeamPixmapWidth / 2))/(nRange/mRangeMap.value(mRangeMap.lastKey()));
//                    double my = dky / (mRangeMap.value(mRangeMap.lastKey()) / (uMultibeamPixmapHeight / 2))/(nRange/mRangeMap.value(mRangeMap.lastKey()));
                    QPointF objPoint(my,-mx);
                    int value = combineVideoList[i].amplitude;
                    int min_amplitude = Utils::Profiles::instance()->value(str_radar,"min_amplitude").toInt();
                    int max_amplitude = Utils::Profiles::instance()->value(str_radar,"max_amplitude").toInt();
                    if(value<min_amplitude || value > max_amplitude)continue;
                    QColor objColor = this->getColor(value);
                    int pen_width = 1;
                    int position = combineVideoList[i].index;

//                    if(position>=100&&position<200)
//                    {
//                        pen_width = 2;
//                    }
//                    if(position>=200&&position<300)
//                    {
//                        pen_width = 2;
//                    }
//                    if(position>=300&&position<400)
//                    {
//                        pen_width = 3;
//                    }
//                    else if(position>=400&&position<600)
//                    {
//                        pen_width = 4;
//                    }
//                    if((mRangeMap.last() / mRangeMap.first()) > 1)
//                        pen_width = pen_width * (mRangeMap.last() / mRangeMap.first());
                    objPainter.setPen(QPen(QColor(objColor),pen_width));//改像素点
                    // objPainter.setPen(QPen(QColor(255,255,255),pen_width));
                    objPainter.drawPoint(objPoint);
                }
            }
        }
        else
        {
            for(int i = 0; i < nodeList.size(); i++)
            {
                double lat;
                double lon;
                double dkx;
                double dky;
                lat = nodeList[i].latlonPair.first;
                lon = nodeList[i].latlonPair.second;
                LatLong startLatLong(m_dCentreLon,m_dCentreLat);
                getDxDy(startLatLong,lat, lon, dkx, dky);
                double mx = dkx / (nRange / (uMultibeamPixmapWidth / 2));
                double my = dky / (nRange / (uMultibeamPixmapHeight / 2));
                QPointF objPoint(my,-mx);
                int value = nodeList[i].amplitude;
                int min_amplitude = Utils::Profiles::instance()->value(str_radar,"min_amplitude").toInt();
                int max_amplitude = Utils::Profiles::instance()->value(str_radar,"max_amplitude").toInt();
                if(value<min_amplitude || value > max_amplitude)continue;
                QColor objColor = this->getColor(value);
                int pen_width = 2;
                int position = nodeList[i].index;

                if(position>=100&&position<200)
                {
                    pen_width = 2;
                }
                if(position>=200&&position<300)
                {
                    pen_width = 2;
                }
                if(position>=300&&position<400)
                {
                    pen_width = 3;
                }
                else if(position>=400&&position<600)
                {
                    pen_width = 4;
                }
                objPainter.setPen(QPen(QColor(objColor),pen_width));//改像素点
                objPainter.drawPoint(objPoint);
            }
        }
    }
#endif
    if(mVideoTrackmap.size() >0)//mVideoTrackmap.size() >0 && !m_showNum
    {
        int i = 0;
        foreach (QList<TrackNode> tList, mVideoTrackmap) {
            if(!tList.size()) continue;
            i++;
            //取4个点,上下左右,画矩形
            QPointF uPot,dPot,lPot,rPot;
            double lat = tList.first().latlonPair.first;
            double lng = tList.first().latlonPair.second;
            LatLong startLatLong(m_dCentreLon,m_dCentreLat);
            double x,y;
            getDxDy(startLatLong,lat,lng,x,y);
            //笛卡转屏幕实际坐标
            double mx = x / (nRange / (uMultibeamPixmapWidth / 2));
            double my = y / (nRange / (uMultibeamPixmapHeight / 2));
            QPointF pos(my,-mx);
            uPot = pos;
            dPot = pos;
            lPot = pos;
            rPot = pos;
            //cout<<"画矩形";
            com::zhichenhaixin::proto::RadarRectDef rectDef;
            QList<TrackNode> mOutsidePointList;
            //获取单回波周围一圈的点,同样采用递归算法,判断上下左右4个方向是否都有相邻点,有的去掉,无则为所求
            int g_searchTable[2048][512];
            memset(g_searchTable, '\0', sizeof(g_searchTable));
            for(int i = 0; i < tList.size(); i++)
            {
                int x = tList[i].trackP.x();
                int y = tList[i].trackP.y();
                if (x >= 0 && x < 2048 && y >= 0 && y < 512)
                {
                    g_searchTable[x][y] = i + 1;
                }
            }
            foreach (TrackNode mTrackNode, tList) {
                //设置颜色,像素点
                //objPainter.setPen(QPen((objColor2),1));
                double lat = mTrackNode.latlonPair.first;
                double lng = mTrackNode.latlonPair.second;
                LatLong startLatLong(m_dCentreLon,m_dCentreLat);
                double x,y;
                getDxDy(startLatLong,lat,lng,x,y);
                //笛卡转屏幕实际坐标
                double mx = x / (nRange / (uMultibeamPixmapWidth / 2));
                double my = y / (nRange / (uMultibeamPixmapHeight / 2));
                QPointF pos(my,-mx);
                //cout<<"pos"<<pos;
                int zbx = mTrackNode.trackP.x();
                int zby = mTrackNode.trackP.y();
                if(g_searchTable[zbx+1][zby] == 0 || g_searchTable[zbx-1][zby] == 0
                   || g_searchTable[zbx][zby+1] == 0 || g_searchTable[zbx][zby -1] == 0)
                {
                    //cout<<"找到最边上的点";
                    mOutsidePointList.append(mTrackNode);
                    zchxSingleVideoBlock *block = rectDef.add_blocks();
                    block->set_latitude(lat);
                    block->set_longitude(lng);
                    objPainter.setPen(QPen(QColor(255,0,0),2));
                    //objPainter.drawPoint(pos);
                }
                //遍历获取最边上的4点
                QPointF tempPot = pos;
                //上
                if(pos.y() - uPot.y() > 0)

                    uPot = tempPot;
                //下
                if(pos.y() - dPot.y() < 0)
                    dPot = tempPot;
                //左
                if(pos.x() - lPot.x() < 0)
                    lPot = tempPot;
                //右
                if(pos.x() - rPot.x() > 0)
                    rPot = tempPot;
                //mSingleVideoBlock = NULL;
            }
            //cout<<"mOutsidePointList"<<mOutsidePointList.size()<<tList.size();
            QPointF topLeft,bottomRight,center;
            topLeft = QPointF(lPot.x(),uPot.y());
            bottomRight = QPointF(rPot.x(),dPot.y());
            //center = QPointF((topLeft.x() + bottomRight.x())/2,((topLeft.y() + bottomRight.y())/2));

            //取最长距离的2点
            double longestDistance = 0;//最长距离
            double startPointLat,startPointLon,endPointLat,endPointLon;
            for(int i = 0; i < mOutsidePointList.size(); i++)
            {
                for(int k = 0; k < mOutsidePointList.size(); k++)
                {
                    if(i == k)continue;
                    double lat1 = mOutsidePointList[i].latlonPair.first;
                    double lng1 = mOutsidePointList[i].latlonPair.second;
                    double lat2 = mOutsidePointList[k].latlonPair.first;
                    double lng2 = mOutsidePointList[k].latlonPair.second;
                    double distance = getDisDeg(lat1,lng1,lat2,lng2);
                    if(distance > longestDistance)
                    {
                        longestDistance = distance;
                        startPointLat = lat1;
                        startPointLon = lng1;
                        endPointLat = lat2;
                        endPointLon = lng2;
                    }
                }
            }

            double x1,y1,x2,y2;
            getDxDy(startLatLong,startPointLat,startPointLon,x1,y1);
            getDxDy(startLatLong,endPointLat,endPointLon,x2,y2);
            //笛卡转屏幕实际坐标
            double mx1 = x1 / (nRange / (uMultibeamPixmapWidth / 2));
            double my1 = y1 / (nRange / (uMultibeamPixmapHeight / 2));
            QPointF pos1(my1,-mx1);
            double mx2 = x2 / (nRange / (uMultibeamPixmapWidth / 2));
            double my2 = y2 / (nRange / (uMultibeamPixmapHeight / 2));
            QPointF pos2(my2,-mx2);
            center = QPointF((pos1.x() + pos2.x())/2,((pos1.y() + pos2.y())/2));

            if(m_showNum)
            {
                objPainter.drawLine(pos1,pos2);
                objPainter.drawPoint(center);
            }

            //笛卡尔转经纬度
            double lat1,lon1;
            //左上经纬度
            double dkx =-topLeft.y() * (nRange / (uMultibeamPixmapWidth / 2));
            double dky =topLeft.x() * (nRange / (uMultibeamPixmapWidth / 2));
            getNewLatLong(startLatLong,lat1, lon1, dkx, dky);
            std::pair<double, double> topLeftlatLonPair(lat1, lon1);
            //右下经纬度
            dkx =-bottomRight.y()* (nRange / (uMultibeamPixmapWidth / 2));
            dky =bottomRight.x()* (nRange / (uMultibeamPixmapWidth / 2));
            getNewLatLong(startLatLong,lat1, lon1, dkx, dky);
            std::pair<double, double> bottomRightlatLonPair(lat1, lon1);
            //中心经纬度
            dkx =-center.y()* (nRange / (uMultibeamPixmapWidth / 2));
            dky =center.x()* (nRange / (uMultibeamPixmapWidth / 2));
            getNewLatLong(startLatLong,lat1, lon1, dkx, dky);
            std::pair<double, double> centerlatLonPair(lat1, lon1);

            rectDef.set_rectnumber(i);
            rectDef.set_topleftlatitude(topLeftlatLonPair.first);
            rectDef.set_topleftlongitude(topLeftlatLonPair.second);
            rectDef.set_bottomrightlatitude(bottomRightlatLonPair.first);
            rectDef.set_bottomrightlongitude(bottomRightlatLonPair.second);
            rectDef.set_centerlatitude(centerlatLonPair.first);
            rectDef.set_centerlongitude(centerlatLonPair.second);
            rectDef.set_startlatitude(startPointLat);
            rectDef.set_startlongitude(startPointLon);
            rectDef.set_endlatitude(endPointLat);
            rectDef.set_endlongitude(endPointLon);
            QGeoCoordinate mGeoCoordinate1(startPointLat,startPointLon) ,mGeoCoordinate2(endPointLat,endPointLon);
            double angle = mGeoCoordinate1.azimuthTo(mGeoCoordinate2);
            //cout<<"角度"<<angle;
            if(angle > 180)
                rectDef.set_angle(angle - 180 + 90);
            else
                 rectDef.set_angle(angle + 90);
            //cout<<"angle"<<angle;
            rectDef.set_sog(0.0);
            rectDef.set_cog(0.0);
            mRadar_Rect_List.append(rectDef);
            //cout<<"mRadarRect"<<mRadarRect.mutable_blocks()->block_size()<<tList.size();

        }
    }
    //cout<<"mRadar_Rect_List"<<mRadar_Rect_List.size();
    //发送回波矩形集合
    if(sendVideoFlag)
    {
        sendVideoFlag = 0;
        signalSendRects(mRadar_Rect_List);
    }
    //cout<<"signalSendRects";
    objPainter.translate(QPoint(-uMultibeamPixmapWidth / 2, -uMultibeamPixmapHeight / 2));
    //objPainter.restore();
    //RadarVideo.clear();
    videoPixmap = objPixmap;
    return videoPixmap;
}
//画小目标
QPixmap ZCHXDrawRadarVideo::drawRadarVideoAndTargetPixmap(zchxTrackPointMap map,double nRange, const Afterglow &dataAfterglow, QPixmap pixmap, bool rotate)
{
    //cout<<"nRange"<<nRange;
    land_limit.clear();
    sea_limit.clear();
    QPainter painter(&pixmap);
    QPointF objPoint;
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.save();
    painter.translate(QPoint(pixmap.width() / 2, pixmap.width() / 2));    // 中心点
    painter.setPen(QPen(QColor(255,255,255), 1));
    painter.drawPoint(0,0);
    //QPointF objPoint_1,objPoint_2;

    //if(rotate)painter.rotate(270);//顺时针旋转270°

    //采集器上画限制区域
    if(m_bLimit)
    {
        painter.setPen(QPen(QColor(255,255,255), 1));

        int i = 0;
        LatLong startLatLong(m_dCentreLon,m_dCentreLat);
        for(int ml = 0; ml <m_landPolygon.size(); ml++)
        {
            for(i=0; i<m_landPolygon[ml].size(); i++)
            {
                double dLat = m_landPolygon[ml][i].x();
                double dLon = m_landPolygon[ml][i].y();
                double posx = 0, posy = 0;
                getDxDy(startLatLong, dLon, dLat, posx, posy);
                QPointF pos_1(posy/ (nRange / (pixmap.height() / 2)), -posx/ (nRange / (pixmap.height() / 2)));
                land_limit<<pos_1;
            }
            painter.drawPolygon(land_limit);
            land_limit.clear();
        }
        land_limit.clear();

        if(m_seaPolygon.size() > 0)
        {
            for(i=0; i<m_seaPolygon.first().size(); i++)
            {
                double dLat = m_seaPolygon.first()[i].x();
                double dLon = m_seaPolygon.first()[i].y();
                double posx = 0, posy = 0;
                getDxDy(startLatLong, dLon, dLat, posx, posy);
                QPointF pos_1(posy/ (nRange / (pixmap.height() / 2)), -posx/ (nRange / (pixmap.height() / 2)));
                sea_limit<<pos_1;
                painter.drawPoint(pos_1);
            }
        }
        painter.drawConvexPolygon(sea_limit);
        sea_limit.clear();

    }

    //历史回波块显示
    if(m_radarRectMap.size() > 0)
    {
        //cout<<"画编号---------------------------------1"<<m_radarRectMap.size();
        foreach (zchxRadarRect obj, m_radarRectMap)
        {
            int num = obj.historyrects_size();
            //int color = 250;
            //cout<<"--开始--"<<num<<m_radarRectMap.size();
            if(m_showNum)
            {
                for(int i = 0; i < num; i++)//(int i = 0; i < num; i++)
                {
                    int color = 250;
                    color -= 10 * (num-i);// color -= 10 * (num - i);
                    QColor objColor3 = QColor(0,color,0);
                    painter.setBrush(QBrush(objColor3));
                    //cout<<"color"<<color;
                    zchxRadarRectDef signle_Rect = obj.historyrects(num-i-1);//
                    double lat = signle_Rect.centerlatitude();
                    double lng = signle_Rect.centerlongitude();
                    LatLong startLatLong(m_dCentreLon,m_dCentreLat);
                    double x,y;
                    getDxDy(startLatLong,lat,lng,x,y);
                    double mx = x / (nRange / (pixmap.width() / 2));
                    double my = y / (nRange / (pixmap.height() / 2));
                    QPointF center(my,-mx);

                    painter.setPen(QPen(objColor3,1));
                    //objPainter.drawRect(QRectF(topLeft,bottomRight));
                    painter.drawRect(QRectF(QPointF(center.x()-2,center.y()+2),QPointF(center.x()+2,center.y()-2)));
                }

            }
            else if (num > 0)
            {
                for(int i = 0; i < 1; i++)//(int i = 0; i < num; i++)
                {
                    int color = 250;
                    //color -= 10 * (num-i);// color -= 10 * (num - i);
                    QColor objColor3 = QColor(0,color,0);
                    painter.setBrush(QBrush(objColor3));
                    //cout<<"color"<<color;
                    zchxRadarRectDef signle_Rect = obj.historyrects(i);//num-i-1
                    double lat = signle_Rect.centerlatitude();
                    double lng = signle_Rect.centerlongitude();
                    LatLong startLatLong(m_dCentreLon,m_dCentreLat);
                    double x,y;
                    getDxDy(startLatLong,lat,lng,x,y);
                    double mx = x / (nRange / (pixmap.width() / 2));
                    double my = y / (nRange / (pixmap.height() / 2));
                    QPointF center(my,-mx);

                    painter.setPen(QPen(objColor3,1));
                    //objPainter.drawRect(QRectF(topLeft,bottomRight));
                    painter.drawRect(QRectF(QPointF(center.x()-2,center.y()+2),QPointF(center.x()+2,center.y()-2)));
                }

            }
        }
    }

    //画回波识别出来的目标点
    foreach (zchxTrackPoint point, map)
    {
        painter.setPen(QPen(QColor(255,0,0), 3));
        double fc = nRange;
        //if(nRange<2600) fc = 2500;
        double x = point.cartesianposx() / (fc / (pixmap.width() / 2));
        double y = point.cartesianposy() / (fc / (pixmap.height() / 2));

        //cout<<"笛卡尔坐标"<<point.cartesianposx()<<point.cartesianposy()<<x<<y;
        objPoint.setX(x);
        objPoint.setY(y);
        QPointF pos(y,-x);
        //cout<<"x"<<x<<"y"<<y<<pos;
        painter.drawPoint(pos);
        painter.setPen(QPen(QColor(255,255,255), 10));
        //目标编号
        painter.drawText(pos,QString::number(point.tracknumber()));
        //cout<<"目标编号"<<QString::number((map.begin() + i).value().tracknumber());
        if(m_showNum)//m_showNum
        {
            //显示方向值
            painter.drawText(pos,QString::number(point.tracknumber()));
            //cout<<point.tracknumber()<<point.wgs84poslong()<<point.wgs84poslat();
            //画方向线
            painter.setPen(QPen(QColor(255,255,255), 2));
            //利用速度和方向,分4种情况计算终点坐标的值
            double range = point.cog();//角度
            double speed = point.cog();//速度
            //cout<<"角度大小"<<range<<20;
            //第一象限
            //range = 350//自动以方向--------------------------------------------------------------
            if(m_showNum)//range != 0
            {
                if(range <= 90)
                {
                    float car_X = point.cartesianposx() + (speed * cos(range*M_PI/180));
                    float car_Y = point.cartesianposy() + (speed * sin(range*M_PI/180));

                    //cout<<"前后坐标对比"<< QString::number(point.cartesianposx())<<QString::number( point.cartesianposy())
                       //<< QString::number(car_X) << QString::number(car_Y)<<200 * sin(range*M_PI/180)<<200 * cos(range*M_PI/180)<<range;
                    int x = car_X / (nRange / (pixmap.width() / 2));
                    int y = car_Y / (nRange / (pixmap.height() / 2));
                    objPoint.setX(y);
                    objPoint.setY(-x);
                    painter.drawLine(pos,objPoint);
                    //cout<<"第一象限 编号-速度-方向:"<<point.tracknumber()<<point.sog()<<point.cog();
                }
                //第二象限
                if(range <= 180 && range > 90)
                {
                    range -=90;
                    float car_X = point.cartesianposx() - (speed * sin(range*M_PI/180));
                    float car_Y = point.cartesianposy() + (speed * cos(range*M_PI/180));
                    //cout<<"前后坐标对比"<< QString::number(point.cartesianposx())<<QString::number( point.cartesianposy())
                       //<< QString::number(car_X) << QString::number(car_Y)<<200 * sin(range*M_PI/180)<<cos(range*M_PI/180)<<range;
                    int x = car_X / (nRange / (pixmap.width() / 2));
                    int y = car_Y / (nRange / (pixmap.height() / 2));
                    objPoint.setX(y);
                    objPoint.setY(-x);
                    painter.drawLine(pos,objPoint);
                    //cout<<"第二象限 编号-速度-方向:"<<point.tracknumber()<<point.sog()<<point.cog();
                }
                //第三象限
                if(range <= 270 && range > 180)
                {
                    range -=180;
                    int car_X = point.cartesianposx() - (speed * cos(range*M_PI/180));
                    int car_Y = point.cartesianposy() - (speed * sin(range*M_PI/180));
                    //cout<<"前后坐标对比"<< QString::number(point.cartesianposx())<<QString::number( point.cartesianposy())
                       //<< QString::number(car_X) << QString::number(car_Y)<<200 * sin(range*M_PI/180)<<cos(range*M_PI/180)<<range;
                    int x = car_X / (nRange / (pixmap.width() / 2));
                    int y = car_Y / (nRange / (pixmap.height() / 2));
                    objPoint.setX(y);
                    objPoint.setY(-x);
                    painter.drawLine(pos,objPoint);
                    //cout<<"第三象限 编号-速度-方向:"<<point.tracknumber()<<point.sog()<<point.cog();
                }
                //第四象限
                if(range <= 360 && range > 270)
                {
                    range -=270;
                    int car_X = point.cartesianposx() + (speed * sin(range*M_PI/180));
                    int car_Y = point.cartesianposy() - (speed * cos(range*M_PI/180));
                    int x = car_X / (nRange / (pixmap.width() / 2));
                    int y = car_Y / (nRange / (pixmap.height() / 2));
                    objPoint.setX(y);
                    objPoint.setY(-x);
                    painter.drawLine(pos,objPoint);
                    //cout<<"第四象限 编号-速度-方向:"<<point.tracknumber()<<point.sog()<<point.cog();
                }

            }
        }
    }

    painter.translate(QPoint(-pixmap.width() / 2, -pixmap.height() / 2));
    painter.restore();

    return pixmap;
}

void ZCHXDrawRadarVideo::slotSetPenwidth(int str)//设置笔的宽度
{
    pen_width = str;
}

void ZCHXDrawRadarVideo::slotDrawAfterglow(double nRange, const Afterglow &dataAfterglow,zchxTrackPointMap map,  bool rotate)
{
    //cout<<"画图";
    m_bProcessing = true;
//    QMutex mutex;
//    mutex.lock();
    QMap<int,RADAR_VIDEO_DATA> RadarVideo = dataAfterglow.m_RadarVideo;
    //qDebug()<<"slotDrawAfterglow--------------------------";
    if(RadarVideo.isEmpty()) {
        m_bProcessing = false;
        return;
    }
    videoPixmap_3 = videoPixmap_2;
    videoPixmap_2 = videoPixmap_1;
    QPixmap videoPixmap = drawRadarVideoPixmap2(dataAfterglow, rotate, nRange);
    // test code:
    //画小目标
    //QPixmap videoAndTargetPixmap = drawRadarVideoAndTargetPixmap(map,nRange, dataAfterglow, videoPixmap, rotate);
    QPixmap videoAndTargetPixmap = drawRadarVideoAndTargetPixmap(map,nRange, dataAfterglow, videoPixmap_3, rotate);

    //cout<<"videoPixmap"<<videoPixmap.size();
    int uMultibeamPixmapWidth = (RadarVideo.first().m_uTotalNum)*2;
    int uMultibeamPixmapHeight = (RadarVideo.first().m_uTotalNum)*2;
    QPixmap objPixmap(uMultibeamPixmapWidth,uMultibeamPixmapHeight);
    objPixmap.fill(Qt::transparent);//用透明色填充
    QPainter objPainter(&objPixmap);
    objPainter.setRenderHint(QPainter::Antialiasing,true);
    objPainter.save();
    objPainter.translate(QPoint(uMultibeamPixmapWidth / 2, uMultibeamPixmapHeight / 2));    // 中心点

    //绘制成灰色供前一张使用
    QPixmap resultPixmap(uMultibeamPixmapWidth,uMultibeamPixmapHeight);
    resultPixmap.fill(Qt::transparent);//用透明色填充
    QPainter prePainter(&resultPixmap);
    prePainter.setRenderHint(QPainter::Antialiasing,true);
    prePainter.save();
    prePainter.translate(QPoint(uMultibeamPixmapWidth / 2, uMultibeamPixmapHeight / 2));

    QMap<int,RADAR_VIDEO_DATA>::iterator it;
    for(it = RadarVideo.begin();it!=RadarVideo.end();it++)
    {
       RADAR_VIDEO_DATA data = it.value();

        double dAzimuth = data.m_uAzimuth*(360.0/data.m_uLineNum)+data.m_uHeading;
        double dArc = dAzimuth*2*PI/180.0;
        //qDebug()<<"dAzimuth"<<dAzimuth;
        QList<int> amplitudeList = data.m_pAmplitude;
        QList<int> indexList = data.m_pIndex;
        if(rotate)
        {
            objPainter.rotate(dAzimuth+180);
            prePainter.rotate(dAzimuth+180);
        }
        QPointF objPoint;
        double dCentreLon = data.m_dCentreLon;//回波中心经度
        double dCentreLat = data.m_dCentreLat;//回波中心纬度
        double dStartDis = data.m_dStartRange;  //开始距离
        double dDisInterval = data.m_dRangeFactor;//距离间隔
        double dLat;
        double dLon;
        for(int i = 0;i<amplitudeList.size();i++)
        {
            int position = indexList[i];
            int value = amplitudeList[i];

            double dDistance = dStartDis +position*dDisInterval;
            distbearTolatlon(dCentreLat,dCentreLon,
                                            dDistance,dAzimuth,dLat,dLon);

            if(m_bLimit)//限制区域
            {
                if(!inLimitArea(dCentreLat,dCentreLon,dAzimuth,position,dStartDis,dDisInterval))
                {
                    continue;
                }
            }
            if(!isInRadarPointRange(dLat,dLon,dataAfterglow.m_path))//雷达目标200米以内
            {
                continue;
            }

            int colorValue = value*2;

            if(colorValue>200)
                colorValue = 200;
            QColor objColor = this->getColor_1(value);
            //qDebug()<<"dAzimuth"<<dAzimuth<<"position"<<position<<"value"<<value;
            //objPainter.setPen(QPen(QColor(colorValue,255,colorValue),1 /*data.uBitResolution*/));
            int nPenWidth = 1;
            if(position>=200&&position<400)
            {
                //cout <<"200+";
                nPenWidth = 2;
            }
            else if(position>=400&&position<600)
            {
                nPenWidth = 4;
            }
            else if(position>=600&&position<800)
            {
                nPenWidth = 4;
            }
            else if(position>=800&&position<1000)
            {
                nPenWidth = 4;
            }
            else if(position>=1000)
            {
                cout <<"1000+";
                nPenWidth = 4;
            }
            objPainter.setPen(QPen(QColor(252,241,1),pen_width));//1_该为一个像素点
            objPoint.setX(0);
            objPoint.setY(position);
            objPainter.drawPoint(objPoint);
            prePainter.setPen(QPen(QColor(19,118,61),pen_width));//1_该为一个像素点,改颜色
            prePainter.drawPoint(objPoint);

        }
        if(rotate)
        {
            objPainter.rotate(-(dAzimuth+180));
            prePainter.rotate(-(dAzimuth+180));
        }
    }
    objPainter.translate(QPoint(-uMultibeamPixmapWidth / 2, -uMultibeamPixmapHeight / 2));
    objPainter.restore();
    prePainter.translate(QPoint(-uMultibeamPixmapWidth / 2, -uMultibeamPixmapHeight / 2));
    prePainter.restore();
    //QPixmap resultPixmap = processPixmap(objPixmap);
    signalRadarAfterglowPixmap(m_uAfterglowIndex,videoPixmap,objPixmap,resultPixmap);//zmq
    emit signalRadarVideoAndTargetPixmap(videoAndTargetPixmap,dataAfterglow);//发送采集器显示回波图信号
    m_uAfterglowIndex++;
    m_uAfterglowIndex = m_uAfterglowIndex%m_uAfterglowType;

    m_bProcessing = false;
//    qDebug()<<"save--------------------------"<<m_uAfterglowIndex;
//    QString path = QCoreApplication::applicationDirPath();
//    QString str = QString("/data/Afterglow_%1.png").arg(m_uAfterglowIndex);
//    path = path+str;
//    qDebug()<<"path"<<path;
//    objPixmap.save(path);
   // mutex.unlock();
    videoPixmap_1 = videoPixmap;
}

void ZCHXDrawRadarVideo::initColor()
{
    m_colorMap.clear();
    const int colorBarLength = 100;
    QColor color;
    float tempLength=colorBarLength/4;
    for(int i=0;i<tempLength/2;i++)// jet
    {
        color.setRgbF(0,0,(tempLength/2+i)/tempLength);
        m_colorMap[i] = color;
    }
    for(int i=tempLength/2+1;i<tempLength/2+tempLength;i++)// jet
    {
        color.setRgbF(0,(i-tempLength/2)/tempLength,1);
        m_colorMap[i] = color;
    }
    for(int i=tempLength/2+tempLength+1;i<tempLength/2+2*tempLength;i++)// jet
    {
        color.setRgbF((i-tempLength-tempLength/2)/tempLength,1,(tempLength*2+tempLength/2-i)/tempLength);
        m_colorMap[i] = color;
    }
    for(int i=tempLength/2+2*tempLength+1;i<tempLength/2+3*tempLength;i++)// jet
    {
        color.setRgbF(1,(tempLength*3+tempLength/2-i)/tempLength,0);
        m_colorMap[i] = color;
    }
    for(int i=tempLength/2+3*tempLength+1;i<colorBarLength;i++)// jet
    {
        color.setRgbF((colorBarLength-i+tempLength/2)/(tempLength),0,0);
        m_colorMap[i] = color;
    }
}

QColor ZCHXDrawRadarVideo::getColor(double dValue)
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
        return objColor1;//QColor(6,144,36);
    }
    //cout<<"颜色6";
    //QColor objColor = m_colorMap[uIndex];
    QColor objColor = objColor2;//QColor(103,236,231);
    return objColor;
}

void ZCHXDrawRadarVideo::slotSetColor(int a1,int a2,int a3,int b1,int b2,int b3)
{
    objColor1 = QColor(a1,a2,a3);
    objColor2 = QColor(b1,b2,b3);
}

QColor ZCHXDrawRadarVideo::getColor_1(double dValue)
{
    double m_dMinZ = 0;
    double m_dMaxZ = 255;
    if(dValue>m_dMaxZ)
    {
        return QColor(99,255,7);
    }
    if(dValue<m_dMinZ)
    {
        return QColor(19,118,61);
    }
    if(m_dMaxZ-m_dMinZ<0.0001)
    {
        return QColor(99,255,7);
    }
    double dTemp = (dValue-m_dMinZ)/(m_dMaxZ-m_dMinZ);
    int uIndex = dTemp*100;
    if(uIndex<0)
    {
        return QColor(19,118,61);
    }
    if(uIndex>=100)
    {
        return QColor(99,255,7);
    }
    //QColor objColor = m_colorMap[uIndex];
    QColor objColor = QColor(19,118,61);
    return objColor;
}

bool ZCHXDrawRadarVideo::isInRadarPointRange(const double dLat, const double dLon, const std::vector<std::pair<double, double> > path)
{
    int nNum = path.size();
    //qDebug()<<"m_path---num"<<nNum;
    if(nNum<=0)
    {
        return false;
    }
    std::vector<std::pair<double, double>> tempPath = path;
    //const int uRange = m_distance;//m
    std::vector<std::pair<double, double>>::iterator it = tempPath.begin();
    for(;it!=tempPath.end();it++)
    {
        std::pair<double, double> data = *it;
        double dPointLat = data.first;
        double dPointLon = data.second;
        QGeoCoordinate objGeo(dPointLat,dPointLon);
        QGeoCoordinate otherGeo(dLat,dLon);
        double dDistance = objGeo.distanceTo(otherGeo);
        //double dDistance = getDisDeg(dPointLat,dPointLon,
         //                                        dLat ,dLon);


        if(dDistance<=m_distance)
        {
            return true;
        }
    }
    return false;
}

QPixmap ZCHXDrawRadarVideo::processPixmap(const QPixmap &objPixmap)
{
        QImage objImage = objPixmap.toImage();
        int nWidth = objImage.width();
        int nHeight = objImage.height();
        for(int i = 0;i<nWidth;i++)
        {
            for(int j = 0;j<nHeight;j++)
            {
                QColor color = objImage.pixelColor(i,j);
                if("#000000"!=color.name())
                {
                    //qDebug()<<"i"<<i<<" j"<<j<<" color"<<color.name();
                    objImage.setPixelColor(i,j,QColor(150,150,150));
                }
            }
        }
        QPixmap resultPixmap = QPixmap::fromImage(objImage);
    //QPixmap resultPixmap = objPixmap;
        return resultPixmap;
}

bool ZCHXDrawRadarVideo::inLimitArea(const double dCentreLat, const double dCentreLon,
                                     const double dAzimuth, const int uPosition,
                                     const double dStartRange, const double dRangeFactor)
{
    bool bOk = false;

    double dDis = dStartRange+uPosition*dRangeFactor;
    double dLon;
    double dLat;
    distbearTolatlon(dCentreLat,dCentreLon,dDis,dAzimuth,dLat,dLon);
    QPointF pos(dLon,dLat);

    for(int uIndex = 0;uIndex<m_seaPolygon.count();uIndex++)
    {
        const QPolygonF curSeaPolygonF = m_seaPolygon[uIndex];
        if(curSeaPolygonF.containsPoint(pos,Qt::OddEvenFill))
        {
            //return true;
            bool bInLand = false;
            for(int i = 0;i<m_landPolygon.count();i++)
            {
                const QPolygonF curLandPolygonF = m_landPolygon[i];
                if(curLandPolygonF.containsPoint(pos,Qt::OddEvenFill))
                {
                    bInLand = true;
                    break;
                }
            }
            bOk = !bInLand;
            return bOk;
        }
    }
    return false;
}

//void ZCHXDrawRadarVideo::land_limit_slot(QPolygonF lim)//在采集器上画限制区域
//{
//    land_limit = lim;
//    cout<<"land_limit_slot(QPolygonF lim)";
//}

void ZCHXDrawRadarVideo::gpsPoint2DescartesPoint(const double latitude, const double longitude, const double altitude, double &x, double &y, double &z)
{
    //wgs84 WGS84 Earth Constants
    double wgs84a = 6378.137;
    double wgs84f = 1.0 / 298.257223563;
//    cout<<"wgs84f"<<wgs84f;
    double wgs84b = wgs84a * (1.0 - wgs84f);
//    cout<<"wgs84b"<<wgs84b;
//    double a = 6378137.0;
//    double b = 6356752.314245;
//    double f = 1.0 / 298.257223563;


    //earthcon
    double f = 1 - wgs84b / wgs84a;
    double eccsq = 1 - (wgs84b* wgs84b) / (wgs84a * wgs84a);
    double ecc = sqrt(eccsq);
    double esq = ecc * ecc;

    //llhxyz
    double dtr = M_PI / 180.0;
    //qDebug() << dtr << gpsPoint.latitude << endl;
    double clat = cos(dtr * latitude);
    double slat = sin(dtr * latitude);
    double clon = cos(dtr * longitude);
    double slon = sin(dtr * longitude);
    //qDebug() << clat << slon << endl;

    //radcur compute the radii at the geodetic latitude lat (in degrees)
    double dsq = 1.0 - eccsq * slat *slat;
    double d = sqrt(dsq);
    //qDebug() << d;
    double rn = wgs84a / d;
    double rm = rn * (1.0 - eccsq) / dsq;

    double rho = rn * clat;
    double zz = (1.0 - eccsq) * rn *slat;
    double rsq = rho * rho + zz*zz;
    double r = sqrt(rsq);

    x = (rn + altitude) * clat * clon;
    y = (rn + altitude) * clat * slon;
    z = ((1 - esq)*rn + altitude) * slat;
}

void ZCHXDrawRadarVideo::showTrackNumSlot(bool flag)
{
    cout<<"显示编号" <<flag;
    m_showNum = flag;
}

void ZCHXDrawRadarVideo::slotTrackMap(QMap<int,QList<TrackNode>> map,int num)
{
    //cout<<"map"<<map.size();
    mVideoTrackmap = map;
    id = num;
    sendVideoFlag = 1;
}

void ZCHXDrawRadarVideo::showTheLastPot(QList<QPointF> xyList,QList<QPointF> zList)
{
    LastPotList1 = xyList;
}

void ZCHXDrawRadarVideo::getRects(zchxRadarRectMap mmp)
{
    m_radarRectMap = mmp;
    //cout<<"m_radarRectMap"<<m_radarRectMap.size();
}

//区域限制功能
bool ZCHXDrawRadarVideo::inLimitAreaForTrack(const double dLat, const double dLon)
{
    bool bOk = false;
    QPointF pos(dLon,dLat);
    if(m_seaPolygon.count()<=0&&m_landPolygon.count()<=0)
    {
        return true;//没有限制区域
        //cout<<"没有限制区域";
    }
    if(m_seaPolygon.count()>0 && m_seaPolygon.first().size())
    {
        //cout<<"有海限制区域"<<m_seaPolygon.first().size()<<m_seaPolygon.first();
        for(int uIndex = 0;uIndex<m_seaPolygon.count();uIndex++)
        {
            const QPolygonF curSeaPolygonF = m_seaPolygon[uIndex];
            if(curSeaPolygonF.containsPoint(pos,Qt::OddEvenFill))
            {
                //return true;
                bool bInLand = false;
                for(int i = 0;i<m_landPolygon.count();i++)
                {
                    const QPolygonF curLandPolygonF = m_landPolygon[i];
                    if(curLandPolygonF.containsPoint(pos,Qt::OddEvenFill))
                    {
                        bInLand = true;
                        break;
                    }
                }
                bOk = !bInLand;
                return bOk;
            }
        }
    }
    else
    {
        //cout<<"无海限制区域"<<m_seaPolygon.count()<<m_landPolygon.count();
        bool bInLand = false;
        for(int i = 0;i<m_landPolygon.count();i++)
        {
            const QPolygonF curLandPolygonF = m_landPolygon[i];
            if(curLandPolygonF.containsPoint(pos,Qt::OddEvenFill))
            {
                bInLand = true;
                break;
            }
        }
        bOk = !bInLand;
        return bOk;
    }

    return false;
}

//双通道回波
void ZCHXDrawRadarVideo::drawCombineVideo(QList<TrackNode> list)
{
    combineVideoList = list;
    cout<<"绘制2个通道的回波"<<combineVideoList.size();
}

void ZCHXDrawRadarVideo::setStradar(QString str)
{
    str_radar = str;
}

//画回波255位黄色,1-244振幅为蓝色
QPixmap ZCHXDrawRadarVideo::drawRadarVideoPixmap2(const Afterglow &dataAfterglow, bool rotate, double nRange)
{
    zchxTimeElapsedCounter counter(__FUNCTION__);
    QMap<int,RADAR_VIDEO_DATA> RadarVideo = dataAfterglow.m_RadarVideo;
    if(RadarVideo.size() == 0) return QPixmap();

    int ratio = 1;
    double uMultibeamPixmapWidth = (RadarVideo.first().m_uTotalNum)*2 * ratio - 1;
    double uMultibeamPixmapHeight = (RadarVideo.first().m_uTotalNum)*2 * ratio - 1;
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
        QList<int> amplitudeList = data.m_pAmplitude;
        QList<int> indexList = data.m_pIndex;

        for (int i = 0; i < amplitudeList.size(); i++)
        {
            int position = indexList[i];
            int value = amplitudeList[i];
            int min_amplitude = Utils::Profiles::instance()->value(str_radar,"min_amplitude").toInt();
            int max_amplitude = Utils::Profiles::instance()->value(str_radar,"max_amplitude").toInt();
            if(value<min_amplitude || value > max_amplitude)continue;
            //开始画扫描点对应的圆弧轨迹
            QRect rect(0, 0, 2*position * ratio, 2*position*ratio);
            rect.moveCenter(QPoint(0, 0));
            QColor objColor = this->getColor(value);
            objPainter.setPen(QPen(QColor(objColor), ratio));//改像素点
            objPainter.drawArc(rect, arc_start, arc_span);
        }
    }

    //通过生成的回波图形,识别各个回波图形
    QImage img = objPixmap.toImage();
    //    objPixmap.save(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz.PNG"), "PNG");
//    qDebug()<<"img:"<<img.format()<<img.size()<<img.bytesPerLine();
    int source_id = RadarVideo.first().m_uSourceID;
    double range_factor = RadarVideo.first().m_dRangeFactor/ ratio;    
    QImage result;
    zchxRadarRectDefList list;
    if(mVideoExtractionWorker)
    {
        mVideoExtractionWorker->parseVideoPieceFromImage(result, list, img, range_factor, mOutputImg);
    }
    //发送回波矩形集合
//    qDebug()<<"parse rect list size:"<<list.size();
    if(list.size() > 0)
    {
        signalSendRects(list);
    }

    //将图片保存
//    objPixmap.save(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz.PNG"), "PNG");
    qDebug()<<"img:"<<img.width()<<img.height()<<" result:"<<result.width()<<result.height();
    return QPixmap::fromImage(result);
}
