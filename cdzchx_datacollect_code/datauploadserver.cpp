#include "datauploadserver.h"
#include "zmq.h"
#include "zmq_utils.h"
#include "zmq.hpp"
#include "profiles.h"
#include "Log.h"
#include "comsendtread.h"
#include <QDebug>
#include <QFile>
#include <ZmqMonitorThread.h>

#define         CLIENT_SERVER_TOPIC         "CLIENT_UPDATE"
#define         CONSTRUCTION_START_TOPIC    "StartConstruction"
#define         CONSTRUCTION_END_TOPIC      "EndConstruction"
#define         PROJECT_INFO_UPLOAD_TOPIC   "ProjectInfo"
#define         SHIP_RECORD_UPLOAD_TOPIC    "ShipRecord"
#define         SHIP_PLAN_UPLOAD_TOPIC      "ShipPlan"
#define         ROUTE_PLAN_UPLOAD_TOPIC      "RoutePlan"
#define         ROUTE_RECORD_UPLOAD_TOPIC      "RouteRecord"
#define         CABLE_BASE_UPLOAD_TOPIC    "CableBase"
#define         CABLE_INTERFACE_UPLOAD_TOPIC      "CableInterface"
#define         COMPLETION_UPLOAD_TOPIC      "CompletionData"
#define         SIMULATION_UPLOAD_TOPIC      "Simulation"
#define         DP_UPLOAD_TOPIC      "DP"


DataUploadServer::DataUploadServer(void* ctx, QObject *parent) : mCtx(ctx), QThread(parent)
{    
//    QStringList dplist = Utils::Profiles::instance()->value("COM", "DPCOM").toStringList();
//    mDpComName = dplist.length() >= 4? dplist[1] : "";
//    mDpBandRate = dplist.length() >= 4? dplist[2].toInt() : 0;
    mDpInitFlag = false;
}

void DataUploadServer::setUploadDPCom(const COMDEVPARAM& dev)
{
    mDpComName = dev.mName;
    mDpBandRate = dev.mBaudRate;
    mDpInitFlag = dev.mStatus;
}

