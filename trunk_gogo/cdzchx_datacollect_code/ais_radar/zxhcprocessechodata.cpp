#include "zxhcprocessechodata.h"
#include "../profiles.h"
#include <QLibrary>
extern "C"

{

#include "ctrl.h"

}

typedef int(*FUN1)(struct SAzmData* psScan, int* pSit);
extern  FUN1 Tracking_Fun1 = NULL;
ZXHCProcessEchoData::ZXHCProcessEchoData(QObject *parent) : QObject(parent)
{
    connect(this,SIGNAL(signalProcess(Map_RadarVideo)),this,SLOT(slotProcess(Map_RadarVideo)));
    m_uTrackSendPort = Utils::Profiles::instance()->value("Echo","Track_Send_Port").toInt();
    m_sTrackTopic = Utils::Profiles::instance()->value("Echo","Track_Topic").toString();
    m_uCellNum = Utils::Profiles::instance()->value("Echo","Cell_Num").toInt();
    m_uLineNum = Utils::Profiles::instance()->value("Echo","Line_Num").toInt();
    m_uHeading = Utils::Profiles::instance()->value("Echo","Heading").toInt();
    m_dCentreLat = Utils::Profiles::instance()->value("Echo","Centre_Lat").toDouble();
    m_dCentreLon = Utils::Profiles::instance()->value("Echo","Centre_Lon").toDouble();
    m_clearRadarTrackTime = Utils::Profiles::instance()->value("Echo","ClearTrack_Time").toInt();
    //调用小雷达目标库
    QLibrary lib("Record.dll");
    if (lib.load())
    {
        qDebug() << "load ok!";


        Tracking_Fun1 = (FUN1)lib.resolve("?Tracking@@YAHPEAUSAzmData@@PEAH@Z");

        if (Tracking_Fun1) {
            qDebug() << "load Tracking ok!";
        }
        else
        {
            qDebug() << "resolve Tracking error!";
        }
    }
    else
    {
        qDebug() << "load error!";
    }

    //发送雷达目标的zmq
    //创建context，zmq的socket 需要在context上进行创建
    m_pTrackContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pTrackLisher= zmq_socket(m_pTrackContext, ZMQ_PUB);

    moveToThread(&m_workThread);
    m_workThread.start();
}

