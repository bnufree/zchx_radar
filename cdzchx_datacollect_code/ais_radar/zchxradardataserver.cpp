#include "zchxradardataserver.h"
#include <QDebug>
#include <QTime>
#include <QDateTime>
#include <QApplication>
#include "../profiles.h"
#include "DataBlock.h"
#include "AsterixData.h"
#include <math.h>
#include <zlib.h>
#include <QBuffer>
#include <QPushButton>
#include <QMetaProperty>
#include "BR24.hpp"
#include <QLibrary>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPolygonF>
#include <QThread>
#include "side_car_parse/Messages/RadarConfig.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

//extern "C"

//{

//#include "ctrl.h"

//}
//////////////////////////////////////////////////////////////////////////////////

//typedef int(*FUN3)(struct SAzmData* psScan, int* pSit);
//extern  FUN3 Tracking_Fun3 = NULL;

//////////////////////////////////////////////////////////////////////////////////////
//static uint8_t BR24MARK[] = {0x00, 0x44, 0x0d, 0x0e};
ZCHXRadarDataServer::ZCHXRadarDataServer(int uSourceID, QObject *parent)
    : QObject(parent),
      mRadarPowerStatus(0),
      m_sReportIP(""),
      m_uReportPort(0),
      m_uSourceID(uSourceID),
      mDataRecvThread(Q_NULLPTR),
      mVideoWorker(Q_NULLPTR)

{
    qRegisterMetaType<RadarStatus>("const RadarStatus&");
    qRegisterMetaType<QList<RadarStatus>>("const QList<RadarStatus>&");
    //从配置文件读取
    QString str_radar = QString("Radar_%1").arg(m_uSourceID);

    m_sTrackIP = Utils::Profiles::instance()->value(str_radar,"Track_IP").toString();
    m_uTrackPort = Utils::Profiles::instance()->value(str_radar,"Track_Port").toInt();
    m_sVideoIP = Utils::Profiles::instance()->value(str_radar,"Video_IP").toString();
    m_uVideoPort = Utils::Profiles::instance()->value(str_radar,"Video_Port").toInt();
    m_sReportIP = Utils::Profiles::instance()->value(str_radar,"Report_IP").toString();
    m_uReportPort = Utils::Profiles::instance()->value(str_radar,"Report_Port").toInt();
    m_bReportOpen = Utils::Profiles::instance()->value(str_radar,"Report_Open").toBool();

    m_sRadarVideoType = Utils::Profiles::instance()->value(str_radar,"Video_Type").toString();
    m_uCellNum = Utils::Profiles::instance()->value(str_radar,"Cell_Num").toInt();
    m_uLineNum = Utils::Profiles::instance()->value(str_radar,"Line_Num").toInt();
    m_uHeading = Utils::Profiles::instance()->value(str_radar,"Heading").toInt();

    m_uHeartTime = Utils::Profiles::instance()->value(str_radar,"Heart_Time").toInt();
    m_sHeartIP = Utils::Profiles::instance()->value(str_radar,"Heart_IP").toString();
    m_uHeartPort = Utils::Profiles::instance()->value(str_radar,"Heart_Port").toInt();
    m_sOptRadarIP = Utils::Profiles::instance()->value(str_radar,"RadarOpt_IP").toString();
    m_uOptRadarPort = Utils::Profiles::instance()->value(str_radar,"RadarOpt_Port").toInt();

    //读取雷达控制的设定值
    parseRadarControlSetting(POWER);
    parseRadarControlSetting(SCAN_SPEED);
    parseRadarControlSetting(ANTENNA_HEIGHT);
    parseRadarControlSetting(BEARING_ALIGNMENT);
    parseRadarControlSetting(RANG);
    parseRadarControlSetting(GAIN);
    parseRadarControlSetting(SEA_CLUTTER);
    parseRadarControlSetting(RAIN_CLUTTER);
    parseRadarControlSetting(NOISE_REJECTION);
    parseRadarControlSetting(SIDE_LOBE_SUPPRESSION);
    parseRadarControlSetting(INTERFERENCE_REJECTION);
    parseRadarControlSetting(LOCAL_INTERFERENCE_REJECTION);
    parseRadarControlSetting(TARGET_EXPANSION);
    parseRadarControlSetting(TARGET_BOOST);
    parseRadarControlSetting(TARGET_SEPARATION);

    m_pHeartSocket = NULL;
    //心跳
    m_pHeartSocket = new QUdpSocket();

    if(!m_pHeartSocket->bind(QHostAddress::AnyIPv4,m_uHeartPort,QAbstractSocket::ShareAddress))
        qDebug()<<"bind heart failed--";

    m_pHeartSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);//禁止本机接收
    if(!m_pHeartSocket->joinMulticastGroup(QHostAddress(m_sHeartIP)))
        qDebug()<<"joinMuticastGroup heart failed--";

    qDebug()<<"ZCHXRadarDataServer thread id :"<<QThread::currentThreadId();
    connect(this,SIGNAL(startProcessSignal()),this,SLOT(startProcessSlot()));
    moveToThread(&m_workThread);
    m_workThread.start();

    //独立线程接收数据
    mDataRecvThread = new MultiCastDataRecvThread(m_sVideoIP, m_uVideoPort, m_sRadarVideoType, m_uCellNum, m_uLineNum, m_uHeading, this);
    mVideoWorker = new VideoDataProcessWorker(new RadarConfig(0));
    connect(mDataRecvThread, SIGNAL(analysisRadar(QByteArray,QString,int,int,int)), mVideoWorker, SLOT(slotRecvVideoRawData(QByteArray)));
    connect(mVideoWorker, SIGNAL(signalSendTrackPoint(QList<TrackPoint>)), this, SLOT(slotRecvTrackPoint(QList<TrackPoint>)));
}

