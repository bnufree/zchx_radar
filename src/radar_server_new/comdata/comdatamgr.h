#ifndef COMDATAMGR_H
#define COMDATAMGR_H

#include <QObject>
#include <QThread>
#include "comdefines.h"
#include "comdataworker.h"
#include "comparser.h"

class ComDataMgr : public QObject
{
    Q_OBJECT
public:
    explicit ComDataMgr(QObject *parent = 0);
    ~ComDataMgr();
    QString comCmd2String(const QByteArray& array);
    void    setComDevParams(const QMap<QString, COMDEVPARAM>& map);
    void    stopComWork(ComDataWorker *data);
    void    start();
    void    stop();

signals:
    void    signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void    signalSendLogMsg(const QString& msg);
public slots:

    void    slotRecvComData(const QString& comName, const QString& topic, qint64 time, const QByteArray& recv);

private:
    QThread                         mWorkThread;
    QMap<QString, ComDataWorker*>  mStartedComList;
    QMap<QString, COMDEVPARAM>      mDevComParamsMap;
    ComParser*                      mParser;

};

#endif // COMDATAMGR_H
