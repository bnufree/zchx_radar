#include "zchxvideorects.h"
#include <QDebug>
#include <QLibrary>
#include <QDateTime>
#include <QMutex>
#include "../profiles.h"
#include <QList>
#include "Log.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

//int g_searchTable[MAX_LINE_NUM][MAX_CELL_NUM];

zchxVideoRects::zchxVideoRects(int m_uSourceID ,QObject *parent) : QObject(parent)
{
    finish = 1;
    sizeOfnodes = 0;
    num = 0;
    str_radar = QString("Radar_%1").arg(m_uSourceID);
    m_dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toDouble();
    m_dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toDouble();
    qRegisterMetaType<zchxTrackPoint>("const zchxTrackPoint&");
    qRegisterMetaType<zchxTrackPointList>("const zchxTrackPointList&");
    qRegisterMetaType<zchxVideoFrameList>("const zchxVideoFrameList&");
    qRegisterMetaType<QList<TrackNode>>("QList<TrackNode>");
    qRegisterMetaType<QMap<int,RADAR_VIDEO_DATA>>("QMap<int,RADAR_VIDEO_DATA>");

    m_bLimit = Utils::Profiles::instance()->value(str_radar,"Limit").toBool();
    m_limit_file = Utils::Profiles::instance()->value(str_radar,"Limit_File").toString();//读取限制区域文件
    //m_workThread.setStackSize(32000000);
    increaseNum = 0;
    rangeFactor = 0;
    m_workThread.setStackSize(64000000);
    moveToThread(&m_workThread);
    m_workThread.start();

    readRadarLimitFormat();
}

zchxVideoRects::~zchxVideoRects()
{

}

void zchxVideoRects::analysisVideoPieceSlot(QMap<int,RADAR_VIDEO_DATA> RadarVideo,double nRange)
{
//    cout<<"开始解析回波块大小"<<RadarVideo.size();
//    cout<<"开始时间:"<<QTime::currentTime();
    QTime t;
    t.start();
    int uMultibeamPixmapWidth = (RadarVideo.first().m_uTotalNum)*2;
    int uMultibeamPixmapHeight = (RadarVideo.first().m_uTotalNum)*2;
    QList<int> amplitudeList;
    QList<int> indexList;
    QPointF objPoint;
    QMap<int,RADAR_VIDEO_DATA>::iterator it;
    QList<TrackNode> nodeList;
    for (it = RadarVideo.begin(); it != RadarVideo.end(); it++)
    {
        //cout<<"it.key"<<it.key();
        RADAR_VIDEO_DATA data = it.value();
        rangeFactor = data.m_dRangeFactor;
        double dAzimuth = data.m_uAzimuth * (360.0 / data.m_uLineNum) + data.m_uHeading;
        double dArc = dAzimuth * PI / 180.0;
        amplitudeList = data.m_pAmplitude;
        indexList = data.m_pIndex;
        for (int i = 0; i < amplitudeList.size(); i++)
        {
            int position = indexList[i];
            int value = amplitudeList[i];

            //直接计算出所有黄色回波块点的坐标
            float x = 0;
            float y = 0;
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

            //转笛卡尔坐标,经纬度
            double dkx = (nRange / (uMultibeamPixmapWidth / 2)) * (-objPoint.y());
            double dky = (nRange / (uMultibeamPixmapHeight / 2)) * (objPoint.x());
            //cout<<"笛卡转屏幕实际坐标"<<dkx<<dky;
//            double mx = dkx / (nRange / (uMultibeamPixmapWidth / 2));
//            double my = dky / (nRange / (uMultibeamPixmapHeight / 2));
            //笛卡尔转经纬度
            LatLong startLatLong(m_dCentreLon,m_dCentreLat);
            double lat ;
            double lon ;
            getNewLatLong(startLatLong,lat, lon, dkx, dky);
            //cout<<"经纬度"<<lon<<lat;
            if(position < 2)continue;
            //先过滤
            //cout<<"m_bLimit"<<m_bLimit;
            if(m_bLimit) {
                if(!inLimitAreaForTrack(lat, lon)) {
                    continue;
                }
            }
            std::pair<double, double> latLonPair(lat, lon);
            //cout<<"value"<<value;
            int min_amplitude = Utils::Profiles::instance()->value(str_radar,"min_amplitude").toInt();
            int max_amplitude = Utils::Profiles::instance()->value(str_radar,"max_amplitude").toInt();

            if(value<=max_amplitude && value >= min_amplitude)//value == 255
            {
                QPointF trackP(it.key(),position);
                TrackNode mTrackNode;
                mTrackNode.have_value = 0;
                mTrackNode.latlonPair = latLonPair;
                mTrackNode.trackP = trackP.toPoint();
                mTrackNode.amplitude = value;
                mTrackNode.index = position;
                nodeList.append(mTrackNode);
            }

        }
    }

    //这里num小于10就赋值,什么意思?现在就会导致一直不解析
#if 0
    if(sizeOfnodes == 0)
        sizeOfnodes = nodeList.size();
    if(num < 10)
    {
        //cout<<"num"<<num<<sizeOfnodes;
        num++;
        sizeOfnodes = nodeList.size();
    }
    if(nodeList.size() - sizeOfnodes > 5000)
    {
        cout<<"目标突然增多,不识别"<<nodeList.size()<<sizeOfnodes;
        return;
    }
    else
    {
        sizeOfnodes = nodeList.size();
    }
#endif
    slotDealViedoTrack(nodeList);
    qDebug()<<"analysisVideoPieceSlot elapsed:"<<t.elapsed()<<nodeList.size();
}

