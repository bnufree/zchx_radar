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

extern "C"

{

#include "ctrl.h"

}
////////////////////////////////////////////////////////////////////////////////

typedef int(*FUN3)(struct SAzmData* psScan, int* pSit);
extern  FUN3 Tracking_Fun3 = NULL;

////////////////////////////////////////////////////////////////////////////////////
static uint8_t BR24MARK[] = {0x00, 0x44, 0x0d, 0x0e};
ZCHXRadarDataServer::ZCHXRadarDataServer(int uSourceID, QObject *parent)
    : QObject(parent),
      m_bLimit(false),
      m_uSourceID(uSourceID)

{
    //从配置文件读取
    QString str_radar = QString("Radar_%1").arg(m_uSourceID);
    m_dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toDouble();
    m_dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toDouble();
    m_uLoopNum = Utils::Profiles::instance()->value(str_radar,"Loop_Num").toInt();
    m_bLimit = Utils::Profiles::instance()->value(str_radar,"Limit").toBool();
    m_uVideoSendPort = Utils::Profiles::instance()->value(str_radar,"video_Send_Port").toInt();
    m_sVideoTopic = Utils::Profiles::instance()->value(str_radar,"video_Topic").toString();
    m_uTrackSendPort = Utils::Profiles::instance()->value(str_radar,"Track_Send_Port").toInt();
    m_sTrackTopic = Utils::Profiles::instance()->value(str_radar,"Track_Topic").toString();

    m_sTrackIP = Utils::Profiles::instance()->value(str_radar,"Track_IP").toString();
    m_uTrackPort = Utils::Profiles::instance()->value(str_radar,"Track_Port").toInt();
    m_sVideoIP = Utils::Profiles::instance()->value(str_radar,"Video_IP").toString();
    m_uVideoPort = Utils::Profiles::instance()->value(str_radar,"Video_Port").toInt();

    m_sRadarVideoType = Utils::Profiles::instance()->value(str_radar,"Video_Type").toString();
    m_uCellNum = Utils::Profiles::instance()->value(str_radar,"Cell_Num").toInt();
    m_uLineNum = Utils::Profiles::instance()->value(str_radar,"Line_Num").toInt();
    m_uHeading = Utils::Profiles::instance()->value(str_radar,"Heading").toInt();

    m_uHeartTime = Utils::Profiles::instance()->value(str_radar,"Heart_Time").toInt();
    m_sHeartIP = Utils::Profiles::instance()->value(str_radar,"Heart_IP").toString();
    m_uHeartPort = Utils::Profiles::instance()->value(str_radar,"Heart_Port").toInt();
    m_sOptRadarIP = Utils::Profiles::instance()->value(str_radar,"RadarOpt_IP").toString();
    m_uOptRadarPort = Utils::Profiles::instance()->value(str_radar,"RadarOpt_Port").toInt();

    m_distance = Utils::Profiles::instance()->value(str_radar,"Distance").toDouble();

    m_clearRadarTrackTime = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt();

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
    connect(this,SIGNAL(processAndSendSignal()),this,SLOT(processAndSendSlot()));
    moveToThread(&m_workThread);
    m_workThread.start();





    //属性系统test
//    QPushButton *pButton = new QPushButton();
//    QObject *object = pButton;
//    const QMetaObject *metaobject = object->metaObject();
//    int count = metaobject->propertyCount();
//    for (int i=0; i<count; ++i) {
//        QMetaProperty metaproperty = metaobject->property(i);
//        const char *name = metaproperty.name();
//        QString str = QString::fromLatin1(name);
//        qDebug()<< "i "<<i<<"name"<<str;
//        QVariant value = object->property(name);

//    }

}

ZCHXRadarDataServer::~ZCHXRadarDataServer()
{
    qDebug()<<"~ZCHXRadarDataServer()_11";

    if(m_workThread.isRunning())
    {
        m_workThread.quit();
    }
    m_workThread.terminate();
    zmq_close(m_pVideoLisher);
    zmq_ctx_destroy(m_pVideoContext);

    zmq_close(m_pTrackLisher);
    zmq_ctx_destroy(m_pTrackContext);

    if(m_pCAsterixFormat)
    {
        delete m_pCAsterixFormat;
        m_pCAsterixFormat = NULL;
    }
    if(m_pCAsterixFormatDescriptor)
    {
        delete m_pCAsterixFormatDescriptor;
        m_pCAsterixFormatDescriptor = NULL;
    }
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
    if(m_DrawRadarVideo)
    {
        delete m_DrawRadarVideo;
        m_DrawRadarVideo = NULL;
    }

    if(m_pTrackMonitorThread->isRunning())
    {
        m_pTrackMonitorThread->quit();
    }
    m_pTrackMonitorThread->terminate();
    if(m_pVideoMonitorThread->isRunning())
    {
        m_pVideoMonitorThread->quit();
    }
    m_pVideoMonitorThread->terminate();
}

void ZCHXRadarDataServer::readRadarLimitFormat()
{
    QString path = QCoreApplication::applicationDirPath();
    QString pathName = path+"/radar_formart.json";
    m_landPolygon.clear();
    m_seaPolygon.clear();
    analysisLonLatTOPolygon(pathName,m_landPolygon,m_seaPolygon);
}

void ZCHXRadarDataServer::openRadar()
{

    //m_asio_server->send_peer_message(radarSourceId, (char *)(Down + 0), 3);
    //m_asio_server->send_peer_message(radarSourceId, (char *)(Down + 3), 3);
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("radar ip = %1,port = %2").arg(m_sHeartIP).arg(m_uHeartPort);
    emit signalSendRecvedContent(utc,"CLOSE_RADAR",sContent);
    //qDebug()<<"close radar"<<m_sHeartIP<<m_sHeartIP;
    QHostAddress objHost(m_sHeartIP);
    unsigned char Boot[6] = { 0x00, 0xc1, 0x01, 0x01, 0xc1, 0x01 }; //00c101/01c101
    m_pHeartSocket->writeDatagram((char *)(Boot + 0),3,objHost,m_uHeartPort);
    m_pHeartSocket->writeDatagram((char *)(Boot + 3),3,objHost,m_uHeartPort);

}

void ZCHXRadarDataServer::closeRadar()
{
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("radar ip = %1,port = %2").arg(m_sHeartIP).arg(m_uHeartPort);
    emit signalSendRecvedContent(utc,"OPEN_RADAR",sContent);
    //qDebug()<<"open radar"<<m_sOptRadarIP<<m_uOptRadarPort;
    QHostAddress objHost(m_sHeartIP);
    unsigned char Down[6] = { 0x00, 0xc1, 0x01, 0x01, 0xc1, 0x00 }; //00 c1 01  /01 c1 00
    m_pHeartSocket->writeDatagram((char *)(Down + 0),3,objHost,m_uHeartPort);
    m_pHeartSocket->writeDatagram((char *)(Down + 3),3,objHost,m_uHeartPort);

    //m_asio_server->send_peer_message(radarSourceId, (char *)(Boot + 0), 3);
    //m_asio_server->send_peer_message(radarSourceId, (char *)(Boot + 3), 3);

}

void ZCHXRadarDataServer::processAndSendSlot()
{
    qDebug()<<"processAndSendSlot thread id :"<<QThread::currentThreadId();
}

void ZCHXRadarDataServer::startProcessSlot()
{
    qDebug()<<"startProcessSlot thread id :"<<QThread::currentThreadId();
    init();
    readRadarLimitFormat();
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
    analysisRadar(datagram,sRadarType);
}

void ZCHXRadarDataServer::displayUdpVideoError(QAbstractSocket::SocketError error)
{
    if(m_pUdpVideoSocket == NULL)
    {
        return;
    }
    qDebug()<<"ZCHXRadarDataUDPServer:"<<m_pUdpVideoSocket->errorString();

}

void ZCHXRadarDataServer::updateVideoUdpProgress()
{
    if(m_pUdpVideoSocket == NULL)
    {
        return;
    }
    //qDebug()<<"updateVideoUdpProgress thread id :"<<QThread::currentThreadId();

    static int num = 0;
    QByteArray datagram;
    // 让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
    datagram.resize(m_pUdpVideoSocket->pendingDatagramSize());//pendingDatagramSize() 当前数据包大小
    // 接收数据报，将其存放到datagram中
    m_pUdpVideoSocket->readDatagram(datagram.data(), datagram.size());//readDatagram将不大于指定长度的数据保存到datagram.data()
    num++;
    //qDebug()<<"num"<<num<<" udp sRadar VideoData size:"<<datagram.size();
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar video data,size = %1").arg(datagram.size());
    emit signalSendRecvedContent(utc,"VIDEO_RECEIVE",sContent);

    emit processAndSendSignal();
    //analysisRadar(datagram,m_sRadarVideoType,m_uLineNum,m_uCellNum,m_uHeading);
}

