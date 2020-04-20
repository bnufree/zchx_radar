#ifndef ZCHXDATAOUTPUTSERVERTHREAD_H
#define ZCHXDATAOUTPUTSERVERTHREAD_H

#include <QThread>
#include <QMutex>

typedef  QByteArrayList         zchxSendTask;
typedef  QList<zchxSendTask>  zchxSendTaskList;

class ZmqMonitorThread;
class zchxDataOutputServerThread : public QThread
{
    Q_OBJECT
public:
    explicit zchxDataOutputServerThread(void* ctx, int port, bool monitor = false, QObject *parent = 0);
    bool     isOK() const {return mIsOk;}
    int      getPort() const {return mPort;}

    ~zchxDataOutputServerThread();

    void     run();
    void     stopMe() {mStop = true;}
private:
    bool     init();

signals:
    void    signalClientInout(QString,QString,int,int);
public slots:
    void    slotRecvContents(const QByteArray& content, const QString& topic);
private:
    zchxSendTaskList          mSendContentList;
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
