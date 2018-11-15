#include "zchxaisdataserver.h"
#include <QDebug>
#include <QTime>
#include "../profiles.h"
#include "ais.h"
#include "Log.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"


ZCHXAisDataServer::ZCHXAisDataServer(QObject *parent) :\
    m_pTcpServer(0),
    m_pTcpSocket(0),
    mAisHeartTimer(0),
    mLastRecvAisDataTime(0),
    QObject(parent)
{
    m_bServer = Utils::Profiles::instance()->value("Ais","IsServer").toBool();
    m_uServerPort = Utils::Profiles::instance()->value("Ais","Server_Port").toInt();
    m_sIP = Utils::Profiles::instance()->value("Ais","IP").toString();
    m_uPort = Utils::Profiles::instance()->value("Ais","Port").toInt();
    mDataTimeOut = Utils::Profiles::instance()->value("Ais", "TimeOut", 1).toInt();
    mAisHeartTimer = new QTimer();
    mAisHeartTimer->setInterval(60*1000);
    connect(mAisHeartTimer, SIGNAL(timeout()), this, SLOT(slotCheckAisRecv()));
    connect(this,SIGNAL(startProcessSignal()),this,SLOT(startProcessSlot()));
    moveToThread(&m_workThread);

    m_workThread.start();
    mAisHeartTimer->start();

}

ZCHXAisDataServer::~ZCHXAisDataServer()
{
    qDebug()<<"~ZCHXAisDataServer()_11";
    if(m_workThread.isRunning())
    {
        m_workThread.quit();
    }
    m_workThread.terminate();

    if(m_pTcpServer)
    {
        delete m_pTcpServer;
        m_pTcpServer = NULL;
    }
    if(m_pTcpSocket)
    {
        delete m_pTcpSocket;
        m_pTcpSocket = NULL;
    }
    qDebug()<<"~ZCHXAisDataServer()_22";

}

void ZCHXAisDataServer::startProcessSlot()
{
    init();
}

void ZCHXAisDataServer::init()
{
    LOG(LOG_RTM, "%s %s %d,socket = %0x",__FILE__,__FUNCTION__, __LINE__,m_pTcpSocket);
    if(m_bServer)
    {
        LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
        if(m_pTcpServer)
        {
            m_pTcpServer->close();
            delete m_pTcpServer;
            m_pTcpServer = 0;
        }
        LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
        m_pTcpServer = new QTcpServer();
        connect(m_pTcpServer,SIGNAL(newConnection()),this,SLOT(acceptConnection()));\
        connect(m_pTcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)),\
                this, SLOT(displayError(QAbstractSocket::SocketError)));
        m_pTcpServer->listen(QHostAddress::Any,m_uServerPort);
        LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
    }
    else
    {
        LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
        if(m_pTcpSocket)
        {
            LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
            m_pTcpSocket->abort();
            delete m_pTcpSocket;
            m_pTcpSocket = 0;
            LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
        }
        LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
        m_pTcpSocket = new QTcpSocket();
        LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
        //connect(m_pTcpSocket, SIGNAL(disconnected()), this, SLOT(OnSocketDisconnected()));
        connect(m_pTcpSocket,SIGNAL(readyRead()),this,SLOT(updateServerProgress()));
        connect(m_pTcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
        connect(m_pTcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(stateChanged(QAbstractSocket::SocketState)));
        m_pTcpSocket->connectToHost(m_sIP,m_uPort);
        LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);

    }
    LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
    BuildNmeaLookup();
    LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);

    m_lastClearAISTime = 0;
    mLastRecvAisDataTime = QDateTime::currentMSecsSinceEpoch();
    LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
}

