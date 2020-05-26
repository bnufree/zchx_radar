#include "zchxlowranceradardataserver.h"
//#include <QDebug>
#include <QTime>
#include <QDateTime>
#include <QApplication>
#include "../profiles.h"
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
#include <QMessageBox>
#include <qdir.h>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QList>
#include <QNetworkAddressEntry>
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
ZCHXLowranceRadarDataServer::ZCHXLowranceRadarDataServer(int source, QObject *parent)
    : QObject(parent),
      mRadarPowerStatus(0),
      m_sReportIP(""),
      m_uReportPort(0),
      m_uSourceID(source)

{
    qRegisterMetaType<RadarStatus>("const RadarStatus&");
    qRegisterMetaType<QList<RadarStatus>>("const QList<RadarStatus>&");
    if(mIPV4List.isEmpty()) mIPV4List = getAllIpv4List();

    prt = false;//初始化打印标志
    tcp_flag = false;
    //从配置文件读取
    QString str_radar = QString("Radar_%1").arg(m_uSourceID);


    uTcpPort = Utils::Profiles::instance()->value(str_radar,"Tcp_Port").toInt();
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
    mac_ip = Utils::Profiles::instance()->value("Radar_Control","Mac_IP").toString();

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
    QStringList netList;
    foreach (QNetworkInterface netInterface, QNetworkInterface::allInterfaces())
    {
        //qDebug() << "设备名:" << netInterface.name()<<"是否激活:"<<netInterface.flags();
        QList<QNetworkAddressEntry> entryList = netInterface.addressEntries();

        //遍历每一个IP地址(每个包含一个IP地址，一个子网掩码和一个广播地址)
        foreach(QNetworkAddressEntry entry, entryList)
        {
            //IP地址
            if(entry.ip().toString().indexOf(".") > 0)
            {
                 qDebug() << "IP地址:" << entry.ip().toString();
                 netList.append(entry.ip().toString());
            }

        }
    }
    foreach (QString ip, netList) {
        QString ip_str = Utils::Profiles::instance()->value("Radar_Control","Mac_IP").toString();
        if(!m_pHeartSocket->bind(QHostAddress(ip_str) ,m_uHeartPort,QAbstractSocket::ShareAddress))//遍历绑定
        {
            //QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("bind heart failed--"));// cout<<"bind heart failed--";
        }
        else
        {
            //QMessageBox::information(0,QStringLiteral("信息"),ip_str);
            cout<<"bind heart succeed--"<<ip;
            break;
        }
    }


    m_pHeartSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);//禁止本机接收

    if(!m_pHeartSocket->joinMulticastGroup(QHostAddress(m_sHeartIP)))
        cout<<"joinMuticastGroup heart failed--";
        //QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("joinMuticastGroup heart failed--"));//cout<<"joinMuticastGroup heart failed--";
    else
        cout<<"joinMuticastGroup heart succeed--";

    cout<<"ZCHXLowranceRadarDataServer thread id :"<<QThread::currentThreadId();
    connect(this,SIGNAL(startProcessSignal()),this,SLOT(startProcessSlot()));
    moveToThread(&m_workThread);
    m_workThread.start();

    //独立线程接收数据
//    u_workThread = new updatevideoudpthread(m_sVideoIP, m_uVideoPort, m_sRadarVideoType, m_uCellNum, m_uLineNum, m_uHeading,0);

//    connect(u_workThread, SIGNAL(u_signalSendRecvedContent(qint64,QString,QString)),
//            this, SIGNAL(signalSendRecvedContent(qint64,QString,QString)) );
//    connect(u_workThread, SIGNAL(analysisRadar(QByteArray,QString,int,int,int)),
//            this, SLOT(analysisRadar(QByteArray,QString,int,int,int)));
//    connect(u_workThread, SIGNAL(joinGropsignal(QString)),
//            this, SIGNAL(joinGropsignal(QString)));


    connect(this, SIGNAL(prtVideoSignal_1(bool)),this,SLOT(prtVideoSlot(bool)));


}

ZCHXLowranceRadarDataServer::~ZCHXLowranceRadarDataServer()
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
    if(m_pUdpReportSocket)
    {
        delete m_pHeartSocket;
        m_pHeartSocket  = NULL;
    }
    /*if(u_workThread->isRunning())
    {
        u_workThread->quit();
    }
    u_workThread->terminate();
    if(mSocket)
    {
        delete mSocket;
        mSocket  = NULL;
    }*/
    if(mServer)
    {
        delete mServer;
        mServer  = NULL;
    }
    cout<<"~ZCHXLowranceRadarDataServer()";
}

