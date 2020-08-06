#ifndef ZCHXDATAOUTPUTSERVERMGR_H
#define ZCHXDATAOUTPUTSERVERMGR_H

#include <QObject>
#include <QMap>

class zchxDataOutputServerThread;
class QTimer;

class zchxDataOutputServerMgr : public QObject
{
    Q_OBJECT
public:
    explicit zchxDataOutputServerMgr(QObject *parent = 0);

    ~zchxDataOutputServerMgr();
    void    appendData(const QByteArray& data, const QString& topic, int port);

private:
    zchxDataOutputServerThread* getThread(int port);

signals:    
    void signalSendPortStartStatus(int port,  int sts,  const QString& topic);

private:
    QMap<int, zchxDataOutputServerThread*>          mThreadList;
    QMap<int, QStringList>                          mPortTopicList;
    void*                                           mCtx;
};

class zchxRadarDataOutputMgr : public zchxDataOutputServerMgr
{
    Q_OBJECT
public:
    explicit zchxRadarDataOutputMgr(QObject *parent = 0);

    ~zchxRadarDataOutputMgr();
    void    appendLimitData(const QByteArray& limit) {mLimteData = limit;}

signals:

public slots:
    void    slotSendLimitData();
private:
    QByteArray                                      mLimteData;
    QTimer*                                         mPublicTimer;
};

#endif // ZCHXDATAOUTPUTSERVERMGR_H