void ZCHXRadarDataServer::setRadarVideoPixmap(const QPixmap &objPixmap)
{
//    qDebug()<<"save--------------------------";
//    QString path = QCoreApplication::applicationDirPath();
//    QString str = QString("/Afterglow.png");
//    path = path+str;
//    qDebug()<<"path"<<path;
//    objPixmap.save(path);

    //
    //图片转二进制
    QByteArray pixArray;
    QBuffer buffer(&pixArray);
    buffer.open(QIODevice::WriteOnly);
    objPixmap.save(&buffer ,"PNG");

    ITF_RadarVideo objRadarVideo;
    //封装proto
    objRadarVideo.set_radarid(m_uSourceID);
    objRadarVideo.set_radarname("雷达");
    objRadarVideo.set_latitude(m_dCentreLat);
    objRadarVideo.set_longitude(m_dCentreLon);
    objRadarVideo.set_utc(QDateTime::currentMSecsSinceEpoch());
    objRadarVideo.set_height(objPixmap.height());
    objRadarVideo.set_width(objPixmap.width());
    objRadarVideo.set_radius(m_dRadius);
    objRadarVideo.set_imagedata(pixArray.data(),pixArray.size());
    //以下是余辉要用的
    objRadarVideo.set_preimagedata(NULL,0);
    objRadarVideo.set_loopnum(-1);
    objRadarVideo.set_curindex(-1);

    //通过zmq发送
    QByteArray sendData;
    sendData.resize(objRadarVideo.ByteSize());
    objRadarVideo.SerializePartialToArray(sendData.data(),sendData.size());

    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uVideoSendPort);

    QString sTopic = m_sVideoTopic;
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();

    zmq_bind(m_pVideoLisher, sIPport.toLatin1().data());//
    zmq_send(m_pVideoLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pVideoLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pVideoLisher, sendData.data(), sendData.size(), 0);

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("send analysis radar video data,size = %1").arg(sendData.size());
    emit signalSendRecvedContent(utc,"VIDEO_SEND",sContent);

}

void ZCHXRadarDataServer::setRadarAfterglowPixmap(const int uIndex, const QPixmap &videoPixmap,const QPixmap &objPixmap, const QPixmap &prePixmap)
{

    //qDebug()<<"save---AfterglowPixmap-----------------<<"<<uIndex;
    QString path = QCoreApplication::applicationDirPath();
    QString str1 = QString("/video.png");
    QString path1 = path+str1;
    videoPixmap.save(path1);

    QString str2 = QString("/Afterglow.png");
    QString path2 = path+str2;
    objPixmap.save(path2);

    QString str3 = QString("/pre_Afterglow.png");
    QString path3 = path+str3;
    prePixmap.save(path3);

    //回波图片转二进制
    QByteArray videoArray;
    QBuffer videoBuffer(&videoArray);
    videoBuffer.open(QIODevice::WriteOnly);
    videoPixmap.save(&videoBuffer ,"PNG");

    //当前余辉图片转二进制
    QByteArray pixArray;
    QBuffer buffer(&pixArray);
    buffer.open(QIODevice::WriteOnly);
    objPixmap.save(&buffer ,"PNG");

    //前一张余辉图片转二进制
    QByteArray preArray;
    QBuffer preBuffer(&preArray);
    preBuffer.open(QIODevice::WriteOnly);
    if(!m_prePixmap.isNull())
        m_prePixmap.save(&preBuffer ,"PNG");

    ITF_RadarVideo objRadarVideo;
    //封装proto
    objRadarVideo.set_radarid(m_uSourceID);
    objRadarVideo.set_radarname("雷达回波余辉");
    objRadarVideo.set_latitude(m_dCentreLat);
    objRadarVideo.set_longitude(m_dCentreLon);
    objRadarVideo.set_utc(QDateTime::currentMSecsSinceEpoch());
    objRadarVideo.set_height(objPixmap.height());
    objRadarVideo.set_width(objPixmap.width());
    objRadarVideo.set_radius(m_dRadius);
    objRadarVideo.set_imagedata(videoArray.data(),videoArray.size());
    //以下是余辉要用的
    objRadarVideo.set_curimagedata(pixArray.data(),pixArray.size());
    if(!m_prePixmap.isNull())
        objRadarVideo.set_preimagedata(preArray.data(),preArray.size());
    else
        objRadarVideo.set_preimagedata(NULL,0);
    objRadarVideo.set_loopnum(m_uLoopNum);
    objRadarVideo.set_curindex(uIndex);

    //通过zmq发送
    QByteArray sendData;
    sendData.resize(objRadarVideo.ByteSize());
    objRadarVideo.SerializePartialToArray(sendData.data(),sendData.size());

    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uVideoSendPort);

    QString sTopic = "RadarVideo";
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();

    zmq_bind(m_pVideoLisher, sIPport.toLatin1().data());//
    zmq_send(m_pVideoLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pVideoLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pVideoLisher, sendData.data(), sendData.size(), 0);
    m_prePixmap = prePixmap;

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("send analysis radar video data,size = %1").arg(sendData.size());
    emit signalSendRecvedContent(utc,"VIDEO________SEND",sContent);
    //qDebug()<<"VIDEO_SEND";
}

void ZCHXRadarDataServer::heartProcessSlot()
{
    QHostAddress objHost(m_sHeartIP);
    unsigned char heartbeat_[8] = { 0Xa0, 0xc1, 0x03, 0xc2, 0x04, 0xc2, 0x05, 0xc2 };
    for (int i = 0; i < 4; i++)
    {
        unsigned char sendC[2] = { 0 };
        memcpy(sendC, heartbeat_ + i * 2, 2);
        m_pHeartSocket->writeDatagram((char *)sendC,2,objHost,m_uHeartPort);
        //m_asio_server->send_peer_message(radarSourceId, (char *)sendC, 2);
    }

}



