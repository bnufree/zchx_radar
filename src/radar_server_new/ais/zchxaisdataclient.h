#ifndef ZCHXAISDATACLIENT_H
#define ZCHXAISDATACLIENT_H

#include "zchxaisdatacollector.h"

class QTcpSocket;


class zchxAisDataClient : public zchxAisDataCollector
{
    Q_OBJECT
public:
    explicit zchxAisDataClient(const QString& host, int port, int time_out_secs, QObject *parent = 0);
    virtual bool init();

signals:

public slots:
    void    slotTimeoutChk();
    virtual void    slotDisplayError(QAbstractSocket::SocketError error);
    virtual void    slotStateChanged(QAbstractSocket::SocketState state);


private:
    void    deleteSocket();
private:
    QString     mHost;
    int         mPort;
    int         mTimeOutSecs;
    QTcpSocket  *mSocket;
};

#endif // ZCHXAISDATACLIENT_H