ZCHXRadarDataServer::~ZCHXRadarDataServer()
{

    if(m_workThread.isRunning())
    {
        m_workThread.quit();
    }
    m_workThread.terminate();

    if(m_pUdpTrackSocket)
    {
        delete m_pUdpTrackSocket;
        m_pUdpTrackSocket = NULL;
    }
    if(m_pUdpVideoSocket)
    {
        delete m_pUdpVideoSocket;
        m_pUdpVideoSocket = NULL;
    }
    if(m_pHeartTimer)
    {
        m_pHeartTimer->stop();
        delete m_pHeartTimer;
        m_pHeartTimer = NULL;
    }
    if(m_pHeartSocket)
    {
        delete m_pHeartSocket;
        m_pHeartSocket  = NULL;
    }

}

void ZCHXRadarDataServer::openRadar()//打开雷达
{

    //m_asio_server->send_peer_message(radarSourceId, (char *)(Down + 0), 3);
    //m_asio_server->send_peer_message(radarSourceId, (char *)(Down + 3), 3);
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("radar ip = %1,port = %2").arg(m_sHeartIP).arg(m_uHeartPort);
    emit signalSendRecvedContent(utc,"OPEN_RADAR",sContent);
    //qDebug()<<"close radar"<<m_sHeartIP<<m_sHeartIP;
    QHostAddress objHost(m_sHeartIP);
    unsigned char Boot[6] = { 0x00, 0xc1, 0x01, 0x01, 0xc1, 0x01 }; //00 c1 01/01 c1 01
//    qint64 s1 = m_pHeartSocket->writeDatagram((char *)(Boot + 0),3,objHost,m_uHeartPort);
//    qint64 s2 = m_pHeartSocket->writeDatagram((char *)(Boot + 3),3,objHost,m_uHeartPort);
//    cout<<"s1"<<s1<<"s2"<<s2;

    QByteArray line;
    line.resize(3);
    line[0] = 0x00;
    line[1] = 0xc1;
    line[2] = 0x01;
    cout<<line;
    m_pHeartSocket->writeDatagram(line,objHost,m_uHeartPort);
    line[0] = 0x01;
    line[1] = 0xc1;
    line[2] = 0x01;
    m_pHeartSocket->writeDatagram(line,objHost,m_uHeartPort);


}

