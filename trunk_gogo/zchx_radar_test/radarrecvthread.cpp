#include "radarrecvthread.h"
#include "zmq.h"
#include "zmq.hpp"
#include "zmq_utils.h"
#include <QDateTime>
#include <QDebug>

RadarRecvThread::RadarRecvThread(QObject *parent) : QThread(parent)
{
    //qRegisterMetaType<ITF_TrackPointList>("const ITF_TrackPointList&");
    qRegisterMetaType<TrackPointList>("const TrackPointList&");
}

void RadarRecvThread::run()
{
    QString url = "tcp://192.168.80.196:5152";
    void* context =  zmq_ctx_new ();
    void* request = zmq_socket (context, ZMQ_SUB);
    zmq_connect (request, url.toUtf8().data());
    //设置消息过滤
    QString topic = "RadarTrack";
    QByteArray optVal;
    if(topic.length() > 0)
    {
        optVal.append(topic.toUtf8());
    } else
    {
        optVal.append("");
    }
    zmq_setsockopt(request, ZMQ_SUBSCRIBE, optVal.data(), optVal.length());
    //设置消息接收等待时间
    int timeout = 5 * 1000; //5s超时限制，没有收到消息就退出
    zmq_setsockopt(request, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    zmq_setsockopt(request, ZMQ_SNDTIMEO, &timeout, sizeof(timeout));

    int errorCount = 0;

    //开始接收数据,数据分3帧进行接收(topic，时间，数据)
    while (1) {
        QByteArrayList recvlist;
        while (1) {
            zmq_msg_t msg;
            zmq_msg_init(&msg);
            zmq_recvmsg(request, &msg, 0);
            QByteArray bytes2 = QByteArray((char*)zmq_msg_data(&msg),(int)(zmq_msg_size(&msg)));
            zmq_msg_close(&msg);
            if(bytes2.length() > 0)
            {
                recvlist.append(bytes2);
                //qDebug()<<"receive msg:"<<QString::fromUtf8(bytes2.data());
                errorCount = 0;
            }
            int64_t more;
            size_t more_size = sizeof (more);
            zmq_getsockopt (request, ZMQ_RCVMORE, &more, &more_size);
            if (!more)      //判断是否是最后消息
            {
                break; // 已到达最后一帧
            }
        }

        //开始进行数据解析
        if(recvlist.length() > 3)
        {
            QString topic = QString::fromUtf8(recvlist.at(0).data());
            QString time = QDateTime::fromMSecsSinceEpoch(recvlist.at(1).toLongLong()).toString("yyyy-MM-dd hh:mm:ss zzz");
            if(topic == "RadarTrack")
            {
                TrackPointList info;
                for(int i=3; i<recvlist.size(); i++)
                {
                    QByteArray recv = recvlist[i];
                    TrackPoint point;
                    if(point.ParseFromArray(recv.data(), recv.size()))
                    {
                        info.append(point);
                    }
                }
                if(info.size() > 0)
                {
                    emit signalSendTrackPointList(info);
                }
//                QByteArray recv = recvlist[2];
//                if(info.ParseFromArray(recv.constData(), recv.size()))
//                {
//                    qDebug()<<"radar info:"<<info.tracks_size();
//                    emit signalSendTrackPointList(info);
//                }
            }
        } else if(recvlist.length() == 0)
        {
            errorCount++;
            //重新连接
            qDebug()<<"receive nothing  from master server";
            if(errorCount == 5)
            {
                //清空雷达显示
                emit signalSendTrackPointList(TrackPointList());
                zmq_close (request);
                request = zmq_socket (context, ZMQ_SUB);
                zmq_connect (request,url.toUtf8().data());
                //设置消息过滤
                QByteArray optVal;
                optVal.append(topic.toUtf8().data());
                zmq_setsockopt(request, ZMQ_SUBSCRIBE, optVal.data(), optVal.length());
                //设置消息接收等待时间
                int timeout = 5 * 1000; //5s超时限制，没有收到消息就退出
                zmq_setsockopt(request, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
            }

            sleep(1);
        }

    }

    zmq_close (request);
    zmq_ctx_destroy (context);
}

