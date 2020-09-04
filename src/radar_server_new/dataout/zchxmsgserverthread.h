#ifndef ZCHXMSGSERVERTHREAD_H
#define ZCHXMSGSERVERTHREAD_H

#include <QThread>
#include <QMutex>
#include "zchxmsgcommon.h"

class ZmqMonitorThread;

class zchxMsgServerThread : public QThread
{
    Q_OBJECT
public:
    explicit zchxMsgServerThread(void* ctx, int port, bool monitor = false, QObject *parent = 0);
    bool     isOK() const {return mIsOk;}
    int      getPort() const {return mPort;}

    ~zchxMsgServerThread();

    void     run();
    void     stopMe() {mStop = true;}
private:
    bool     init();
    bool     processFilterAreaMsg(int cmd, const QJsonValue& val);

signals:
    void    signalClientInout(QString,QString,int,int);
public slots:
    void    slotRecvContents(const QByteArray& content, const QString& topic);
private:
    int                     mPort;
    void                    *mCtx;
    bool                    mIsOk;
    QMutex                  mMutex;
    bool                    mStop;
    void                    *mSocket;
    QString                 mUrl;
    ZmqMonitorThread        *mMonitorThread;
    bool                    mMonitorClient;
};

#endif // ZCHXDATAOUTPUTSERVERTHREAD_H
