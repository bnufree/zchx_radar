#include "zmq.h"
#include "zmqradarechothread.h"
#include <QDebug>
#include <QFile>
#include <QSettings>
#include "../profiles.h"
ZMQRadarEchoThread::ZMQRadarEchoThread(QObject *parent)
    : QThread(parent),
      isOver(false)
{
    qRegisterMetaType<ITF_VideoFrame>("ITF_VideoFrame");
    qRegisterMetaType<Map_RadarVideo>("Map_RadarVideo");
    m_lastUpdateRadarEchoTime = 0;
    m_videoFrameMap.clear();
}

void ZMQRadarEchoThread::run()
{
    void *pCtx = NULL;
    void *pSock = NULL;

    //创建context，zmq的socket 需要在context上进行创建
    if((pCtx = zmq_ctx_new()) == NULL)
    {
        return;
    }
    //创建zmq socket ，socket目前有6中属性 ，这里使用SUB方式
    //具体使用方式请参考zmq官方文档（zmq手册）
    if((pSock = zmq_socket(pCtx, ZMQ_SUB)) == NULL)
    {
        zmq_ctx_destroy(pCtx);
        return;
    }
    //绑定地址
    //也就是使用tcp协议进行通信，使用网络端口
    QString sIP = Utils::Profiles::instance()->value("Echo", "IP").toString();
    QString sPort = Utils::Profiles::instance()->value("Echo", "PORT").toString();
    QString sTopic = Utils::Profiles::instance()->value("Echo", "TOPIC").toString();

    //设置topic 过滤
    QByteArray bTopic = sTopic.toLatin1();
    const char *filter = bTopic.data();
    if(zmq_setsockopt(pSock, ZMQ_SUBSCRIBE, filter, strlen(filter)))
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return;
    }

    QString sAddress = "tcp://"+sIP+":"+sPort;
    QByteArray byte1 =sAddress.toLocal8Bit();
    const char *pAddr = byte1.data();// = sAddress.toLocal8Bit().data();

    int nFlag = zmq_connect(pSock, pAddr);
    qDebug()<<"---------Echo ip addr------"<<sAddress;
    qDebug()<<"---------Echo topic ------"<<sTopic;
    qDebug()<<" Echo --------nFlag:"<<nFlag;
    if(nFlag < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        qDebug()<<" echo zmq_connect() fail!";
        return;
    }

    /////////////////////////////////
    //开始接收客户端上传的数据
//    QByteArrayList byteslist;
//    while (1) {
//        zmq_msg_t msg;
//        zmq_msg_init(&msg);
//        zmq_recvmsg(reply, &msg, 0);
//        QByteArray bytes2 = QByteArray((char*)zmq_msg_data(&msg),(int)(zmq_msg_size(&msg)));
//        zmq_msg_close(&msg);
//        if(bytes2.length() > 0)
//        {
//            byteslist.append(bytes2);
//            qDebug()<<"upload receive msg:"<<QString::fromUtf8(bytes2.data());
//        }
//        int64_t more = 0;
//        size_t more_size = sizeof (more);
//        zmq_getsockopt (reply, ZMQ_RCVMORE, &more, &more_size);
//        if (!more)      //判断是否是最后消息
//        {
//            break; // 已到达最后一帧
//        }
//    }


    ////////////////////////////////

    unsigned char *pBufferRec = NULL;
    ITF_VideoFrame objVideoFrame;
    while(!isOver)
    {
        //qDebug()<<"---start echo recv-------------";
        bool bOk = false;
        zmq_msg_t reply;
        zmq_msg_init(&reply);
        zmq_recvmsg(pSock, &reply, 0);//0表示非阻塞
        qint64 size = zmq_msg_size(&reply);
        if(size<15)
            continue;
        //qDebug()<<"echo recv, size :"<<size;
        QByteArray res = QByteArray((char*)zmq_msg_data(&reply),(int)(zmq_msg_size(&reply)));

        pBufferRec = (unsigned char*)zmq_msg_data(&reply);
        //qDebug()<<"echo recv, data :"<<res;

        if(objVideoFrame.ParseFromArray(pBufferRec,size))
        {
           //qDebug()<<"回波数据:";
           dealRadarEchoData(objVideoFrame);

       }
//        else
//        {
//            qDebug()<<"echo recv, data :"<<res;
//        }
       zmq_msg_close(&reply);
    }
    zmq_close(pSock);
    zmq_ctx_destroy(pCtx);
}

bool ZMQRadarEchoThread::getIsOver() const
{
    return isOver;
}

void ZMQRadarEchoThread::setIsOver(bool value)
{
    isOver = value;
}

void ZMQRadarEchoThread::dealRadarEchoData(const ITF_VideoFrame &objVideoFrame)
{
    int uMsgIndex = objVideoFrame.msgindex();
    int uAzimuth = objVideoFrame.azimuth();
    int bitResolution = objVideoFrame.bitresolution();
    //qDebug()<<"uMsgIndex"<<uMsgIndex;
    //qDebug()<<"uAzimuth"<<uAzimuth;
//    qDebug()<<"bitResolution"<<bitResolution;
    m_videoFrameMap[uAzimuth] = objVideoFrame;
    //////间隔一定时间更新状态
    int nInterval = Utils::Profiles::instance()->value("Echo", "update_interval_s").toInt();
    qint64 curtime = QDateTime::currentMSecsSinceEpoch();

    if((curtime - m_lastUpdateRadarEchoTime)/1000  < nInterval)
    {
        return;
    }

    //qDebug()<<"m_videoFrameMap size"<<m_videoFrameMap.count();
    m_lastUpdateRadarEchoTime =  curtime;
    //emit sendMsg(m_videoFrameMap);

    if(m_videoFrameMap.count()>1)
    {
        emit sendMsg(m_videoFrameMap);
    }
    m_videoFrameMap.clear();

}