void zchxVideoRects::readRadarLimitFormat()
{
    QString path = QCoreApplication::applicationDirPath();
    QString pathName;
    QRegExp na("(\/)(\\w)+(\\.)(json)"); //初始化名称结果
    QString name("");
    if(na.indexIn(m_limit_file) != -1)
    {
        //匹配成功
        name = na.cap(0);
        //cout<<"name"<<name;
    }
    //cout<<"打印区域文件地址";
    //pathName = m_limit_file;
    pathName = path+name;
    //cout<<"地址pathName"<< pathName;
    m_landPolygon.clear();
    m_seaPolygon.clear();
    analysisLonLatTOPolygon(pathName,m_landPolygon,m_seaPolygon);

    //test
}

//从区域文件读取限制区域
void zchxVideoRects::analysisLonLatTOPolygon(const QString sFileName, QList<QPolygonF> &landPolygon, QList<QPolygonF> &seaPolygon)
{
    if(sFileName.isEmpty())
    {
        return;
    }
    landPolygon.clear();
    seaPolygon.clear();
    //qDebug()<<"filepath"<<sFileName;
    QFile objFile(sFileName);
    if(!objFile.open(QIODevice::ReadOnly))
    {
        return;
    }
    QString sAllData = "";
    while (!objFile.atEnd())
    {
        QByteArray LineArray = objFile.readLine();
        QString str(LineArray);
        str = str.trimmed();
        sAllData +=str;
    }
    //cout<<"sAllData"<<sAllData;


    QJsonParseError err;
    QJsonDocument docRcv = QJsonDocument::fromJson(sAllData.toLatin1(), &err);

    if(err.error != QJsonParseError::NoError)
    {
        qDebug()<<"parse completetion list error:"<<err.error;
        return ;
    }
    if(!docRcv.isObject())
    {
        qDebug()<<" status statistics list with wrong format.";
        return ;
    }
    QJsonArray objSeaArray = docRcv.object().value("watercourse").toArray();
    QJsonArray objLandArray1 = docRcv.object().value("land1").toArray();
    QJsonArray objLandArray2 = docRcv.object().value("land2").toArray();
    cout<<"objSeaArray.size():--------"<<objSeaArray.size();
    cout<<"objLandArray1.size():--------"<<objLandArray1.size();
    cout<<"objLandArray2.size():--------"<<objLandArray2.size();
    QVector<QPointF> pointVec;
    for(int i = 0; i < objSeaArray.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objSeaArray.at(i).toArray();
        cout<<"objSeaArray.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        seaPolygon.append(objPolygon);
    }
    cout<<"seaPolygon.size()"<<seaPolygon.size();
    for(int i = 0; i < objLandArray1.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objLandArray1.at(i).toArray();
        cout<<"objLandArray1.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        landPolygon.append(objPolygon);
    }

    for(int i = 0; i < objLandArray2.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objLandArray2.at(i).toArray();
        cout<<"objLandArray2.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        landPolygon.append(objPolygon);
    }
    //陆地3
    QJsonArray objLandArray3 = docRcv.object().value("land3").toArray();
    for(int i = 0; i < objLandArray3.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objLandArray3.at(i).toArray();
        cout<<"objLandArray3.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        landPolygon.append(objPolygon);
    }
    cout<<"landPolygon.size()"<<landPolygon.size();


}