#if 0
void ZCHXAisDataServer::initZmq()
{
    m_pAISContext = zmq_ctx_new();
    m_pAISLisher= zmq_socket(m_pAISContext, ZMQ_PUB);

    //监听zmq
    QString monitorAisUrl = "inproc://monitor.aisclient";
    zmq_socket_monitor (m_pAISLisher, monitorAisUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pMonitorThread = new ZmqMonitorThread(m_pAISContext, monitorAisUrl, 0);
    connect(m_pMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pMonitorThread, SIGNAL(finished()), m_pMonitorThread, SLOT(deleteLater()));
    m_pMonitorThread->start();

    QString sIPport = QString("tcp://*:%1").arg(m_uAISSendPort);
    zmq_bind(m_pAISLisher, sIPport.toLatin1().data());

}

void ZCHXAisDataServer::analysisAIS(const QString sAisData)
{
    // qDebug()<<"analysisAIS thread id :"<<QThread::currentThreadId();
    if(sAisData.size()<20)
    {
        return;
    }
    QString sAisHeader = "!";
    QString sAisTail = "\x0a";
    qint64 uBeginPos = 0;
    qint64 uEndPos = 0;
    QString sAisCell;
    qint64 uAisIndex = 1;
    QString sAisWholeData = "";//完整的数据体
    while(true)
    {
        uBeginPos = sAisData.indexOf(sAisHeader,uEndPos);
        if(uBeginPos<0)
        {
            return;
        }
        uEndPos = sAisData.indexOf(sAisTail,uBeginPos);
        if(uEndPos<0)
        {
            return;
        }
        sAisCell = sAisData.mid(uBeginPos,uEndPos-uBeginPos);//找出单条ais数据
        //sAisCell = sAisData.section(sAisHeader,uAisIndex,uAisIndex);//找出单条ais数据
        sAisCell = sAisCell.trimmed();
        //qDebug()<<"单条数据:"<<sAisCell;
        //单条数据模板 ABVDM,1,1,3,A,B>p@a?h0Hit;TH2UoatiGwW5oP06,0*39
        //或者  ABVDM,2,1,3,A,59RRLH02Oe4O=M5T0010QDtpN8p4n1=@5800001A:0I<540000000000,0*54
        //     ABVDM,2,2,3,A,000000000000000,2*2C
        //判断是否有拼接的数据

        QStringList sCellAisList = sAisCell.split(',');
        int uNum = sCellAisList.size();
        if(uNum == 7)//有效数据
        {
            QString strMax = sCellAisList[1];
            QString strCur = sCellAisList[2];
            int uMax = strMax.toInt();
            int uCur = strCur.toInt();
            QString sAisCellBody = sCellAisList[5];
            sAisWholeData.append(sAisCellBody);
            if(uCur == uMax)//完整的数据
            {
                int uPad = GetPad(sAisCell.toStdString());
                if(uPad<0||uPad>5)
                {
                    sAisWholeData.clear();
                    qDebug()<<"uPad error value "<<uPad;
                    continue;
                }

                analysisCellAIS(sAisWholeData,uPad);//解析单条ais数据部分
                sAisWholeData.clear();
            }

        }

    }
}

QJsonObject ZCHXAisDataServer::ais1_2_3_to_json(const QString &sAisBody, const int uPad, qint64  recivetime)
{
    QJsonObject objObject;
    Ais1_2_3 msg(sAisBody.toLatin1().constData(), uPad);

    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("ship_type", "A");
    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);

    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("mmsi", msg.mmsi);

    objObject.insert("nav_status", msg.nav_status);
    objObject.insert("rot", msg.rot);
    objObject.insert("sog", msg.sog);
    objObject.insert("position_accuracy", msg.position_accuracy);
    objObject.insert("x", msg.x);
    objObject.insert("y", msg.y);
    objObject.insert("cog", msg.cog);
    objObject.insert("true_heading", msg.true_heading);
    objObject.insert("timestamp", msg.timestamp);
    objObject.insert("special_manoeuvre", msg.special_manoeuvre);
    objObject.insert("spare", msg.spare);
    objObject.insert("raim", msg.raim);

    // COMM States
    objObject.insert("sync_state", msg.sync_state); // Both SOTDMA & ITDMA

    // SOTDMA
    if (msg.message_id == 1 || msg.message_id == 2) {
        objObject.insert("slot_timeout", msg.slot_timeout);

        if (msg.received_stations_valid)
            objObject.insert("received_stations", msg.received_stations);
        if (msg.slot_number_valid)
            objObject.insert("slot_number", msg.slot_number);
        if (msg.utc_valid) {
            objObject.insert("utc_hour", msg.utc_hour);
            objObject.insert("utc_min", msg.utc_min);
            objObject.insert("utc_spare", msg.utc_spare);
        }

        if (msg.slot_offset_valid)
            objObject.insert("slot_offset", msg.slot_offset);

    }

    // ITDMA
    if (msg.slot_increment_valid) {
        objObject.insert("slot_increment", msg.slot_increment);
        objObject.insert("slots_to_allocate", msg.slots_to_allocate);
        objObject.insert("keep_flag", msg.keep_flag);
    }

    objObject.insert("UTC", recivetime);
    objObject.insert("IS_OK", true);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais4_11_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais4_11 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);


    objObject.insert("mmsi", msg.mmsi);

    objObject.insert("year", msg.year);
    objObject.insert("month", msg.month);
    objObject.insert("rotday", msg.day);
    objObject.insert("hour", msg.hour);
    objObject.insert("minute", msg.minute);
    objObject.insert("second", msg.second);

    objObject.insert("position_accuracy", msg.position_accuracy);
    objObject.insert("x", msg.x);
    objObject.insert("y", msg.y);

    objObject.insert("fix_type", msg.fix_type);
    objObject.insert("transmission_ctl", msg.transmission_ctl);
    objObject.insert("spare", msg.spare);
    objObject.insert("raim", msg.raim);


    // SOTDMA
    objObject.insert("sync_state", msg.sync_state); // Both SOTDMA & ITDMA
    objObject.insert("slot_timeout", msg.slot_timeout);


    if (msg.received_stations_valid)
        objObject.insert("received_stations", msg.received_stations);
    if (msg.slot_number_valid)
        objObject.insert("slot_number", msg.slot_number);
    if (msg.utc_valid) {
        objObject.insert("utc_hour", msg.utc_hour);
        objObject.insert("utc_min", msg.utc_min);
        objObject.insert("utc_spare", msg.utc_spare);
    }

    if (msg.slot_offset_valid)
        objObject.insert("slot_offset", msg.slot_offset);


    objObject.insert("UTC", recivetime);
    objObject.insert("IS_OK", true);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais5_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais5 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("ship_type", "A");
    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);

    if (0 == msg.mmsi)
    {
       objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("mmsi", msg.mmsi);
    std::string strMmsi = QString::number(msg.mmsi).toStdString();
    objObject.insert("country", QString::fromStdString(strMmsi.substr(0, 3)));//The first three digits convey information about the country in which the ID was issued <<ITU-MID>>

    objObject.insert("ais_version", msg.ais_version);
    objObject.insert("imo_num", msg.imo_num);

    if (msg.callsign.empty())
    {
        objObject.insert("callsign", "---");
    }
    else
    {
        std::string  callsign = msg.callsign;

//        boost::algorithm::erase_all(callsign, "@");
//        boost::algorithm::trim_right_if(callsign, boost::algorithm::is_any_of(" "));
        QString sTemp = QString::fromStdString(callsign);
        sTemp.remove("@");
        sTemp.remove(" ");
       objObject.insert("callsign", sTemp);
    }
    if (msg.name.empty())
    {
        objObject.insert("name", "---");
    }
    else
    {
        std::string  name = msg.name;
//        boost::algorithm::erase_all(name, "@");
//        boost::algorithm::trim_right_if(name, boost::algorithm::is_any_of(" "));
        QString sTemp = QString::fromStdString(name);
        sTemp.remove("@");
        sTemp.remove(" ");
        objObject.insert("name",sTemp );
    }

    objObject.insert("type_and_cargo", msg.type_and_cargo);
    objObject.insert("dim_a", msg.dim_a);
    objObject.insert("dim_b", msg.dim_b);
    objObject.insert("dim_c", msg.dim_c);
    objObject.insert("dim_d", msg.dim_d);
    objObject.insert("fix_type", msg.fix_type);
    objObject.insert("eta_month", msg.eta_month);
    objObject.insert("eta_day", msg.eta_day);
    objObject.insert("eta_hour", msg.eta_hour);
    objObject.insert("eta_minute", msg.eta_minute);
    objObject.insert("draught", msg.draught);
    if (msg.destination.empty())
    {
        objObject.insert("destination", "---");
    }
    else
    {
        std::string  destination = msg.destination;
//        boost::algorithm::erase_all(destination, "@");
//        boost::algorithm::trim_right_if(destination, boost::algorithm::is_any_of(" "));
        QString sTemp = QString::fromStdString(destination);
        sTemp.remove("@");
        sTemp.remove(" ");
        objObject.insert("destination", sTemp);
    }

    objObject.insert("dte", msg.dte);
    objObject.insert("spare", msg.spare);

    objObject.insert("UTC", recivetime);
    objObject.insert("IS_OK", true);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais6_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais6 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    bool isKepp = false;
    std::string shipName = "";
    if (msg.dac == AIS_DAC_1_INTERNATIONAL)
    {
        switch (msg.fi)
        {
        case 0:
            break;
        case 11:
            break;
        case 13:
            break;
        case 15:
            break;
        case 20:
        {
            Ais6_1_20 msg6(sAisBody.toLatin1().constData(), uPad);
            isKepp = true;
            shipName = msg6.name;
            //qDebug()<<"shipName: "<< shipName.c_str();
        }
            break;
        default:
            break;
        }
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);

    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);

    objObject.insert("seq",msg.seq);
    objObject.insert("mmsi_dest",msg.mmsi_dest);
    objObject.insert("retransmit",msg.retransmit);
    objObject.insert("spare",msg.spare);
    objObject.insert("dac",msg.dac);
    objObject.insert("fi",msg.fi);
    objObject.insert("IS_OK", true);
    return objObject;
}

QJsonObject ZCHXAisDataServer::ais7_13_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais7_13 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);

    objObject.insert("spare",msg.spare);
    objObject.insert("IS_OK", true);

//    if(msg.dest_mmsi.size()>0 )
//    {
//        for(size_t i = 0;i<msg.dest_mmsi.size();i++ )
//        {
//            ZCHXLOG_DEBUG("===目的台"<<(i+1)<<"===\n"
//                          <<"目的台MMSI: "<<msg.dest_mmsi[i]<<"\n"
//                          <<"目的台序列号: "<<msg.seq_num[i]<<"\n"
//                          );
//        }
//    }


    return objObject;
}

QJsonObject ZCHXAisDataServer::ais8_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais8 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);

    bool isKepp = false;
    //AisMsg *keepMsg = NULL;
    std::string shipName = "";
    if (msg.dac == AIS_DAC_1_INTERNATIONAL)
    {
        switch (msg.fi)
        {
        case 0:
            break;
        case 11:
            break;
        case 13:
            break;
        case 19:
            {
                Ais8_1_19 msg8(sAisBody.toLatin1().constData(), uPad);
                isKepp = true;
                shipName = msg8.name;
                //ZCHXLOG_DEBUG("shipName: " << shipName.c_str());
            }
            break;
        default:
            break;
        }
    }

    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);

    objObject.insert("spare", msg.spare);
    objObject.insert("dac", msg.dac);
    objObject.insert("fi", msg.fi);
    objObject.insert("IS_OK", true);
    return objObject;

}

QJsonObject ZCHXAisDataServer::ais9_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais9 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);

    objObject.insert("alt", msg.alt);
    objObject.insert("sog", msg.sog);
    objObject.insert("position_accuracy", msg.position_accuracy);
    objObject.insert("x", msg.x);
    objObject.insert("y", msg.y);
    objObject.insert("cog", msg.cog);
    objObject.insert("timestamp", msg.timestamp);
    objObject.insert("alt_sensor", msg.alt_sensor);
    objObject.insert("spare", msg.spare);
    objObject.insert("dte", msg.dte);
    objObject.insert("spare2", msg.spare2);
    objObject.insert("assigned_mode", msg.assigned_mode);
    objObject.insert("raim", msg.raim);
    objObject.insert("commstate_flag", msg.commstate_flag);
    objObject.insert("sync_state", msg.sync_state);

    if(0 == msg.commstate_flag)
    {
        objObject.insert("slot_timeout", msg.slot_timeout);
        switch (msg.slot_timeout)
        {
        case 0:
            objObject.insert("slot_offset", msg.slot_offset);
            break;
        case 1:
            objObject.insert("utc_hour", msg.utc_hour);
            objObject.insert("utc_min", msg.utc_min);
            objObject.insert("utc_spare", msg.utc_spare);
            break;
        case 2:
        case 4:
        case 6:
            objObject.insert("slot_number", msg.slot_number);
            break;
        case 3:
        case 5:
        case 7:
            objObject.insert("received_stations", msg.received_stations);
            break;
        }
    }
    objObject.insert("UTC", recivetime);
    objObject.insert("IS_OK", true);
    return objObject;
}

