#include "webserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QDataStream>

WebServer::WebServer(int port, QObject* parent):QWebSocketServer(tr("SCCMMS_WebSocketServer"),NonSecureMode, parent)
{
    mPort = port;
}

bool WebServer::startListen()
{
    connect(this, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));
    bool sts = this->listen(QHostAddress::Any, mPort);

    emit signalListenStatus(tr("listen as web server %1 at port %2").arg(sts == true? "success" : "failed").arg(mPort));
    return sts;
}

void WebServer::slotNewConnection()
{
    QWebSocket* socket = this->nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), this, SLOT(slotConnectionLeft()));
    if(!mClientList.contains(socket))
    {
        mClientList.append(socket);
    }

    QString num("25[0-5]{1}|2[0-4]{1}[0-9]{1}|1[0-9]{2}|[1-9]{1}[0-9]{1}|[0-9]{1}");
    QRegExp reg(QString("(%1).(%2).(%3).(%4)").arg(num).arg(num).arg(num).arg(num));
    int index = reg.indexIn(socket->peerAddress().toString(), 0);
    QString ip = reg.cap();
    if(ip.length() == 0)
    {
        ip = socket->peerAddress().toString();
    }

    emit signalConnectionStatus(ip, socket->peerName(), socket->peerPort(), 1);

    //向新连接的客户端推送信息
    foreach (QString topic, mRecvInfos.keys()) {
        socket->sendTextMessage(mRecvInfos[topic]);
    }

}

void WebServer::slotConnectionLeft()
{
    QWebSocket* socket = static_cast<QWebSocket*>(sender());
    emit signalConnectionStatus(socket->peerAddress().toString(), socket->peerName(), socket->peerPort(), 0);
    mClientList.removeAll(socket);
    socket->deleteLater();
}

void WebServer::slotReceiveUploadProject(const QString &topic, const QString &info)
{
    //qDebug()<<"topic:"<<topic<<"   value:"<<info;
    mRecvInfos[topic] = info;

    //发送给各个client
    foreach (QWebSocket *socket, mClientList) {
        //socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
        socket->sendTextMessage(info);
    }
}