void ZCHXRadarDataServer::init()
{
    m_radarPointMap.clear();
    m_radarTrackMap.clear();
    m_radarVideoMap.clear();
    m_DrawRadarVideo = NULL;
    //m_pRunnable = NULL;
    m_lastClearRadarVideoTime = 0;

    m_pUdpTrackSocket = NULL;
    m_pUdpVideoSocket = NULL;

    m_pHeartTimer = new QTimer();
    connect(m_pHeartTimer,SIGNAL(timeout()),this,SLOT(heartProcessSlot()));
    m_pHeartTimer->start(m_uHeartTime*1000);//开始心跳

    //track
    //配置中没有该配置时设置一个默认值
    QString str_radar = QString("Radar_%1").arg(m_uSourceID);
//    Utils::Profiles::instance()->setDefault(str_radar,"Centre_Lat",22);
//    Utils::Profiles::instance()->setDefault(str_radar,"Centre_Lon",112);
//    Utils::Profiles::instance()->setDefault(str_radar,"Display_Type",1);
//    Utils::Profiles::instance()->setDefault(str_radar,"Loop_Num",3);

//    Utils::Profiles::instance()->setDefault(str_radar,"Track_IP","239.255.50.41");
//    Utils::Profiles::instance()->setDefault(str_radar,"Track_Port",50001);
//    Utils::Profiles::instance()->setDefault(str_radar,"Track_Type","cat010");





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


    //video
//    Utils::Profiles::instance()->setDefault(str_radar,"Video_IP","239.255.50.41");
//    Utils::Profiles::instance()->setDefault(str_radar,"Video_Port",50002);
//    Utils::Profiles::instance()->setDefault(str_radar,"Video_Type","cat240");
//    Utils::Profiles::instance()->setDefault(str_radar,"Line_Num",820);
//    Utils::Profiles::instance()->setDefault(str_radar,"Cell_Num",1364);
//    Utils::Profiles::instance()->setDefault(str_radar,"Heading",0);

    m_pUdpVideoSocket = new QUdpSocket();
    //udp接收(组播形式)
    //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
    //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
    if(!m_pUdpVideoSocket->bind(QHostAddress::AnyIPv4,m_uVideoPort,QAbstractSocket::ShareAddress))
        qDebug()<<"bind video failed--";

    if(!m_pUdpVideoSocket->joinMulticastGroup(QHostAddress(m_sVideoIP)))
        qDebug()<<"joinMuticastGroup video failed--";
    qDebug()<<"joinMuticastGroup video succeed-- ";

    connect(m_pUdpVideoSocket, SIGNAL(readyRead()),this, SLOT(updateVideoUdpProgress()));
    connect(m_pUdpVideoSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpVideoError(QAbstractSocket::SocketError)));

    //发送回波和余辉的zmq
    //创建context，zmq的socket 需要在context上进行创建
    m_pVideoContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pVideoLisher= zmq_socket(m_pVideoContext, ZMQ_PUB);



    //发送雷达目标的zmq
    //创建context，zmq的socket 需要在context上进行创建
    m_pTrackContext = zmq_ctx_new();
    //创建zmq socket ，socket目前有6中属性 ，这里使用PUB方式(广播)
    //具体使用方式请参考zmq官方文档（zmq手册）
    m_pTrackLisher= zmq_socket(m_pTrackContext, ZMQ_PUB);



    //监听回波和余辉zmq
    QString monitorVideoUrl = "inproc://monitor.radarVideoclient";
    zmq_socket_monitor (m_pVideoLisher, monitorVideoUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pVideoMonitorThread = new ZmqMonitorThread(m_pVideoContext, monitorVideoUrl, 0);
    connect(m_pVideoMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pVideoMonitorThread, SIGNAL(finished()), m_pVideoMonitorThread, SLOT(deleteLater()));
    m_pVideoMonitorThread->start();

    //监听雷达目标zmq
    QString monitorTrackUrl = "inproc://monitor.radarTrackclient";
    zmq_socket_monitor (m_pTrackLisher, monitorTrackUrl.toStdString().c_str(), ZMQ_EVENT_ALL);
    m_pTrackMonitorThread = new ZmqMonitorThread(m_pTrackContext, monitorTrackUrl, 0);
    connect(m_pTrackMonitorThread, SIGNAL(signalClientInOut(QString,QString,int,int)), this, SIGNAL(signalClientInout(QString,QString,int,int)));
    connect(m_pTrackMonitorThread, SIGNAL(finished()), m_pTrackMonitorThread, SLOT(deleteLater()));
    m_pTrackMonitorThread->start();

    //初始新科雷达解析库的调用
    QString sAppPath = QApplication::applicationDirPath();
    m_sPath = sAppPath+"/AsterixSpecification/asterix.ini";
    std::string str = m_sPath.toStdString();
    const char* asterixDefinitionsFile = str.data();
    //qDebug()<<"path"<<asterixDefinitionsFile;
    m_pCAsterixFormat = new CAsterixFormat(asterixDefinitionsFile);
    m_pCAsterixFormatDescriptor = dynamic_cast<CAsterixFormatDescriptor*>(m_pCAsterixFormat->CreateFormatDescriptor(0, ""));

    //调用小雷达目标库
    QLibrary lib("Record.dll");
    if (lib.load())
    {
        qDebug() << "load ok!";


        Tracking_Fun3 = (FUN3)lib.resolve("?Tracking@@YAHPEAUSAzmData@@PEAH@Z");

        if (Tracking_Fun3) {
            qDebug() << "load Tracking ok!";
        }
        else
        {
            qDebug() << "resolve Tracking error!";
        }
    }
    else
    {
        qDebug() << "load error!";
    }

}

void ZCHXRadarDataServer::analysisRadar(const QByteArray &sRadarData, const QString &sRadarType, int uLineNum, int uCellNum, int uHeading)
{


    qint64 utc = QDateTime::currentMSecsSinceEpoch();

    if(sRadarType == "Lowrance")//小雷达
    {
        QString sContent = tr("process Lowrance radar ");
        emit signalSendRecvedContent(utc,"LOWRANCE",sContent);
        analysisLowranceRadar(sRadarData,uLineNum,uCellNum,uHeading);

    }
    if(sRadarType == "cat010"||sRadarType == "cat240")
    {
        //使用解析库进行解析cat010,cat240协议雷达
        if(m_pCAsterixFormatDescriptor == NULL)
        {
            qDebug()<<"m_pCAsterixFormatDescriptor is NULL";
            return;
        }
        if(m_pCAsterixFormatDescriptor->m_pAsterixData)
        {
            delete m_pCAsterixFormatDescriptor->m_pAsterixData;
            m_pCAsterixFormatDescriptor->m_pAsterixData = NULL;
        }
        m_pCAsterixFormatDescriptor->m_pAsterixData = m_pCAsterixFormatDescriptor->m_InputParser.parsePacket((const unsigned char*)(sRadarData.constData()), sRadarData.size());
        DataBlock* pDB = m_pCAsterixFormatDescriptor->m_pAsterixData->m_lDataBlocks.front();
        if(sRadarType == "cat010")
        {
            QString sContent = tr("process cat010 radar ");
            emit signalSendRecvedContent(utc,"CAT010",sContent);
            analysisCat010Radar(pDB);
        }
        else if(sRadarType == "cat240")
        {
            QString sContent = tr("process cat240 radar ");
            emit signalSendRecvedContent(utc,"CAT240",sContent);
            analysisCat240Radar(pDB,uLineNum,uCellNum,uHeading);
        }

    }


}