QJsonObject ZCHXAisDataServer::ais10_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais10 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("spare", msg.spare);
    objObject.insert("dest_mmsi", msg.dest_mmsi);
    objObject.insert("spare2", msg.spare2);
    return objObject;
}

QJsonObject ZCHXAisDataServer::ais12_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais12 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("seq_num", msg.seq_num);
    objObject.insert("dest_mmsi", msg.dest_mmsi);
    objObject.insert("retransmitted", msg.retransmitted);
    objObject.insert("spare", msg.spare);
    objObject.insert("text", QString::fromStdString(msg.text));
    return objObject;
}

QJsonObject ZCHXAisDataServer::ais14_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais14 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("spare", msg.spare);
    objObject.insert("text", QString::fromStdString(msg.text));
    objObject.insert("expected_num_spare_bits", msg.expected_num_spare_bits);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais15_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais15 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("spare", msg.spare);
    objObject.insert("mmsi_1", msg.mmsi_1);
    objObject.insert("msg_1_1", msg.msg_1_1);
    objObject.insert("slot_offset_1_1", msg.slot_offset_1_1);
    objObject.insert("spare2", msg.spare2);
    objObject.insert("dest_msg_1_2", msg.dest_msg_1_2);
    objObject.insert("slot_offset_1_2", msg.slot_offset_1_2);
    objObject.insert("spare3", msg.spare3);
    objObject.insert("mmsi_2", msg.mmsi_2);
    objObject.insert("slot_offset_2", msg.slot_offset_2);
    objObject.insert("spare4", msg.spare4);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais16_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais16 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("spare", msg.spare);
    objObject.insert("dest_mmsi_a", msg.dest_mmsi_a);
    objObject.insert("offset_a", msg.offset_a);
    objObject.insert("inc_a", msg.inc_a);
    objObject.insert("dest_mmsi_b", msg.dest_mmsi_b);
    objObject.insert("offset_b", msg.offset_b);
    objObject.insert("inc_b", msg.inc_b);
    objObject.insert("spare2", msg.spare2);
    return objObject;
}

QJsonObject ZCHXAisDataServer::ais17_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais17 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("spare", msg.spare);
    objObject.insert("x", msg.x);
    objObject.insert("y", msg.y);
    objObject.insert("spare2", msg.spare2);
    objObject.insert("gnss_type", msg.gnss_type);
    objObject.insert("z_cnt", msg.z_cnt);
    objObject.insert("station", msg.station);
    objObject.insert("seq", msg.seq);
    objObject.insert("health", msg.health);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais18_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais18 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("ship_type", "---");
    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);

    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("spare", msg.spare);
    objObject.insert("sog", msg.sog);
    objObject.insert("position_accuracy", msg.position_accuracy);
    objObject.insert("x", msg.x);
    objObject.insert("y", msg.y);
    objObject.insert("cog", msg.cog);
    objObject.insert("true_heading", msg.true_heading);
    objObject.insert("timestamp", msg.timestamp);

    objObject.insert("spare2", msg.spare2);

    objObject.insert("unit_flag", msg.unit_flag);
    objObject.insert("display_flag", msg.display_flag);
    objObject.insert("dsc_flag", msg.dsc_flag);
    objObject.insert("band_flag", msg.band_flag);
    objObject.insert("m22_flag", msg.m22_flag);
    objObject.insert("mode_flag", msg.mode_flag);

    objObject.insert("raim", msg.raim);


    objObject.insert("commstate_flag", msg.commstate_flag);
    if (0 == msg.unit_flag) {
        if (0 == msg.commstate_flag) {
            // SOTDMA
            objObject.insert("slot_timeout", msg.slot_timeout);
            switch (msg.slot_timeout) {
            case 0:
                objObject.insert("slot_offset", msg.slot_offset);
                break;
            case 1:
                objObject.insert("utc_hour", msg.utc_hour);
                objObject.insert("utc_min", msg.utc_min);
                objObject.insert("utc_spare", msg.utc_spare);
                break;
            case 2:  // FALLTHROUGH
            case 4:  // FALLTHROUGH
            case 6:
                objObject.insert("slot_number", msg.slot_number);
                break;
            case 3:  // FALLTHROUGH
            case 5:  // FALLTHROUGH
            case 7:
                objObject.insert("received_stations", msg.received_stations);
                break;
            }

        }
        else {
            // ITDMA
            objObject.insert("slot_increment", msg.slot_increment);
            objObject.insert("slots_to_allocate", msg.slots_to_allocate);
            objObject.insert("keep_flag", msg.keep_flag);
        }
    }  // do nothing if unit flag is 1... in CS mode and no commstate


    objObject.insert("UTC", recivetime);
    objObject.insert("IS_OK", true);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais19_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais19 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("ship_type", "BSO");
    static bool aisType = true;//false=19,true=9;
    if (aisType)
    {
        objObject.insert("id", msg.message_id);
        //aisType = false;
    }
//    else
//    {
//        objObject.insert("id", (msg.message_id -1) );
//        aisType = true;
//    }

    objObject.insert("repeat_indicator", msg.repeat_indicator);

    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    std::string strMmsi = QString::number(msg.mmsi).toStdString();
    objObject.insert("country", QString::fromStdString(strMmsi.substr(0, 3)));//The first three digits convey information about the country in which the ID was issued <<ITU-MID>>

    objObject.insert("sog", msg.sog);
    objObject.insert("position_accuracy", msg.position_accuracy);
    objObject.insert("x", msg.x);
    objObject.insert("y", msg.y);
    objObject.insert("cog", msg.cog);
    objObject.insert("true_heading", msg.true_heading);
    objObject.insert("timestamp", msg.timestamp);
    objObject.insert("spare2", msg.spare2);

    if (msg.name.empty())
    {
        objObject.insert("name", "---");
    }
    else
    {
        std::string  name = msg.name;
//        boost::algorithm::erase_all(name, "@");
//        boost::algorithm::trim_right_if(name, boost::algorithm::is_any_of(" "));
        QString sTemp = QString::fromStdString(name);
        sTemp.remove("@");
        sTemp.remove(" ");
        objObject.insert("name", sTemp);
    }

    //ais19
    objObject.insert("callsign", "---");
    objObject.insert("vendor_id", "---");
    objObject.insert("part_num",1);

    objObject.insert("type_and_cargo", msg.type_and_cargo);
    objObject.insert("dim_a", msg.dim_a);
    objObject.insert("dim_b", msg.dim_b);
    objObject.insert("dim_c", msg.dim_c);
    objObject.insert("dim_d", msg.dim_d);
    objObject.insert("fix_type", msg.fix_type);

    objObject.insert("raim", msg.raim);

    objObject.insert("dte", msg.dte);
    objObject.insert("assigned_mode", msg.assigned_mode);
    objObject.insert("spare3", msg.spare3);

    objObject.insert("UTC", recivetime);
    objObject.insert("IS_OK", true);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais20_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais20 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("spare", msg.spare);
    objObject.insert("offset_1", msg.offset_1);
    objObject.insert("num_slots_1", msg.num_slots_1);
    objObject.insert("timeout_1", msg.timeout_1);
    objObject.insert("incr_1", msg.incr_1);

    objObject.insert("group_valid_2", msg.group_valid_2);
    if(msg.group_valid_2)
    {
        objObject.insert("offset_2", msg.offset_2);
        objObject.insert("num_slots_2", msg.num_slots_2);
        objObject.insert("timeout_2", msg.timeout_2);
        objObject.insert("incr_2", msg.incr_2);
    }

    objObject.insert("group_valid_3", msg.group_valid_3);
    if(msg.group_valid_3)
    {
        objObject.insert("offset_3", msg.offset_3);
        objObject.insert("num_slots_3", msg.num_slots_3);
        objObject.insert("timeout_3", msg.timeout_3);
        objObject.insert("incr_3", msg.incr_3);
    }

    objObject.insert("group_valid_4", msg.group_valid_4);
    if(msg.group_valid_4)
    {
        objObject.insert("offset_4", msg.offset_4);
        objObject.insert("num_slots_4", msg.num_slots_4);
        objObject.insert("timeout_4", msg.timeout_4);
        objObject.insert("incr_4", msg.incr_4);
    }

    objObject.insert("spare2", msg.spare2);
    return objObject;
}

