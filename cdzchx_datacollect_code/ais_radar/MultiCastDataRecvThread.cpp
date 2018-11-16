#include "MultiCastDataRecvThread.h"
#include <QDebug>
#include "BR24.hpp"
#include <QDateTime>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

MultiCastDataRecvThread::MultiCastDataRecvThread(QString a,int b,QString c,int d,int e,int f,QObject * parent)
: m_sVideoIP(a),m_uVideoPort(b),m_sRadarVideoType(c),m_uCellNum(d),m_uLineNum(e),m_uHeading(f),QThread(parent)
{

}

void MultiCastDataRecvThread::run()
{
     qDebug()<<"进入线程"<<"thread id :"<<QThread::currentThreadId();;
     cout<<"m_sVideoIP:"<<m_sVideoIP;
     cout<<"m_uVideoPort:"<<m_uVideoPort;
     cout<<"m_sRadarVideoType:"<<m_sRadarVideoType;
     cout<<"m_uCellNum:"<<m_uCellNum;
     cout<<"m_uLineNum:"<<m_uLineNum;
     cout<<"m_uCellNum"<<m_uCellNum;

     //开始接收
     m_pUdpVideoSocket = new QUdpSocket();
     //udp接收(组播形式)
     //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
     //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
     if(!m_pUdpVideoSocket->bind(QHostAddress::AnyIPv4,m_uVideoPort,QAbstractSocket::ShareAddress))
         qDebug()<<"bind video failed--";


     if(!m_pUdpVideoSocket->joinMulticastGroup(QHostAddress(m_sVideoIP)))
         qDebug()<<"joinMuticastGroup video failed--";
     qDebug()<<"joinMuticastGroup video succeed-- ";
     cout<<m_pUdpVideoSocket;

     //connect(m_pUdpVideoSocket, SIGNAL(readyRead()), this, SLOT(updateVideoUdpProgress())); //由于是线程,信号槽慎用
     cout<<"m_pUdpVideoSocket大小:"<<m_pUdpVideoSocket->size();
     connect(m_pUdpVideoSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpVideoError(QAbstractSocket::SocketError)));


     while (m_pUdpVideoSocket->size()>=0) {
         if(m_pUdpVideoSocket->hasPendingDatagrams()) {
             //1.	接收数据部分改为线程方式， 避免使用信号槽
             updateVideoUdpProgress();
         }
         //qDebug()<<m_pUdpVideoSocket->error()<<m_pUdpVideoSocket->errorString();
     }
}

//  改为独立线程
void MultiCastDataRecvThread::displayUdpVideoError(QAbstractSocket::SocketError error)
{
    if(m_pUdpVideoSocket == NULL)
    {
        qDebug()<<"ZCHXRadarDataUDPServer:"<<m_pUdpVideoSocket->errorString();
        cout << "0";
        return;
    }
    qDebug()<<"ZCHXRadarDataUDPServer:"<<m_pUdpVideoSocket->errorString();
    cout << "错误";

}

void MultiCastDataRecvThread::updateVideoUdpProgress()
{
//    qDebug()<<"!!!!!!!!!!!!!!!!!!!!!!";
    if(m_pUdpVideoSocket == NULL)
    {
        return;
    }
    //cout<<"接收到数据了";
    //qDebug()<<"updateVideoUdpProgress thread id :"<<QThread::currentThreadId();
    QByteArray datagram;
    // 让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
    datagram.resize(m_pUdpVideoSocket->pendingDatagramSize());//pendingDatagramSize() 当前数据包大小
    // 接收数据报，将其存放到datagram中
    // read all data and split data by the frame_pkt size
    m_pUdpVideoSocket->readDatagram(datagram.data(), datagram.size());
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar video data,size of bytes = %1").arg(datagram.size());
    emit u_signalSendRecvedContent(utc,"VIDEO_RECEIVE",sContent);
    //data preprocess
    mRecvContent.append(datagram);
    int target_size = sizeof(BR24::Constants::radar_frame_pkt);
//    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss zzz")<<"recv:"<<mRecvContent.size()<<" target:"<<target_size;
    if(mRecvContent.size() >= target_size)
    {
        QByteArray target = mRecvContent.mid(0, target_size);
        emit analysisRadar(target,m_sRadarVideoType,m_uLineNum,m_uCellNum,m_uHeading);
        mRecvContent = mRecvContent.right(mRecvContent.size() - target_size);
    }

}
