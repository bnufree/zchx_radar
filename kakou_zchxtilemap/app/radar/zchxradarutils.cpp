#include "zchxradarutils.h"
#include "zmq.h"

using namespace ZCHX_RADAR_RECEIVER;


ZCHXReceiverThread::ZCHXReceiverThread(int type, const ZCHX_Radar_Setting_Param &param, QObject *parent)
    :isOver(true)
    ,mCtx(0)
    ,mSocket(0)
    ,mIsConnect(false)
    ,mRadarCommonSettings(param)
    ,QThread(parent)
    ,mType(type)
{
    mCtx = zmq_ctx_new();
    mUrl = QString("tcp://%1:%2").arg(mRadarCommonSettings.m_sIP).arg(mRadarCommonSettings.m_sPort);
}

ZCHXReceiverThread::~ZCHXReceiverThread()
{
    disconnectToHost();
    if(mCtx) zmq_ctx_destroy(mCtx);
}

void ZCHXReceiverThread::disconnectToHost()
{
    if(mSocket)
    {
        qDebug()<<"data recv has been canceled:"<<mType;
        zmq_close(mSocket);
    }
    mSocket = 0;
}

bool ZCHXReceiverThread::connectToHost()
{
    disconnectToHost();
    if((!mCtx) && ((mCtx = zmq_ctx_new()) == NULL)) return false;
    if((mSocket = zmq_socket(mCtx, ZMQ_SUB)) == NULL) return false;

    //设置topic 过滤
    foreach (QString topic, mRadarCommonSettings.m_sTopicList) {
        QByteArray temp = topic.toLatin1();
        const char *filter = temp.constData();
        if(zmq_setsockopt(mSocket, ZMQ_SUBSCRIBE, filter, strlen(filter))) return false;
    }
    //设置等待时间
    int timeout = 5 * 1000; //5s超时限制，没有收到消息就退出
    if(zmq_setsockopt(mSocket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout))) return false;
    //开始连接
    QByteArray temp = mUrl.toLatin1();
    const char* sAddress = temp.constData();
    int sts = zmq_connect(mSocket, sAddress);
    if(sts != 0)
    {
        qDebug()<<"connect to server failed .code:"<<zmq_errno()<<" str:"<<zmq_strerror(zmq_errno())<<sAddress;
    } else
    {
        qDebug()<<"connect to server succeed."<<sAddress;
    }

    return sts == 0;

}

void ZCHXReceiverThread::run()
{
    int no_recv_num = 0;
    while(1)
    {
        if(isOver)
        {
            disconnectToHost();
            mIsConnect = false;
            msleep(1000);
        } else
        {
            //检查是否需要重连
            if(!mIsConnect)
            {
                QString url;
                url.sprintf("tcp://%s:%s", mRadarCommonSettings.m_sIP.toStdString().data(), mRadarCommonSettings.m_sPort.toStdString().data());
                qDebug()<<"start connect to server:"<<mRadarCommonSettings.m_sIP<<mRadarCommonSettings.m_sPort<<mRadarCommonSettings.m_sTopicList;
                //开始连接服务器
                if(!connectToHost())
                {
                    url.append(" ");
                    url.append(QString::fromStdString(zmq_strerror(zmq_errno())));
                    emit signalConnectedStatus(false, url);
                    sleep(3);
                    continue;
                } else
                {
                    mIsConnect = true;
                    emit signalConnectedStatus(true, QString());
                }
            }
            //开始接收数据,数据有可能多帧发送
            QByteArrayList recvlist;
            int length = 0;
            while (1) {
                int64_t more = 0;
                size_t more_size = sizeof (more);
                zmq_msg_t msg;
                zmq_msg_init(&msg);
                zmq_recvmsg(mSocket, &msg, 0);
                QByteArray bytes2 = QByteArray((char*)zmq_msg_data(&msg),(int)(zmq_msg_size(&msg)));
                zmq_msg_close(&msg);
                if(bytes2.length() > 0)
                {
                    recvlist.append(bytes2);
                }
                length += bytes2.length();
                zmq_getsockopt (mSocket, ZMQ_RCVMORE, &more, &more_size);
                if (more == 0)      //判断是否是最后消息
                {
                    break; // 已到达最后一帧
                }
                //msleep(1000);
            }
            if(recvlist.length() == 0)
            {
                //没有接收到数据
                no_recv_num++;
                if(no_recv_num >= 10)
                {
                    //这里清除掉旧的记录,自动重连
                    mIsConnect = false;
                }
                continue;
            } else
            {
                no_recv_num = 0;
            }
            emit signalRecvDataNow(mType, length);
            parseRecvData(recvlist);
        }
    }
}