QJsonObject ZCHXAisDataServer::ais21_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais21 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }


    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("spare", msg.spare);

    objObject.insert("aton_type", msg.aton_type);

    if (msg.name.empty())
    {
        objObject.insert("name", "---");
    }
    else
    {
        std::string  name = msg.name;
//        boost::algorithm::erase_all(name, "@");
//        boost::algorithm::trim_right_if(name, boost::algorithm::is_any_of(" "));
        QString sTemp = QString::fromStdString(name);
        sTemp.remove("@");
        sTemp.remove(" ");
        objObject.insert("name", sTemp);

    }

    objObject.insert("position_accuracy", msg.position_accuracy);
    objObject.insert("x", msg.x);
    objObject.insert("y", msg.y);
    objObject.insert("dim_a", msg.dim_a);
    objObject.insert("dim_b", msg.dim_b);
    objObject.insert("dim_c", msg.dim_c);
    objObject.insert("dim_d", msg.dim_d);
    objObject.insert("fix_type", msg.fix_type);
    objObject.insert("timestamp", msg.timestamp);
    objObject.insert("raim", msg.raim);
    objObject.insert("virtual_aton", msg.virtual_aton);
    objObject.insert("assigned_mode", msg.assigned_mode);


    objObject.insert("UTC", recivetime);
    objObject.insert("IS_OK", true);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais22_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais22 msg(sAisBody.toLatin1().constData(), uPad);
     if (msg.had_error()) {
         objObject.insert("IS_OK", false);
         return objObject;
     }

     objObject.insert("id", msg.message_id);
     objObject.insert("repeat_indicator", msg.repeat_indicator);
     if (0 == msg.mmsi)
     {
         objObject.insert("IS_OK", false);
         return objObject;
     }
     objObject.insert("mmsi", msg.mmsi);
     objObject.insert("IS_OK", true);

     objObject.insert("spare", msg.spare);
     objObject.insert("chan_a", msg.chan_a);
     objObject.insert("chan_b", msg.chan_b);
     objObject.insert("txrx_mode", msg.txrx_mode);
     objObject.insert("power_low", msg.power_low);

     objObject.insert("pos_valid", msg.pos_valid);
     objObject.insert("x1", msg.x1);
     objObject.insert("y1", msg.y1);
     objObject.insert("x2", msg.x2);
     objObject.insert("y2", msg.y2);

     objObject.insert("dest_valid", msg.dest_valid);
     objObject.insert("dest_mmsi_1", msg.dest_mmsi_1);
     objObject.insert("dest_mmsi_2", msg.dest_mmsi_2);
     objObject.insert("chan_a_bandwidth", msg.chan_a_bandwidth);
     objObject.insert("chan_b_bandwidth", msg.chan_b_bandwidth);
     objObject.insert("zone_size", msg.zone_size);
     objObject.insert("spare2", msg.spare2);

     return objObject;
}

QJsonObject ZCHXAisDataServer::ais23_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais23 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("spare", msg.spare);
    objObject.insert("x1", msg.x1);
    objObject.insert("y1", msg.y1);
    objObject.insert("x2", msg.x2);
    objObject.insert("y2", msg.y2);
    objObject.insert("station_type", msg.station_type);
    objObject.insert("type_and_cargo", msg.type_and_cargo);
    objObject.insert("spare2", msg.spare2);
    objObject.insert("txrx_mode", msg.txrx_mode);
    objObject.insert("interval_raw", msg.interval_raw);
    objObject.insert("quiet", msg.quiet);
    objObject.insert("spare3", msg.spare3);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais24_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais24 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("ship_type", "BCS");
    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);

    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("mmsi", msg.mmsi);
    std::string strMmsi = QString::number(msg.mmsi).toStdString();
    objObject.insert("country", QString::fromStdString(strMmsi.substr(0, 3)));//The first three digits convey information about the country in which the ID was issued <<ITU-MID>>
    objObject.insert("part_num", msg.part_num);

    switch (msg.part_num) {
    case 0:  // Part A
        if (msg.name.empty())
        {
            objObject.insert("name", "---");
        }
        else
        {
            std::string  name = msg.name;
//            boost::algorithm::erase_all(name, "@");
//            boost::algorithm::trim_right_if(name, boost::algorithm::is_any_of(" "));
            QString sTemp = QString::fromStdString(name);
            sTemp.remove("@");
            sTemp.remove(" ");
            objObject.insert("name",sTemp);
        }
        break;
    case 1:  // Part B
        objObject.insert("type_and_cargo", msg.type_and_cargo);

        if (msg.vendor_id.empty())
        {
            objObject.insert("vendor_id", "---");
        }
        else
        {
            std::string  vendor_id = msg.vendor_id;
//            boost::algorithm::erase_all(vendor_id, "@");
//            boost::algorithm::trim_right_if(vendor_id, boost::algorithm::is_any_of(" "));
            QString sTemp = QString::fromStdString(vendor_id);
            sTemp.remove("@");
            sTemp.remove(" ");
            objObject.insert("vendor_id", sTemp);
        }

        if (msg.callsign.empty())
        {
            objObject.insert("callsign", "---");
        }
        else
        {
            std::string  callsign = msg.callsign;
//            boost::algorithm::erase_all(callsign, "@");
//            boost::algorithm::trim_right_if(callsign, boost::algorithm::is_any_of(" "));
            QString sTemp = QString::fromStdString(callsign);
            sTemp.remove("@");
            sTemp.remove(" ");
            objObject.insert("callsign",sTemp );
        }

        objObject.insert("dim_a", msg.dim_a);
        objObject.insert("dim_b", msg.dim_b);
        objObject.insert("dim_c", msg.dim_c);
        objObject.insert("dim_d", msg.dim_d);
        objObject.insert("spare", msg.spare);

        break;
    case 2:  // FALLTHROUGH - not yet defined by ITU
    case 3:  // FALLTHROUGH - not yet defined by ITU
    default:
        // status = AIS_ERR_BAD_MSG_CONTENT;
        // TODO(schwehr): setup python exception
        //return NULL;
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("UTC", recivetime);
    objObject.insert("IS_OK", true);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais25_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais25 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("use_app_id", msg.use_app_id);
    objObject.insert("dest_mmsi_valid", msg.dest_mmsi_valid);
    objObject.insert("dest_mmsi", msg.dest_mmsi);
    objObject.insert("dac", msg.dac);
    objObject.insert("fi", msg.fi);

    return objObject;
}

QJsonObject ZCHXAisDataServer::ais26_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais26 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);
    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }
    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("use_app_id", msg.use_app_id);
    objObject.insert("dest_mmsi_valid", msg.dest_mmsi_valid);
    objObject.insert("dest_mmsi", msg.dest_mmsi);
    objObject.insert("dac", msg.dac);
    objObject.insert("fi", msg.fi);

    objObject.insert("commstate_flag", msg.commstate_flag);
    if (0 == msg.commstate_flag)
    {
        // SOTDMA
        objObject.insert("sync_state", msg.sync_state);
        objObject.insert("slot_timeout_valid", msg.slot_timeout_valid);
        if(msg.slot_timeout_valid)
        {
            objObject.insert("slot_timeout", msg.slot_timeout);
            switch (msg.slot_timeout)
            {
            case 0:
                objObject.insert("slot_offset_valid", msg.slot_offset_valid);
                if(msg.slot_offset_valid)
                {
                    objObject.insert("slot_offset", msg.slot_offset);
                }
                break;
            case 1:
                objObject.insert("utc_valid", msg.utc_valid);
                if(msg.utc_valid)
                {
                    objObject.insert("utc_hour", msg.utc_hour);
                    objObject.insert("utc_min", msg.utc_min);
                    objObject.insert("utc_spare", msg.utc_spare);
                }
                break;
            case 2:  // FALLTHROUGH
            case 4:  // FALLTHROUGH
            case 6:
                objObject.insert("slot_number_valid", msg.slot_number_valid);
                if(msg.slot_number_valid)
                {
                    objObject.insert("slot_number", msg.slot_number);
                }
                break;
            case 3:  // FALLTHROUGH
            case 5:  // FALLTHROUGH
            case 7:
                objObject.insert("received_stations_valid", msg.received_stations_valid);
                if(msg.received_stations_valid)
                {
                    objObject.insert("received_stations", msg.received_stations);
                }
                break;
            }
        }
    }
    else {
        // ITDMA
        objObject.insert("slot_increment", msg.slot_increment);
        objObject.insert("slots_to_allocate", msg.slots_to_allocate);
        objObject.insert("keep_flag", msg.keep_flag);
    }
    return objObject;
}

