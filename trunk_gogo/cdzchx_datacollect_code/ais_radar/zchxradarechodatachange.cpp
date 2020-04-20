#include "zchxradarechodatachange.h"
#include<QDebug>
using  namespace std;
ZCHXRadarEchoDataChange::ZCHXRadarEchoDataChange(QObject *parent):
    QObject(parent)
{
    m_pZMQRadarEchoThread = new ZMQRadarEchoThread();
    connect(m_pZMQRadarEchoThread,SIGNAL(sendMsg(Map_RadarVideo)),this,SLOT(reciveMsg(Map_RadarVideo)));
}

ZCHXRadarEchoDataChange::~ZCHXRadarEchoDataChange()
{
    if(m_pZMQRadarEchoThread)
    {
        qDebug()<<"close ZMQRadarEchoThread";
        closeRadarEchoData();
//        m_pZMQRadarEchoThread->terminate();
//        m_pZMQRadarEchoThread->quit();
        delete m_pZMQRadarEchoThread;
        m_pZMQRadarEchoThread = NULL;
    }
}
void ZCHXRadarEchoDataChange::updateRadarEchoData()
{
    if(m_pZMQRadarEchoThread->isRunning())
    {
        qDebug()<<"zmq thread is rinning";
        return;
    }
    qDebug()<<"start radar_echo thread!";
    m_pZMQRadarEchoThread->setIsOver(false);
    m_pZMQRadarEchoThread->start();
}

void ZCHXRadarEchoDataChange::closeRadarEchoData()
{
    m_pZMQRadarEchoThread->setIsOver(true);
    m_pZMQRadarEchoThread->quit();
}
void ZCHXRadarEchoDataChange::reciveMsg(const Map_RadarVideo &radarVideoMap)
{
    //qDebug()<<"1111111111111111111111";
    emit sendMsg(radarVideoMap);
}

