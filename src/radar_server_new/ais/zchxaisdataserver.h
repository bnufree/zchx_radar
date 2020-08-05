#ifndef ZCHXAISDATASERVER_H
#define ZCHXAISDATASERVER_H

#include "zchxaisdatacollector.h"
#include <QTcpServer>


class zchxAisDataServer : public zchxAisDataCollector
{
    Q_OBJECT
public:
    explicit zchxAisDataServer(int port, QObject *parent = 0);
    ~zchxAisDataServer();
signals:
public slots:
    void    slotAcceptConnection();
    void    slotDisplayError(QAbstractSocket::SocketError error);
    void    slotStateChanged(QAbstractSocket::SocketState state);
private:
    bool init();
private:
    QTcpServer* mServer;//1_监听套接字
    int         mPort;
};

#endif // ZCHXAISDATASERVER_H