void ZCHXRadarDataServer::analysisCat010Radar(DataBlock *pDB)
{
    int uNum = pDB->m_lDataRecords.size();
    //qDebug()<<"uNum"<<uNum;
    if(uNum<=0)
    {
        return;
    }

    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    int time_of_day = startDateTime.secsTo(curDateTime);

    std::string strValue;

    //std::vector<std::pair<double, double>> latLonVec;
    //latLonVec.clear();
    for(int i=0; i< uNum; i++)
    {
        QJsonObject objObject;
        com::zhichenhaixin::proto::TrackPoint trackPoint;
        if(pDB->getItemValue(i, strValue, "010", "SAC"))
        {
            trackPoint.set_systemareacode(atoi(strValue.c_str()));
            //objObject.insert("SAC",atoi(strValue.c_str()));
        }

        if(pDB->getItemValue(i, strValue, "010", "SIC"))
        {
            trackPoint.set_systemidentificationcode(atoi(strValue.c_str()));
            //objObject.insert("SIC",atoi(strValue.c_str()));
        }

        if(pDB->getItemValue(i, strValue, "001", "MsgTyp"))
        {
            int tempValue = atoi(strValue.c_str());
            trackPoint.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(tempValue));
            //objObject.insert("MsgTyp",tempValue);
        }

        int tempTrackNumber = 0 ;
        if(pDB->getItemValue(i, strValue, "161", "TrkNb"))
        {
            tempTrackNumber = atoi(strValue.c_str());
        }
        trackPoint.set_tracknumber(tempTrackNumber);
        //objObject.insert("TrkNb",tempTrackNumber);

        float tempCartesianPosX = 0.0;
        float tempCartesianPosY = 0.0;
        if(pDB->getItemValue(i, strValue, "042", "X"))
        {
            tempCartesianPosX = atof(strValue.c_str());
        }
        if(pDB->getItemValue(i, strValue, "042", "Y"))
        {
            tempCartesianPosY = atof(strValue.c_str());
        }
        trackPoint.set_cartesianposx(tempCartesianPosX);
        trackPoint.set_cartesianposy(tempCartesianPosY);
        //objObject.insert("X",tempCartesianPosX);
        //objObject.insert("Y",tempCartesianPosY);

        double tempWgs84PosLat = 0.0;
        double tempWgs84PosLong = 0.0;
//        if(pDB->getItemValue(i, strValue, "041", "Lat"))
//        {
//            tempWgs84PosLat = atof(strValue.c_str());
//        }
//        if(pDB->getItemValue(i, strValue, "041", "Lon"))
//        {
//            tempWgs84PosLong = atof(strValue.c_str());
//        }

        double dTempLon = m_dCentreLon;
        double dTempLat = m_dCentreLat;

//        if(0 == tempWgs84PosLong || 0 == tempWgs84PosLat)
//        {
            convertXYtoWGS(dTempLat,dTempLon,tempCartesianPosX, tempCartesianPosY, &tempWgs84PosLat ,&tempWgs84PosLong);
            //ZCHXLOG_DEBUG("convertXYtoWGS");
        //}

        trackPoint.set_wgs84poslat(tempWgs84PosLat);
        trackPoint.set_wgs84poslong(tempWgs84PosLong);
        //objObject.insert("Lat",tempWgs84PosLat);
        //objObject.insert("Lon",tempWgs84PosLong);
        //std::pair<double, double> latLonPair(tempWgs84PosLat,tempWgs84PosLong);
        //latLonVec.push_back(latLonPair);
//        if(pDB->getItemValue(i, strValue, "140", "ToD"))
//        {
//            trackPoint.set_timeofday(atof(strValue.c_str()));
//            //objObject.insert("ToD",atof(strValue.c_str()));
//        }
        //当日时间，使用接收到的时间
        trackPoint.set_timeofday(time_of_day);

        if(pDB->getItemValue(i, strValue, "170", "CNF"))
        {
            int tempValue = atoi(strValue.c_str());
            trackPoint.set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(tempValue));
            //objObject.insert("CNF",tempValue);
        }

        int tempTrackLastReport = 0;
        if(pDB->getItemValue(i, strValue, "170", "TRE"))
        {
            tempTrackLastReport = atoi(strValue.c_str());
        }
        trackPoint.set_tracklastreport(tempTrackLastReport);
        //objObject.insert("TRE",tempTrackLastReport);

        if(pDB->getItemValue(i, strValue, "170", "CST"))
        {
            int tempValue = atoi(strValue.c_str());;
            trackPoint.set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(tempValue));
            //objObject.insert("CST",tempValue);
        }

        if(pDB->getItemValue(i, strValue, "170", "STH"))
        {
            int tempValue = atoi(strValue.c_str());;
            trackPoint.set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(tempValue));
            //objObject.insert("STH",tempValue);
        }

        if(pDB->getItemValue(i, strValue, "500", "Qx"))
        {
            trackPoint.set_sigmax(atof(strValue.c_str()));
            //objObject.insert("Qx",atof(strValue.c_str()));
        }
        if(pDB->getItemValue(i, strValue, "500", "Qy"))
        {
            trackPoint.set_sigmay(atof(strValue.c_str()));
            //objObject.insert("Qy",atof(strValue.c_str()));
        }
        if(pDB->getItemValue(i, strValue, "500", "Qxy"))
        {
            trackPoint.set_sigmaxy(atof(strValue.c_str()));
            //objObject.insert("Qxy",atof(strValue.c_str()));
        }

        if(pDB->getItemValue(i, strValue, "131", "PAM"))
        {
            trackPoint.set_ampofpriplot(atof(strValue.c_str()));
            //objObject.insert("PAM",atof(strValue.c_str()));
        }

        double tempCartesianTrkVel_vx = 0.0;
        double tempCartesianTrkVel_vy = 0.0;
        double cog = 0.0;
        double sog = 0.0;
        if(pDB->getItemValue(i, strValue, "202", "Vx"))
        {
            tempCartesianTrkVel_vx =  atof(strValue.c_str());
        }
        if(pDB->getItemValue(i, strValue, "202", "Vy"))
        {
            tempCartesianTrkVel_vy =  atof(strValue.c_str());
        }
        trackPoint.set_cartesiantrkvel_vx(tempCartesianTrkVel_vx);
        trackPoint.set_cartesiantrkvel_vy(tempCartesianTrkVel_vy);
        //objObject.insert("Vx",tempCartesianTrkVel_vx);
        //objObject.insert("Vy",tempCartesianTrkVel_vy);
        cog = calCog(tempCartesianTrkVel_vx,tempCartesianTrkVel_vy);
        sog = calSog(tempCartesianTrkVel_vx,tempCartesianTrkVel_vy);
        trackPoint.set_cog(cog);
        trackPoint.set_sog(sog);
        //objObject.insert("Cog",cog);
        //objObject.insert("Sog",sog);
        m_radarPointMap[tempTrackNumber] = trackPoint;


//        qDebug()<<"轨迹点信息如下:"<< "  \n"
//               << "系统区域代码: "<< objObject.value("SAC").toInt() << "  \n"
//               << "系统识别代码 :" << objObject.value("SIC").toInt() << "  \n"
//               << "消息类型  :" << objObject.value("MsgTyp").toInt() << "  \n"
//               << "航迹号  :" << objObject.value("TrkNb").toInt() << "  \n"
//               << "笛卡尔坐标计算X位置 :" << objObject.value("X").toDouble() << " \n"
//               << "笛卡尔坐标计算Y位置 :" << objObject.value("Y").toDouble() << " \n"
//               << "经度 :" << objObject.value("Lat").toDouble()  << " \n"
//               << "纬度 :" << objObject.value("Lon").toDouble() << " \n"
//               << "当日时间 :" << objObject.value("ToD").toDouble() << " \n"
//               << "航迹状态 :" << objObject.value("CNF").toInt() << " \n"
//               << "当前目标最后一次上报 :" << objObject.value("TRE").toInt() << " \n"
//               << "外推法 :" << objObject.value("CST").toInt() << " \n"
//               << "位置来历 :" << objObject.value("STH").toInt() << " \n"
//               << "x轴标准差 :" << objObject.value("Qx").toDouble() << " \n"
//               << "y轴标准差 :" << objObject.value("Qy").toDouble() << " \n"
//               << "2轴平方方差 :" << objObject.value("Qxy").toDouble() << " \n"
//               << "震荡波强度检测 :" << objObject.value("PAM").toDouble() << " \n"
//               << "迪卡尔坐标航迹计算x速度(米/秒) :" << objObject.value("Vx").toDouble() << " \n"
//               << "迪卡尔坐标航迹计算y速度(米/秒) :" << objObject.value("Vy").toDouble() << " \n"
//               << "方位角 :" << objObject.value("Cog").toDouble() << " \n"
//               << "速度 :" << objObject.value("Sog").toDouble() << " \n";

    }
    //m_latLonVec = latLonVec;
    clearRadarTrack();
    sendRadarTrack();
}