void ZCHXLowranceRadarDataServer::openRadar()//打开雷达
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
    cout<<line<<"打开雷达";
    m_pHeartSocket->writeDatagram(line,objHost,m_uHeartPort);
    line[0] = 0x01;
    line[1] = 0xc1;
    line[2] = 0x01;
    m_pHeartSocket->writeDatagram(line,objHost,m_uHeartPort);


}

void ZCHXLowranceRadarDataServer::closeRadar()//关闭雷达
{
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("radar ip = %1,port = %2").arg(m_sHeartIP).arg(m_uHeartPort);
    emit signalSendRecvedContent(utc,"CLOSE_RADAR",sContent);
    qDebug()<<"close radar"<<m_sOptRadarIP<<m_uOptRadarPort;
    QHostAddress objHost(m_sHeartIP);
    unsigned char Down[6] = { 0x00, 0xc1, 0x01, 0x01, 0xc1, 0x00 }; //00 c1 01  /01 c1 00
    m_pHeartSocket->writeDatagram((char *)(Down + 0),3,objHost,m_uHeartPort);
    m_pHeartSocket->writeDatagram((char *)(Down + 3),3,objHost,m_uHeartPort);

    //m_asio_server->send_peer_message(radarSourceId, (char *)(Boot + 0), 3);
    //m_asio_server->send_peer_message(radarSourceId, (char *)(Boot + 3), 3);

}

void ZCHXLowranceRadarDataServer::startProcessSlot()
{
    qDebug()<<"startProcessSlot thread id :"<<QThread::currentThreadId();
    cout<<"RadarDataServer!初始化开始-------------------------------------------";
    init();
    openRadar();
    //readRadarLimitFormat();
}

void ZCHXLowranceRadarDataServer::displayUdpVideoError(QAbstractSocket::SocketError error)
{
    if(m_pUdpVideoSocket == NULL)
    {
        return;
    }
    //qDebug()<<"ZCHXRadarDataUDPServer:"<<m_pUdpVideoSocket->errorString();

}

void ZCHXLowranceRadarDataServer::updateVideoUdpProgress(const QByteArray& data)
{
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar video data,size of bytes = %1").arg(data.size());
    //emit signalSendRecvedContent(utc,"VIDEO_RECEIVE",sContent);

    analysisRadar(data,m_sRadarVideoType,m_uLineNum,m_uCellNum,m_uHeading);
//    //通过TCP转发收到的原始回波数据
//    if(tcp_flag)
//    {
//        mSocket->write(data);
//    }
}

void ZCHXLowranceRadarDataServer::displayUdpTrackError(QAbstractSocket::SocketError error)
{

    if(m_pUdpTrackSocket == NULL)
        return;
    cout<<"目标套接字发送错误";
    qDebug()<<"ZCHXRadarDataUDPServer:"<<m_pUdpTrackSocket->errorString();
}

void ZCHXLowranceRadarDataServer::updateTrackUdpProgress()
{
    //cout<<"------------------updateTrackUdpProgress----------------------";
    if(m_pUdpTrackSocket == NULL)
        return;
    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    float time_of_day = startDateTime.msecsTo(curDateTime);
    if(mTod == 0)
    {
        mTod = time_of_day;
    }
    //cout<<"发送回波数据";
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
    //cout<<"updateTrackUdpProgress()";
    analysisRadar(datagram,sRadarType);

    //cout<<"------------------updateTrackUdpProgress----------------------";
}

