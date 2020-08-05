#include "zchxaisdataserver.h"
//#include <QDebug>
#include <QTime>

zchxAisDataServer::zchxAisDataServer(int port, QObject *parent) :\
    mServer(0),
    mPort(port),
    zchxAisDataCollector(Ais_Collector_Server, parent)
{
    init();
}

zchxAisDataServer::~zchxAisDataServer()
{
    if(mServer)
    {
        delete mServer;
        mServer = NULL;
    }
}


bool zchxAisDataServer::init()
{
    zchxAisDataCollector::init();
    if(mServer)
    {
        mServer->close();
        delete mServer;
        mServer = 0;
    }
    mServer = new QTcpServer();
    connect(mServer,SIGNAL(newConnection()),this,SLOT(slotAcceptConnection()));
    connect(mServer, SIGNAL(acceptError(QAbstractSocket::SocketError)),
            this, SLOT(slotDisplayError(QAbstractSocket::SocketError)));
    return mServer->listen(QHostAddress::Any, mPort);
}

void zchxAisDataServer::slotDisplayError(QAbstractSocket::SocketError error)
{
    QTcpServer* server = qobject_cast<QTcpServer*>(sender());
    if(!server) return;
    emit signalSendMsg(QString("zchxAisDataServer(Server):%1").arg(server->errorString()));

}

void zchxAisDataServer::slotAcceptConnection()
{
    QTcpServer* server = qobject_cast<QTcpServer*>(sender());
    if(!server) return;
    QTcpSocket* socket = server->nextPendingConnection();
    if(!socket) return;
    connect(socket, SIGNAL(readyRead()), this, SLOT(slotSocketReadyRead()));
}

void zchxAisDataServer::slotStateChanged(QAbstractSocket::SocketState state)
{

}


