#include "ZCHXRadarDataChange.h"
#include "zchxradarrectthread.h"
#include "zchxradarpointthread.h"
#include "zchxradarechothread.h"
#include "zchxaisthread.h"
#include "zchxradarlimitareathread.h"
#include<QDebug>

using namespace ZCHX_RADAR_RECEIVER;


ZCHXRadarDataChange::ZCHXRadarDataChange(QObject *parent)
    : QObject(parent)
{

}

ZCHXRadarDataChange::~ZCHXRadarDataChange()
{
    stop();
}

void ZCHXRadarDataChange::stop()
{
    foreach (ZCHXReceiverThread* thread, mThreadList) {
        thread->setIsOver(true);
    }
}

void ZCHXRadarDataChange::appendRadarVideo(const ZCHX_Radar_Setting_Param &param)
{
    if(mRadarVideoList.contains(param)) return;
    ZCHXRadarEchoThread* thread = new ZCHXRadarEchoThread(param);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(signalRecvDataNow(int,int)), this, SIGNAL(signalRecvDataNow(int,int)));
    connect(thread, SIGNAL(signalConnectedStatus(bool, QString, QString)), this, SIGNAL(sendConnectionStatus(bool, QString, QString)));
    connect(thread, SIGNAL(sendMsg(int,double,double,double,int,int,int,QByteArray,QByteArray))
            , this, SIGNAL(sendRadarVideo(int,double,double,double,int,int,int,QByteArray,QByteArray)));
    mThreadList.append(thread);
    thread->start();
    mRadarVideoList.append(param);
}

void ZCHXRadarDataChange::appendAis(const ZCHX_Radar_Setting_Param &param)
{
    if(mAisList.contains(param)) return;
    ZCHXAisThread* thread = new ZCHXAisThread(param);
    connect(thread, SIGNAL(signalRecvDataNow(int,int)), this, SIGNAL(signalRecvDataNow(int,int)));
    connect(thread, SIGNAL(signalConnectedStatus(bool, QString, QString)), this, SIGNAL(sendConnectionStatus(bool, QString, QString)));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(sendMsg(QList<ZCHX::Data::ITF_AIS>)), this, SIGNAL(sendAisDataList(QList<ZCHX::Data::ITF_AIS>)));
    mThreadList.append(thread);
    thread->start();
    mAisList.append(param);
}

void ZCHXRadarDataChange::appendAisChart(const ZCHX_Radar_Setting_Param &param)
{
    if(mAisChartList.contains(param)) return;
    ZCHXAisChartThread* thread = new ZCHXAisChartThread(param);
    connect(thread, SIGNAL(signalRecvDataNow(int,int)), this, SIGNAL(signalRecvDataNow(int,int)));
    connect(thread, SIGNAL(signalConnectedStatus(bool, QString, QString)), this, SIGNAL(sendConnectionStatus(bool, QString, QString)));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(sendMsg(ZCHX::Data::ITF_AIS_Chart)), this, SIGNAL(sendAisChart(ZCHX::Data::ITF_AIS_Chart)));
    mThreadList.append(thread);
    thread->start();
    mAisChartList.append(param);
}

void ZCHXRadarDataChange::appendLimit(const ZCHX_Radar_Setting_Param &param)
{
    if(mLimitList.contains(param)) return;
    zchxRadarLimitAreaThread* thread = new zchxRadarLimitAreaThread(param);
    connect(thread, SIGNAL(signalRecvDataNow(int,int)), this, SIGNAL(signalRecvDataNow(int,int)));
    connect(thread, SIGNAL(signalConnectedStatus(bool, QString, QString)), this, SIGNAL(sendConnectionStatus(bool, QString, QString)));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(sendMsg(QList<ZCHX::Data::ITF_IslandLine>)), this, SIGNAL(sendLimitDataList(QList<ZCHX::Data::ITF_IslandLine>)));
    mThreadList.append(thread);
    thread->start();
    mLimitList.append(param);
}

void ZCHXRadarDataChange::appendRadarPoint(const ZCHX_Radar_Setting_Param &param)
{
    if(mRadarPointList.contains(param)) return;
    ZCHXRadarPointThread* thread = new ZCHXRadarPointThread(param);
    connect(thread, SIGNAL(signalRecvDataNow(int,int)), this, SIGNAL(signalRecvDataNow(int,int)));
    connect(thread, SIGNAL(signalConnectedStatus(bool, QString, QString)), this, SIGNAL(sendConnectionStatus(bool, QString, QString)));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(sendMsg(int,QList<ZCHX::Data::ITF_RadarPoint>)), this, SIGNAL(sendRadarPoint(int,QList<ZCHX::Data::ITF_RadarPoint>)));
    connect(thread, SIGNAL(sendMsg(QList<ZCHX::Data::ITF_RadarRouteNode>)),
            this, SIGNAL(sendRadarRoute(QList<ZCHX::Data::ITF_RadarRouteNode>)));
    mThreadList.append(thread);
    thread->start();
    mRadarPointList.append(param);

}

void ZCHXRadarDataChange::appendRadarPointList(const QList<ZCHX_Radar_Setting_Param>& list)
{
    foreach (ZCHX_Radar_Setting_Param param, list) {
        appendRadarPoint(param);
    }
}


void ZCHXRadarDataChange::appendRadarRect(const ZCHX_RadarRect_Param &param)
{
    if(mRadarRectList.contains(param)) return;
    ZCHXRadarRectThread* thread = new ZCHXRadarRectThread(param);
    connect(thread, SIGNAL(signalRecvDataNow(int,int)), this, SIGNAL(signalRecvDataNow(int,int)));
    connect(thread, SIGNAL(signalConnectedStatus(bool, QString, QString)), this, SIGNAL(sendConnectionStatus(bool, QString, QString)));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(sendVideoMsg(int, QList<ZCHX::Data::ITF_RadarRect>)), this, SIGNAL(sendRadarRect(int,QList<ZCHX::Data::ITF_RadarRect>)));
    mThreadList.append(thread);
    thread->start();
    mRadarRectList.append(param);
}

void ZCHXRadarDataChange::appendRadarRectList(const QList<ZCHX_RadarRect_Param>& list)
{
    foreach (ZCHX_RadarRect_Param param, list) {
        appendRadarRect(param);
    }
}

void ZCHXRadarDataChange::slotSetThreadStatus(int type, bool ison)
{
    bool found = false;
    foreach (ZCHXReceiverThread* thread, mThreadList) {
        if(thread->getType() == type){
            found = true;
            thread->setIsOver(!ison);
            break;
        }
    }
//    qDebug()<<"type thread found status:"<<type<<found;
}