void ZCHXLowranceRadarDataServer::heartProcessSlot()
{
    //openRadar();
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

void ZCHXLowranceRadarDataServer::init()
{

    m_pUdpTrackSocket = NULL;

    m_pHeartTimer = new QTimer();
    connect(m_pHeartTimer,SIGNAL(timeout()),this,SLOT(heartProcessSlot()));
    m_pHeartTimer->start(m_uHeartTime*1000);//开始心跳
    cout<<"开始心跳";

    m_pUdpTrackSocket = new QUdpSocket();
    //udp接收(组播形式)
    //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
    //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
    if(!m_pUdpTrackSocket->bind(QHostAddress::AnyIPv4 ,m_uTrackPort,QAbstractSocket::ShareAddress))
        cout<<"bind track failed--"<<m_uTrackPort;
    else
        cout<<"bind track succeed--"<<m_uTrackPort;

    if(!m_pUdpTrackSocket->joinMulticastGroup(QHostAddress(m_sTrackIP)))
        cout<<"joinMuticastGroup track failed--";
    else
        cout<<"joinMuticastGroup track succeed-- ";
    connect(m_pUdpTrackSocket, SIGNAL(readyRead()),this, SLOT(updateTrackUdpProgress()));
    connect(m_pUdpTrackSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpTrackError(QAbstractSocket::SocketError)));

    //TCP传输小雷达原始数据
    cout<<"开始监听"<<QThread::currentThreadId();
    mSocket = new QTcpSocket();
    mServer = new QTcpServer();
    //关联客户端连接信号newConnection
    connect(mServer,SIGNAL(newConnection()),this,SLOT(new_client())); //连接客户端
    mServer->listen(QHostAddress::Any,uTcpPort);
    //改为独立进程
    //u_workThread->start(); //以线程1的方式接收数据
    m_pUdpVideoSocket = new zchxVideoDataRecvThread(m_sVideoIP, m_uVideoPort, 0/*, QDir::currentPath() + "/data/weihai"*/);
    connect(m_pUdpVideoSocket, SIGNAL(signalSendRecvData(QByteArray)), this, SLOT(updateVideoUdpProgress(QByteArray)));
    m_pUdpVideoSocket->startRecv();
    //接收雷达参数信息
    if(m_bReportOpen && m_sReportIP.trimmed().length() > 0 && m_uReportPort > 0)
    {
        m_pUdpReportSocket = NULL;
        m_pUdpReportSocket = new QUdpSocket();
        //udp接收(组播形式)
        //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
        //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)("192.168.80.254")
        //if(!m_pUdpReportSocket->bind(QHostAddress::AnyIPv4 ,m_uReportPort,QAbstractSocket::ShareAddress))
        QString ip_str = Utils::Profiles::instance()->value("Radar_Control","Mac_IP").toString();

        if(!m_pUdpReportSocket->bind(QHostAddress(ip_str) ,m_uReportPort,QAbstractSocket::ShareAddress))
        {
            cout<<"bind m_sReportIP failed--"<<m_pUdpReportSocket->error();
        }

        if(!m_pUdpReportSocket->joinMulticastGroup(QHostAddress(m_sReportIP)))//m_sReportIP
            cout<<"joinMuticastGroup report failed--";
        else
            cout<<"joinMuticastGroup report succeed--";
        connect(m_pUdpReportSocket, SIGNAL(readyRead()),this, SLOT(updateReportUdpProgress()));
        connect(m_pUdpReportSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpReportError(QAbstractSocket::SocketError)));
    }
}

void ZCHXLowranceRadarDataServer::analysisRadar(const QByteArray &sRadarData, const QString &sRadarType, int uLineNum, int uCellNum, int uHeading)
{
    //cout<<"m_uSourceID"<<m_uSourceID<<sRadarData.size();
    //打印单条回波原始数据集合
    //QString info_r =  sRadarData.toHex();
    qint64 a = 1;

    //cout<<"标志打印"<<prt;
    if(prt)
    {
        QDir dir;
        dir.cd("../");  //进入某文件夹
        if(!dir.exists("回波数据"))//判断需要创建的文件夹是否存在
        {
            dir.mkdir("回波数据"); //创建文件夹
        }
        QString file_name ="../回波数据/回波数据_" + QString::number(name_num) + ".dat";
        QFile file(file_name);//创建文件对象
        bool isOk = file.open(QIODevice::WriteOnly |QIODevice::Append);
        //cout<<"当前文本大小:"<<file.size()/1024;
//        int a = file.size()/1024;
//        if(a > 30720) //当文本大于30M时 新建另一个文本写入数据
        int a = file.size()/1024;
        if(a>30720) //当文本大于30M时 新建另一个文本写入数据
        {
            name_num++;
        }
        if(false == isOk)
        {
            cout <<"打开文件失败";
            return;
        }
        if(true == isOk)
        {
            //file.write(info_r.toStdString().data());
            cout <<"sRadarData大小"<<sRadarData.size();
            file.write(sRadarData,sRadarData.size());
        }
        file.close();
    }
    //小雷达
    if (sRadarType == "zchx240") {
//        qDebug()<<"send analysisLowranceRadar now...........";
        emit analysisLowranceRadar(sRadarData,uLineNum,uCellNum,uHeading);
        if(tcp_flag)
        {
            cout<<"sRadarData"<<sRadarData.size()<<sRadarData;
//            mSocket->write(sRadarData);
//            mSocket->flush();
        }
    } else if (sRadarType == "cat010" || sRadarType == "cat240" || sRadarType == "cat020" || sRadarType == "cat253") {
        emit analysisCatRadar(sRadarData,uLineNum,uCellNum,uHeading,sRadarType);
        if(sRadarType == "cat010")
        {
            cout<<"sRadarData"<<sRadarData.size()<<sRadarData;
            //cout<<"已经发送了010信号sRadarType"<<sRadarType;
        }
    }
}

