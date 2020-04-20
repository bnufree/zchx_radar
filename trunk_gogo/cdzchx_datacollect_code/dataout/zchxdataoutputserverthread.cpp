#include "zchxdataoutputserverthread.h"
#include <QDebug>
#include <QDateTime>
#include "zmq.h"
#include "ZmqMonitorThread.h"

zchxDataOutputServerThread::zchxDataOutputServerThread(void* ctx, int port, bool monitor, QObject *parent)
    : QThread(parent)
    , mCtx(ctx)
    , mPort(port)
    , mIsOk(false)
    , mStop(false)
    , mSocket(0)
    , mMonitorThread(0)
    , mMonitorClient(monitor)
{
    mUrl = QString("tcp://*:%1").arg(mPort);
    mIsOk = init();
    if(mIsOk && mMonitorClient)
    {
        //监听雷达目标zmq
        QString monitorTrackUrl = QString("inproc://monitor.%1").arg(QString::number(mPort));
        zmq_socket_monitor (mSocket, monitorTrackUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
        mMonitorThread = new ZmqMonitorThread(mSocket, monitorTrackUrl, 0);
        connect(mMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
        connect(mMonitorThread, SIGNAL(finished()), mMonitorThread, SLOT(deleteLater()));
        mMonitorThread->start();
    }

    if(!isOK())
    {
        qDebug()<<"error init occured."<<zmq_errno()<<zmq_strerror(zmq_errno());
    }
}

zchxDataOutputServerThread::~zchxDataOutputServerThread()
{
    stopMe();
    if(mMonitorThread)
    {
        mMonitorThread->quit();
    }
    if(mSocket)
    {
        zmq_close(mSocket);
    }
}

bool zchxDataOutputServerThread::init()
{
    if(mCtx == 0)
    {
        qDebug()<<"Context is null. invalid....";
        return false;
    }

    mSocket = zmq_socket(mCtx, ZMQ_PUB);
    if(mSocket == 0)
    {
        qDebug()<<"create publish socket error";
        return false;
    }
    const char* addr = mUrl.toLatin1().constData();
    int sts = zmq_bind(mSocket, addr);//
    if(sts != 0)
    {
        qDebug()<<"bind server to port:"<<mPort<<" failed..."<<" url:"<<addr;
        return false;
    }
    return true;
}

void zchxDataOutputServerThread::run()
{
    while (!mStop)
    {
        //获取当前的任务
        zchxSendTaskList sendList;
        {
            QMutexLocker locker(&mMutex);
            if(mSendContentList.size() > 0)
            {
                sendList.append(mSendContentList);
            }
            mSendContentList.clear();
        }
        if(sendList.size() > 0 && isOK())
        {
            foreach (zchxSendTask task, sendList)
            {
                //数据分成3帧进行发送(时间+topic+内容)  
                QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();
//                qDebug()<<sTimeArray<<task[0]<<task[1].size();
                zmq_send(mSocket, task[0].data(), task[0].size(), ZMQ_SNDMORE);
                zmq_send(mSocket, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
                zmq_send(mSocket, task[1].data(), task[1].size(), 0);
            }
        }

        msleep(1000);
    }
}

void zchxDataOutputServerThread::slotRecvContents(const QByteArray& content, const QString& topic)
{
    QMutexLocker locker(&mMutex);
    zchxSendTask task;
    task.append(topic.toUtf8());
    task.append(content);
    mSendContentList.append(task);
}
