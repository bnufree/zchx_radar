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
ZCHXRadarDataServer::ZCHXRadarDataServer(ZCHX::Messages::RadarConfig* cfg, QObject *parent)
    : QObject(parent),
      mRadarPowerStatus(0),
      mRadarConfig(cfg),
      mDataRecvThread(Q_NULLPTR),
      mVideoWorker(Q_NULLPTR),
      mHeartObj(Q_NULLPTR),
      mCtrlObj(Q_NULLPTR),
      mReportObj(Q_NULLPTR)

{
    qRegisterMetaType<RadarStatus>("const RadarStatus&");
    qRegisterMetaType<QList<RadarStatus>>("const QList<RadarStatus>&");

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

    //初始化心跳
    mHeartObj = new zchxRadarHeartWorker(mRadarConfig, new QThread, this);
    //初始化雷达控制
    if(mHeartObj->isFine())
    {
        mCtrlObj = new zchxRadarCtrlWorker(mHeartObj->socket(), mRadarConfig, this);
    }
    //初始化雷达参数报告
    mReportObj = new zchxRadarReportWorker(mRadarConfig, new QThread, this);


    connect(this,SIGNAL(startProcessSignal()),this,SLOT(startProcessSlot()));
    moveToThread(&m_workThread);
    m_workThread.start();

    //独立线程接收数据
    mDataRecvThread = new MultiCastDataRecvThread(m_sVideoIP, m_uVideoPort, m_sRadarVideoType, m_uCellNum, m_uLineNum, m_uHeading, this);
    mVideoWorker = new VideoDataProcessWorker(new RadarConfig(0));
    connect(mDataRecvThread, SIGNAL(analysisRadar(QByteArray,QString,int,int,int)), mVideoWorker, SLOT(slotRecvVideoRawData(QByteArray)));
    //connect(mVideoWorker, SIGNAL(signalSendTrackPoint(QList<TrackPoint>)), this, SLOT(slotRecvTrackPoint(QList<TrackPoint>)));
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