bool ZCHXLowranceRadarDataServer::getControlValueRange(INFOTYPE type, int &min, int &max)
{
    if(!mRadarStatusMap.contains(type)) return false;
    RadarStatus sts = mRadarStatusMap[type];
    min = sts.getMin();
    max = sts.getMax();
    return true;
}

bool ZCHXLowranceRadarDataServer::getControlAutoAvailable(INFOTYPE type)
{
    if(!mRadarStatusMap.contains(type)) return false;
    RadarStatus sts = mRadarStatusMap[type];
    return sts.getAutoAvailable();
}

void ZCHXLowranceRadarDataServer::setControlValue(INFOTYPE infotype, int value)
{

    cout<<"上传雷达状态值!!!!!setControlValue";
    emit signalSendRecvedContent(QDateTime::currentMSecsSinceEpoch(), "RadarControl", QString("%1:%2").arg(RadarStatus::getTypeString(infotype)).arg(value));

    //检查当前值是否存在
    if(mRadarStatusMap.contains(infotype))
    {
        RadarStatus& sts = mRadarStatusMap[infotype];
        if(sts.getValue() != value)
        {
            QHostAddress objHost(m_sHeartIP);
            switch (infotype) {
            case INFOTYPE::POWER:
            {
                if(value == 0)
                {
                    closeRadar();
                } else if(value == 1)
                {
                    openRadar();
                }
                break;
            }
            case INFOTYPE::SCAN_SPEED:
            {
                if(value >= sts.getMin() && value <= sts.getMax())
                {
                    UINT8 cmd[] = {0x0f, 0xc1, (UINT8)value  }; //00 c1 01  /01 c1 00
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("扫描速度值设置不正确,上传失败！"));
                }
                break;
            }
            case INFOTYPE::ANTENNA_HEIGHT:
            {
                if(value >= sts.getMin() && value <= sts.getMax())
                {
                    int v = value * 1000;  // radar wants millimeters, not meters
                    int v1 = v / 256;
                    int v2 = v & 255;
                    UINT8 cmd[10] = { 0x30, 0xc1, 0x01, 0, 0, 0, (UINT8)v2, (UINT8)v1, 0, 0 };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("天线高度值设置不正确,上传失败！"));
                }

                break;
            }
            case INFOTYPE::BEARING_ALIGNMENT:
            {
                if(value >= sts.getMin() && value <= sts.getMax())//(value >= sts.min && value <= sts.max)
                {
                    if (value < 0)  value += 360;
                    int v = value * 10;
                    int v1 = v / 256;
                    int v2 = v & 255;
                    UINT8 cmd[4] = { 0x05, 0xc1, (UINT8)v2, (UINT8)v1 };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("方位校准值设置不正确,上传失败！"));
                }

                break;
            }
            case INFOTYPE::RANG:
            {
                if(value >= 50 && value <= 130000)//72704
                {
                    unsigned int decimeters = (unsigned int)value * 10;
                    UINT8 cmd[] = { 0x03,0xc1,
                        (UINT8)((decimeters >> 0) & 0XFFL),
                        (UINT8)((decimeters >> 8) & 0XFFL),
                        (UINT8)((decimeters >> 16) & 0XFFL),
                        (UINT8)((decimeters >> 24) & 0XFFL),
                    };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("半径值设置不正确,上传失败！"));
                }

                break;
            }
            case INFOTYPE::GAIN:
            {
                qDebug()<<"gain range:"<<sts.getMin()<<sts.getMax();
                if(value >= sts.getMin() && value <= sts.getMax())
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
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("增益值设置不正确,上传失败！"));
                }

                break;
            }
            case INFOTYPE::SEA_CLUTTER:
            {
                if(value >= sts.getMin() && value <= sts.getMax())
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
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("海杂波值设置不正确,上传失败！"));
                }

                break;
            }

            case INFOTYPE::RAIN_CLUTTER: // 8
            {
                if (value >= sts.getMin() &&  value <= sts.getMax())
                {
                    int v = (value + 1) * 255 / 100;
                    if (v > 255) v = 255;
                    UINT8 cmd[] = { 0x06, 0xc1, 0x04, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("雨杂波值设置不正确,上传失败！"));
                }
                break;
            }
            case INFOTYPE::NOISE_REJECTION: // 9
            {
                if (value >= sts.getMin() &&  value <= sts.getMax())
                {
                    UINT8 cmd[] = { 0x21, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("噪声抑制值设置不正确,上传失败！"));
                }
                break;
            }
            case INFOTYPE::SIDE_LOBE_SUPPRESSION: // 10
            {
                if (value >= sts.getMin() &&  value <= sts.getMax())
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
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("旁瓣抑制值设置不正确,上传失败！"));
                }
                break;
            }
            case INFOTYPE::INTERFERENCE_REJECTION: // 11
            {
                if (value >= sts.getMin() &&  value <= sts.getMax())
                {
                    UINT8 cmd[] = { 0x08, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("抗干扰值设置不正确,上传失败！"));
                }
                break;
            }
            case INFOTYPE::LOCAL_INTERFERENCE_REJECTION:  // 12
            {

                if (value >= sts.getMin() &&  value <= sts.getMax())
                {
                    if (value < 0) value = 0;
                    if (value > 3) value = 3;
                    UINT8 cmd[] = { 0x0e, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("本地抗干扰值设置不正确,上传失败！"));
                }
                break;
            }
            case INFOTYPE::TARGET_EXPANSION: // 13
            {
                if (value >= sts.getMin() &&  value <= sts.getMax()){
                    UINT8 cmd[] = { 0x09, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("目标扩展值设置不正确,上传失败！"));
                }
                break;
            }
                break;
            case INFOTYPE::TARGET_BOOST: // 14
            {
                if (value >= sts.getMin() &&  value <= sts.getMax()){
                    UINT8 cmd[] = { 0x0a, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("目标推进值设置不正确,上传失败！"));
                }
                break;
            }
            case INFOTYPE::TARGET_SEPARATION: // 15
            {

                if (value >= sts.getMin() &&  value <= sts.getMax()){
                    UINT8 cmd[] = { 0x22, 0xc1, (UINT8)value };
                    m_pHeartSocket->writeDatagram((char *)(cmd),sizeof(cmd),objHost,m_uHeartPort);
                }
                else
                {
                    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("目标分离值设置不正确,上传失败！"));
                }
                break;
            }
            default:
                break;
            }
        }
    }

}

