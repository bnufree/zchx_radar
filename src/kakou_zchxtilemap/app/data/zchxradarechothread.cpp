#include "zchxradarechothread.h"
#include <QDebug>
#include <QFile>
#include <QSettings>

using namespace ZCHX_RADAR_RECEIVER;

ZCHXRadarEchoThread::ZCHXRadarEchoThread(const ZCHX_Radar_Setting_Param& param, QObject *parent)
    : ZCHXReceiverThread(ZCHX_RECV_RADAR_VIDEO, param, parent)
{
    qRegisterMetaType<zchxRadarVideoImg>("const zchxRadarVideoImg&");
    m_lastUpdateRadarEchoTime = 0;
}

void ZCHXRadarEchoThread::parseRecvData(const QByteArrayList& list)
{
    if(list.size() == 0) return;
    zchxFuntionTimer t(mRadarCommonSettings.m_sTopicList.join(","));

    zchxRadarVideoImg objVideoImg;

    if(!objVideoImg.ParseFromArray(list.last().data(), list.last().size())) return;
    dealRadarEchoData(objVideoImg);

}


void ZCHXRadarEchoThread::dealRadarEchoData(const zchxRadarVideoImg &objVideoImg)
{
    double dCentreLon = objVideoImg.center().longitude();
    double dCentreLat = objVideoImg.center().latitude();
    double dDistance = objVideoImg.radius();
    QByteArray objPixmap = QByteArray(objVideoImg.imagedata().c_str(), objVideoImg.imagedata().length());

    emit sendMsg(mRadarCommonSettings.m_sSiteID, dCentreLon, dCentreLat, dDistance, objPixmap);
}