void ZCHXRadarDataServer::analysisCat240Radar(DataBlock *pDB, int uLineNum, int uCellNum, int uHeading)
{
    std::string strValue;
    int uNum = pDB->m_lDataRecords.size();
    if(uNum<=0)
    {
        return;
    }
    for(int i=0; i<uNum; i++)
    {

        int messageType = 0;
        if (pDB->getItemValue(i, strValue, "000", "MsgTyp"))
        {
            //ZCHXLOG_INFO("messageType: " << strValue);
            messageType = atoi(strValue.c_str());

        }

        int systemAreaCode = 0 ;
        if(pDB->getItemValue(i, strValue, "010", "SAC"))
        {
            //ZCHXLOG_INFO("systemAreaCode: " << strValue);
            systemAreaCode = atoi(strValue.c_str());
        }

        int systemIdentificationCode = 0;
        if(pDB->getItemValue(i, strValue, "010", "SIC"))
        {
            //ZCHXLOG_INFO("systemIdentificationCode: " << strValue);
            systemIdentificationCode = atoi(strValue.c_str());
        }


        int msgIndex = 0; // 消息唯一序列号
        if(pDB->getItemValue(i, strValue, "020", "MSG_INDEX"))
        {
            //ZCHXLOG_INFO("msgIndex: " << strValue);
            msgIndex = atoi(strValue.c_str());
        }


        int rep = 0;
        if (pDB->getItemValue(i, strValue, "030", "REP"))
        {
            //ZCHXLOG_INFO("rep: " << strValue);
            rep = atoi(strValue.c_str());
        }


        float start_az = 0;  // 方向角起始位置
        if(pDB->getItemValue(i, strValue, "040", "START_AZ"))
        {
            //ZCHXLOG_INFO("方向角起始位置: " << strValue);
            start_az = atof(strValue.c_str());
        }

        float end_az = 0;    // 方向角结束位置
        if(pDB->getItemValue(i, strValue, "040", "END_AZ"))
        {
            //ZCHXLOG_INFO("方向角结束位置: " << strValue);
            end_az = atof(strValue.c_str());
        }

        unsigned long  start_rg = 0 ;  // 开始区域号
        if(pDB->getItemValue(i, strValue, "040", "START_RG"))
        {
            //ZCHXLOG_INFO("start_rg: " << strValue);
            start_rg = atol(strValue.c_str());
        }

        double  cell_dur = 0.0 ;  // 持续时间
        if(pDB->getItemValue(i, strValue, "040", "CELL_DUR"))
        {
            //ZCHXLOG_INFO("cell_dur: " << strValue);
            cell_dur = atof(strValue.c_str());
        }

        int  bit_resolution = 0;  //视频分辨率 默认值为4
        if(pDB->getItemValue(i, strValue, "048", "RES"))
        {
            //ZCHXLOG_INFO("bit_resolution: " << strValue);
            bit_resolution = atoi(strValue.c_str());

        }
        if (bit_resolution ==0)
        {
            bit_resolution = 4;
        }

        int time_of_day = 0 ;
        if(pDB->getItemValue(i, strValue, "140", "ToD"))
        {
            //ZCHXLOG_INFO("time_of_day: " << strValue);
            time_of_day = std::round(atoi(strValue.c_str())/128); // 1/128 s
        }
        if (time_of_day == 0)
        {
            //ZCHXLOG_DEBUG("...... time_of_day == 0....");
//            boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
//            boost::posix_time::time_duration td = now.time_of_day();
//            time_of_day = td.total_seconds();
            time_of_day = QDateTime::currentMSecsSinceEpoch();
        }

        double range_factor = CELL_RANGE(cell_dur);
//        qDebug()<<"扇区频谱信息如下:"<< "  \n"
//               << "系统区域代码: "<< systemAreaCode << "  \n"
//               << "系统识别代码: " << systemIdentificationCode << "  \n"
//               << "消息唯一序列号: " << msgIndex << " \n"
//               << "消息类型: " << messageType << "  \n"
//               <<"消息转发次数: " <<rep<<"\n"
//              << "方向角起始位置: " << start_az << "  \n"
//              << "方向角结束位置: " << end_az << " \n"
//              << "扫描起始距离: " << start_rg << " \n"
//              << "持续时间: " << cell_dur  << " \n"
//              << "距离因子: " << range_factor  << " \n"
//              << "视频分辨率: " << bit_resolution << " \n"
//              << "当日时间: " << time_of_day << " \n"
//                 ;

        int numRadials = end_az - start_az;

        if(pDB->getItemBinary(i, strValue, "052"))
        {

            //ZCHXLOG_DEBUG("config.range_cell: "<< config.range_cell);
            unsigned long int nDestLen = numRadials *uCellNum;

            unsigned char* pDest = new unsigned char[nDestLen];

            unsigned char* pSrcBuf = (unsigned char*)strValue.data()+1;
            unsigned long int  nSrcLen = strValue.length()-1;
            if (0 != pDest)
            {
                int ret = uncompress(pDest, &nDestLen, pSrcBuf, nSrcLen);
                if (ret == Z_OK)
                {
                    // ================调试16进制的内容: 开始=====================
                    //int  body_length = numRadials *config.range_cell*2 +1;
                    //ZCHXLOG_DEBUG("body_length:" << body_length);
                    //char converted[111849];
                    //int i;
                    //for(i=0;i<nDestLen;i++)
                    //{
                    //  sprintf(&converted[i*2], "%02X", pDest[i]);
                    //}
                    //std::string str(converted);
                    //ZCHXLOG_DEBUG( "hex:" <<   str );
                    // ================调试16进制的内容: 结束=====================



                    int position = 0;
                    for (int line = 0; line < numRadials; line++)
                    {

//                        com::zchxlab::radar::protobuf::VideoFrame videoFrame;
//                        videoFrame.set_systemareacode(systemAreaCode);
//                        videoFrame.set_systemidentificationcode(systemIdentificationCode);
//                        videoFrame.set_msgindex(msgIndex);
//                        videoFrame.set_azimuth(start_az+line);
//                        videoFrame.set_startrange(start_rg*range_factor);
//                        videoFrame.set_rangefactor(range_factor);
//                        videoFrame.set_bitresolution(boost::numeric_cast<com::zchxlab::radar::protobuf::RES ,int>(bit_resolution));
//                        videoFrame.set_timeofday(time_of_day);
//                        if (line == 0)
//                            qDebug()<<"start 扫描方位: " << start_az + line;
//                        if (line + 1 == numRadials)
//                            qDebug()<<"end   扫描方位: " << start_az + line;

                        RADAR_VIDEO_DATA objVideoData;
                        objVideoData.m_uSourceID = m_uSourceID;
                        objVideoData.m_uSystemAreaCode = systemAreaCode;
                        objVideoData.m_uSystemIdentificationCode = systemIdentificationCode;
                        objVideoData.m_uMsgIndex = msgIndex;
                        objVideoData.m_uAzimuth = start_az+line;
                        objVideoData.m_dStartRange = start_rg*range_factor;
                        objVideoData.m_dRangeFactor = range_factor;
                        objVideoData.m_uTotalNum = uCellNum;
                        objVideoData.m_dCentreLon = m_dCentreLon;
                        objVideoData.m_dCentreLat = m_dCentreLat;
                        objVideoData.m_uLineNum = uLineNum;
                        objVideoData.m_uHeading = uHeading;
                        QList<int> AmplitudeList;
                        QList<int> pIndexList;
                        AmplitudeList.clear();
                        pIndexList.clear();
                        for (int range = 0; range < uCellNum; range++)
                        {
                            int value =  (int)(pDest[position]);
                            //videoFrame.add_amplitude(pDest[position]);
                            if(value>0)
                            {
                                AmplitudeList.append(value);
                                pIndexList.append(range);
                            }
                            position++;
                        }
                        objVideoData.m_pAmplitude = AmplitudeList;
                        objVideoData.m_pIndex = pIndexList;

                        //半径
                        m_dRadius = objVideoData.m_dStartRange+objVideoData.m_dRangeFactor*uCellNum;

//                        qDebug()<<"AgilTrack 视频帧信息如下:"<< "  \n"
//                        << "系统区域代码: "<< objVideoData.m_uSystemAreaCode << "  \n"
//                        << "系统识别代码: "<< objVideoData.m_uSystemIdentificationCode << "  \n"
//                        << "消息唯一序列号 : "<< objVideoData.m_uMsgIndex << "  \n"
//                        << "扫描方位: "<< objVideoData.m_uAzimuth << "  \n"
//                        << "扫描起始距离: "<< objVideoData.m_dStartRange << "  \n"
//                        << "距离因子 :" << objVideoData.m_dRangeFactor << "  \n"
//                        << "一条线上点个数  :" << objVideoData.m_uTotalNum << "  \n"
//                        << "总共线的个数  :" << objVideoData.m_uLineNum << "  \n"
//                        << "中心纬度  :" << objVideoData.m_dCentreLat << "  \n"
//                        << "中心经度  :" << objVideoData.m_dCentreLon << "  \n"
//                        ;

//                        qint64 utc = QDateTime::currentMSecsSinceEpoch();
//                        QString sContent = "";
//                        sContent+= QString("扫描方位: %1").arg(objVideoData.m_uAzimuth);
//                        sContent+= "  \n";
//                        sContent+= QString("持续时间: %1").arg(cell_dur);
//                        sContent+= "  \n";
//                        sContent+= QString("距离因子: %1").arg(objVideoData.m_dRangeFactor);
//                        sContent+= "  \n";
//                        emit signalSendRecvedContent(utc,"",sContent);


                        m_radarVideoMap[objVideoData.m_uAzimuth] = objVideoData;
                        //序列化
//                        std::string pstring;
//                        videoFrame.SerializeToString(&pstring);
//                        message->push_back(pstring.c_str(), pstring.length());
                    }
                    delete[]pDest;
                    //ZCHXLOG_DEBUG("encode finish:" << " position:" << position );
                }
                else
                {
                    qDebug()<<"uncompress is failed";
                    delete[]pDest;
                    return;
                }
            }
        }
        else if(pDB->getItemBinary(i, strValue, "051"))
        {
            //ZCHXLOG_DEBUG("data is 051");
            unsigned long int nDestLen = numRadials *uCellNum;

            unsigned char* pDest = new unsigned char[nDestLen];

            unsigned char* pSrcBuf = (unsigned char*)strValue.data() + 1;
            unsigned long int  nSrcLen = strValue.length() - 1;
            if (0 != pDest)
            {
                int ret = uncompress(pDest, &nDestLen, pSrcBuf, nSrcLen);
                if (ret == Z_OK)
                {
                    //ZCHXLOG_DEBUG("uncompress ok");
                }
                else
                {
                    //ZCHXLOG_DEBUG("uncompress error");
                }
            }
        }
    }
    processVideoData();
}