QJsonObject ZCHXAisDataServer::ais27_to_json(const QString &sAisBody, const int uPad, qint64 recivetime)
{
    QJsonObject objObject;
    Ais27 msg(sAisBody.toLatin1().constData(), uPad);
    if (msg.had_error()) {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("id", msg.message_id);
    objObject.insert("repeat_indicator", msg.repeat_indicator);

    if (0 == msg.mmsi)
    {
        objObject.insert("IS_OK", false);
        return objObject;
    }

    objObject.insert("mmsi", msg.mmsi);
    objObject.insert("IS_OK", true);

    objObject.insert("position_accuracy", msg.position_accuracy);
    objObject.insert("raim", msg.raim);
    objObject.insert("nav_status", msg.nav_status);
    objObject.insert("x", msg.x);
    objObject.insert("y", msg.y);
    objObject.insert("sog", msg.sog);
    objObject.insert("cog", msg.cog);
    objObject.insert("gnss", msg.gnss);
    objObject.insert("spare", msg.spare);

    return objObject;
}

//解析数据体部分
bool ZCHXAisDataServer::analysisCellAIS(const QString sCellAisData, int uPad)
{
    if(sCellAisData.isEmpty())
    {
        return false;
    }
    //qint64 recivetime = QTime::currentTime().msecsSinceStartOfDay();
    qint64 recivetime = QDateTime::currentMSecsSinceEpoch();
    char flag = sCellAisData[0].toLatin1();
    //qDebug()<<"数据体："<<sCellAisData;
    QJsonObject objObject;
    switch (flag) {
    // Class A Position
    case '1':
    case '2':
    case '3':
        //qDebug()<<"[RX] 收到消息为1 2 3 的A类船舶动态信息!";
        objObject = ais1_2_3_to_json(sCellAisData, uPad, recivetime);
        break;
    case '4':  //  4 - Basestation report
    case ';':  //  11 - UTC date response
        //qDebug()<<"[RX] 收到消息为4 11 的基地台报告类信息!";
        objObject = ais4_11_to_json(sCellAisData, uPad, recivetime);
        break;
    case '5':  // 5 - Ship and Cargo
        //qDebug()<<"[RX] 收到消息为5的A类船舶静态信息!";
        objObject = ais5_to_json(sCellAisData, uPad, recivetime);
        break;
    case '6': //6 - Addressed binary message
        //qDebug()<<"[RX] 收到消息为6的船舶信息(二进制编址信息)";
        objObject = ais6_to_json(sCellAisData, uPad, recivetime);
        break;
    case '7': // 7  FALLTHROUGH - 7 - ACK for addressed binary message
    case '=': // 13 - ASRM Ack  (safety message)
        //qDebug()<<"[RX] 收到消息为7 13 的船舶信息(消息确认,消息7:二进制确认,消息13:安全确认)";
        objObject = ais7_13_to_json(sCellAisData, uPad, recivetime);
        break;
    case '8': //8 - Binary broadcast message (BBM)
        //qDebug()<<"[RX] 收到消息为8的船舶信息(广播二进制信息)";
        objObject = ais8_to_json(sCellAisData, uPad, recivetime);
        break;
    case '9': //9 - SAR Position
        //qDebug()<<"[RX] 收到消息为9的船舶信息(标准搜救飞机位置报告)";
        objObject = ais9_to_json(sCellAisData, uPad, recivetime);
        break;
    case ':': //10 - UTC Query
        //qDebug()<<"[RX] 收到消息为10的船舶信息(UTC询问)";
        objObject = ais10_to_json(sCellAisData, uPad, recivetime);
        break;
    case '<': // 12 - Addressed Safety Related Messages (ASRM)
        //qDebug()<<"[RX] 收到消息为12的船舶信息(编址安全信息)";
        objObject = ais12_to_json(sCellAisData, uPad, recivetime);
        break;
    case '>': // 14 - Safety Related Broadcast Message (SRBM)
        //qDebug()<<"[RX] 收到消息为14的船舶信息(安全广播信息)";
        objObject = ais14_to_json(sCellAisData, uPad, recivetime);
        break;
    case '?': // 15 - Interrogation
        //qDebug()<<"[RX] 收到消息为15的船舶信息(询问消息(除UTC))";
        objObject = ais15_to_json(sCellAisData, uPad, recivetime);
        break;
    case '@': // 16 - Assigned mode command
        //qDebug()<<"[RX] 收到消息为16的船舶信息(分配模式指令)";
        objObject = ais16_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'A': //17 - GNSS broadcast
        //qDebug()<<"[RX] 收到消息为17的船舶信息(全球导航卫星系统(GNSS)广播二进制信息)";
        objObject = ais17_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'B':  // 18 - Position, Class B
        //qDebug()<<"[RX] 收到消息为18的B类船舶动态信息!";
        objObject = ais18_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'C':  // 19 - Position and ship, Class B
        //qDebug()<<"[RX] 收到消息为19的B类船舶静态及动态信息!";
        objObject = ais19_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'D': // 20 - Data link management
        //qDebug()<<"[RX] 收到消息为20的船舶信息(数据链路管理消息)";
        objObject = ais20_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'E':  // 21 - Aids to navigation report
        //qDebug()<<"[RX] 收到消息为21的助航信息!";
        objObject = ais21_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'F':  // 22 - Channel Management
    case 'f':
        //qDebug()<<"[RX] 收到消息为22的船舶信息(信道管理)";
        objObject = ais22_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'G':  //23 - Group Assignment Command
        //qDebug()<<"[RX] 收到消息为23的船舶信息";
        objObject = ais23_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'H':  // 24 - Static data report
    case 'h':
        //qDebug()<<"[RX] 收到消息为24的B类船舶静态信息!";
        objObject = ais24_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'I':  // 25 - Single slot binary message
        //qDebug()<<"[RX] 收到消息为25的船舶信息";
        objObject = ais25_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'J':  // 26 - Multi slot binary message with comm state
        //qDebug()<<"[RX] 收到消息为26的船舶信息";
        objObject = ais26_to_json(sCellAisData, uPad, recivetime);
        break;
    case 'K':  // 27 - Long-range AIS broadcast message
        //qDebug()<<"[RX] 收到消息为27的船舶信息";
        objObject = ais27_to_json(sCellAisData, uPad, recivetime);
        break;
    default:
        objObject.insert("IS_OK", false);
    }
    QJsonDocument doc;
    ITF_AISList objAisList;
    if(objObject.value("IS_OK").toBool())
    {
        objAisList.Clear();
        buildAis(objObject,objAisList);
        if(objAisList.ais_size()>0)
        {
            sendAis(objAisList);
        }

    }


}

void ZCHXAisDataServer::buildAis(const QJsonObject &obj, ITF_AISList &aisList)
{
    int uID = obj.value("id").toInt();
    ITF_AIS  ais;
    ITF_AIS*  temp_ais;
    bool bOK = false;
    //消息为1 2 3 的A类船舶动态信息,消息为18的B类船舶动态信息,消息为19的B类船舶静态及动态信息

    if ((uID == 1) || (uID == 2) || (uID == 3) || (uID == 18))
    {
          ais = buildVesselTrack(obj,bOK);
    }
    //消息为4 11 的基地台报告类信息
    if((uID==4) || (uID==11))
    {
        ais = buildBasestationInfo(obj,bOK);
    }
    //消息为5的A类船舶静态信息
    if (uID == 5)
    {
        ais = buildVesselInfo(obj,bOK);
    }
    //消息为21的助航信息
    if(uID==21)
    {
        ais = buildNavigationInfo(obj,bOK);
    }
    //消息为24的B类船舶静态信息
    if ( (uID == 24) || (uID == 19) )
    {
        ais = buildPartialVesselInfo(obj,bOK);
    }
    if(bOK)
    {
        temp_ais  =  aisList.add_ais();
        temp_ais->CopyFrom(ais);
    }
    if(uID == 19)
    {
        ais = buildVesselTrack(obj,bOK);
        if(bOK)
        {
            temp_ais  =  aisList.add_ais();
            temp_ais->CopyFrom(ais);
        }
    }
}

void ZCHXAisDataServer::sendAis( ITF_AISList &objAisList)
{

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    objAisList.set_utc(utc);

    //通过zmq发送
    QByteArray sendData;
    sendData.resize(objAisList.ByteSize());
    objAisList.SerializePartialToArray(sendData.data(),sendData.size());
    //qDebug()<<"m_uAISSendPort"<<m_uAISSendPort;

    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uAISSendPort);

    QString sTopic = m_sAISTopic;
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();

//    zmq_bind(m_pAISLisher, sIPport.toLatin1().data());//
    zmq_send(m_pAISLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pAISLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pAISLisher, sendData.data(), sendData.size(), 0);

    QString sContent = tr("send analysis ais data,size = %1").arg(sendData.size());
    emit signalSendRecvedContent(utc,"AIS_SEND",sContent);

}