void ZCHXLowranceRadarDataServer::updateValue(INFOTYPE controlType, int value)
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
    if(sts.getValue() != value)
    {
        sts.setValue(value);
        emit signalRadarStatusChanged(mRadarStatusMap.values(), m_uSourceID);
    }

}

void ZCHXLowranceRadarDataServer::displayUdpReportError(QAbstractSocket::SocketError error)
{
    if(m_pUdpReportSocket == NULL)
        return;

    qDebug()<<"ZCHXRadarDataUDPServer:"<<m_pUdpReportSocket->errorString();
}

void ZCHXLowranceRadarDataServer::updateReportUdpProgress()
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
    //emit signalSendRecvedContent(utc,"REPORT_RECEIVE",sContent);
//    cout<<"接收到雷达状态数据啦"<<datagram;
//    cout<<"----------------";
    ProcessReport(datagram, datagram.size());
}

void ZCHXLowranceRadarDataServer::ProcessReport(const QByteArray& bytes, int len)
{
    //cout<<"状态-------------"<<QString::fromStdString(bytes.toHex().data());
    //cout<<"标志打印"<<prt;
    if(prt)
    {
        QDir dir;
        dir.cd("../");  //进入某文件夹
        if(!dir.exists("状态数据"))//判断需要创建的文件夹是否存在
        {
            dir.mkdir("状态数据"); //创建文件夹
        }
        QString file_name ="../状态数据/状态数据_" + QString::number(zt_name) + ".dat";
        QFile file(file_name);//创建文件对象
        bool isOk = file.open(QIODevice::WriteOnly |QIODevice::Append);
        int a = file.size()/1024;
        if(a>30720) //当文本大于30M时 新建另一个文本写入数据
        {
            zt_name++;
        }
        if(false == isOk)
        {
            cout <<"打开文件失败";
            return;
        }
        if(true == isOk)
        {
            file.write(bytes,bytes.size());
            file.write("\r\n");
        }
        file.close();
    }
    emit signalSendRecvedContent(QDateTime::currentMSecsSinceEpoch(), "RadarRpoert", QString::fromStdString(bytes.toHex().mid(0, 100).data()));
    if(len < 3 ) return;
    unsigned char val = bytes[1];
    //cout<<val;
    if (val == 0xC4)
    {
        //cout<<"解析雷达数据-------------------------------------------";
        switch ((len << 8) + bytes[0])
        {
        case (18 << 8) + 0x01:  //4068 + 1
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
                qDebug()<<"current radar is BR24";
                emit signalSendRadarType(RADAR_BR24);
                break;
            case 0x08:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_3G");
                qDebug()<<"current radar is 3G";
                emit signalSendRadarType(RADAR_3G);
                break;
            case 0x01:
                //ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_4G");
                qDebug()<<"current radar is 4G";
                emit signalSendRadarType(RADAR_4G);
                break;
            case 0x00:
                qDebug()<<"current radar is 6G";
                emit signalSendRadarType(RADAR_6G);
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
        case (21 << 8) + 0x08:
        {  // length 21, 08 C4
            // contains Doppler data in extra 3 bytes
            RadarReport_08C4_21 *s08 = (RadarReport_08C4_21 *)bytes.data();
            updateValue(INFOTYPE::SCAN_SPEED, s08->old.scan_speed);
            updateValue(INFOTYPE::NOISE_REJECTION, s08->old.noise_rejection);
            updateValue(INFOTYPE::TARGET_SEPARATION, s08->old.target_sep);
            if (s08->old.sls_auto == 1)
                updateValue(INFOTYPE::SIDE_LOBE_SUPPRESSION,-1);
            else
                updateValue(INFOTYPE::SIDE_LOBE_SUPPRESSION, s08->old.side_lobe_suppression * 100 / 255);
            updateValue(INFOTYPE::LOCAL_INTERFERENCE_REJECTION, s08->old.local_interference_rejection);
            uint8_t state = s08->doppler_state;
            uint8_t speed = s08->doppler_speed;
            qDebug()<<"doppler state:"<<state<<speed;
        }
        default:
            break;
        }
    }
    else if(bytes[1] == 0xF5){
        cout<<"不知道是什么类型";
        //ZCHXLOG_DEBUG("unknown: buf[1]=0xF5");
    }
    return ;
}

