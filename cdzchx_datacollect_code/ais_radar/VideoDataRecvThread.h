#ifndef VIDEODATARECVTHREAD_H
#define VIDEODATARECVTHREAD_H

#include <QThread>
#include "zchxMulticastDataSocket.h"


class VideoDataRecvThread : public zchxMulticastDataScoket
{
    Q_OBJECT
public:
    VideoDataRecvThread(const QString host, int port, QObject * parent = 0);
    void processRecvData(const QByteArray &data);
signals:
    void analysisRadar(const QByteArray&);
    void start();
private slots:
    void run();
private:
    QThread mThread;
};
#endif // VIDEODATARECVTHREAD_H
