#include "zchxdataoutputservermgr.h"
#include "zchxdataoutputserverthread.h"
#include "zmq.h"
#include <QTimer>
#include <QDebug>

zchxDataOutputServerMgr::zchxDataOutputServerMgr(QObject *parent) : QObject(parent)
{
    mCtx = zmq_ctx_new();
}

zchxDataOutputServerMgr::~zchxDataOutputServerMgr()
{ 
    foreach (zchxDataOutputServerThread* thread, mThreadList) {
        if(thread){
            thread->stopMe();
            thread->deleteLater();
        }
    }
    if(mCtx) zmq_ctx_destroy(mCtx);
}

void zchxDataOutputServerMgr::appendData(const QByteArray &data, const QString &topic, int port)
{
    zchxDataOutputServerThread* thread = getThread(port);
    if(thread)
    {
        thread->slotRecvContents(data, topic);
        if(!mPortTopicList[port].contains(topic))
        {
            mPortTopicList[port].append(topic);
            emit signalSendPortStartStatus(port, thread->isOK(), mPortTopicList[port].join(","));
        }
    }
}

zchxDataOutputServerThread* zchxDataOutputServerMgr::getThread(int port)
{
    zchxDataOutputServerThread* thread = mThreadList.value(port, 0);
    if(!thread)
    {
        thread = new zchxDataOutputServerThread(mCtx, port);
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        mThreadList[port] = thread;
    }
    if(!thread->isRunning()) thread->start();

    return thread;
}

zchxRadarDataOutputMgr::zchxRadarDataOutputMgr(QObject *parent)
    :zchxDataOutputServerMgr(parent)
{
    mPublicTimer = new QTimer(this);
    mPublicTimer->setInterval(30*1000);
    connect(mPublicTimer, SIGNAL(timeout()), this, SLOT(slotSendLimitData()));
    mPublicTimer->start();
}

zchxRadarDataOutputMgr::~zchxRadarDataOutputMgr()
{
    if(mPublicTimer)
    {
        mPublicTimer->stop();
        mPublicTimer->deleteLater();
    }
}

void zchxRadarDataOutputMgr::slotSendLimitData()
{
    if(mLimteData.size() == 0) return;
    qDebug()<<"now send limit data size:"<<mLimteData.size();
    appendData(mLimteData, "Limit_Area", 10086);
}
