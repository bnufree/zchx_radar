#include "zchxradarechothread.h"
#include <QDebug>
#include <QFile>
#include <QSettings>

using namespace ZCHX_RADAR_RECEIVER;

ZCHXRadarEchoThread::ZCHXRadarEchoThread(const ZCHX_Radar_Setting_Param& param, QObject *parent)
    : ZCHXReceiverThread(ZCHX_RECV_RADAR_VIDEO, param, parent)
{
    qRegisterMetaType<ITF_VideoFrame>("ITF_VideoFrame");
    qRegisterMetaType<Map_RadarVideo>("Map_RadarVideo");
    m_lastUpdateRadarEchoTime = 0;
    m_videoFrameMap.clear();
}

void ZCHXRadarEchoThread::parseRecvData(const QByteArrayList& list)
{
    if(list.size() == 0) return;
    zchxFuntionTimer t(mRadarCommonSettings.m_sTopicList.join(","));

    ITF_VideoFrame objVideoFrame;

    if(!objVideoFrame.ParseFromArray(list.last().data(), list.last().size())) return;
    dealRadarEchoData(objVideoFrame);

}


void ZCHXRadarEchoThread::dealRadarEchoData(const ITF_VideoFrame &objVideoFrame)
{
    double dCentreLon = objVideoFrame.longitude();
    double dCentreLat = objVideoFrame.latitude();
    int uDisplayType = 1;//1回波-2余辉
    int uLoopNum = objVideoFrame.loopnum();
    double dDistance = objVideoFrame.radius();
    int uCurIndex = objVideoFrame.curindex();
    QByteArray objPixmap = QByteArray(objVideoFrame.imagedata().c_str(), objVideoFrame.imagedata().length());
    QByteArray prePixmap = QByteArray(objVideoFrame.curimagedata().c_str(), objVideoFrame.curimagedata().length());

    emit sendMsg(mRadarCommonSettings.m_sSiteID, dCentreLon, dCentreLat, dDistance, uDisplayType, uLoopNum, uCurIndex, objPixmap, prePixmap);
}
