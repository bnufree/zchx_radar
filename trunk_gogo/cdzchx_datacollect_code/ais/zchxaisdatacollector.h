#ifndef ZCHXAISDATACOLLECTOR_H
#define ZCHXAISDATACOLLECTOR_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>

enum Ais_Collector_Mode
{
    Ais_Collector_Client = 0,
    Ais_Collector_Server,
};

class zchxAisDataCollector : public QObject
{
    Q_OBJECT
public:
    explicit zchxAisDataCollector(Ais_Collector_Mode mode, QObject *parent = 0);
    virtual bool init();
    virtual ~zchxAisDataCollector();
    int     type() const {return mType;}

signals:
    void    signalSendAisData(const QByteArray& bytes);
    void    signalSendMsg(const QString& msg);
public slots:
    virtual void    slotSocketReadyRead();
    virtual void    slotDisplayError(QAbstractSocket::SocketError error);
    virtual void    slotStateChanged(QAbstractSocket::SocketState state);


protected:
    qint64          mLastRecvDataTime;
    QThread         mWorkThread;
    int             mType;
};

#endif // ZCHXAISDATACOLLECTOR_H