int ZCHXLowranceRadarDataServer::sourceID() const
{
    return m_uSourceID;
}

void ZCHXLowranceRadarDataServer::parseRadarControlSetting(INFOTYPE infotype)
{
    QString str_radar_cmd = QString("Radar_Command_%1").arg(m_uSourceID);
    QStringList list = Utils::Profiles::instance()->value(str_radar_cmd,\
                                                           RadarStatus::getTypeString(infotype,STR_MODE_ENG)\
                                                          ).toStringList();
    if(list.length() < 2) return;
    mRadarStatusMap[infotype] = RadarStatus(infotype, list[0].toInt(), list[1].toInt());
}

void ZCHXLowranceRadarDataServer::prtVideoSlot(bool a)//打印回波数据
{
    prt = a;
}

void ZCHXLowranceRadarDataServer::new_client()
{
    cout<<"新客户段连接";
    emit signalSendRecvedContent(QDateTime::currentMSecsSinceEpoch(), "新客户段连接", "采集器控制端");

    mSocket = mServer->nextPendingConnection();//与客户端通信的套接字
    //关联接收客户端数据信号readyRead信号（客户端有数据就会发readyRead信号）
    connect(mSocket,SIGNAL(readyRead()),this,SLOT(read_client_data()));
    //检测掉线信号
    connect(mSocket,SIGNAL(disconnected()),this,SLOT(client_dis()));
    tcp_flag = true;
}

void ZCHXLowranceRadarDataServer::client_dis()
{
    QTcpSocket *obj = (QTcpSocket*)sender();//掉线对象
    cout<<obj->peerAddress().toString();//打印出掉线对象的ip
    cout<<"断开了";
    tcp_flag = false;
}