void DataUploadServer::run()
{
    if(!mCtx) return;
    void    *reply = zmq_socket (mCtx, ZMQ_REP);
    //监听zmq
    QString monitorUrl = "inproc://monitor.uploadfromclient";
    zmq_socket_monitor (reply, monitorUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    ZmqMonitorThread *thread = new ZmqMonitorThread(mCtx, monitorUrl, this);
    connect(thread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    thread->start();

    int      portNum = Utils::Profiles::instance()->value("Host", "UploadPort").toInt();

    //通过请求应答模式开启上传服务，监听客户端数据上传
    QString url = QString("tcp://*:%1").arg(portNum);
    int res = zmq_bind(reply, url.toUtf8().data());
    int sndtimeout = 2 * 1000; //2s超时限制，没有发送消息就退出
    zmq_setsockopt(reply, ZMQ_SNDTIMEO, &sndtimeout, sizeof(sndtimeout));
    int rcvtimeout = 5 * 1000; //5s超时限制，没有收到消息就退出
    zmq_setsockopt(reply, ZMQ_RCVTIMEO, &rcvtimeout, sizeof(rcvtimeout));
    LOG(LOG_RTM, "start zmq upload server port = %d. bind returned:%d", portNum, res);


    while(true)
    {
        //开始接收客户端上传的数据
        QByteArrayList byteslist;
        while (1) {
            zmq_msg_t msg;
            zmq_msg_init(&msg);
            zmq_recvmsg(reply, &msg, 0);
            QByteArray bytes2 = QByteArray((char*)zmq_msg_data(&msg),(int)(zmq_msg_size(&msg)));
            zmq_msg_close(&msg);
            if(bytes2.length() > 0)
            {
                byteslist.append(bytes2);
               // qDebug()<<"upload receive msg:"<<QString::fromUtf8(bytes2.data());
            }
            int64_t more = 0;
            size_t more_size = sizeof (more);
            zmq_getsockopt (reply, ZMQ_RCVMORE, &more, &more_size);
            if (!more)      //判断是否是最后消息
            {
                break; // 已到达最后一帧
            }
        }
        //处理上传的数据,根据上传的topic，转发数据或者发送到对应的DP线程写数据
        QByteArray ret;
        if(byteslist.length() == 2)
        {
            //接收到两帧数据(Topic+数据)
            QString topic = QString::fromUtf8(byteslist[0].data());            
            LOG(LOG_RTM, "topic:%s \n data:%s", topic.toUtf8().data(), byteslist[1].data());
            if(topic == DP_UPLOAD_TOPIC)
            {
                //slotSendDpMsg(QString::fromUtf8(byteslist[1].data()), ret);
                emit signalSendDpUploadMsg(byteslist[1]);
                ret.append("receive dp data and wrtite to com port now");
            } else if(topic == CONSTRUCTION_START_TOPIC)
            {
                //施工开始的信号，通知发送线程开始进行施工，发送数据, 模拟施工才使用                
                //emit signalSendDBInfoUpdated(byteslist[0], byteslist[1]);
                //emit signalStartConstruction();
                emit signalConstructionCommand(byteslist[0], byteslist[1]);
                ret.append(QString::fromUtf8("施工开始接收数据！！") + QString::fromUtf8(byteslist[1]));
            } else if(topic == CONSTRUCTION_END_TOPIC){
                //施工结束的信号，通知发送线程结束施工，数据停止                
                //emit signalSendDBInfoUpdated(byteslist[0], byteslist[1]);
                //emit signalStopConstruction();
                emit signalConstructionCommand(byteslist[0], byteslist[1]);
                ret.append(QString::fromUtf8("施工结束中断数据！！") + QString::fromUtf8(byteslist[1]));
            } else if(topic == PROJECT_INFO_UPLOAD_TOPIC ||
                      topic == SHIP_PLAN_UPLOAD_TOPIC ||
                      topic == SHIP_RECORD_UPLOAD_TOPIC ||
                      topic == ROUTE_PLAN_UPLOAD_TOPIC ||
                      topic == ROUTE_RECORD_UPLOAD_TOPIC ||
                      topic == CABLE_BASE_UPLOAD_TOPIC ||
                      topic == CABLE_INTERFACE_UPLOAD_TOPIC ||
                      topic == COMPLETION_UPLOAD_TOPIC)
            {
                emit signalSendUploadProject(topic, QString::fromUtf8(byteslist[1]));
                ret.append(tr("%1 data received").arg(topic));
            } else if(topic == SIMULATION_UPLOAD_TOPIC)
            {
                //将收到的写入文件，文件名为test.csv
                //Utils::Profiles::instance()->setValue("Host","FakeFileName", "test.csv");
                QString fileName = Utils::Profiles::instance()->value("Host","FakeFileName").toString();
                emit signalSendLogMsg(tr("receive simulaiton data.now start write %1 ").arg(fileName));

                //生成CSV文件
                QFile file(fileName);
                if(!file.open(QIODevice::WriteOnly))
                {
                    emit signalSendLogMsg(tr("open simulation file %1 failed").arg(fileName));
                } else
                {
                    file.write(byteslist[1]);
                    file.close();
                    emit signalSendLogMsg(tr("write simulation file %1 success").arg(fileName));
                    emit signalSetupSimulationData();
                }
                //将文件转发给备份服务器
                ret.append(tr("%1 data received").arg(topic));
            } else if(topic == CLIENT_SERVER_TOPIC)
            {
                emit signalSendDBInfoUpdated(byteslist[0], byteslist[1]);
                ret.append(tr("%1 info recevied.transfer now.").arg(topic));
            }
            else
            {
                ret.append("unsupported topic " + topic);
            }
        } else
        {
            //发送处理结果
            ret.append("data fortmat is not correct(topic+data)");
        }
        zmq_send(reply, ret.data(), ret.size(), 0);
    }

    if(reply)
    {
        zmq_close(reply);
    }
//    if(context)
//    {
//        zmq_ctx_destroy(context);
//    }
}

void DataUploadServer::slotSendDpMsg(const QString& msg, QByteArray& ret)
{
    if(!mDpInitFlag) return;
    ComSendTread *dpcom = new ComSendTread(mDpComName, mDpBandRate);
    if(dpcom->sendData(msg))
    {
        ret.append("data send success. good luck.");
    } else
    {
        ret.append("data send failed for timetout.");
    }

    delete dpcom;
}
