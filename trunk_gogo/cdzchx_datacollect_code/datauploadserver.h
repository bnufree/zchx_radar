#ifndef DATAUPLOADSERVER_H
#define DATAUPLOADSERVER_H

#include <QThread>
#include "systemconfigsettingdefines.h"


class DataUploadServer : public QThread
{
    Q_OBJECT
public:
    explicit DataUploadServer(void* ctx = 0, QObject *parent = 0);
protected:
    void run();


signals:
    void signalConstructionCommand(const QByteArray& topic, const QByteArray& content);
    void signalSendLogMsg(const QString& msg);
    void signalSendUploadProject(const QString& topic, const QString& info);
    void signalClientInOut(const QString& ip, const QString& name, int port, int inout);
    void signalSetupSimulationData();
    void signalSendDBInfoUpdated(const QByteArray& topic, const QByteArray& content);
    void signalSendDpUploadMsg(const QByteArray& msg);

public slots:
    void slotSendDpMsg(const QString& msg, QByteArray& ret);    
    void setUploadDPCom(const COMDEVPARAM& dev);
private:
    int         mDpBandRate;
    QString     mDpComName;
    bool        mDpInitFlag;

    void        *mCtx;
};

#endif // DATAUPLOADSERVER_H