void ZCHXRadarDataServer::closeRadar()//关闭雷达
{
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("radar ip = %1,port = %2").arg(m_sHeartIP).arg(m_uHeartPort);
    emit signalSendRecvedContent(utc,"CLOSE_RADAR",sContent);
    //qDebug()<<"open radar"<<m_sOptRadarIP<<m_uOptRadarPort;
    QHostAddress objHost(m_sHeartIP);
    unsigned char Down[6] = { 0x00, 0xc1, 0x01, 0x01, 0xc1, 0x00 }; //00 c1 01  /01 c1 00
    m_pHeartSocket->writeDatagram((char *)(Down + 0),3,objHost,m_uHeartPort);
    m_pHeartSocket->writeDatagram((char *)(Down + 3),3,objHost,m_uHeartPort);

    //m_asio_server->send_peer_message(radarSourceId, (char *)(Boot + 0), 3);
    //m_asio_server->send_peer_message(radarSourceId, (char *)(Boot + 3), 3);

}

void ZCHXRadarDataServer::startProcessSlot()
{
    qDebug()<<"startProcessSlot thread id :"<<QThread::currentThreadId();
    init();
    //readRadarLimitFormat();
}


void ZCHXRadarDataServer::displayUdpTrackError(QAbstractSocket::SocketError error)
{

    if(m_pUdpTrackSocket == NULL)
        return;

    qDebug()<<"ZCHXRadarDataUDPServer:"<<m_pUdpTrackSocket->errorString();
}


void ZCHXRadarDataServer::updateTrackUdpProgress()
{
    if(m_pUdpTrackSocket == NULL)
        return;
    QString str_radar = QString("Radar_%1").arg(m_uSourceID);
    QString sRadarType = Utils::Profiles::instance()->value(str_radar,"Track_Type").toString();
    QByteArray datagram;
    // 让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
    datagram.resize(m_pUdpTrackSocket->pendingDatagramSize());//pendingDatagramSize() 当前数据包大小
    // 接收数据报，将其存放到datagram中
    m_pUdpTrackSocket->readDatagram(datagram.data(), datagram.size());//readDatagram将不大于指定长度的数据保存到datagram.data()
    //qDebug()<<"udp sRadar TrackData size:"<<datagram.size();
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar track data,size = %1").arg(datagram.size());
    emit signalSendRecvedContent(utc,"TRACK_RECEIVE",sContent);
    cout<<"updateTrackUdpProgress()";
    analysisRadar(datagram,sRadarType);
}

void ZCHXRadarDataServer::heartProcessSlot()
{

    QHostAddress objHost(m_sHeartIP);
//    unsigned char heartbeat_[8] = { 0Xa0, 0xc1, 0x03, 0xc2, 0x04, 0xc2,
//                                    0x05, 0xc2 };
//    for (int i = 0; i < 4; i++)
//    {
//        unsigned char sendC[2] = { 0 };
//        //cout<<"sendC"<<sendC<<"(char *)sendC"<<(char *)sendC;
//        memcpy(sendC, heartbeat_ + i * 2, 2);
//        //cout<<"sendC"<<sendC<<(char *)sendC<<i<<heartbeat_ + i * 2 <<heartbeat_[i*2];
//        m_pHeartSocket->writeDatagram((char *)sendC,2,objHost,m_uHeartPort);
//        //m_asio_server->send_peer_message(radarSourceId, (char *)sendC, 2);
//    }
    //修改发送数据方式
    QByteArray line;
    line.resize(2);
    line[0] = 0Xa0;
    line[1] = 0xc1;
    m_pHeartSocket->writeDatagram(line,objHost,m_uHeartPort);
    line[0] = 0x03;
    line[1] = 0xc2;
    m_pHeartSocket->writeDatagram(line,objHost,m_uHeartPort);
    line[0] = 0x04;
    line[1] = 0xc2;
    m_pHeartSocket->writeDatagram(line,objHost,m_uHeartPort);
    line[0] = 0x05;
    line[1] = 0xc2;
    m_pHeartSocket->writeDatagram(line,objHost,m_uHeartPort);


}

