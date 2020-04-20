#ifndef ZCHXMAPMONITORTHREAD_H
#define ZCHXMAPMONITORTHREAD_H

#include <QThread>
#include <QProcess>

class zchxMapMonitorThread : public QThread
{
    Q_OBJECT
public:
    explicit zchxMapMonitorThread(const QString& app, QObject *parent = 0);
    void run();
    ~zchxMapMonitorThread();
    void runApp();

signals:

public slots:
private:
    QString mAppName;
    qint64  mAppID;
    QString mDogFile;
    QProcess*  mSub;
};

#endif // ZCHXMAPMONITORTHREAD_H
