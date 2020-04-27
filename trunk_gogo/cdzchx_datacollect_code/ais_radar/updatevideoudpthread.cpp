#include "updatevideoudpthread.h"
//#include <QDebug>
#include "BR24.hpp"
#include <QNetworkInterface>
#include <QDir>
#include <QByteArray>
#include <QMessageBox>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QList>
#include <QNetworkAddressEntry>
#include "profiles.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

updatevideoudpthread::updatevideoudpthread(QString a,int b,QString c,int d,int e,int f,QObject * parent)
    : m_sVideoIP(a),
      m_uVideoPort(b),
      m_sRadarVideoType(c),
      m_uCellNum(d),
      m_uLineNum(e),
      m_uHeading(f),
      QThread(parent)
{
    //prt = false;//初始化打印标志
    //开始接收
    m_pUdpVideoSocket = NULL;

}

updatevideoudpthread::~updatevideoudpthread()
{
    m_pUdpVideoSocket->deleteLater();
}
void updatevideoudpthread::run()
{
     m_pUdpVideoSocket = new QUdpSocket();
     cout<<"进入线程"<<"thread id :"<<QThread::currentThreadId();
     cout<<"m_sVideoIP:"<<m_sVideoIP;
     cout<<"m_uVideoPort:"<<m_uVideoPort;
     cout<<"m_sRadarVideoType:"<<m_sRadarVideoType;
     cout<<"m_uCellNum:"<<m_uCellNum;
     cout<<"m_uLineNum:"<<m_uLineNum;
     cout<<"m_uCellNum"<<m_uCellNum;


     //此处的bind连接端口，采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
     //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)

     if(!m_pUdpVideoSocket->bind(QHostAddress::AnyIPv4,m_uVideoPort,QAbstractSocket::ShareAddress ))
         cout<<"bind video failed--";

     if(!m_pUdpVideoSocket->joinMulticastGroup(QHostAddress(m_sVideoIP)))
     {
         //emit joinGropsignal("f");
         cout<<"joinMuticastGroup video failed--";
     }
     else
     {
         //emit joinGropsignal("s");
         cout<<"joinMuticastGroup video succeed-- ";
     }
     cout<<m_pUdpVideoSocket;

     //connect(m_pUdpVideoSocket, SIGNAL(readyRead()), this, SLOT(updateVideoUdpProgress())); //由于是线程,信号槽慎用
     cout<<"m_pUdpVideoSocket大小:"<<m_pUdpVideoSocket->size();
     connect(m_pUdpVideoSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayUdpVideoError(QAbstractSocket::SocketError)));
     connect(m_pUdpVideoSocket,SIGNAL(readyRead()),this,SLOT(updateVideoUdpProgress()));
//     while (m_pUdpVideoSocket->size()>=0) {
//         if(m_pUdpVideoSocket->hasPendingDatagrams()) {
//             //1.	接收数据部分改为线程方式， 避免使用信号槽
//             updateVideoUdpProgress();
//         }
//     }
     exec();
}

//  改为独立线程
void updatevideoudpthread::displayUdpVideoError(QAbstractSocket::SocketError error)
{
    if(m_pUdpVideoSocket == NULL)
    {
        cout<<"ZCHXRadarDataUDPServer:"<<m_pUdpVideoSocket->errorString();
        cout << "0";
        return;
    }
    cout<<"ZCHXRadarDataUDPServer:"<<m_pUdpVideoSocket->errorString();
    cout << "错误";

}

void updatevideoudpthread::updateVideoUdpProgress()
{
    if(m_pUdpVideoSocket == NULL) {
        return;
        cout<<"m_pUdpVideoSocket为空了";
    }
    //cout<<"接收到数据了";
    //cout<<"updateVideoUdpProgress thread id :"<<QThread::currentThreadId();

    QByteArray datagram;
    // 让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
    datagram.resize(m_pUdpVideoSocket->pendingDatagramSize());//pendingDatagramSize() 当前数据包大小
    // 接收数据报，将其存放到datagram中
    //m_pUdpVideoSocket->readDatagram(datagram.data(), sizeof(BR24::Constants::radar_frame_pkt));//2.	接收数据包大小， 改为固定大小，为radar_frame_pkt结构体大小
    m_pUdpVideoSocket->readDatagram(datagram.data(), datagram.size());//readDatagram将不大于指定长度的数据保存到datagram.data()

    //cout<<"回波原始数据大小:"<<datagram.size();
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar video data,size of bytes = %1").arg(datagram.size());
    emit u_signalSendRecvedContent(utc,"VIDEO_RECEIVE",sContent);

    analysisRadar(datagram,m_sRadarVideoType,m_uLineNum,m_uCellNum,m_uHeading);
}