void ZXHCProcessEchoData::slotProcess(const Map_RadarVideo &radarVideoMap)
{
    qDebug()<<"从张邦伟那里接受雷达数据";
    //打印日志接收到回波数据
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar video data to get track data");
    //emit signalSendRecvedContent(utc,"VIDEO________RECEIVE",sContent);

    struct SAzmData sAzmData;
    std::list<TrackInfo> trackList;
    QMap<int, ITF_VideoFrame >::const_iterator itor = radarVideoMap.begin();
    for(itor;itor!=radarVideoMap.end();itor++)
    {
        ITF_VideoFrame objVideoFrame = itor.value();
        // 调用跟踪函数
        int igSit[102400];
        int iTrack = 0;
        int* piSit = &(igSit[2]);

        sAzmData.sHead.iArea = 1;
        sAzmData.sHead.iSys = 1;
        sAzmData.sHead.iMsg = objVideoFrame.msgindex();
        sAzmData.sHead.iAzm = objVideoFrame.azimuth();
        sAzmData.sHead.iHead = m_uHeading;
        sAzmData.sHead.fR0 = objVideoFrame.startrange();
        sAzmData.sHead.fDR = objVideoFrame.rangefactor();
        sAzmData.sHead.iBit = 4;
        sAzmData.sHead.iTime = objVideoFrame.timeofday();
        for (int range = 0; range < m_uCellNum; range++)
        {
            int value =  (int)objVideoFrame.amplitude(range);
            sAzmData.iRawData[range] = 0;
            if(value>0)
            {
                {
                    sAzmData.iRawData[range] = value;
                }
            }
        }


        //qDebug() <<"Tracking_Fun";
        iTrack = Tracking_Fun1(&sAzmData, igSit);
        //qDebug() <<"iTrack"<<iTrack;
        // 跟踪结果写盘
        if (iTrack == 1)
        {
            //qDebug() <<"iTrack"<<iTrack;
            trackList.clear();
            piSit = OutPlot(igSit[1], piSit);
            piSit = OutTrack(igSit[1], piSit, trackList);
            piSit = &(igSit[2]);

            size_t trackLen = trackList.size();
            //qDebug()<<"track size: " << trackLen;
            if (trackLen)
            {
                std::list<TrackInfo>::iterator iter = trackList.begin();
                for (; iter != trackList.end(); iter++)
                {
                    TrackInfo trackInfo = *iter;
                    com::zhichenhaixin::proto::TrackPoint trackPoint;
                    //std::string pstrTrack;
                    trackPoint.set_systemareacode(0);
                    trackPoint.set_systemidentificationcode(1);
                    trackPoint.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
                    trackPoint.set_tracknumber(trackInfo.iTraIndex);

                    double tempWgs84PosLat = 0.0;
                    double tempWgs84PosLong = 0.0;

                    LatLong startLatLong(m_dCentreLon,m_dCentreLat);

                    double t_azm = trackInfo.fAzm * 180 / PI; //(-180,180),正比0'（顺时针增加）

                    if (t_azm<0)
                    {
                        t_azm = 360 + t_azm;
                    }
                    getLatLong(startLatLong, trackInfo.fRng/1000, t_azm, tempWgs84PosLat, tempWgs84PosLong);
                    //getLatLong(startLatLong, trackInfo.fRng / 1000, trackInfo.fAzm * 180 / PI, tempWgs84PosLat, tempWgs84PosLong);

                    //std::pair<double, double> latLonPair(tempWgs84PosLat,tempWgs84PosLong);
                    //latLonVec.push_back(latLonPair);

                    float cartesianposx = 0;
                    float cartesianposy = 0;

                    cartesianposx = trackInfo.fRng*cos(trackInfo.fAzm);
                    cartesianposy = trackInfo.fRng*sin(trackInfo.fAzm);

                    trackPoint.set_cartesianposx(cartesianposx);
                    trackPoint.set_cartesianposy(cartesianposy);

                    trackPoint.set_wgs84poslat(tempWgs84PosLat);
                    trackPoint.set_wgs84poslong(tempWgs84PosLong);
                    trackPoint.set_timeofday(objVideoFrame.timeofday());
                    trackPoint.set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(0));
                    trackPoint.set_tracklastreport(0);
                    trackPoint.set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(1));
                    trackPoint.set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(1));
                    trackPoint.set_sigmax(0);
                    trackPoint.set_sigmay(0);
                    trackPoint.set_sigmaxy(0);
                    trackPoint.set_ampofpriplot(0);
                    double car_vX = trackInfo.fSpeed* cos(trackInfo.fCourse);
                    double car_vY = trackInfo.fSpeed* sin(trackInfo.fCourse);
                    trackPoint.set_cartesiantrkvel_vx(car_vX);
                    trackPoint.set_cartesiantrkvel_vy(car_vY);

                    double t_cog = t_azm;
                    trackPoint.set_cog(trackInfo.fCourse*180/PI);
                    trackPoint.set_sog(trackInfo.fSpeed);


//                    qDebug()<<"轨迹点信息如下:"<< "  \n"
//                                  << "系统区域代码: "<< trackPoint.systemareacode() << "  \n"
//                                  << "系统识别代码 :" << trackPoint.systemidentificationcode() << "  \n"
//                                  << "消息类型  :" << trackPoint.messagetype() << "  \n"
//                                  << "航迹号  :" << trackInfo.iTraIndex << "  \n"
//                                  << "笛卡尔坐标计算X位置 :" << cartesianposx << " \n"
//                                  << "笛卡尔坐标计算Y位置 :" << cartesianposy << " \n"
//                                  << "经度 :" << tempWgs84PosLong  << " \n"
//                                  << "纬度 :" << tempWgs84PosLat << " \n"
//                                  << "当日时间 :" << objVideoFrame.timeofday() << " \n"
//                                  << "航迹状态 :" << trackPoint.tracktype() << " \n"
//                                  << "当前目标最后一次上报 :" << trackPoint.tracklastreport() << " \n"
//                                  << "外推法 :" << trackPoint.extrapolation() << " \n"
//                                  << "位置来历 :" << trackPoint.trackpositioncode() << " \n"
//                                  << "x轴标准差 :" << trackPoint.sigmax() << " \n"
//                                  << "y轴标准差 :" << trackPoint.sigmay() << " \n"
//                                  << "2轴平方方差 :" << trackPoint.sigmaxy() << " \n"
//                                  << "震荡波强度检测 :" << trackPoint.ampofpriplot() << " \n"
//                                  << "迪卡尔坐标航迹计算x速度(米/秒) :" << car_vX << " \n"
//                                  << "迪卡尔坐标航迹计算y速度(米/秒) :" << car_vY << " \n"
//                                  << "方位角 :" << trackInfo.fCourse*180/PI << " \n"
//                                  << "速度 :" << trackInfo.fSpeed << " \n";

                    m_radarPointMap[trackInfo.iTraIndex] = trackPoint;


                }

                trackList.clear();
            }
            //ZCHXLOG_INFO("雷达目标数据组装完成");
        }
        else
        {
            //ZCHXLOG_ERROR("跟踪 video 失败!!!");
        }
    }
    clearRadarTrack();
    sendRadarTrack();
}

