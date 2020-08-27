#include "msgserver.h"
#include "profiles.h"

MsgServer::MsgServer(QObject *parent) : QObject(parent)
  , mServer(0)
  , mPort(0)
{
    mPort = PROFILES_INSTANCE->value("General", "Port").toInt();
    if(mPort > 0)
    {
        mServer = new QTcpServer(this);
    }
    bool  isOk = false;
    if(mServer)
    {
        connect(mServer, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));
        connect(mServer, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(slotAcceptError(QAbstractSocket::SocketError)));
        isOk = mServer->listen(QHostAddress::Any, mPort);
    }
    if(!isOk)
    {
        qDebug()<<"start msg server failed."<<mPort;
    }

}

void MsgServer::slotAcceptError(QAbstractSocket::SocketError socketError)
{
    qDebug()<<"receive error now:"<<socketError;
    QTcpServer* server = qobject_cast<QTcpServer*> (sender());
    if(server)
    {
        qDebug()<<"server reply error string:"<<server->errorString();
    }

}

void MsgServer::slotNewConnection()
{
    QTcpSocket *socket = mServer->nextPendingConnection();
    if(!socket) return;
    connect(socket, SIGNAL(readyRead()), this, SLOT(slotReadClientContent()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(slotClientDisconnect()));
    if(!mClientList.contains(socket))
    {
        //给客户端发送服务器的基本参数信息
        emit signalSendServerParamMsgToSocket(socket);
        mClientList.append(socket);
    }
}

void MsgServer::slotReadClientContent()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if(!socket) return;
    QByteArray recv = socket->readAll();
}

void MsgServer::slotClientDisconnect()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if(!socket) return;
    if(mClientList.contains(socket))
    {
        mClientList.removeOne(socket);
        socket->deleteLater();
    }
}
