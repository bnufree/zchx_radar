#ifndef HAWORKER_H
#define HAWORKER_H

#include <QObject>
#include <QThread>

class HAWorker : public QObject
{
    Q_OBJECT
public:
    explicit HAWorker(QObject *parent = 0);
    ~HAWorker();
    void init();
    QString  getVirtualIp();
    bool     IsFinish();

signals:
    void startSetVirtualIp();
    void startDelVirtualIp();
    void signalSetVipStatus(QString vip, bool val, QString res);
    //void addVirtualIPSuccess();
    void removeVirtualIPSuccess();
    void signalWorkingString(const QString& msg);
public slots:
    void setVirtualIp();
    void delVirtualIp();
private:
    QString     mVirtualIp;
    QString     mSubNet;
    QString     mNetConnectName;
    QThread     mWorkThead;

};

#endif // HAWORKER_H
