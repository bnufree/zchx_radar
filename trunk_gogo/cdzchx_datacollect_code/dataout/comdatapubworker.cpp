#include "comdatapubworker.h"
#include "QTcpSocket"
#include "Log.h"
#include "globlefun/glogfunction.h"
#include <QDateTime>
#include "profiles.h"
#include "common.h"

ComDataPubWorker::ComDataPubWorker(int mode, QObject *parent):
    QObject(parent),
    mWorkMode(mode),
    mServerIP(""),
    mServerPort(0),
    mOutTimer(0)
{
    mOutTimer = new QTimer;
    mOutTimer->setInterval(180*1000);
    connect(mOutTimer, SIGNAL(timeout()), this, SLOT(slotOutputData()));

    this->moveToThread(&mWorkThread);
    mWorkThread.start();
    mOutTimer->start();

    m_uTrackSendPort = PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_Send_Port").toInt();
    //发送雷达目标的zmq
    //创建context，zmq的socket 需要在context上进行创建
    m_pTrackContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pTrackLisher= zmq_socket(m_pTrackContext, ZMQ_PUB);
    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uTrackSendPort);
    zmq_bind(m_pTrackLisher, sIPport.toLatin1().data());//
    //监听雷达目标zmq
    QString monitorTrackUrl = "inproc://monitor.radarTrackclient";
    zmq_socket_monitor (m_pTrackLisher, monitorTrackUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pTrackMonitorThread = new ZmqMonitorThread(m_pTrackContext, monitorTrackUrl, 0);
    connect(m_pTrackMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pTrackMonitorThread, SIGNAL(finished()), m_pTrackMonitorThread, SLOT(deleteLater()));
    m_pTrackMonitorThread->start();
    mTempContent = mContent;
}

ComDataPubWorker::~ComDataPubWorker()
{
    if(mOutTimer)
    {
        mOutTimer->stop();
        mOutTimer->deleteLater();
    }

    mWorkThread.quit();
}

void ComDataPubWorker::setTimerInterval(int inter)
{
    if(mOutTimer && mOutTimer->interval() != inter * 1000)
    {
        mOutTimer->setInterval(inter * 1000);
        if(mOutTimer->isActive())
        {
            mOutTimer->start();
        }
    }
}

void ComDataPubWorker::slotRecvPubData(const QByteArray &content)
{
    mContent = content;
}

void ComDataPubWorker::slotOutputData()
{
    //qDebug()<<"ZMQ发送数据";
        if(mContent.size() == 0) return;
        if(mTempContent != mContent)
        {
            if(mWorkMode == DataOutputServer) return;
            if(!m_pTrackLisher) return;
            //通过zmq发送
            QString sIPport = "tcp://*:";
            sIPport += QString::number(m_uTrackSendPort);

            QString sTopic = m_sTrackTopic;
            QByteArray sTopicArray = sTopic.toUtf8();
    //        QString sId = PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_ID").toString();
    //        QByteArray sIdArray = sId.toUtf8();
            QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();
            //构造发送的数据
    //        QByteArray send = GlogFunction::instance()->int2Bytes(mContent.size(), HIGH_FIRST);
    //        send.append(mContent);
            QByteArray send = mContent;
            //qDebug()<<"ZMQ发送数据"<<send.size()<<send.toHex().data();
            //zmq_send(m_pTrackLisher, sId.data(), sIdArray.size(), ZMQ_SNDMORE);
            zmq_send(m_pTrackLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
            zmq_send(m_pTrackLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
            zmq_send(m_pTrackLisher, send.data(), send.size(), 0);
            mTempContent = mContent;
        }

//    if(mWorkMode == DataOutputServer) return;
//    if(mServerIP.isEmpty() || mServerPort == 0) return;
//    //短连接发送数据
//    QTcpSocket socket;
//    socket.connectToHost(mServerIP, mServerPort);
//    if(!socket.waitForConnected(10000))
//    {
//        LOG(LOG_RTM, "connect to host(%s:%d) time out", mServerIP.toStdString().data(), mServerPort);
//        return;
//    }
//    //构造发送的数据
//    QByteArray send = GlogFunction::instance()->int2Bytes(mContent.size(), HIGH_FIRST);
//    send.append(mContent);
//    socket.write(send);
//    if(!socket.waitForBytesWritten(30000))
//    {
//        LOG(LOG_RTM, "send data to host(%s:%d) time out", mServerIP.toStdString().data(), mServerPort);
//        return;
//    }

}