void ZCHXRadarDataServer::analysisLowranceRadar(const QByteArray &sRadarData, int uLineNum, int uCellNum, int uHeading)
{

    const char *buf = sRadarData.constData();
    int len = sRadarData.size();
    //qDebug()<<"len:"<<len;
    BR24::Constants::radar_frame_pkt *packet = (BR24::Constants::radar_frame_pkt *)buf;


    //qDebug()<<" packet len:"<<(int)sizeof(packet->frame_hdr);
    if (len < (int)sizeof(packet->frame_hdr)) {
        // The packet is so small it contains no scan_lines, quit!
        qDebug()<<"此包长度不吻合，丢包！";
        return;
    }

    int scanlines_in_packet = (len - sizeof(packet->frame_hdr)) / sizeof(BR24::Constants::radar_line);
    if (scanlines_in_packet != 32) {
        qDebug()<<"此包没有32条扫描线，丢包！";
        return;
    }

    //qDebug()<<"scanlines_in_packet:"<<scanlines_in_packet;
    //int systemAreaCode = config.SAC ;
    //int systemIdentificationCode = config.SIC;


    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    int time_of_day = startDateTime.secsTo(curDateTime);

    //std::vector<std::pair<double, double>> latLonVec;
    //latLonVec.clear();

    QList<int> AmplitudeList;
    QList<int> pIndexList;
    struct SAzmData sAzmData;
    std::list<TrackInfo> trackList;
    for (int scanline = 0; scanline < scanlines_in_packet; scanline++)
    {

        BR24::Constants::radar_line *line = &packet->line[scanline];

        // Validate the spoke
        int spoke = line->common.scan_number[0] | (line->common.scan_number[1] << 8);

        if (line->common.headerLen != 0x18)
        {
            qDebug()<<"strange header length:" << line->common.headerLen;
            qDebug()<<"该"<< scanline << "扫描线头长度不是24字节，丢包！";
            continue;
        }
        //probably status: 02 valid data; 18 spin up
        if (line->common.status != 0x02 && line->common.status != 0x12) {
            qDebug()<<"strange status:" << line->common.status;
            qDebug()<<"该"<< scanline << "扫描线头长度不是24字节，无效！";
        }

        int range_raw = 0;
        int angle_raw = 0;
        short int heading_raw = 0;
        int range_meters = 0;

        heading_raw = (line->common.heading[1] << 8) | line->common.heading[0];

        if (memcmp(line->br24.mark, BR24MARK, sizeof(BR24MARK)) == 0)
        {
            // BR24 and 3G mode
            range_raw = ((line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
            angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
            range_meters = (int)((double)range_raw * 10.0 / sqrt(2.0));

        } else {
            // 4G mode
            short int large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
            short int small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];

            //ZCHXLOG_DEBUG("large_range=" << large_range );
            //ZCHXLOG_DEBUG("small_range=" << small_range );

            angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
            if (large_range == 0x80) {
                if (small_range == -1) {
                    range_raw = 0;  // Invalid range received
                } else {
                    range_raw = small_range;
                }

            } else {
                range_raw = large_range * 256;
            }
            range_meters = range_raw / 4;
        }

        int azimuth_cell = uLineNum;
        bool radar_heading_valid = (((heading_raw) & ~(HEADING_TRUE_FLAG | (azimuth_cell - 1))) == 0);
        double heading;
        if (radar_heading_valid)
        {
            double heading_rotation = (((heading_raw) + 2 * azimuth_cell) % azimuth_cell);
            double heading_degrees = ((heading_rotation) * (double)DEGREES_PER_ROTATION / azimuth_cell);
            heading = (fmod(heading_degrees + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION));
        }else
        {
            //ZCHXLOG_DEBUG("radar_heading_valid=" << radar_heading_valid );
        }


        angle_raw = MOD_ROTATION2048(angle_raw / 2);
        //qDebug()<<"angle_raw:"<<angle_raw;

        double start_range = 0.0 ;
        double range_factor = range_meters/uCellNum;


        AmplitudeList.clear();
        pIndexList.clear();
        double dAzimuth = angle_raw*(360.0/(uLineNum/2))+uHeading;
        for (int range = 0; range < uCellNum; range++)
        {
            int value =  (int)(line->data[range]);
            sAzmData.iRawData[range] = 0;
            if(value>0)
            {
                //qDebug()<<"range"<<range<<"value"<<value;
//                if(m_bLimit)
//                {
//                    if(inLimitArea(m_dCentreLat,m_dCentreLon,dAzimuth,range,start_range,range_factor))
//                    {
//                        AmplitudeList.append(value);
//                        pIndexList.append(range);
//                        sAzmData.iRawData[range] = value;
//                    }
//                }
//                else
                {
                    AmplitudeList.append(value);
                    pIndexList.append(range);
                    sAzmData.iRawData[range] = value;
                }
            }
        }
        RADAR_VIDEO_DATA objVideoData;
        objVideoData.m_uSourceID = m_uSourceID;
        objVideoData.m_uSystemAreaCode = 1;
        objVideoData.m_uSystemIdentificationCode = 1;
        objVideoData.m_uMsgIndex = spoke;
        objVideoData.m_uAzimuth = angle_raw;
        objVideoData.m_dStartRange = start_range;
        objVideoData.m_dRangeFactor = range_factor;
        objVideoData.m_uTotalNum = uCellNum;
        objVideoData.m_dCentreLon = m_dCentreLon;
        objVideoData.m_dCentreLat = m_dCentreLat;
        objVideoData.m_uLineNum = uLineNum/2;
        objVideoData.m_uHeading = uHeading;

        objVideoData.m_pAmplitude = AmplitudeList;
        objVideoData.m_pIndex = pIndexList;
        //半径
        m_dRadius = range_meters;
        m_radarVideoMap[objVideoData.m_uAzimuth] = objVideoData;



//        qint64 utc = QDateTime::currentMSecsSinceEpoch();
//        QString sContent = "";
//        sContent+= QString("扫描方位: %1").arg(angle_raw);
//        sContent+= "  \n";
//        sContent+= QString("扫描起始距离: %1").arg(start_range);
//        sContent+= "  \n";
//        sContent+= QString("距离因子: %1").arg(range_factor);
//        sContent+= "  \n";
//        emit signalSendRecvedContent(utc,"",sContent);

//        qDebug()<<"视频帧信息如下:"<< "  \n"
//            //<< "系统区域代码: "<< systemAreaCode << "  \n"
//            //<< "系统识别代码: "<< systemIdentificationCode << "  \n"
//            << "消息唯一序列号 : "<< spoke << "  \n"
//            << "扫描方位: "<< angle_raw << "  \n"
//            << "扫描起始距离: "<< start_range << "  \n"
//            << "距离因子 :" << range_factor << "  \n"
//        //<< "当日时间  :" << time_of_day << "  \n"
        ;
        // 调用跟踪函数
        int igSit[102400];
        int iTrack = 0;
        int* piSit = &(igSit[2]);

        sAzmData.sHead.iArea = 1;
        sAzmData.sHead.iSys = 1;
        sAzmData.sHead.iMsg = spoke;
        sAzmData.sHead.iAzm = angle_raw;
        sAzmData.sHead.iHead = uHeading;
        sAzmData.sHead.fR0 = start_range;
        sAzmData.sHead.fDR = range_factor;
        sAzmData.sHead.iBit = 4;
        sAzmData.sHead.iTime = time_of_day;


        //qDebug() <<"Tracking_Fun";
        iTrack = Tracking_Fun3(&sAzmData, igSit);
        //qDebug() <<"iTrack"<<iTrack;
        // 跟踪结果写盘
        if (iTrack == 1)
        {
            //qDebug() <<"iTrack"<<iTrack;
            trackList.clear();
            piSit = OutPlot(igSit[1], piSit);
            piSit = OutTrack(igSit[1], piSit, trackList);
            piSit = &(igSit[2]);

            size_t trackLen = trackList.size();
            //qDebug()<<"track size: " << trackLen;
            if (trackLen)
            {
                std::list<TrackInfo>::iterator iter = trackList.begin();
                for (; iter != trackList.end(); iter++)
                {
                    TrackInfo trackInfo = *iter;
                    com::zhichenhaixin::proto::TrackPoint trackPoint;
                    //std::string pstrTrack;
                    trackPoint.set_systemareacode(0);
                    trackPoint.set_systemidentificationcode(1);
                    trackPoint.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
                    trackPoint.set_tracknumber(trackInfo.iTraIndex);

                    double tempWgs84PosLat = 0.0;
                    double tempWgs84PosLong = 0.0;

                    LatLong startLatLong(m_dCentreLon,m_dCentreLat);

                    double t_azm = trackInfo.fAzm * 180 / PI; //(-180,180),正比0'（顺时针增加）

                    if (t_azm<0)
                    {
                        t_azm = 360 + t_azm;
                    }
                    getLatLong(startLatLong, trackInfo.fRng/1000, t_azm, tempWgs84PosLat, tempWgs84PosLong);
                    //getLatLong(startLatLong, trackInfo.fRng / 1000, trackInfo.fAzm * 180 / PI, tempWgs84PosLat, tempWgs84PosLong);

                    //std::pair<double, double> latLonPair(tempWgs84PosLat,tempWgs84PosLong);
                    //latLonVec.push_back(latLonPair);

                    float cartesianposx = 0;
                    float cartesianposy = 0;

                    cartesianposx = trackInfo.fRng*cos(trackInfo.fAzm);
                    cartesianposy = trackInfo.fRng*sin(trackInfo.fAzm);

                    trackPoint.set_cartesianposx(cartesianposx);
                    trackPoint.set_cartesianposy(cartesianposy);

                    trackPoint.set_wgs84poslat(tempWgs84PosLat);
                    trackPoint.set_wgs84poslong(tempWgs84PosLong);
                    trackPoint.set_timeofday(time_of_day);
                    trackPoint.set_tracktype(static_cast<com::zhichenhaixin::proto::CNF>(0));
                    trackPoint.set_tracklastreport(0);
                    trackPoint.set_extrapolation(static_cast<com::zhichenhaixin::proto::CST>(1));
                    trackPoint.set_trackpositioncode(static_cast<com::zhichenhaixin::proto::STH>(1));
                    trackPoint.set_sigmax(0);
                    trackPoint.set_sigmay(0);
                    trackPoint.set_sigmaxy(0);
                    trackPoint.set_ampofpriplot(0);
                    double car_vX = trackInfo.fSpeed* cos(trackInfo.fCourse);;
                    double car_vY = trackInfo.fSpeed* sin(trackInfo.fCourse);
                    trackPoint.set_cartesiantrkvel_vx(car_vX);
                    trackPoint.set_cartesiantrkvel_vy(car_vY);

                    double t_cog = t_azm;
                    trackPoint.set_cog(trackInfo.fCourse*180/PI);
                    trackPoint.set_sog(trackInfo.fSpeed);


//                    qDebug()<<"轨迹点信息如下:"<< "  \n"
//                                  << "系统区域代码: "<< trackPoint.systemareacode() << "  \n"
//                                  << "系统识别代码 :" << trackPoint.systemidentificationcode() << "  \n"
//                                  << "消息类型  :" << trackPoint.messagetype() << "  \n"
//                                  << "航迹号  :" << trackInfo.iTraIndex << "  \n"
//                                  << "笛卡尔坐标计算X位置 :" << cartesianposx << " \n"
//                                  << "笛卡尔坐标计算Y位置 :" << cartesianposy << " \n"
//                                  << "经度 :" << tempWgs84PosLong  << " \n"
//                                  << "纬度 :" << tempWgs84PosLat << " \n"
//                                  << "当日时间 :" << time_of_day << " \n"
//                                  << "航迹状态 :" << trackPoint.tracktype() << " \n"
//                                  << "当前目标最后一次上报 :" << trackPoint.tracklastreport() << " \n"
//                                  << "外推法 :" << trackPoint.extrapolation() << " \n"
//                                  << "位置来历 :" << trackPoint.trackpositioncode() << " \n"
//                                  << "x轴标准差 :" << trackPoint.sigmax() << " \n"
//                                  << "y轴标准差 :" << trackPoint.sigmay() << " \n"
//                                  << "2轴平方方差 :" << trackPoint.sigmaxy() << " \n"
//                                  << "震荡波强度检测 :" << trackPoint.ampofpriplot() << " \n"
//                                  << "迪卡尔坐标航迹计算x速度(米/秒) :" << car_vX << " \n"
//                                  << "迪卡尔坐标航迹计算y速度(米/秒) :" << car_vY << " \n"
//                                  << "方位角 :" << trackInfo.fCourse*180/PI << " \n"
//                                  << "速度 :" << trackInfo.fSpeed << " \n";

                    m_radarPointMap[trackInfo.iTraIndex] = trackPoint;


                }

                trackList.clear();
            }
            //ZCHXLOG_INFO("雷达目标数据组装完成");
        }
        else
        {
            //ZCHXLOG_ERROR("跟踪 video 失败!!!");
        }
    }
    //m_latLonVec = latLonVec;
    //qDebug()<<"轨迹点个数:"<<m_latLonVec.size();
    clearRadarTrack();
    sendRadarTrack();
    processVideoData();

}

void ZCHXRadarDataServer::processVideoData()
{
    //间隔一定时间清理回波数据
//    int nInterval = 60;//60s
//    qint64 curtime = QDateTime::currentMSecsSinceEpoch();

//    if(m_lastClearRadarVideoTime!=0&&((curtime - m_lastClearRadarVideoTime)/1000  > nInterval))
//    {
//        m_radarVideoMap.clear();
//        m_lastClearRadarVideoTime = curtime;
//        return;
//    }
//    if(m_lastClearRadarVideoTime == 0)
//    {
//        m_lastClearRadarVideoTime = curtime;
//    }
    if(m_radarVideoMap.isEmpty())
    {
        return;
    }

//    //使用开线程池
//     ;
//    objThreadPool.setMaxThreadCount(1);
//    if(m_pRunnable == NULL)
//    {
//        m_pRunnable = new ZCHXDrawVideoRunnable();
//        //用直接连接会造成内存泄漏
////        connect(m_pRunnable,SIGNAL(signalRadarVideoPixmap(QPixmap)),
////                            this,SLOT(setRadarVideoPixmap(QPixmap))/*,Qt::BlockingQueuedConnection*/);
//    }
//    Afterglow objAfterglow;
//    objAfterglow.m_RadarVideo = m_radarVideoMap;
//    objAfterglow.m_path = m_latLonVec;
//    m_pRunnable->setAfterglow(objAfterglow);
//    objThreadPool.start(m_pRunnable);


    //使用开线程
    if(m_DrawRadarVideo == NULL)
    {
        m_DrawRadarVideo = new ZCHXDrawRadarVideo();
        //用直接连接会造成内存泄漏
//        connect(m_DrawRadarVideo,SIGNAL(signalRadarVideoPixmap(QPixmap)),
//                            this,SLOT(setRadarVideoPixmap(QPixmap))/*,Qt::BlockingQueuedConnection*/);
        connect(m_DrawRadarVideo,SIGNAL(signalRadarAfterglowPixmap(int,QPixmap,QPixmap,QPixmap)),
                            this,SLOT(setRadarAfterglowPixmap(int,QPixmap,QPixmap,QPixmap)));
    }
    bool bProcessing = m_DrawRadarVideo->getIsProcessing();
    if(bProcessing)
    {
        return;
    }
    Afterglow objAfterglow;
    objAfterglow.m_RadarVideo = m_radarVideoMap;
    objAfterglow.m_path = m_latLonVec;

    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("proces video data to pixmap");
    emit signalSendRecvedContent(utc,"VIDEO_PROCESS",sContent);

    //if(m_uDisplayType == 2)//余辉
    {
        m_DrawRadarVideo->setLimit(m_bLimit);
        m_DrawRadarVideo->setLimitArea(m_landPolygon,m_seaPolygon);
        m_DrawRadarVideo->setDistance(m_distance);
        m_DrawRadarVideo->setAfterglowType(m_uLoopNum);
        m_DrawRadarVideo->signalDrawAfterglow(objAfterglow);
    }
//    if(m_uDisplayType == 1)//回波
//    {
//        //m_DrawRadarVideo->setLimit(true);
//        m_DrawRadarVideo->setLimitArea(m_landPolygon,m_seaPolygon);
//        m_DrawRadarVideo->signalDrawRadarVideo(objAfterglow);
//    }
}

void ZCHXRadarDataServer::sendRadarTrack()
{

    if(m_radarPointMap.size()<=0)
        return;
    //通过zmq发送

    int uNum = m_radarPointMap.size();
    QString sNum = QString::number(uNum);
    QString sIPport = "tcp://*:";
    sIPport += QString::number(m_uTrackSendPort);

    QString sTopic = m_sTrackTopic;
    QByteArray sTopicArray = sTopic.toUtf8();
    QByteArray sTimeArray = QString::number(QDateTime::currentMSecsSinceEpoch()).toUtf8();
    QByteArray sNumArray = sNum.toUtf8();

    zmq_bind(m_pTrackLisher, sIPport.toLatin1().data());//
    zmq_send(m_pTrackLisher, sTopicArray.data(), sTopicArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pTrackLisher, sTimeArray.data(), sTimeArray.size(), ZMQ_SNDMORE);
    zmq_send(m_pTrackLisher, sNumArray.data(), sNumArray.size(), ZMQ_SNDMORE);
    QMap<int,ITF_RadarPoint>::iterator itor = m_radarPointMap.begin();
    std::vector<std::pair<double, double>> latLonVec;
    latLonVec.clear();
    QVector<ITF_RadarPoint> filterRadarPointVec;
    filterRadarPointVec.clear();
    //先过滤
    for(itor;itor!=m_radarPointMap.end();itor++)
    {
        ITF_RadarPoint objRadarPoint = itor.value();

        if(m_bLimit)
        {
            if(!inLimitAreaForTrack(objRadarPoint.wgs84poslat(),objRadarPoint.wgs84poslong()))
            {
                continue;
            }
        }
        std::pair<double, double> latLonPair(objRadarPoint.wgs84poslat(),objRadarPoint.wgs84poslong());
        latLonVec.push_back(latLonPair);

        filterRadarPointVec.append(objRadarPoint);
    }
    int uFilterNum = filterRadarPointVec.size();
    for(int i = 0;i<uFilterNum;i++)
    {
        ITF_RadarPoint objRadarPoint = filterRadarPointVec[i];
        QByteArray sendData;
        sendData.resize(objRadarPoint.ByteSize());
        objRadarPoint.SerializePartialToArray(sendData.data(),sendData.size());
        if(i!=uFilterNum-1)
        {
            zmq_send(m_pTrackLisher, sendData.data(), sendData.size(), ZMQ_SNDMORE);
        }
        else
        {
            zmq_send(m_pTrackLisher, sendData.data(), sendData.size(), 0);
        }
    }
    m_latLonVec = latLonVec;
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("send analysis radar track data,num = %1").arg(uFilterNum);
    emit signalSendRecvedContent(utc,"TRACK________SEND",sContent);

}

void ZCHXRadarDataServer::clearRadarTrack()
{
    //间隔一定时间清理回波数据
    int nInterval = m_clearRadarTrackTime*60;//秒

    QDateTime curDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(QDate(curDateTime.date().year(),curDateTime.date().month(),
                                  curDateTime.date().day()),QTime(0, 0));
    int time_of_day = startDateTime.secsTo(curDateTime);
    QMap<int,ITF_RadarPoint>::iterator itor = m_radarPointMap.begin();
    //qDebug()<<"ITF_RadarPoint num "<<m_radarPointMap.size();
    for(itor;itor!=m_radarPointMap.end();)
    {
        ITF_RadarPoint objRadarPoint = itor.value();
        int uKey = itor.key();
        //qDebug()<<"time_of_day"<<time_of_day;
        //qDebug()<<"objRadarPoint.tracknumber()"<<objRadarPoint.tracknumber();
       // qDebug()<<"objRadarPoint.timeofday()"<<objRadarPoint.timeofday();
        if(time_of_day-objRadarPoint.timeofday()>nInterval)
        {
            //qDebug()<<"remove";
            itor = m_radarPointMap.erase(itor);
        }
        else
        {
            itor++;
        }
    }

}

void ZCHXRadarDataServer::analysisLonLatTOPolygon(const QString sFileName, QList<QPolygonF> &landPolygon, QList<QPolygonF> &seaPolygon)
{
    if(sFileName.isEmpty())
    {
        return;
    }
    landPolygon.clear();
    seaPolygon.clear();
    //qDebug()<<"filepath"<<sFileName;
    QFile objFile(sFileName);
    if(!objFile.open(QIODevice::ReadOnly))
    {
        return;
    }
    QString sAllData = "";
    while (!objFile.atEnd())
    {
        QByteArray LineArray = objFile.readLine();
        QString str(LineArray);
        str = str.trimmed();
        sAllData +=str;
    }
    //qDebug()<<"sAllData"<<sAllData;


    QJsonParseError err;
    QJsonDocument docRcv = QJsonDocument::fromJson(sAllData.toLatin1(), &err);

    if(err.error != QJsonParseError::NoError)
    {
        qDebug()<<"parse completetion list error:"<<err.error;
        return ;
    }
    if(!docRcv.isObject())
    {
        qDebug()<<" status statistics list with wrong format.";
        return ;
    }
    QJsonArray objSeaArray = docRcv.object().value("watercourse").toArray();
    QJsonArray objLandArray = docRcv.object().value("land").toArray();

    QVector<QPointF> pointVec;
    for(int i = 0; i < objSeaArray.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objSeaArray.at(i).toArray();
        //qDebug()<<"objArray.size()"<<objArray.size();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        seaPolygon.append(objPolygon);
    }



    for(int i = 0; i < objLandArray.size(); ++i)
    {
        pointVec.clear();
        QJsonArray objArray = objLandArray.at(i).toArray();
        for(int j = 0; j < objArray.size(); ++j)
        {
            QJsonArray cellAraay = objArray.at(j).toArray();
            double dLon = cellAraay.at(0).toDouble();
            double dLat = cellAraay.at(1).toDouble();
            QPointF pos(dLon,dLat);
            pointVec.append(pos);

        }
        QPolygonF objPolygon(pointVec);
        landPolygon.append(objPolygon);
    }
}

bool ZCHXRadarDataServer::inLimitArea(const double dCentreLat, const double dCentreLon, const double dAzimuth, const int uPosition, const double dStartRange, const double dRangeFactor)
{
    bool bOk = false;

    double dDis = dStartRange+uPosition*dRangeFactor;
    double dLon;
    double dLat;
    distbearTolatlon(dCentreLat,dCentreLon,dDis,dAzimuth,dLat,dLon);
    QPointF pos(dLon,dLat);

    for(int uIndex = 0;uIndex<m_seaPolygon.count();uIndex++)
    {
        const QPolygonF curSeaPolygonF = m_seaPolygon[uIndex];
        if(curSeaPolygonF.containsPoint(pos,Qt::OddEvenFill))
        {
            //return true;
            bool bInLand = false;
            for(int i = 0;i<m_landPolygon.count();i++)
            {
                const QPolygonF curLandPolygonF = m_landPolygon[i];
                if(curLandPolygonF.containsPoint(pos,Qt::OddEvenFill))
                {
                    bInLand = true;
                    break;
                }
            }
            bOk = !bInLand;
            return bOk;
        }
    }
    return false;
}

bool ZCHXRadarDataServer::inLimitAreaForTrack(const double dLat, const double dLon)
{
    bool bOk = false;
    QPointF pos(dLon,dLat);
    if(m_seaPolygon.count()<=0&&m_landPolygon.count()<=0)
    {
        return true;//没有限制区域
    }

    for(int uIndex = 0;uIndex<m_seaPolygon.count();uIndex++)
    {
        const QPolygonF curSeaPolygonF = m_seaPolygon[uIndex];
        if(curSeaPolygonF.containsPoint(pos,Qt::OddEvenFill))
        {
            //return true;
            bool bInLand = false;
            for(int i = 0;i<m_landPolygon.count();i++)
            {
                const QPolygonF curLandPolygonF = m_landPolygon[i];
                if(curLandPolygonF.containsPoint(pos,Qt::OddEvenFill))
                {
                    bInLand = true;
                    break;
                }
            }
            bOk = !bInLand;
            return bOk;
        }
    }
    return false;
}


