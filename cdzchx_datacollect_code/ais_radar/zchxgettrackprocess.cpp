#include "zchxgettrackprocess.h"
#include <QDebug>
#include <QLibrary>
#include <QDateTime>
#include <QMutex>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
////////////////////////////////////////////////////////////////////////////////

typedef int(*FUN2)(struct SAzmData* psScan, int* pSit);
extern  FUN2 Tracking_Fun = NULL;

////////////////////////////////////////////////////////////////////////////////////

ZCHXGetTrackProcess::ZCHXGetTrackProcess(double dLat, double dLon, QObject *parent)
    : QObject(parent),
      m_dCentreLat(dLat),
      m_dCentreLon(dLon)
{

    qRegisterMetaType<SAzmData>("SAzmData");
    qRegisterMetaType<ITF_Track_point>("ITF_Track_point");
    qRegisterMetaType<SAzmDataList>("const SAzmDataList&");
    qRegisterMetaType<TrackObjList>("const TrackObjList&");
    //调用小雷达目标库
    QLibrary lib("Record.dll");
    if (lib.load())
    {
        qDebug() << "load ok!";


        Tracking_Fun = (FUN2)lib.resolve("?Tracking@@YAHPEAUSAzmData@@PEAH@Z");

        if (Tracking_Fun) {
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
    connect(this,SIGNAL(getTrackProcessSignal(SAzmData)),
            this,SLOT(getTrackProcessSlot(SAzmData)));
    connect(this, SIGNAL(getTrackProcessSignal(SAzmDataList)),
            this, SLOT(getTrackProcessSlot(SAzmDataList)));
    moveToThread(&m_workThread);
    m_workThread.start();
}

ZCHXGetTrackProcess::~ZCHXGetTrackProcess()
{
    if(m_workThread.isRunning())
    {

        m_workThread.quit();

    }
    m_workThread.terminate();
}

void ZCHXGetTrackProcess::getTrackProcessSlot( SAzmData sAzmData)
{
    QTime t1 = QTime::currentTime();
    t1.start();
    QMutex mutex;
    mutex.lock();
    std::list<TrackInfo> trackList;
    int igSit[102400];
    int iTrack = 0;
    int* piSit = &(igSit[2]);
    iTrack = Tracking_Fun(&sAzmData, igSit);
    // 跟踪结果写盘
    if (iTrack == 1)
    {

        qDebug()<<__FUNCTION__<<__LINE__<<t1.elapsed()<<iTrack;
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
                ITF_Track_point trackPoint;
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
                trackPoint.set_timeofday(sAzmData.sHead.iTime);
                trackPoint.set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(0));
                trackPoint.set_tracklastreport(0);
                trackPoint.set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(1));
                trackPoint.set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(1));
                trackPoint.set_sigmax(0);
                trackPoint.set_sigmay(0);
                trackPoint.set_sigmaxy(0);
                trackPoint.set_ampofpriplot(0);
                double car_vX = trackInfo.fSpeed* cos(trackInfo.fCourse);;
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
//                                  << "当日时间 :" << sAzmData.sHead.iTime << " \n"
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

                //m_radarPointMap[trackInfo.iTraIndex] = trackPoint;
                emit sendTrack(trackInfo.iTraIndex,trackPoint);


            }

            trackList.clear();
        }
        //ZCHXLOG_INFO("雷达目标数据组装完成");
    }

    mutex.unlock();
}

