#include "zchxdrawradarvideo.h"
#include <QDebug>
#include <math.h>
#include <QMutex>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QGeoCoordinate>
#include <QCoreApplication>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
RADAR_VIDEO_DATA::RADAR_VIDEO_DATA()
{
    m_uSourceID = 0;
    m_uSystemAreaCode = 0;
    m_uSystemIdentificationCode = 0;
    m_uMsgIndex = 0;
    m_uLineNum = 0;
    m_uAzimuth = 0;
    m_uHeading = 0;
    m_dStartRange = 0;
    m_dRangeFactor = 0;
    m_bitResolution = ZCHX_RES::MONOBIT_RESOLUTION;
    m_pAmplitude.clear();
    m_pIndex.clear();
    m_uTotalNum = 0;
    m_dCentreLon = 0;
    m_dCentreLat = 0;
}

RADAR_VIDEO_DATA::~RADAR_VIDEO_DATA()
{
    m_pAmplitude.clear();
    m_pIndex.clear();
}

ZCHXDrawRadarVideo::ZCHXDrawRadarVideo(QObject *parent) : QObject(parent)
{
    pen_width = 1;
    initColor();
    m_uAfterglowIndex = 0;
    m_uAfterglowType = 3;
    m_bLimit = false;
    m_distance = 200;
    moveToThread(&m_threadWork);

    m_bProcessing = false;
    qRegisterMetaType<Afterglow>("Afterglow");
    qRegisterMetaType<Afterglow>("&Afterglow");
    QObject::connect(this,SIGNAL(signalDrawRadarVideo(Afterglow)),this,SLOT(slotDrawRadarVideo(Afterglow)));
    QObject::connect(this,SIGNAL(signalDrawAfterglow(Afterglow)),this,SLOT(slotDrawAfterglow(Afterglow)));
    connect(&m_threadWork,&QThread::finished,this,&QObject::deleteLater);
    m_threadWork.start();
}

ZCHXDrawRadarVideo::~ZCHXDrawRadarVideo()
{
    if(m_threadWork.isRunning())
    {

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

QPixmap ZCHXDrawRadarVideo::drawRadarVideoPixmap(const Afterglow &dataAfterglow)
{
    QPixmap videoPixmap;
    QMap<int,RADAR_VIDEO_DATA> RadarVideo = dataAfterglow.m_RadarVideo;

    if(RadarVideo.isEmpty())
        return videoPixmap;
    int uMultibeamPixmapWidth = (RadarVideo.first().m_uTotalNum)*2;
    int uMultibeamPixmapHeight = (RadarVideo.first().m_uTotalNum)*2;
    QPixmap objPixmap(uMultibeamPixmapWidth,uMultibeamPixmapHeight);
    objPixmap.fill(Qt::transparent);//用透明色填充
    //objPixmap.fill(QColor(200,200,200,100));
    QPainter objPainter(&objPixmap);
    objPainter.setRenderHint(QPainter::Antialiasing,true);
    objPainter.save();//1_保存油漆状态在堆栈上,后续必须end()在堆栈中取出来,解除堆栈。
    objPainter.translate(QPoint(uMultibeamPixmapWidth / 2, uMultibeamPixmapHeight / 2));    // 中心点
    QList<int> amplitudeList;
    QList<int> indexList;
    QPointF objPoint;
    QMap<int,RADAR_VIDEO_DATA>::iterator it;
    int a = 0;
    for(it = RadarVideo.begin();it!=RadarVideo.end();it++)
    {
//        a++;
//        cout<<"大小"<<RadarVideo.size()<<"循环了几次"<<a<<"传进来的扫描线"<<dataAfterglow.m_RadarVideo.size();
//        cout<<"多少条线" <<it.key()<<it.value().m_uMsgIndex<<RadarVideo.begin().value().m_uMsgIndex<<"循环了几次"<<a;
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

//            if(m_bLimit)//限制区域
//            {
//                if(!inLimitArea(dCentreLat,dCentreLon,dAzimuth,position,dStartDis,dDisInterval))
//                {
//                    continue;
//                }
//            }
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
                nPenWidth = 4;
            }
            else if(position>=800&&position<1000)
            {
                nPenWidth = 4;
            }
            else if(position>=1000)
            {
                nPenWidth = 4;
            }
            //objPainter.setPen(QPen(objColor,nPenWidth));
            //objPainter.setPen(QPen(QColor(37,124,37),nPenWidth));
            //objPainter.setPen(QPen(QColor(68,131,216),pen_width));//1_改成1个像素点
            objPainter.setPen(QPen(QColor(objColor),pen_width));//1_改成1个像素点
            objPoint.setX(0);
            objPoint.setY(position);
            objPainter.drawPoint(objPoint);

        }
        objPainter.rotate(-(dAzimuth+180));
    }
    objPainter.translate(QPoint(-uMultibeamPixmapWidth / 2, -uMultibeamPixmapHeight / 2));
    objPainter.restore();
    RadarVideo.clear();
    videoPixmap = objPixmap;
    return videoPixmap;

}

void ZCHXDrawRadarVideo::slotSetPenwidth(int str)//设置笔的宽度
{
    pen_width = str;
}

void ZCHXDrawRadarVideo::slotDrawAfterglow(const Afterglow &dataAfterglow)
{

    m_bProcessing = true;
//    QMutex mutex;
//    mutex.lock();
    QMap<int,RADAR_VIDEO_DATA> RadarVideo = dataAfterglow.m_RadarVideo;
    //qDebug()<<"slotDrawAfterglow--------------------------";
    if(RadarVideo.isEmpty())
    {
        m_bProcessing = false;
        return;
    }
    QPixmap videoPixmap = drawRadarVideoPixmap(dataAfterglow);

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
        objPainter.rotate(dAzimuth+180);
        prePainter.rotate(dAzimuth+180);
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
            //cout <<"nPenWidth:"<<nPenWidth;
            //objPainter.setPen(QPen(objColor,nPenWidth));
            //objPainter.setPen(QPen(QColor(37,124,37),nPenWidth));
            //objPainter.setPen(QPen(QColor(68,131,216),pen_width));//1_该为一个像素点
            objPainter.setPen(QPen(QColor(252,241,1),pen_width));//1_该为一个像素点
            objPoint.setX(0);
            objPoint.setY(position);
            objPainter.drawPoint(objPoint);
            //
            //prePainter.setPen(QPen(QColor(123,215,123),nPenWidth));
            //prePainter.setPen(QPen(QColor(123,215,123),pen_width));//1_该为一个像素点
            //prePainter.setPen(QPen(QColor(139,166,202),pen_width));//1_该为一个像素点,改颜色
            prePainter.setPen(QPen(QColor(19,118,61),pen_width));//1_该为一个像素点,改颜色
            prePainter.drawPoint(objPoint);

        }
        objPainter.rotate(-(dAzimuth+180));
        prePainter.rotate(-(dAzimuth+180));
    }
    objPainter.translate(QPoint(-uMultibeamPixmapWidth / 2, -uMultibeamPixmapHeight / 2));
    objPainter.restore();
    prePainter.translate(QPoint(-uMultibeamPixmapWidth / 2, -uMultibeamPixmapHeight / 2));
    prePainter.restore();
    //QPixmap resultPixmap = processPixmap(objPixmap);
    signalRadarAfterglowPixmap(m_uAfterglowIndex,videoPixmap,objPixmap,resultPixmap);
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
       // cout<<"颜色5";
        return QColor(252,241,1);
    }
   // cout<<"颜色6";
    //QColor objColor = m_colorMap[uIndex];
    QColor objColor = QColor(19,118,61);
    return objColor;
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