ITF_AIS ZCHXAisDataServer::buildVesselTrack(const QJsonObject &obj, bool &bOK)
{
    ITF_AIS objAis;
    int id = obj.value("id").toInt();
    int mmsi = obj.value("mmsi").toInt();
    float sog = obj.value("sog").toDouble();
    //int position_accuracy = obj.value("position_accuracy").toInt();
    double x = obj.value("x").toDouble();
    double y = obj.value("y").toDouble();
    float cog = obj.value("cog").toDouble();
    uint true_heading = obj.value("true_heading").toInt();

    int nav_status = 15; //未定义 Not defined (default)
    float rot = 0.0;
    if ((id == 1) || (id == 2) || (id == 3))
    {
        nav_status = obj.value("nav_status").toInt();
        rot = obj.value("rot").toDouble();
    }

    com::zhichenhaixin::proto::VesselTrack vesselTrack;

    vesselTrack.set_utc(obj.value("UTC").toVariant().toLongLong());
    vesselTrack.set_mmsi(mmsi);

    //std::string strMessageId = boost::lexical_cast<std::string>(id);
    QString strMmsi = QString::number(mmsi);

    QString ship_type = obj.value("ship_type").toString();
    QString sID = "AIS_" + ship_type + "__" + strMmsi;
    vesselTrack.set_id(sID.toLatin1().constData());
    vesselTrack.set_shiptype(ship_type.toLatin1().constData());

    if ((nav_status >= 0) && (nav_status <= 15))
    {
        vesselTrack.set_navstatus(static_cast<com::zhichenhaixin::proto::NAVI_STATUS>(nav_status));
    }
    else
    {
        vesselTrack.set_navstatus(static_cast<com::zhichenhaixin::proto::NAVI_STATUS>(15));
    }


    if (sog < SOG_KNOTS_MIN)
    {
        vesselTrack.set_sog(SOG_KNOTS_MIN);
    }
    else if (sog > SOG_KNOTS_MAX)
    {
        vesselTrack.set_sog(SOG_KNOTS_MAX);
    }
    else
    {
        vesselTrack.set_sog(sog);
    }

    if (x > LONG_DEGREES_MAX || x < LONG_DEGREES_MIN)
    {
        vesselTrack.set_lon(LON_DEGREES_NA);
    }
    else
    {
        vesselTrack.set_lon(x);
    }


    if (y > LAT_DEGREES_MAX || y < LAT_DEGREES_MIN)
    {
        vesselTrack.set_lat(LAT_DEGREES_NA);
    }
    else
    {
        vesselTrack.set_lat(y);
    }


    if (cog < COG_DEGREES_MIN)
    {
        vesselTrack.set_cog(COG_DEGREES_MIN);
    }
    else if (cog > COG_DEGREES_MAX)
    {
        vesselTrack.set_cog(COG_DEGREES_MAX);
    }
    else
    {
        vesselTrack.set_cog(cog);
    }

    if (true_heading > HEADING_DEGREE_MAX || true_heading < HEADING_DEGREE_MIN)
    {
        vesselTrack.set_heading(HEADING_DEGREE_NA);
    }
    else {
        vesselTrack.set_heading(true_heading);
    }
    vesselTrack.set_rot(rot);


//    qDebug()<<"船舶动态实例信息如下:" << "  \n"
//                  << "用户识别码: " << vesselTrack.mmsi() << "  \n"
//                  << "唯一识别码: " << QString::fromStdString(vesselTrack.id()) << "  \n"
//                  << "船舶种类: " << QString::fromStdString(vesselTrack.shiptype()) << "  \n"
//                  << "船舶航行状态: " << int(vesselTrack.navstatus()) << "  \n"
//                  << "船舶转向率: " << vesselTrack.rot() << " \n"
//                  << "对地航速: " << vesselTrack.sog() << " \n"
//                  << "经度: " << vesselTrack.lon() << " \n"
//                  << "纬度: " << vesselTrack.lat() << " \n"
//                  << "对地航向: " << vesselTrack.cog() << " \n"
//                  << "船艏向: " << vesselTrack.heading() << " \n"
//                  << "时间标记: " << vesselTrack.utc() << " \n"
//                  <<"时间标记UTC秒: "<<obj.value("timestamp").toInt()
                  ;
    // p1_2_3.add<int>("timestamp", msg.timestamp);pt.get<float>("cog");//

    objAis.set_flag(id);
    QString sSourceID = "1";
    objAis.set_sourceid(sSourceID.toLatin1().constData());
    objAis.mutable_vesseltrack()->CopyFrom(vesselTrack);
    bOK = true;
    return objAis;
}

ITF_AIS ZCHXAisDataServer::buildBasestationInfo(const QJsonObject &obj,bool &bOK)
{
    ITF_AIS objAis;
    int id=obj.value("id").toInt();
    int repeatIndicator=obj.value("repeat_indicator").toInt();

    int mmsi=obj.value("mmsi").toInt();
    int t_year=obj.value("year").toInt();
    int t_month=obj.value("month").toInt();
    int t_day=obj.value("rotday").toInt();
    int t_hour=obj.value("hour").toInt();
    int t_minute=obj.value("minute").toInt();
    int t_second=obj.value("second").toInt();
    int positionAccuracy=obj.value("position_accuracy").toInt();
    double x=obj.value("x").toDouble();
    double y=obj.value("y").toDouble();
    int fix_type=obj.value("fix_type").toInt();


    com::zhichenhaixin::proto::BaseStationReport baseStationInfo;

    baseStationInfo.set_mmsi(mmsi);
    baseStationInfo.set_id(id);//
    baseStationInfo.set_repeatindicator(repeatIndicator);
    baseStationInfo.set_year(t_year);
    baseStationInfo.set_month(t_month);
    baseStationInfo.set_day(t_day);
    baseStationInfo.set_hour(t_hour);
    baseStationInfo.set_minute(t_minute);
    baseStationInfo.set_second(t_second);

    baseStationInfo.set_positionaccuracy(positionAccuracy);
    baseStationInfo.set_lon(x);
    baseStationInfo.set_lat(y);

    switch (fix_type)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        baseStationInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(fix_type));
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        baseStationInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(8));
        break;
    default:
        baseStationInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(0));
    }

    baseStationInfo.set_utc(obj.value("UTC").toVariant().toLongLong());


//    ZCHXLOG_DEBUG( "基地台报告信息如下: " << "\n"
//                   << "唯一识别码: " << baseStationInfo.id() << "\n"
//                   << "转发指示符重发次数: " << baseStationInfo.repeatindicator() << "\n"
//                   << "用户识别码: " << baseStationInfo.mmsi() << "\n"
//                   << "UTC年份: " << baseStationInfo.year() << "\n"
//                   << "UTC月份: " << baseStationInfo.month() << "\n"
//                   << "UTC日期: " << baseStationInfo.day() << "\n"
//                   << "UTC小时: " << baseStationInfo.hour() << "\n"
//                   << "UTC分钟: " << baseStationInfo.minute() << "\n"
//                   << "UTC秒: " << baseStationInfo.second() << "\n"
//                   //<< "船位精准度 :" << baseStationInfo.positionaccuracy() << "\n"
//                   // << "经度 :" << baseStationInfo.lon() << "\n"
//                   // << "纬度 :" << baseStationInfo.lat() << "\n"
//                   // << "电子定位装置类型 :" << baseStationInfo.fixtype() << "\n"
//                   << "时间标记 :" << baseStationInfo.utc() << "\n"
//                   );

    objAis.set_flag(id);
    QString sSourceID = "1";
    objAis.set_sourceid(sSourceID.toLatin1().constData());
    objAis.mutable_basestationreport()->CopyFrom(baseStationInfo);
    bOK = true;
    return objAis;
}