//区域限制功能
bool zchxVideoRects::inLimitAreaForTrack(const double dLat, const double dLon)
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

//回波块识别目标函数
void zchxVideoRects::slotDealViedoTrack(QList<TrackNode> nodeList)
{
    //cout<<"nodeList"<<nodeList.size()<<QDateTime::currentDateTime();
    if(finish == 0 || nodeList.size() ==0)
    {
        return;
    }
    finish = 0;
    memset(g_searchTable, '\0', sizeof(g_searchTable));
    for(int i = 0; i < nodeList.size(); i++)
    {
        int x = nodeList[i].trackP.x();
        int y = nodeList[i].trackP.y();
        if (x >= 0 && x < MAX_LINE_NUM && y >= 0 && y < MAX_CELL_NUM)
        {
            g_searchTable[x][y] = i + 1;
        }
    }
    //目标集合
    QMap<int, QList<TrackNode>> mTrackMap;
    //cout<<"开始"<<nodeList.size();
    for(int i= 0; i < nodeList.size(); i++)
    {
        //cout<<"i----------------------"<<i;
        TrackNode mTrackNode = nodeList[i];
        //若果该点已经属于某个目标,进入下一次循环
        if(mTrackNode.have_value == 1)
        {
            //cout<<"跳过";
            continue;
        }
        else
        {
            mTrackNode.have_value = 1;
            //右,下,左循环递归遍历
            QList<TrackNode> list;
            recursionSearchProcess(i, nodeList, g_searchTable, list);

            //cout<<"list"<<list.size();
            track_max_radius = Utils::Profiles::instance()->value(str_radar,"track_radius").toDouble() * rangeFactor;
            track_min_radius =  Utils::Profiles::instance()->value(str_radar,"track_min_radius").toDouble() * rangeFactor;
            if(list.size()>track_max_radius || list.size() < track_min_radius) continue;
            mTrackMap[i] = list;

        }
    }

    //cout<<"完成"<<nodeList.size();
    this->signalSendTrackNodes(mTrackMap);
    finish = 1;
}

//遍历递归函数
void zchxVideoRects::recursionSearchProcess(int k, QList<TrackNode> &nodeList, int searchTable[MAX_LINE_NUM][MAX_CELL_NUM], QList<TrackNode> &list)
{
    //cout<<"遍历递归函数"<<k;
    int i = k;
    int x = nodeList[i].trackP.x();
    int y = nodeList[i].trackP.y();

    //右递归
    int nextX = x + 1;
    int nextY = y;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //右下
    nextX = x + 1;
    nextY = y - 1;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //下
    nextX = x;
    nextY = y - 1;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //左下
    nextX = x - 1;
    nextY = y - 1;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //左
    nextX = x - 1;
    nextY = y;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //左上
    nextX = x - 1;
    nextY = y + 1;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //上
    nextX = x;
    nextY = y + 1;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //右上
    nextX = x + 1;
    nextY = y + 1;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);
/*
    //2个格子
    //右递归
    nextX = x + 2;
    nextY = y;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //右下
    nextX = x + 2;
    nextY = y - 2;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //下
    nextX = x;
    nextY = y - 2;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //左下
    nextX = x - 2;
    nextY = y - 2;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //左
    nextX = x - 2;
    nextY = y;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //左上
    nextX = x - 2;
    nextY = y + 2;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //上
    nextX = x;
    nextY = y + 2;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);

    //右上
    nextX = x + 2;
    nextY = y + 2;
    nextRecursion(nextX, nextY, nodeList, searchTable, list);*/
}

void zchxVideoRects::nextRecursion(int nextX, int nextY, QList<TrackNode> &nodeList, int searchTable[MAX_LINE_NUM][MAX_CELL_NUM], QList<TrackNode> &list)
{
    if (nextX >= 0 && nextX < MAX_LINE_NUM && nextY >= 0 && nextY < MAX_CELL_NUM)
    {
        int i = searchTable[nextX][nextY] - 1;
        if (i >= 0 && nodeList[i].have_value == 0)
        {
            list.append(nodeList[i]);
            nodeList[i].have_value = 1;
            recursionSearchProcess(i, nodeList, searchTable, list);
        }
    }
}

//是否处理完成
bool zchxVideoRects::isFinishProcess()
{
    return finish;
}