void ZCHXGetTrackProcess::getTrackProcessSlot( const SAzmDataList& sAzmDataList)
{
    TrackObjList resList;
    //cout<<sAzmDataList.size();
    foreach (SAzmData sAzmData, sAzmDataList) {
        std::list<TrackInfo> trackList;
        int igSit[102400];
        int iTrack = 0;
        int* piSit = &(igSit[2]);
        iTrack = Tracking_Fun(&sAzmData, igSit);
        // 跟踪结果写盘
        if (iTrack == 1)
        {
            qDebug() <<"iTrack"<<iTrack;
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
                    ITF_Track_point trackPoint;
                    //std::string pstrTrack;
                    trackPoint.set_systemareacode(0);
                    trackPoint.set_systemidentificationcode(1);
                    trackPoint.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
                    trackPoint.set_tracknumber(trackInfo.iTraIndex);

                    double tempWgs84PosLat = 0.0;       //纬度
                    double tempWgs84PosLong = 0.0;      //经度

                    LatLong startLatLong(m_dCentreLon,m_dCentreLat);

                    double t_azm = trackInfo.fAzm * 180 / PI; //(-180,180),正比0'（顺时针增加）

                    if (t_azm<0)
                    {
                        t_azm = 360 + t_azm;
                    }
                    double posx = 0, posy = 0;
                    double tempWgs84PosLat_1 = 0.0;       //纬度
                    double tempWgs84PosLong_1 = 0.0;      //经度
                    getLatLong(startLatLong, trackInfo.fRng/1000, t_azm, tempWgs84PosLat, tempWgs84PosLong);
//                    distbearTolatlon(m_dCentreLat, m_dCentreLon, trackInfo.fRng, t_azm, tempWgs84PosLat_1, tempWgs84PosLong_1);
//                    qDebug()<<QString::number(tempWgs84PosLong, 'f', 6)<<QString::number(tempWgs84PosLat, 'f', 6)<<QString::number(tempWgs84PosLong_1, 'f', 6)<<QString::number(tempWgs84PosLat_1, 'f', 6);
                    //getLatLong_1(startLatLong, trackInfo.fRng/1000, t_azm, tempWgs84PosLat_1, tempWgs84PosLong_1, posx, posy);
                    float cartesianposx = 0;
                    float cartesianposy = 0;


                    cartesianposx = trackInfo.fRng*cos(trackInfo.fAzm);
                    cartesianposy = trackInfo.fRng*sin(trackInfo.fAzm);

//                    trackPoint.set_cartesianposx(cartesianposx*1.18);
//                    trackPoint.set_cartesianposy(cartesianposy*1.18);
                    trackPoint.set_cartesianposx(cartesianposx);
                    trackPoint.set_cartesianposy(cartesianposy);


//                    cout<<"[原始积坐标_x]:"<<cartesianposx<<"[原始积坐标_y]:"<<cartesianposy;
//                    cout<<"[转换积坐标_x]:"<<posy         <<"[转换积坐标_y]:"<<posx;
//                    cout<<"[原始经纬度_经度]:"<<tempWgs84PosLat  <<"[原始经纬度_经度]:"<<tempWgs84PosLong;
//                    cout<<"[转换经纬度_经度]:"<<tempWgs84PosLat_1<<"[转换经纬度_经度]:"<<tempWgs84PosLong_1;


                    trackPoint.set_wgs84poslat(tempWgs84PosLat);
                    trackPoint.set_wgs84poslong(tempWgs84PosLong);
//                    trackPoint.set_wgs84poslat(tempWgs84PosLat_1);
//                    trackPoint.set_wgs84poslong(tempWgs84PosLong_1);
                    trackPoint.set_timeofday(sAzmData.sHead.iTime);
                    trackPoint.set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(0));
                    trackPoint.set_tracklastreport(0);
                    trackPoint.set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(1));
                    trackPoint.set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(1));
                    trackPoint.set_sigmax(0);
                    trackPoint.set_sigmay(0);
                    trackPoint.set_sigmaxy(0);
                    trackPoint.set_ampofpriplot(0);
                    double car_vX = trackInfo.fSpeed* cos(trackInfo.fCourse);;
                    double car_vY = trackInfo.fSpeed* sin(trackInfo.fCourse);
                    trackPoint.set_cartesiantrkvel_vx(car_vX);
                    trackPoint.set_cartesiantrkvel_vy(car_vY);

                    double t_cog = t_azm;
                    trackPoint.set_cog(trackInfo.fCourse*180/PI);
                    trackPoint.set_sog(trackInfo.fSpeed);

                    TrackObj obj;
                    obj.uKey = trackInfo.iTraIndex;
                    obj.radarPoint = trackPoint;
                    resList.append(obj);
                    //cout<<resList.size();
                    //emit sendTrack(trackInfo.iTraIndex,trackPoint);


                }

                trackList.clear();
            }
            //ZCHXLOG_INFO("雷达目标数据组装完成");
        }
    }
 //   qDebug()<<__FUNCTION__<<__LINE__<<sAzmDataList.size()<<resList.size();
    if(resList.size() > 0)
    {
        //cout<<"本次解析有数据"<<resList.size();
        qDebug()<<resList.size();
        emit sendTrack(resList);
    }
}