ITF_AIS ZCHXAisDataServer::buildVesselInfo(const QJsonObject &obj, bool &bOK)
{
    ITF_AIS objAis;
    int id = obj.value("id").toInt();
    int mmsi = obj.value("mmsi").toInt();
    QString country = obj.value("country").toString();
    QString name = obj.value("name").toString();
    int type_and_cargo = obj.value("type_and_cargo").toInt();
    int dim_a = obj.value("dim_a").toInt();
    int dim_b = obj.value("dim_b").toInt();
    int dim_c = obj.value("dim_c").toInt();
    int dim_d = obj.value("dim_d").toInt();


    com::zhichenhaixin::proto::VesselInfo vesselInfo;

    vesselInfo.set_utc(obj.value("UTC").toVariant().toLongLong());
    vesselInfo.set_mmsi(mmsi);
    //std::string strMessageId = boost::lexical_cast<std::string>(id);
    QString strMmsi = QString::number(mmsi);

    QString ship_type = obj.value("ship_type").toString();
    QString sID = "AIS_" + ship_type + "__" + strMmsi;
    vesselInfo.set_id(sID.toLatin1().constData());

    vesselInfo.set_country(country.toLatin1().constData());
    vesselInfo.set_shiptype( ship_type.toLatin1().constData()  );
    vesselInfo.set_shipname(name.toLatin1().constData());
    vesselInfo.set_cargotype(type_and_cargo);
    vesselInfo.set_tobow(dim_a);
    vesselInfo.set_tostern(dim_b);
    vesselInfo.set_toport(dim_c);
    vesselInfo.set_tostarboard(dim_d);
    vesselInfo.set_shiplength(dim_a + dim_b);
    vesselInfo.set_shipwidth(dim_c + dim_d);

    //必须先初始化，否则Java订阅端会出错！
    vesselInfo.set_imo(IMO_NUM_NA);
    vesselInfo.set_callsign("---");
    vesselInfo.set_vendorid("---");
    vesselInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(0));
    vesselInfo.set_eta("---");
    vesselInfo.set_draught(DRAUGHT_NA);
    vesselInfo.set_dest("---");

    if (id == 5)
    {
        int imo_num = obj.value("imo_num").toInt();
        QString callsign = obj.value("callsign").toString();
        int eta_month = obj.value("eta_month").toInt();
        int eta_day = obj.value("eta_day").toInt();
        int eta_hour = obj.value("eta_hour").toInt();
        int eta_minute = obj.value("eta_minute").toInt();
        //boost::format fmt("%02d-%02d %02d:%02d");
        //fmt % eta_month % eta_day % eta_hour % eta_minute;
        QString eta = QString("%1-%2 %3:%4").arg(eta_month).arg(eta_day).arg(eta_hour).arg(eta_minute);
        float draught = obj.value("draught").toDouble();
        QString destination = obj.value("destination").toString();
        int fix_type = obj.value("fix_type").toInt();

        switch (fix_type)
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            vesselInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(fix_type));
            break;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            vesselInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(8));
            break;
        default:
            vesselInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(0));
        }

        vesselInfo.set_imo(imo_num);
        vesselInfo.set_callsign(callsign.toLatin1().constData());


        vesselInfo.set_eta(eta.toLatin1().constData());

        if (draught > DRAUGHT_MAX || draught < DRAUGHT_MIN)
        {
            vesselInfo.set_draught(DRAUGHT_NA);
        }
        else {
            vesselInfo.set_draught(draught);
        }

        vesselInfo.set_dest(destination.toLatin1().constData());

    }


//    ZCHXLOG_DEBUG("船舶静态实例信息如下:" << "  \n"
//        << "用户识别码: " << vesselInfo.mmsi() << "  \n"
//        << "唯一识别码: " << vesselInfo.id() << "  \n"
//        << "船舶种类: " << vesselInfo.shiptype() << "  \n"
//        << "IMO 号码: " << vesselInfo.imo() << " \n"
//        << "Call Sign 呼号: " << vesselInfo.callsign() << " \n"
//        << "船名: " << vesselInfo.shipname() << " \n"
//        << "船舶类型: " << vesselInfo.cargotype() << " \n"
//        << "国籍: " << vesselInfo.country() << " \n"
//        << "制造商ID: " << vesselInfo.vendorid() << " \n"
//        << "船长: " << vesselInfo.shiplength() << " \n"
//        << "船宽: " << vesselInfo.shipwidth() << " \n"
//        << "dim to a: " << vesselInfo.tobow() << " \n"
//        << "dim to b: " << vesselInfo.tostern() << " \n"
//        << "dim to c: " << vesselInfo.toport() << " \n"
//        << "dim to d: " << vesselInfo.tostarboard() << " \n"
//        << "电子定位装置类型: " << vesselInfo.fixtype() << " \n"
//        << "预计到达时间: " << vesselInfo.eta() << " \n"
//        << "当前最深静态吃水量: " << vesselInfo.draught() << " \n"
//        << "目的地: " << vesselInfo.dest() << " \n"
//        << "时间标记: " << vesselInfo.utc() << " \n");


    objAis.set_flag(id);
    QString sSourceID = "1";
    objAis.set_sourceid(sSourceID.toLatin1().constData());
    objAis.mutable_vesselinfo()->CopyFrom(vesselInfo);
    bOK = true;
    return objAis;
}

ITF_AIS ZCHXAisDataServer::buildNavigationInfo(const QJsonObject &obj,bool &bOK)
{
    ITF_AIS objAis;
    int id=obj.value("id").toInt();
    int repeatindicator=obj.value("repeat_indicator").toInt();
    int mmsi=obj.value("mmsi").toInt();


    QString name=obj.value("name").toString();
    int positionAccuracy=obj.value("position_accuracy").toInt();
    double lon=obj.value("y").toDouble();
    double lat=obj.value("y").toDouble();
    int toBow=obj.value("dim_a").toInt();
    int toStern=obj.value("dim_b").toInt();
    int toPort=obj.value("dim_c").toInt();
    int toStarboard=obj.value("dim_d").toInt();
    int fix_type=obj.value("fix_type").toInt();
    long long UTC=obj.value("UTC").toVariant().toLongLong();

    com::zhichenhaixin::proto::AidtoNavigationReport aidtoNav;

    aidtoNav.set_id(id);
    aidtoNav.set_repeatindicator(repeatindicator);
    aidtoNav.set_mmsi(mmsi);

    int aton_type = obj.value("aton_type").toInt();

    if((aton_type>=0) && (aton_type<=31))
    {
        aidtoNav.set_atontype(static_cast<com::zhichenhaixin::proto::ATON_TYPE>(aton_type));
    }
    else
    {
        aidtoNav.set_atontype(static_cast<com::zhichenhaixin::proto::ATON_TYPE>(0));
    }
    aidtoNav.set_name(name.toLatin1().constData());
    aidtoNav.set_positionaccuracy(positionAccuracy);
    aidtoNav.set_lon(lon);
    aidtoNav.set_lat(lon);
    aidtoNav.set_tobow(toBow);
    aidtoNav.set_tostern(toStern);
    aidtoNav.set_toport(toPort);
    aidtoNav.set_tostarboard(toStarboard);

    switch (fix_type)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        aidtoNav.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(fix_type));
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        aidtoNav.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(8));
        break;
    default:
        aidtoNav.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(0));
    }


    aidtoNav.set_utc(UTC);

