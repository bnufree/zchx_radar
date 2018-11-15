#include "qdatatcpreceiver.h"
//#include <QTcpSocket>
#include "profiles.h"
#include "Log.h"
#include <QDateTime>
QDataTcpReceiver::QDataTcpReceiver(QObject *parent) : QObject(parent)
{
    mSocket = new QTcpSocket;
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(slotReadContent()));
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotRecvError(QAbstractSocket::SocketError)));
    QString url = Utils::Profiles::instance()->value("TCP", "Server", "").toString();
    int port = Utils::Profiles::instance()->value("TCP", "Port", 12345).toInt();
    if(url.length() > 0 && port > 0)
    {
        mSocket->connectToHost(url, port);
    }
}

QDataTcpReceiver::~QDataTcpReceiver()
{
    if(mSocket)
    {
        mSocket->abort();
        delete mSocket;
    }
}


void QDataTcpReceiver::slotReadContent()
{
    QByteArray bytes = mSocket->readAll();
    LOG(LOG_RTM, "receive content:%s", bytes.data());
    emit signalRecvContent(QDateTime::currentMSecsSinceEpoch(), "TCP", QString::fromLatin1(bytes));
}

void QDataTcpReceiver::slotRecvError(QAbstractSocket::SocketError err)
{
    QString error = mSocket->errorString();
    LOG(LOG_RTM, "receive error from server: %d(%s)", err, error.toStdString().data());
    emit signalRecvContent(QDateTime::currentMSecsSinceEpoch(),"TCP", "error：" + error);
}