void ZCHXRadarDataServer::init()
{

    m_pUdpTrackSocket = NULL;
    m_pUdpVideoSocket = NULL;

    m_pHeartTimer = new QTimer();
    connect(m_pHeartTimer,SIGNAL(timeout()),this,SLOT(heartProcessSlot()));
    m_pHeartTimer->start(m_uHeartTime*1000);//开始心跳
    cout<<"开始心跳";

    m_pUdpTrackSocket = new QUdpSocket();
    //udp接收(组播形式)
    //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
    //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
    if(!m_pUdpTrackSocket->bind(QHostAddress::AnyIPv4,m_uTrackPort,QAbstractSocket::ShareAddress))
        qDebug()<<"bind track failed--";

    if(!m_pUdpTrackSocket->joinMulticastGroup(QHostAddress(m_sTrackIP)))
        qDebug()<<"joinMuticastGroup track failed--";
    qDebug()<<"joinMuticastGroup track succeed-- ";
    connect(m_pUdpTrackSocket, SIGNAL(readyRead()),this, SLOT(updateTrackUdpProgress()));
    connect(m_pUdpTrackSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpTrackError(QAbstractSocket::SocketError)));


    //改为独立进程
    if(mDataRecvThread)  mDataRecvThread->start(); //以线程1的方式接收数据
    //u2_workThread->start(); //以线程2的方式接收数据
//    m_pUdpVideoSocket = new QUdpSocket();
//    //udp接收(组播形式)
//    //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
//    //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
//    if(!m_pUdpVideoSocket->bind(QHostAddress::AnyIPv4,m_uVideoPort,QAbstractSocket::ShareAddress))
//        qDebug()<<"bind video failed--";

//    if(!m_pUdpVideoSocket->joinMulticastGroup(QHostAddress(m_sVideoIP)))
//        qDebug()<<"joinMuticastGroup video failed--";
//    qDebug()<<"joinMuticastGroup video succeed-- ";

//    connect(m_pUdpVideoSocket, SIGNAL(readyRead()),this, SLOT(updateVideoUdpProgress()));
//    connect(m_pUdpVideoSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpVideoError(QAbstractSocket::SocketError)));

    //接收雷达参数信息
    if(m_bReportOpen && m_sReportIP.trimmed().length() > 0 && m_uReportPort > 0)
    {
        m_pUdpReportSocket = new QUdpSocket();
        //udp接收(组播形式)
        //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
        //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
        if(!m_pUdpReportSocket->bind(QHostAddress::AnyIPv4,m_uReportPort,QAbstractSocket::ShareAddress))
            qDebug()<<"bind track failed--";

        if(!m_pUdpReportSocket->joinMulticastGroup(QHostAddress(m_sReportIP)))
            qDebug()<<"joinMuticastGroup report failed--";
        qDebug()<<"joinMuticastGroup report succeed-- ";
        connect(m_pUdpReportSocket, SIGNAL(readyRead()),this, SLOT(updateReportUdpProgress()));
        connect(m_pUdpReportSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpReportError(QAbstractSocket::SocketError)));
    }

}

