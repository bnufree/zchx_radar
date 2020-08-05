#ifndef VIDEODATARECVTHREAD_H
#define VIDEODATARECVTHREAD_H

#include <QThread>
#include "zchxMulticastDataSocket.h"


class zchxVideoDataRecvThread : public zchxMulticastDataScoket
{
    Q_OBJECT
public:
    zchxVideoDataRecvThread(const QString host, int port, int size, const QString& fakePath = QString());
    void processRecvData(const QByteArray &data);
    void startRecv();
signals:
    void signalSendRecvData(const QByteArray&);
    void start();
private slots:
    void run();
private:
    QThread mThread;
    bool    mFake;
    QString mFakePath;
};
#endif // VIDEODATARECVTHREAD_H