void ZXHCProcessEchoData::sendRadarTrack()
{
    if(m_radarPointMap.size()<=0)
        return;
    //通过zmq发送

    int uNum = m_radarPointMap.size();
    QString sNum = QString::number(uNum);
    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uTrackSendPort);

    QString sTopic = m_sTrackTopic;
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();
    QByteArray sNumArray = sNum.toUtf8();

    zmq_bind(m_pTrackLisher, sIPport.toLatin1().data());//
    zmq_send(m_pTrackLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pTrackLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pTrackLisher, sNumArray.data(), sNumArray.size(), ZMQ_SNDMORE);
    QMap<int,com::zhichenhaixin::proto::TrackPoint>::iterator itor = m_radarPointMap.begin();
    int uIndex = 0;
    for(itor;itor!=m_radarPointMap.end();itor++,uIndex++)
    {
        com::zhichenhaixin::proto::TrackPoint objRadarPoint = itor.value();
        QByteArray sendData;
        sendData.resize(objRadarPoint.ByteSize());
        objRadarPoint.SerializePartialToArray(sendData.data(),sendData.size());
        if(uIndex!=uNum-1)
        {
            zmq_send(m_pTrackLisher, sendData.data(), sendData.size(), ZMQ_SNDMORE);
        }
        else
        {
            zmq_send(m_pTrackLisher, sendData.data(), sendData.size(), 0);
        }

    }

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("send analysis radar track data,num = %1").arg(uNum);
    emit signalSendRecvedContent(utc,"TRACK________SEND",sContent);
}

void ZXHCProcessEchoData::clearRadarTrack()
{
    //间隔一定时间清理回波数据
    int nInterval = m_clearRadarTrackTime;//秒

    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    int time_of_day = startDateTime.secsTo(curDateTime);
    QMap<int,com::zhichenhaixin::proto::TrackPoint>::iterator itor = m_radarPointMap.begin();
    //qDebug()<<"ITF_RadarPoint num "<<m_radarPointMap.size();
    for(itor;itor!=m_radarPointMap.end();)
    {
        com::zhichenhaixin::proto::TrackPoint objRadarPoint = itor.value();
        int uKey = itor.key();
        //qDebug()<<"time_of_day"<<time_of_day;
        //qDebug()<<"objRadarPoint.tracknumber()"<<objRadarPoint.tracknumber();
       // qDebug()<<"objRadarPoint.timeofday()"<<objRadarPoint.timeofday();
        if(time_of_day-objRadarPoint.timeofday()>nInterval)
        {
            //qDebug()<<"remove";
            itor = m_radarPointMap.erase(itor);
        }
        else
        {
            itor++;
        }
    }
}
