#include "zchxMulticastDataReceiver.h"
#include "common.h"

zchxMulticastDataReceiver::zchxMulticastDataReceiver(const QString& host, int port, const QString& tag, int data_size, int mode, QObject *parent)
    : QObject(parent),
      mHost(host),
      mPort(port),
      mTag(tag),
      mDataSize(data_size),
      mMode(mode),
      mSocket(Q_NULLPTR),
      mInit(false)
{
    init();
}

void zchxMulticastDataReceiver::init()
{
    if(mHost.length() == 0 || mPort == 0) return;
    mSocket = new QUdpSocket();

    if(!mSocket->bind(QHostAddress::AnyIPv4, mPort,QAbstractSocket::ShareAddress))
    {
        LOG_FUNC_DBG<<"bind port failed:"<<mPort;
        return;
    }

    mSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);//禁止本机接收
    if(!mSocket->joinMulticastGroup(QHostAddress(mHost)))
    {
         LOG_FUNC_DBG<<"joinMuticastGroup host failed:"<<mHost;
         return;
    }
    if(mMode == ModeAsyncRecv)
    {
        connect(mSocket, SIGNAL(readyRead()),this, SLOT(slotReadyReadMulticastData()));
        connect(mSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(slotDisplayUdpReportError(QAbstractSocket::SocketError)));
    }

    mInit = true;

    LOG_FUNC_DBG<<"init multicast succeed."<<mHost<<":"<<mPort;

    return;
}

void zchxMulticastDataReceiver::slotReadyReadMulticastData()
{
    if(!mInit || !mSocket) return;

    QByteArray datagram;
    // 让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
    datagram.resize(mSocket->pendingDatagramSize());//pendingDatagramSize() 当前数据包大小
    // 接收数据报，将其存放到datagram中
    mSocket->readDatagram(datagram.data(), datagram.size());//readDatagram将不大于指定长度的数据保存到datagram.data()
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    QString sContent = tr("receive radar report data,size = %1").arg(datagram.size());
    emit signalSendRecvedContent(utc,"REPORT_RECEIVE",sContent)
}

void zchxMulticastDataReceiver::startRecv()
{
    if((!mInit) || mMode == ModeAsyncRecv) return;
    while(1){
        if(mSocket->hasPendingDatagrams())
        {
            slotReadyReadMulticastData();
        }
    }
}
