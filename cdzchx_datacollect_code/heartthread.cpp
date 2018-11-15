#include "heartthread.h"
#include "zmq.h"
#include "zmq_utils.h"
#include "zmq.hpp"
#include "profiles.h"
#include "Log.h"

HeartThread::HeartThread(QObject *parent) : QThread(parent)
{
    mRunning = true;
    mIsPublish = false;
    mGuiConfig.clear();
}

void HeartThread::setRun(bool brun)
{
    mRunning = brun;
}

void HeartThread::run()
{
    void    *context =  zmq_ctx_new ();
    void    *pair = 0;
    int      portNum = Utils::Profiles::instance()->value("Host", "HeartPort").toInt();
    QString  masterIP = Utils::Profiles::instance()->value("Host", "MasterIP", "192.168.8.220").toString();
    bool     isMaster = Utils::Profiles::instance()->value("Host", "Master", 0).toBool();
    QString url;
    int     res = -1;

    //创建socket
    pair = zmq_socket (context, ZMQ_PAIR);
    int sndtimeout = 2 * 1000; //2s超时限制，没有发送消息就退出
    zmq_setsockopt(pair, ZMQ_SNDTIMEO, &sndtimeout, sizeof(sndtimeout));
    int recvout = 5*1000;
    zmq_setsockopt(pair, ZMQ_RCVTIMEO, &recvout, sizeof(recvout));

    if(isMaster)
    {        
        mIsPublish = true;
        //作为主服务器使用, 作为心跳服务器
        url.sprintf("tcp://*:%d", portNum);
        res = zmq_bind(pair, url.toUtf8().data());

        LOG(LOG_RTM, "start zmq heart pub port = %d. bind returned:%d", portNum, res);

        //开始发送心跳数据
        while(mRunning)
        {
            //每隔2S发送心跳信息
            QByteArray sendMsg;
            if(mGuiConfig.length() == 0)
            {
                sendMsg = QString("8888").toUtf8();
                //emit hearmsg(QString("send data to slave server:%1. no reply pls").arg(sendMsg.data()));
            } else
            {
                sendMsg = mGuiConfig.first();
                //emit hearmsg(QString("send data to slave server:%1. reply pls!!!!").arg(sendMsg.data()));
            }
            zmq_send(pair, sendMsg.data(), sendMsg.length(), 0);
            //接收传回的信息
            zmq_msg_t reply;
            zmq_msg_init(&reply);
            zmq_recvmsg(pair, &reply, 0);
            QByteArray bytes2 = QByteArray((char*)zmq_msg_data(&reply),(int)(zmq_msg_size(&reply)));
            zmq_msg_close(&reply);
            if(bytes2.length() != 0)
            {
                emit hearmsg(QString("receive data from slave server:%1.").arg(bytes2.data()));
                QString recvmsg = QString::fromUtf8(bytes2);
                if(recvmsg == "ok")
                {
                    //备份机已经收到消息
                    if(mGuiConfig.length())
                        mGuiConfig.removeFirst();

                } else
                {
                    //收到备份机修改的配置
                    emit signalUpdateGui(bytes2);
                    sendMsg = QString("ok").toUtf8();
                    zmq_send(pair, sendMsg.data(), sendMsg.length(), 0);
                    emit hearmsg(QString("send data to slave server:%1. update my gui").arg(sendMsg.data()));
                }

            }
        }
    } else
    {
        //作为备份机使用,连接心跳服务器
        url.sprintf("tcp://%s:%d", masterIP.toUtf8().data(), portNum);
        res = zmq_connect (pair, url.toUtf8().data());
        LOG(LOG_RTM, "start zmq heart sub port = %d. connect returned:%d", portNum, res);


        mIsPublish = false;
        int count = 0;

        //接收消息，超出时间限制，认为主服务器已经下线。备份服务器开始开始启动发送数据
        while(mRunning)
        {
            zmq_msg_t reply;
            zmq_msg_init(&reply);
            zmq_recvmsg(pair, &reply, 0);
            QByteArray bytes2 = QByteArray((char*)zmq_msg_data(&reply),(int)(zmq_msg_size(&reply)));
            zmq_msg_close(&reply);
            if(bytes2.length() == 0)
            {
                LOG(LOG_RTM, "receive nothing data from master server");
                emit hearmsg(QString("receive nothing  from master server count = %1").arg(count+1));
                //没有收到主服务器的数据
                if(mIsPublish == false)
                {
                    count++;
                    if(count == 10)
                    {
                        emit hearmsg(QString("start output data services"));
                        emit startPublish();
                        mIsPublish = true;
                    }
                }

                //关闭socket重新连接
                zmq_close(pair);
                pair = zmq_socket(context, ZMQ_PAIR);;
                zmq_setsockopt(pair, ZMQ_SNDTIMEO, &sndtimeout, sizeof(sndtimeout));
                zmq_setsockopt(pair, ZMQ_RCVTIMEO, &recvout, sizeof(recvout));
                zmq_connect (pair, url.toUtf8().data());

            } else
            {
                QByteArray reply;
                count = 0;
                QString recvmsg = QString::fromUtf8(bytes2);
                if(recvmsg == "8888")
                {
                    //一般的心跳信息，不处理
                } else if(recvmsg == "ok")
                {
                    if(!mGuiConfig.isEmpty())
                    {
                        mGuiConfig.removeFirst();
                    }
                } else
                {
                    //接收到的是参数配置消息
                    emit signalUpdateGui(bytes2);
                    reply = QString("ok").toUtf8();
                }
                LOG(LOG_RTM, "receive data from master server:%s", bytes2.data());

                emit hearmsg(QString("receive data from master server:%1").arg(bytes2.data()));
                //主服务器正在运行，结束当前正在提供的数据服务
                if(mIsPublish)
                {
                    emit hearmsg(QString("stop output data services"));
                    emit stopPublish();
                    mIsPublish = false;
                }

                //检测当前备份端的配置是否更新
                if(!mGuiConfig.isEmpty())
                {
                    reply = mGuiConfig.first();
                }
                if(!reply.isEmpty())
                {
                    //发送给主服务器
                    zmq_send(pair, reply.data(), reply.length(), 0);
                }
                if(!reply.isEmpty())
                {
                    emit hearmsg(QString("send data to master server:%1").arg(reply.data()));
                }

            }
        }

    }

    if(pair)
    {
        zmq_close(pair);
    }
    if(context)
    {
        zmq_ctx_destroy(context);
    }

}

void HeartThread::setHeartMsg(const QByteArray &msg)
{
    mGuiConfig.append(msg);
}

