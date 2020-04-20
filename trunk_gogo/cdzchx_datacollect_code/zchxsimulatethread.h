#ifndef ZCHXSIMULATETHREAD_H
#define ZCHXSIMULATETHREAD_H

#include <QThread>

class zchxSimulateThread : public QThread
{
    Q_OBJECT
public:
    explicit zchxSimulateThread(const QString& dirName,  int default_len = -1, QObject *parent = 0);
    void setCancel(bool sts) {mCancelFlg = sts;}

protected:
    void run();

signals:
    void signalSendContents(const QByteArray& bytes, int len);
public slots:

private:
    QString         mDirName;
    int             mDefaultBytesLen;
    bool            mCancelFlg;
};

#endif // ZCHXSIMULATETHREAD_H