//    ZCHXLOG_DEBUG("助航设备报告如下: "<<"\n"
//                  <<"唯一识别码: "<<aidtoNav.id()<<"\n"
//                  <<"转发指示符重发次数: "<<aidtoNav.repeatindicator()<<"\n"
//                  <<"用户识别码: "<<aidtoNav.mmsi()<<"\n"
//                  <<"助航类型: "<<aidtoNav.name()<<"\n"
//                  <<"助航名称: "<<aidtoNav.lon()<<"\n"
//                  <<"船位精准度: "<<aidtoNav.positionaccuracy()<<"\n"
//                  <<"经度: "<<aidtoNav.lon()<<"\n"
//                  <<"纬度: "<<aidtoNav.lat()<<"\n"
//                  <<"toBow: "<<aidtoNav.tobow()<<"\n"
//                  <<"toStern: "<<aidtoNav.tostern()<<"\n"
//                  <<"toPort: "<<aidtoNav.toport()<<"\n"
//                  <<"toStarboard: "<<aidtoNav.tostarboard()<<"\n"
//                  <<"fixType: "<<aidtoNav.fixtype()<<"\n"
//                  <<"UTC: "<<aidtoNav.utc()<<"\n");
    objAis.set_flag(id);
    QString sSourceID = "1";
    objAis.set_sourceid(sSourceID.toLatin1().constData());
    objAis.mutable_aidtonavigationreport()->CopyFrom(aidtoNav);
    bOK = true;
    return objAis;
}

ITF_AIS ZCHXAisDataServer::buildPartialVesselInfo(const QJsonObject &obj, bool &bOK)
{
    ITF_AIS objAis;
    int id = obj.value("id").toInt();
    int mmsi = obj.value("mmsi").toInt();
    QString country = obj.value("country").toString();
    int part_num = obj.value("part_num").toInt();


    com::zhichenhaixin::proto::VesselInfo vesselInfo;

    vesselInfo.set_utc(obj.value("UTC").toVariant().toLongLong());
    vesselInfo.set_mmsi(mmsi);

    QString strMmsi = QString::number(mmsi);


    QString ship_type = obj.value("ship_type").toString();
    QString strMessageId = "AIS_" + ship_type + "__" + strMmsi;
    vesselInfo.set_id(strMessageId.toLatin1().constData());

    vesselInfo.set_shiptype(ship_type.toLatin1().constData());
    vesselInfo.set_country(country.toLatin1().constData());


    //初始化
    vesselInfo.set_imo(IMO_NUM_NA);
    vesselInfo.set_callsign("---");
    vesselInfo.set_shipname("---");
    vesselInfo.set_cargotype(CARGO_TYPE_NA);
    vesselInfo.set_vendorid("---");
    vesselInfo.set_tobow(0);
    vesselInfo.set_tostern(0);
    vesselInfo.set_toport(0);
    vesselInfo.set_tostarboard(0);
    vesselInfo.set_shiplength(0);
    vesselInfo.set_shipwidth(0);
    vesselInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(0));
    vesselInfo.set_eta("---");
    vesselInfo.set_draught(DRAUGHT_NA);
    vesselInfo.set_dest("---");


    if (part_num == 0)
    {
        QString name = obj.value("name").toString();
        if ((name.isEmpty()) && (name.size() == 0))
        {
//            qDebug()<<" do_build_partial_vessel_info encounter an empty name data: party data info:"
//                          << "用户识别码: " << vesselInfo.mmsi() << "  \n"
//                          << "唯一识别码: " << QString::fromStdString(vesselInfo.id()) << "  \n"
//                          << "船舶种类: " << QString::fromStdString(vesselInfo.shiptype()) << "  \n"
//            ;
            bOK = false;
            return objAis;
        }

        vesselInfo.set_shipname(name.toLatin1().constData());
    }
    else
    {

        int type_and_cargo = obj.value("type_and_cargo").toInt();


        QString vendor_id = obj.value("vendor_id").toString();
        QString callsign = obj.value("callsign").toString();
        int dim_a = obj.value("dim_a").toInt();
        int dim_b = obj.value("dim_b").toInt();
        int dim_c = obj.value("dim_c").toInt();
        int dim_d = obj.value("dim_d").toInt();

        vesselInfo.set_cargotype(type_and_cargo);
        vesselInfo.set_vendorid(vendor_id.toLatin1().constData());
        vesselInfo.set_callsign(callsign.toLatin1().constData());
        vesselInfo.set_tobow(dim_a);
        vesselInfo.set_tostern(dim_b);
        vesselInfo.set_toport(dim_c);
        vesselInfo.set_tostarboard(dim_d);
        vesselInfo.set_shiplength(dim_a + dim_b);
        vesselInfo.set_shipwidth(dim_c + dim_d);
    }


//    ZCHXLOG_DEBUG("船舶静态实例信息如下: " << "  \n"
//        << "用户识别码: " << vesselInfo.mmsi() << "  \n"
//        << "唯一识别码: " << vesselInfo.id() << "  \n"
//        << "船舶种类: " << vesselInfo.shiptype() << "  \n"
//        << "IMO 号码: " << vesselInfo.imo() << " \n"
//        << "Call Sign 呼号: " << vesselInfo.callsign() << " \n"
//        << "船名: " << vesselInfo.shipname() << " \n"
//        << "船舶类型: " << vesselInfo.cargotype() << " \n"
//        << "国籍: " << vesselInfo.country() << " \n"
//        << "制造商ID: " << vesselInfo.vendorid() << " \n"
//        << "船长: " << vesselInfo.shiplength() << " \n"
//        << "船宽: " << vesselInfo.shipwidth() << " \n"
//        << "dim to a: " << vesselInfo.tobow() << " \n"
//        << "dim to b: " << vesselInfo.tostern() << " \n"
//        << "dim to c: " << vesselInfo.toport() << " \n"
//        << "dim to d: " << vesselInfo.tostarboard() << " \n"
//        << "电子定位装置类型: " << vesselInfo.fixtype() << " \n"
//        << "预计到达时间: " << vesselInfo.eta() << " \n"
//        << "当前最深静态吃水量: " << vesselInfo.draught() << " \n"
//        << "目的地: " << vesselInfo.dest() << " \n"
//        << "时间标记: " << vesselInfo.utc() << " \n");

    objAis.set_flag(id);
    QString sSourceID = "1";
    objAis.set_sourceid(sSourceID.toLatin1().constData());
    objAis.mutable_vesselinfo()->CopyFrom(vesselInfo);
    bOK = true;
    return objAis;
}

#endif

void ZCHXAisDataServer::displayError(QAbstractSocket::SocketError error)
{
    if(m_bServer)
    {
        if(m_pTcpServer)
            emit signalSocketMsg(QString("ZCHXAisDataServer(Server):%1").arg(m_pTcpServer->errorString()));
    } else
    {
        if(m_pTcpSocket)
            emit signalSocketMsg(QString("ZCHXAisDataServer(Client):%1").arg(m_pTcpSocket->errorString()));
    }
}

void ZCHXAisDataServer::updateServerProgress()
{
    if(m_pTcpSocket == NULL)
    {
        return;
    }
    QByteArray aisArray = m_pTcpSocket->readAll();
    emit signalSendAisData(aisArray);
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive ais data,size = %1").arg(aisArray.size());
    emit signalSendRecvedContent(utc,"AIS_RECEIVE",sContent);
    mLastRecvAisDataTime = QDateTime::currentMSecsSinceEpoch();
}

void ZCHXAisDataServer::acceptConnection()
{
    if(m_pTcpServer == NULL)
    {
        return;
    }
    m_pTcpSocket = m_pTcpServer->nextPendingConnection();
    connect(m_pTcpSocket,SIGNAL(readyRead()),this,SLOT(updateServerProgress()));
}

void ZCHXAisDataServer::stateChanged(QAbstractSocket::SocketState state)
{
    if(state == QAbstractSocket::SocketState::ConnectedState)
    {
         LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
        emit signalSocketMsg(QString("ZCHXAisDataServer(Client): connect server success."));
    } else if(state == QAbstractSocket::SocketState::UnconnectedState)
    {
         LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
        emit signalSocketMsg(QString("ZCHXAisDataServer(Client): disconnect server"));
        cout<<"disconnect server";
        //init();
    }
}

void ZCHXAisDataServer::slotCheckAisRecv()
{
    //1分钟未接收到数据就重连
    if(QDateTime::currentMSecsSinceEpoch() - mLastRecvAisDataTime > mDataTimeOut *60 *1000 )
    {
        LOG(LOG_RTM, "last recv time:%s, now:%s, now reconnect.",\
            QDateTime::fromMSecsSinceEpoch(mLastRecvAisDataTime).toString("yyyy-MM-dd hh:mm:ss").toStdString().data(),\
            QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString().data()
            );
        emit signalSocketMsg(QString("ZCHXAisDataServer: receive data timeout. reconnect now."));
        init();
         LOG(LOG_RTM, "%s %s %d,socket = %0x", __FILE__, __FUNCTION__,__LINE__, m_pTcpSocket);
    }
}
