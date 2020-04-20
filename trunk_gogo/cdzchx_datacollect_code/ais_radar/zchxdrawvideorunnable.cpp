#include "zchxdrawvideorunnable.h"
#include <QDebug>
#include <math.h>
#include <QPixmap>
#include <QPainter>
#include <QImage>
ZCHXDrawVideoRunnable::ZCHXDrawVideoRunnable(QObject *parent)
    : /*QObject(parent),*/
      QRunnable(),
      m_bLimit(true)
{
    initColor();
}

void ZCHXDrawVideoRunnable::run()
{
    //QMutexLocker locker(&m_mutex);
    //qDebug()<<"slotDrawRadarVideo--------------------------";
    QMap<int,RADAR_VIDEO_DATA> RadarVideo = m_dataAfterglow.m_RadarVideo;

    int uMultibeamPixmapWidth = (RadarVideo.first().m_uTotalNum)*2;
    int uMultibeamPixmapHeight = (RadarVideo.first().m_uTotalNum)*2;
    QPixmap objPixmap(uMultibeamPixmapWidth,uMultibeamPixmapHeight);
    objPixmap.fill(Qt::transparent);//用透明色填充
    //objPixmap.fill(QColor(200,200,200,100));
    QPainter objPainter(&objPixmap);
    objPainter.setRenderHint(QPainter::Antialiasing,true);
    objPainter.save();
    objPainter.translate(QPoint(uMultibeamPixmapWidth / 2, uMultibeamPixmapHeight / 2));    // 中心点
    QList<int> amplitudeList;
    QList<int> indexList;
    QPointF objPoint;
    QMap<int,RADAR_VIDEO_DATA>::iterator it;
    for(it = RadarVideo.begin();it!=RadarVideo.end();it++)
    {
        RADAR_VIDEO_DATA data = it.value();

        double dAzimuth = data.m_uAzimuth*(360.0/data.m_uLineNum)+data.m_uHeading;
        double dArc = dAzimuth*2*PI/180.0;
        //qDebug()<<"dAzimuth"<<dAzimuth;
        amplitudeList = data.m_pAmplitude;
        indexList = data.m_pIndex;
        objPainter.rotate(dAzimuth+180);


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

//            distbearTolatlon(dCentreLat,dCentreLon,
//                                            dDistance,dAzimuth,dLat,dLon);

//            if(!isInRadarPointRange(dLat,dLon,dataAfterglow.m_path))
//            {
//                continue;
//            }

            if(m_bLimit)//限制区域
            {
                if(!inLimitArea(dCentreLat,dCentreLon,dAzimuth,position,dStartDis,dDisInterval))
                {
                    continue;
                }
            }
            int colorValue = value*2;

            if(colorValue>200)
                colorValue = 200;
            QColor objColor = this->getColor(value);
            //qDebug()<<"dAzimuth"<<dAzimuth<<"position"<<position<<"value"<<value;
//            objPainter.setPen(QPen(QColor(colorValue,255,colorValue),1 /*data.uBitResolution*/));
            int nPenWidth = 1;
            if(position>=200&&position<400)
            {
                nPenWidth = 2;
            }
            else if(position>=400&&position<600)
            {
                nPenWidth = 4;
            }
            else if(position>=600&&position<800)
            {
                nPenWidth = 6;
            }
            else if(position>=800&&position<1000)
            {
                nPenWidth = 8;
            }
            else if(position>=1000)
            {
                nPenWidth = 10;
            }
            objPainter.setPen(QPen(objColor,nPenWidth));
            objPoint.setX(0);
            objPoint.setY(position);
            objPainter.drawPoint(objPoint);

        }
        objPainter.rotate(-(dAzimuth+180));
    }
    objPainter.translate(QPoint(-uMultibeamPixmapWidth / 2, -uMultibeamPixmapHeight / 2));
    objPainter.restore();
    //qDebug()<<"emit--------------------------";
    //signalRadarVideoPixmap(objPixmap);

    RadarVideo.clear();
}

void ZCHXDrawVideoRunnable::setAfterglow(const Afterglow &dataAfterglow)
{
    //QMutexLocker locker(&m_mutex);
    m_dataAfterglow = dataAfterglow;
}

void ZCHXDrawVideoRunnable::initColor()
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

QColor ZCHXDrawVideoRunnable::getColor(double dValue)
{
    double m_dMinZ = 0;
    double m_dMaxZ = 255;
    if(dValue>m_dMaxZ)
    {
        return QColor(255,0,0);
    }
    if(dValue<m_dMinZ)
    {
        return QColor(0,0,255);
    }
    if(m_dMaxZ-m_dMinZ<0.0001)
    {
        return QColor(255,0,0);
    }
    double dTemp = (dValue-m_dMinZ)/(m_dMaxZ-m_dMinZ);
    int uIndex = dTemp*100;
    if(uIndex<0)
    {
        return QColor(0,0,255);
    }
    if(uIndex>=100)
    {
        return QColor(255,0,0);
    }
    QColor objColor = m_colorMap[uIndex];
    return objColor;
}

//bool ZCHXDrawVideoRunnable::isInRadarPointRange(const double dLat, const double dLon, const std::vector<std::pair<double, double> > path)
//{
//    int nNum = path.size();
//    //qDebug()<<"m_path---num"<<nNum;
//    if(nNum<=0)
//    {
//        return true;
//    }
//    std::vector<std::pair<double, double>> tempPath = path;
//    const int uRange = 100;//m
//    std::vector<std::pair<double, double>>::iterator it = tempPath.begin();
//    for(;it!=tempPath.end();it++)
//    {
//        std::pair<double, double> data = *it;
//        double dPointLat = data.first;
//        double dPointLon = data.second;
//        double dDistance = getDisDeg(dPointLat,dPointLon,
//                                                 dLat ,dLon);


//        if(dDistance<=uRange)
//        {
//            return true;
//        }
//    }
//    return false;
//}

bool ZCHXDrawVideoRunnable::inLimitArea(const double dCentreLat, const double dCentreLon, const double dAzimuth, const int uPosition, const double dStartRange, const double dRangeFactor)
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
