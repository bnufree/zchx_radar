#include "zchxmsgserverthread.h"
#include <QDebug>
#include <QDateTime>
#include "zmq.h"
#include "ZmqMonitorThread.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include "mainprocess.h"

zchxMsgServerThread::zchxMsgServerThread(void* ctx, int port, bool monitor, QObject *parent)
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
        //监听当前客户端连接
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

zchxMsgServerThread::~zchxMsgServerThread()
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

bool zchxMsgServerThread::init()
{
    if(mCtx == 0)
    {
        qDebug()<<"Context is null. invalid....";
        return false;
    }

    mSocket = zmq_socket(mCtx, ZMQ_REP);
    if(mSocket == 0)
    {
        qDebug()<<"create reply socket error";
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

void zchxMsgServerThread::run()
{
    if(isOK()) return;
    while (!mStop)
    {
        //获取客户端发送的信息，客户端发送2段信息，时间,内容
        QByteArrayList recvlist;
        while (1) {
            int64_t more = 0;
            size_t more_size = sizeof(more);
            zmq_msg_t msg;
            zmq_msg_init(&msg);
            zmq_recvmsg(mSocket, &msg, 0);
            QByteArray bytes = QByteArray((char*)zmq_msg_data(&msg),(int)(zmq_msg_size(&msg)));
            zmq_msg_close(&msg);
            if(bytes.length() > 0)
            {
                recvlist.append(bytes);
            }
            zmq_getsockopt (mSocket, ZMQ_RCVMORE, &more, &more_size);
            if (more == 0) break;
        }
        if(recvlist.size()  == 2)
        {

            QDateTime msg_time = QDateTime::fromMSecsSinceEpoch(recvlist[0].toLongLong());
            QJsonObject retObj;
            int cmd = Msg_Undefined;
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(recvlist[1], &error);
            if(error.error == QJsonParseError::NoError && doc.isObject())
            {
                QJsonObject obj = doc.object();
                cmd = (MsgCmd)(obj.value(JSON_CMD).toInt());
                bool sts = false;
                switch (cmd)
                {
                //用户操作相关
                case Msg_Edit_FilterArea:
                case Msg_Delete_FilterArea:
                    sts = processFilterAreaMsg(cmd, obj.value(JSON_VAL));
                    break;


                default:
                    break;
                }

                retObj.insert(JSON_STATUS, sts ? JSON_OK : JSON_ERROR);
                if(!sts) retObj.insert(JSON_STATUS_STR, "contact server for more information");
                retObj.insert(JSON_CMD, cmd);
            } else
            {
                retObj.insert(JSON_STATUS, JSON_ERROR);
                retObj.insert(JSON_STATUS_STR, "parse json error");
                retObj.insert(JSON_VAL, QString::fromUtf8(recvlist[1]));
            }

            QByteArray ret = QJsonDocument(retObj).toJson();
            QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();
            zmq_send(mSocket, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
            zmq_send(mSocket, ret.data(), ret.size(), 0);
        }
        msleep(500);
    }

    if(mSocket)
    {
        zmq_close(mSocket);
    }
}

void zchxMsgServerThread::slotRecvContents(const QByteArray& content, const QString& topic)
{

}

bool zchxMsgServerThread::processFilterAreaMsg(int cmd, const QJsonValue &val)
{
    zchxMsg::filterArea area(val.toObject());
    return MainProc->processFilterAreaMsg(cmd, area);
}