void ZCHXLowranceRadarDataServer::read_client_data()
{
    QString str_radar = QString("Radar_%1").arg(m_uSourceID);
    QString sVideoIP = Utils::Profiles::instance()->value(str_radar,"Video_IP").toString();
    QString uVideoPort = Utils::Profiles::instance()->value(str_radar,"Video_Port").toString();
    QString sHeartIP = Utils::Profiles::instance()->value(str_radar,"Heart_IP").toString();
    QString uHeartPort = Utils::Profiles::instance()->value(str_radar,"Heart_Port").toString();
    QString dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toString();
    QString dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toString();
    QString sControlIP = Utils::Profiles::instance()->value(str_radar,"Report_IP").toString();
    QString uControlPort = Utils::Profiles::instance()->value(str_radar,"Report_Port").toString();
    //QString sLimit_File = Utils::Profiles::instance()->value(str_radar,"Limit_File").toString();
    QString m_jupmdis = Utils::Profiles::instance()->value(str_radar,"jump_distance").toString();
    QString radar_num = Utils::Profiles::instance()->value(str_radar,"radar_num").toString();
    QString track_max_radius = Utils::Profiles::instance()->value(str_radar,"track_radius").toString();
    QString track_min_radius =  Utils::Profiles::instance()->value(str_radar,"track_min_radius").toString();
    QString ClearTrack_Time = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toString();
    QString Radius = Utils::Profiles::instance()->value(str_radar,"Radius").toString();
    QString min_amplitude = Utils::Profiles::instance()->value(str_radar,"min_amplitude").toString();
    QString max_amplitude = Utils::Profiles::instance()->value(str_radar,"max_amplitude").toString();
    QString historyNum = Utils::Profiles::instance()->value(str_radar,"historyNum").toString();
    QString RadiusCoefficient = Utils::Profiles::instance()->value(str_radar,"RadiusCoefficient").toString();
    QString a1 =(Utils::Profiles::instance()->value("Color","color1_R",255).toString());
    QString a2 = (Utils::Profiles::instance()->value("Color","color1_G",255).toString());
    QString a3 = (Utils::Profiles::instance()->value("Color","color1_B",255).toString());
    QString b1 = (Utils::Profiles::instance()->value("Color","color2_R",255).toString());
    QString b2 = (Utils::Profiles::instance()->value("Color","color2_G",255).toString());
    QString b3 = (Utils::Profiles::instance()->value("Color","color2_B",255).toString());
    QString Limit_File = Utils::Profiles::instance()->value(str_radar,"Limit_File").toString();
    //发送设置
    QString video_Topic = Utils::Profiles::instance()->value(str_radar,"video_Topic").toString();
    QString video_Send_Port = Utils::Profiles::instance()->value(str_radar,"video_Send_Port").toString();
    QString Track_Topic = Utils::Profiles::instance()->value(str_radar,"Track_Topic").toString();
    QString Track_Send_Port = Utils::Profiles::instance()->value(str_radar,"Track_Send_Port").toString();
    QString Yuhui_Topic = Utils::Profiles::instance()->value(str_radar,"Yuhui_Topic").toString();
    QString Yuhui_Send_Port = Utils::Profiles::instance()->value(str_radar,"Yuhui_Send_Port").toString();
    //全部状态
    QString Power;                                             //  雷达电源控制
    QString Scan_speed;                                        //  扫描速度
    QString Antenna_height                                    ;//  天线高度
    QString Bearing_alignment                                 ;//  方位校准
    QString Rang                                              ;//  半径
    QString Gain                                              ;//  增益
    QString Sea_clutter                                       ;//  海杂波
    QString Rain_clutter                                      ;//  雨杂波
    QString Noise_rejection                                   ;//  噪声抑制
    QString Side_lobe_suppression                             ;//  旁瓣抑制
    QString Interference_rejection                            ;//  抗干扰
    QString Local_interference_rejection                      ;//  本地抗干扰
    QString Target_expansion                                  ;//  目标扩展
    QString Target_boost                                      ;//  目标推进
    QString Target_separation                                 ;//  目标分离
    if(mRadarStatusMap.size() >= 15)
    {
        foreach (RadarStatus element, mRadarStatusMap) {
            int elelmentID = element.getId();// 消息类型
            int value = element.getValue();// 当前值
            if(elelmentID == 1)
                Power = QString::number(value);
            if(elelmentID == 2)
                Scan_speed = QString::number(value);
            if(elelmentID == 3)
                Antenna_height = QString::number(value);
            if(elelmentID == 4)
                Bearing_alignment = QString::number(value);
            if(elelmentID == 5)
               Rang = QString::number(value);
            if(elelmentID == 6)
               Gain = QString::number(value);
            if(elelmentID == 7)
               Sea_clutter = QString::number(value);
            if(elelmentID == 8)
               Rain_clutter = QString::number(value);
            if(elelmentID == 9)
               Noise_rejection = QString::number(value);
            if(elelmentID == 10)
               Side_lobe_suppression = QString::number(value);
            if(elelmentID == 11)
               Interference_rejection = QString::number(value);
            if(elelmentID == 12)
               Local_interference_rejection = QString::number(value);
            if(elelmentID == 13)
              Target_expansion = QString::number(value);
            if(elelmentID == 14)
              Target_boost = QString::number(value);
            if(elelmentID == 15)
              Target_separation = QString::number(value);
        }
    }
    //构造配置字符串
    QString mProData = sVideoIP+","+uVideoPort+","+sHeartIP+","+uHeartPort+","+sControlIP+","+uControlPort+","+
            dCentreLat+","+dCentreLon+","+track_min_radius+","+track_max_radius+","+min_amplitude+","+max_amplitude
            +","+m_jupmdis+","+ClearTrack_Time+","+radar_num+","+historyNum+","+Radius+","+RadiusCoefficient+","+a1+
            ","+a2+","+a3+","+b1+","+b2+","+b3+","+Limit_File+","+video_Send_Port+","+video_Topic+","+Track_Send_Port
            +","+Track_Topic+","+Yuhui_Send_Port+","+Yuhui_Topic+","+Power+","+Scan_speed+","+Antenna_height+","+Bearing_alignment
            +","+Rang+","+Gain+","+Sea_clutter+","+Rain_clutter+","+Noise_rejection+","+Side_lobe_suppression+","+Interference_rejection+","+Local_interference_rejection
            +","+Target_expansion+","+Target_boost+","+Target_separation+",PNG";
    if(mSocket == NULL)
    {
        return;
    }
    QByteArray aisArray = mSocket->readAll();
    Sleep(200);
    QString data = aisArray;
    if(data == "get")
    {
        cout<<"收到请求"<<mProData.size();
        mSocket->write(mProData.toLatin1().data());
    }
    else
    {
        QStringList list = data.split(",");
        cout<<"上传状态"<<list.size()<<data;
        if(list.size() == 15)
        {
            for(int i = 0; i < list.size(); i++)
            {
                INFOTYPE c;
                c = (INFOTYPE)(i+1);
                signalRadarConfigChanged(m_uSourceID, c,list[i].toInt());
            }
        }
        if(data == "open")
        {
            signalRadarConfigChanged(m_uSourceID, POWER,1);
        }
        if(data == "close")
        {
            signalRadarConfigChanged(m_uSourceID, POWER,0);
        }
        if(list.size() == 31)
        {
            Utils::Profiles::instance()->setValue(str_radar,"Video_IP",list[0]);
            Utils::Profiles::instance()->setValue(str_radar,"Video_Port",list[1]);
            Utils::Profiles::instance()->setValue(str_radar,"Heart_IP",list[2]);
            Utils::Profiles::instance()->setValue(str_radar,"Heart_Port",list[3]);
            Utils::Profiles::instance()->setValue(str_radar,"Centre_Lat",list[7]);
            Utils::Profiles::instance()->setValue(str_radar,"Centre_Lon",list[6]);
            Utils::Profiles::instance()->setValue(str_radar,"Report_IP",list[4]);
            Utils::Profiles::instance()->setValue(str_radar,"Report_Port",list[5]);
            Utils::Profiles::instance()->setValue(str_radar,"Limit_File",list[8]);
            Utils::Profiles::instance()->setValue(str_radar,"jump_distance",list[13]);
            Utils::Profiles::instance()->setValue(str_radar,"radar_num",list[15]);
            Utils::Profiles::instance()->setValue(str_radar,"track_radius",list[10]);
            Utils::Profiles::instance()->setValue(str_radar,"track_min_radius",list[9]);
            Utils::Profiles::instance()->setValue(str_radar,"ClearTrack_Time",list[14]);
            Utils::Profiles::instance()->setValue(str_radar,"Radius",list[17]);
            Utils::Profiles::instance()->setValue(str_radar,"min_amplitude",list[11]);
            Utils::Profiles::instance()->setValue(str_radar,"max_amplitude",list[12]);
            Utils::Profiles::instance()->setValue(str_radar,"historyNum",list[16]);
            Utils::Profiles::instance()->setValue(str_radar,"RadiusCoefficient",list[18]);
            (Utils::Profiles::instance()->setValue("Color","color1_R",list[19]));
            (Utils::Profiles::instance()->setValue("Color","color1_G",list[20]));
            (Utils::Profiles::instance()->setValue("Color","color1_B",list[21]));
            (Utils::Profiles::instance()->setValue("Color","color2_R",list[22]));
            (Utils::Profiles::instance()->setValue("Color","color2_G",list[23]));
            (Utils::Profiles::instance()->setValue("Color","color2_B",list[24]));
            //发送设置
            Utils::Profiles::instance()->setValue(str_radar,"video_Topic",list[28]);
            Utils::Profiles::instance()->setValue(str_radar,"video_Send_Port",list[27]);
            Utils::Profiles::instance()->setValue(str_radar,"Track_Topic",list[26]);
            Utils::Profiles::instance()->setValue(str_radar,"Track_Send_Port",list[25]);
            Utils::Profiles::instance()->setValue(str_radar,"Yuhui_Topic",list[30]);
            Utils::Profiles::instance()->setValue(str_radar,"Yuhui_Send_Port",list[29]);
            colorSetSignal(list[19].toInt(),list[20].toInt(),list[21].toInt(),list[22].toInt(),list[23].toInt(),list[24].toInt());//回波颜色设置
        }
    }
}

void ZCHXLowranceRadarDataServer::setPixSlot(QPixmap pix)
{
    cout<<"setPixSlot";
    QImage videoAndTargetPixmap;
    videoAndTargetPixmap = pix.toImage();
    if(mSocket == NULL)
    {
        cout<<"return";
        return;
    }
    QBuffer buffer;
    videoAndTargetPixmap.save(&buffer,"PNG");
    mSocket->write(buffer.data());
}

