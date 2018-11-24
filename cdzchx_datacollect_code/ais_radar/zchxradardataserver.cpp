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
ZCHXRadarDataServer::ZCHXRadarDataServer(ZCHX::Messages::RadarConfig* cfg, QRadarStatusSettingWidget* widget, QObject *parent)
    : QObject(parent),
      mStatusWidget(widget),
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
    qRegisterMetaType<QList<TrackPoint>>("const QList<TrackPoint>&");

    //读取雷达控制的设定值,初始化状态控制
    initStatusWidget();
#if 0
    //初始化心跳
    mHeartObj = new zchxRadarHeartWorker(mRadarConfig->getCmdIP(),
                                         mRadarConfig->getCmdPort(),
                                         mRadarConfig->getHeartTimeInterval() * 1000,
                                         new QThread,
                                         this);
    //初始化雷达控制
    if(mHeartObj->isFine())
    {
        mCtrlObj = new zchxRadarCtrlWorker(mHeartObj, this);
        if(mStatusWidget)
        {
            connect(mStatusWidget, SIGNAL(signalRadarConfigChanged(int,int)), mCtrlObj, SLOT(setCtrValue(int,int)));
        }
    }
#endif
    //初始化雷达参数报告
    if(mRadarConfig->getReportOpen())
    {
        mReportObj = new zchxRadarReportWorker(mRadarConfig->getReportIP(), mRadarConfig->getReportPort(), new QThread);
        if(mStatusWidget)
        {
            connect(mReportObj, SIGNAL(signalRadarStatusChanged(int, int)), mStatusWidget, SLOT(slotValueChangedFromServer(int,int)));
        }
    }

    connect(this,SIGNAL(startProcessSignal()),this,SLOT(startProcessSlot()));
    moveToThread(&m_workThread);
    m_workThread.start();

    //独立线程接收数据
    mDataRecvThread = new VideoDataRecvThread(mRadarConfig->getVideoIP(), mRadarConfig->getVideoPort());
    mVideoWorker = new VideoDataProcessWorker(mRadarConfig);
    connect(mDataRecvThread, SIGNAL(analysisRadar(QByteArray)), mVideoWorker, SLOT(slotRecvVideoRawData(QByteArray)));
    connect(mVideoWorker, SIGNAL(signalSendTrackPoint(QList<TrackPoint>)), this, SIGNAL(signalRecvTrackPoint(QList<TrackPoint>)));
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


}

void ZCHXRadarDataServer::initStatusWidget()
{
    //widget存在的情况下,才解析参数进行显示控制
    if(!mStatusWidget) return;
    QList<RadarStatus> list;
    for(int i=POWER; i<RESVERED; i++)
    {
        QString str_radar_cmd = QString("Radar_Command_%1").arg(sourceID());
        QStringList strlist = Utils::Profiles::instance()->value(str_radar_cmd, RadarStatus::getTypeString(INFOTYPE(i),STR_MODE_ENG)).toStringList();
        if(strlist.length() < 2) return;
        list.append(RadarStatus(INFOTYPE(i), strlist[0].toInt(), strlist[1].toInt()));
    }
    mStatusWidget->setRadarReportSeting(list);
}

void ZCHXRadarDataServer::openRadar()//打开雷达
{
    if(mCtrlObj) mCtrlObj->open();
}

void ZCHXRadarDataServer::closeRadar()//关闭雷达
{
    if(mCtrlObj) mCtrlObj->close();
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
    QString str_radar = QString("Radar_%1").arg(sourceID());
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

    m_pUdpTrackSocket = new QUdpSocket();
    //udp接收(组播形式)
    //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
    //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
    if(!m_pUdpTrackSocket->bind(QHostAddress::AnyIPv4,mRadarConfig->getTrackPort(),QAbstractSocket::ShareAddress))
        qDebug()<<"bind track failed--";

    if(!m_pUdpTrackSocket->joinMulticastGroup(QHostAddress(mRadarConfig->getTrackIP())))
        qDebug()<<"joinMuticastGroup track failed--";
    qDebug()<<"joinMuticastGroup track succeed-- ";
    connect(m_pUdpTrackSocket, SIGNAL(readyRead()),this, SLOT(updateTrackUdpProgress()));
    connect(m_pUdpTrackSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpTrackError(QAbstractSocket::SocketError)));


    if(mHeartObj) mHeartObj->startHeart();
    //改为独立进程
    if(mDataRecvThread)
    {
        LOG_FUNC_DBG<<" start video receive thread";
        mDataRecvThread->start();
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
    if(mCtrlObj) mCtrlObj->setCtrValue(infotype, value);

}






int ZCHXRadarDataServer::sourceID() const
{
    return mRadarConfig->getID();
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
    qDebug()<<"recv track point size:"<<list.size();
    emit signalRecvTrackPoint(list);
}


