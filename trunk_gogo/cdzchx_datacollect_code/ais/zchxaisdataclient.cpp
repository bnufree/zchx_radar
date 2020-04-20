#include "zchxaisdataclient.h"
#include <QTcpSocket>
#include <QTimer>
#include <QDateTime>

zchxAisDataClient::zchxAisDataClient(const QString& host, int port, int time_out_secs, QObject *parent)
    : zchxAisDataCollector(Ais_Collector_Client, parent)
    , mHost(host)
    , mPort(port)
    , mTimeOutSecs(time_out_secs)
    , mSocket(0)
{
    if(mTimeOutSecs > 0)
    {
        QTimer *timer = new QTimer(this);
        timer->setInterval(60 * 1000);
        connect(timer, SIGNAL(timeout()), this, SLOT(slotTimeoutChk()), Qt::DirectConnection);
        timer->start();
    }
    init();
}

void zchxAisDataClient::deleteSocket()
{
    if(!mSocket) return;
    mSocket->abort();
    delete mSocket;
    mSocket = 0;
}

bool zchxAisDataClient::init()
{
    zchxAisDataCollector::init();
    deleteSocket();
    mSocket = new QTcpSocket();
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(slotSocketReadyRead()));
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(slotDisplayError(QAbstractSocket::SocketError)));
    connect(mSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(slotStateChanged(QAbstractSocket::SocketState)));
    mSocket->connectToHost(mHost, mPort);
    return true;
}

void zchxAisDataClient::slotTimeoutChk()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if((now - mLastRecvDataTime) >= mTimeOutSecs * 1000)
    {
        //重新连接到host
        init();
    }
}


void zchxAisDataClient::slotDisplayError(QAbstractSocket::SocketError error)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if(!socket) return;

    emit signalSendMsg(QString("ZCHXAisDataCollector(Client):%1").arg(socket->errorString()));
}


void zchxAisDataClient::slotStateChanged(QAbstractSocket::SocketState state)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if(!socket) return;

    if(state == QAbstractSocket::SocketState::ConnectedState)
    {
        emit signalSendMsg(QString("ZCHXAisDataCollector(Client): connect server success."));
    } else if(state == QAbstractSocket::SocketState::UnconnectedState)
    {
        emit signalSendMsg(QString("ZCHXAisDataCollector(Client): disconnect server"));
    }
}

