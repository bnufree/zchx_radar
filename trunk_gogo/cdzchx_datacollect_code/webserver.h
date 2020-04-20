#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QWebSocket>
#include <QWebSocketServer>
#include <QString>


class WebServer : public QWebSocketServer
{
    Q_OBJECT
public:
    WebServer(int port, QObject* parent = Q_NULLPTR);
    bool startListen();

signals:
    void signalConnectionStatus(const QString& ip, const QString& name, int port, int inout);
    void signalListenStatus(const QString& msg);
public slots:
    void slotReceiveUploadProject(const QString& topic, const QString& info);
    void slotNewConnection();
    void slotConnectionLeft();
private:
    QList<QWebSocket*>  mClientList;
    int             mPort;
    QMap<QString, QString>  mRecvInfos;         //保存已经收到的信息，便于向新连接的客户端推送消息
};

#endif // WEBSERVER_H