void ZCHXRadarDataServer::analysisRadar(const QByteArray &sRadarData, const QString &sRadarType, int uLineNum, int uCellNum, int uHeading)
{

    if(sRadarType == "zchx240")//小雷达
    {
        emit  analysisLowranceRadar(sRadarData,uLineNum,uCellNum,uHeading);

    }
    if(sRadarType == "cat010"||sRadarType == "cat240")
    {
        emit analysisCatRadar(sRadarData,uLineNum,uCellNum,uHeading,sRadarType);
    }

}

void ZCHXRadarDataServer::setControlValue(INFOTYPE infotype, int value)
{
    cout<<"上传雷达状态值!!!!!setControlValue";
    emit signalSendRecvedContent(QDateTime::currentMSecsSinceEpoch(), "RadarControl", QString("%1:%2").arg(RadarStatus::getTypeString(infotype)).arg(value));

    //检查当前值是否存在
    if(mRadarStatusMap.contains(infotype))
    {
        RadarStatus& sts = mRadarStatusMap[infotype];
        if(sts.value != value)
        {
            QHostAddress objHost(m_sHeartIP);
            switch (infotype) {
            case INFOTYPE::POWER:
            {
                if(value >= sts.min && value <= sts.max)
                {
                    if(value == 0)
                    {
                        closeRadar();
                    } else
                    {
                        openRadar();
                    }
                }
                break;
            }
            case INFOTYPE::SCAN_SPEED:
            {
                if(value >= sts.min && value <= sts.max)
                {
                    UINT8 cmd[] = {0x0f, 0xc1, (UINT8)value  }; //00 c1 01  /01 c1 00
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }

                break;
            }
            case INFOTYPE::ANTENNA_HEIGHT:
            {
                if(value >= sts.min && value <= sts.max)
                {
                    int v = value * 1000;  // radar wants millimeters, not meters
                    int v1 = v / 256;
                    int v2 = v & 255;
                    UINT8 cmd[10] = { 0x30, 0xc1, 0x01, 0, 0, 0, (UINT8)v2, (UINT8)v1, 0, 0 };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }

                break;
            }
            case INFOTYPE::BEARING_ALIGNMENT:
            {
                if(value >= sts.min && value <= sts.max)
                {
                    if (value < 0)  value += 360;
                    int v = value * 10;
                    int v1 = v / 256;
                    int v2 = v & 255;
                    UINT8 cmd[4] = { 0x05, 0xc1, (UINT8)v2, (UINT8)v1 };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }

                break;
            }
            case INFOTYPE::RANG:
            {
                if(value >= 50 && value <= 72704)
                {
                    unsigned int decimeters = (unsigned int)value * 10;
                    UINT8 cmd[] = { 0x03,0xc1,
                        (UINT8)((decimeters >> 0) & 0XFFL),
                        (UINT8)((decimeters >> 8) & 0XFFL),
                        (UINT8)((decimeters >> 16) & 0XFFL),
                    };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }

                break;
            }
            case INFOTYPE::GAIN:
            {
                if(value >= sts.min && value <= sts.max)
                {
                    if(value < 0)
                    {
                        // 自动增益
                        UINT8 cmd[] = {0x06, 0xc1, 0, 0, 0, 0, 0x01, 0, 0, 0, 0xad };
                        m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                    } else if (value >= 0) {
                        // Manual Gain
                        int v = (value + 1) * 255 / 100;
                        if (v > 255) v = 255;
                        UINT8 cmd[] = { 0x06, 0xc1, 0, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
                        m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                    }
                }

                break;
            }
            case INFOTYPE::SEA_CLUTTER:
            {
                if(value >= sts.min && value <= sts.max)
                {
                    if(value < 0)
                    {
                        // 自动
                        UINT8 cmd[11] = { 0x06, 0xc1, 0x02, 0, 0, 0, 0x01, 0, 0, 0, 0xd3 };
                        m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                    } else if (value >= 0) {
                        // Manual
                        int v = (value + 1) * 255 / 100;
                        if (v > 255) v = 255;
                        UINT8 cmd[] = { 0x06, 0xc1, 0x02, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
                        m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                    }
                }

                break;
            }

            case INFOTYPE::RAIN_CLUTTER: // 8
            {
                if (value >= sts.min &&  value <= sts.max)
                {
                    int v = (value + 1) * 255 / 100;
                    if (v > 255) v = 255;
                    UINT8 cmd[] = { 0x06, 0xc1, 0x04, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                break;
            }
            case INFOTYPE::NOISE_REJECTION: // 9
            {
                if (value >= sts.min &&  value <= sts.max)
                {
                    UINT8 cmd[] = { 0x21, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                break;
            }
            case INFOTYPE::SIDE_LOBE_SUPPRESSION: // 10
            {
                if (value >= sts.min &&  value <= sts.max)
                {
                    if (value < 0) {
                        //自动
                        UINT8 cmd[] = {0x06, 0xc1, 0x05, 0, 0, 0, 0x01, 0, 0, 0, 0xc0 };
                        m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                    }else {
                        int v = (value + 1) * 255 / 100;
                        if (v > 255) v = 255;
                        UINT8 cmd[] = { 0x6, 0xc1, 0x05, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
                        m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                    }
                }
                break;
            }
            case INFOTYPE::INTERFERENCE_REJECTION: // 11
            {
                if (value >= sts.min &&  value <= sts.max)
                {
                    UINT8 cmd[] = { 0x08, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                break;
            }
            case INFOTYPE::LOCAL_INTERFERENCE_REJECTION:  // 12
            {

                if (value >= sts.min &&  value <= sts.max)
                {
                    if (value < 0) value = 0;
                    if (value > 3) value = 3;
                    UINT8 cmd[] = { 0x0e, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                break;
            }
            case INFOTYPE::TARGET_EXPANSION: // 13
            {
                if (value >= sts.min &&  value <= sts.max){
                    UINT8 cmd[] = { 0x09, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                break;
            }
                break;
            case INFOTYPE::TARGET_BOOST: // 14
            {
                if (value >= sts.min &&  value <= sts.max){
                    UINT8 cmd[] = { 0x0a, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                break;
            }
            case INFOTYPE::TARGET_SEPARATION: // 15
            {

                if (value >= sts.min &&  value <= sts.max){
                    UINT8 cmd[] = { 0x22, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                break;
            }
            default:
                break;
            }
        }
    }

}

void ZCHXRadarDataServer::updateValue(INFOTYPE controlType, int value)
{
    //检查值的范围
    if(controlType <= INFOTYPE::UNKNOWN ||  controlType > INFOTYPE::RESVERED)
    {
        return;
    }
    //cout<<"mRadarStatusMap容器大小--------------"<<mRadarStatusMap.size();
    if(!mRadarStatusMap.contains(controlType))
    {
        mRadarStatusMap[controlType] = RadarStatus(controlType);
    }
    RadarStatus &sts = mRadarStatusMap[controlType];
    //cout<<"sts.value"<<sts.value<<"value"<<value;
    if(sts.value != value)
    {

        sts.value = value;
        emit signalRadarStatusChanged(mRadarStatusMap.values(), m_uSourceID);
    }

}

void ZCHXRadarDataServer::displayUdpReportError(QAbstractSocket::SocketError error)
{
    if(m_pUdpReportSocket == NULL)
        return;

    qDebug()<<"ZCHXRadarDataUDPServer:"<<m_pUdpReportSocket->errorString();
}

void ZCHXRadarDataServer::updateReportUdpProgress()
{
    if(m_pUdpReportSocket == NULL)
        return;
    QByteArray datagram;
    // 让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
    datagram.resize(m_pUdpReportSocket->pendingDatagramSize());//pendingDatagramSize() 当前数据包大小
    // 接收数据报，将其存放到datagram中
    m_pUdpReportSocket->readDatagram(datagram.data(), datagram.size());//readDatagram将不大于指定长度的数据保存到datagram.data()
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar report data,size = %1").arg(datagram.size());
    emit signalSendRecvedContent(utc,"REPORT_RECEIVE",sContent);
//    cout<<"接收到雷达状态数据啦"<<datagram;
//    cout<<"----------------";
    ProcessReport(datagram, datagram.size());
}

void ZCHXRadarDataServer::ProcessReport(const QByteArray& bytes, size_t len)
{
    //cout<<"状态-------------"<<QString::fromStdString(bytes.toHex().data());

    emit signalSendRecvedContent(QDateTime::currentMSecsSinceEpoch(), "RadarRpoert", QString::fromStdString(bytes.toHex().data()));
    if(len < 3 ) return;
    unsigned char val = bytes[1];
    //cout<<val;
    if (val == 0xC4)
    {
        //cout<<"解析雷达数据-------------------------------------------";
        switch ((len << 8) + bytes[0])
        {
        case (18 << 8) + 0x01:
        {
            RadarReport_01C4_18 *s = (RadarReport_01C4_18 *)bytes.data();
            if (mRadarPowerStatus != s->radar_status)
            {
                mRadarPowerStatus = s->radar_status;
                switch (bytes[2])
                {
                case 0x01:
                    cout<<"待机状态";
                    //ZCHXLOG_DEBUG("reports status RADAR_STANDBY");
                    updateValue(INFOTYPE::POWER,0);
                    break;
                case 0x02:
                    cout<<"传输状态";
                    //ZCHXLOG_DEBUG("reports status TRANSMIT");
                    updateValue(INFOTYPE::POWER,1);
                    break;
                case 0x05:
                    cout<<"唤醒状态";
                    //ZCHXLOG_DEBUG("reports status WAKING UP");
                    break;
                default:
                    break;
                }
            }
            break;
        }
        case (99 << 8) + 0x02: // length 99, 02 C4,contains gain,rain,interference rejection,sea
            //target_boost, target_expansion,range
        {
            //cout<<"进来了_2 02，C4，包含增益，雨，干扰抑制，海洋,target_boost, target_expansion,range";
            RadarReport_02C4_99 *s = (RadarReport_02C4_99 *)bytes.data();
            //gain
            if (s->field8 == 1)        // 1 for auto
                updateValue(INFOTYPE::GAIN,-1);
            else
                updateValue(INFOTYPE::GAIN, s->gain * 100 / 255);
            //sea
            if (s->field13 == 0x01)
                updateValue(INFOTYPE::SEA_CLUTTER,-1);  // auto sea
            else
                updateValue(INFOTYPE::SEA_CLUTTER,s->sea * 100 / 255);
            //rain
            updateValue(INFOTYPE::RAIN_CLUTTER, s->rain * 100 / 255);
            //target boost
            updateValue(INFOTYPE::TARGET_BOOST, s->target_boost);
            //s->interference rejection
            updateValue(INFOTYPE::INTERFERENCE_REJECTION, s->interference_rejection);
            //target expansion
            updateValue(INFOTYPE::TARGET_EXPANSION, s->target_expansion);
            //range
            updateValue(INFOTYPE::RANG, s->range / 10);
            break;
        }
        case (129 << 8) + 0x03: // 129 bytes starting with 03 C4
        {
            //cout<<"进来了_3 129 bytes starting with 03 C4";
            RadarReport_03C4_129 *s = (RadarReport_03C4_129 *)bytes.data();
            switch (s->radar_type) {
            case 0x0f:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_BR24");
                break;
            case 0x08:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_3G");
                break;
            case 0x01:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_4G");
                break;
            default:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: " << s->radar_type);
                return ;
            }
            break;
        }
        case (66 << 8) + 0x04: // 66 bytes starting with 04 C4,contains bearing alignment,antenna height
        {
            //cout<<"进来了_4 从04 C4开始的66个字节，包含轴承对齐，天线高度";
            RadarReport_04C4_66 *data = (RadarReport_04C4_66 *)bytes.data();
            // bearing alignment
            int ba = (int)data->bearing_alignment / 10;
            if (ba > 180) ba = ba - 360;
            updateValue(INFOTYPE::BEARING_ALIGNMENT, ba);
            // antenna height
            updateValue(INFOTYPE::ANTENNA_HEIGHT, data->antenna_height / 1000);
            break;
        }
        case (564 << 8) + 0x05:
        {
            //cout<<"进来了_5 内容未知，但我们知道BR24雷达发送这个";
            // Content unknown, but we know that BR24 radomes send this
            //ZCHXLOG_DEBUG("Navico BR24: msg");
            break;
        }
        case (18 << 8) + 0x08: // length 18, 08 C4,
            //contains scan speed, noise rejection and target_separation and sidelobe suppression,local_interference_rejection
        {
            //cout<<"进来了_6 包含扫描速度，噪声抑制和目标分离和侧面抑制，局部干扰抑制";
            RadarReport_08C4_18 *s08 = (RadarReport_08C4_18 *)bytes.data();
            updateValue(INFOTYPE::SCAN_SPEED, s08->scan_speed);
            updateValue(INFOTYPE::NOISE_REJECTION, s08->noise_rejection);
            updateValue(INFOTYPE::TARGET_SEPARATION, s08->target_sep);
            if (s08->sls_auto == 1)
                updateValue(INFOTYPE::SIDE_LOBE_SUPPRESSION,-1);
            else
                updateValue(INFOTYPE::SIDE_LOBE_SUPPRESSION, s08->side_lobe_suppression * 100 / 255);
            updateValue(INFOTYPE::LOCAL_INTERFERENCE_REJECTION, s08->local_interference_rejection);
            break;
        }
        default:
            break;
        }
    }
    else if(bytes[1] == 0xF5){
        cout<<"不知道是什么东西";
        //ZCHXLOG_DEBUG("unknown: buf[1]=0xF5");
    }
    return ;
}

int ZCHXRadarDataServer::sourceID() const
{
    return m_uSourceID;
}

void ZCHXRadarDataServer::parseRadarControlSetting(INFOTYPE infotype)
{
    QString str_radar_cmd = QString("Radar_Command_%1").arg(m_uSourceID);
    QStringList list = Utils::Profiles::instance()->value(str_radar_cmd,\
                                                           RadarStatus::getTypeString(infotype,STR_MODE_ENG)\
                                                          ).toStringList();
    if(list.length() < 2) return;
    mRadarStatusMap[infotype] = RadarStatus(infotype, list[0].toInt(), list[1].toInt());
}

QByteArray ZCHXRadarDataServer::HexStringToByteArray(QString HexString)
{
    bool ok;
    QByteArray ret;
    HexString = HexString.trimmed();
    HexString = HexString.simplified();
    QStringList sl = HexString.split(" ");

    foreach (QString s, sl) {
        if(!s.isEmpty()) {
            char c = s.toInt(&ok,16)&0xFF;
            qDebug()<<"c-------"<<c;
            if(ok){
                static int i = 0;
                ret[i] = c;
                i++;
                //ret.append(c);
            }else{
                qDebug()<<"非法的16进制字符："<<s;
            }
        }
    }
    return ret;
}

QString ZCHXRadarDataServer::ByteArrayToHexString(QByteArray &ba)
{
    QDataStream out(&ba,QIODevice::ReadWrite);   //将str的数据 读到out里面去
    QString buf;
    while(!out.atEnd())
    {
        qint8 outChar = 0;
        out >> outChar;   //每次一个字节的填充到 outchar
        QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0')).toUpper() + QString(" ");   //2 字符宽度
        buf += str;
    }
    return buf;
}

void ZCHXRadarDataServer::slotRecvTrackPoint(const QList<TrackPoint> &list)
{

}


