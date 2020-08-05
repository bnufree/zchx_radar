#ifndef COMDATAPUBWORKER_H
#define COMDATAPUBWORKER_H

#include<QObject>
#include<QTimer>
#include<QThread>
#include "zmq.h"
#include<ZmqMonitorThread.h>
//#include <QDebug>
//#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

enum    DataOutputMode{
    DataOutputServer = 0,
    DataOutputClient,
};

class ComDataPubWorker : public QObject
{
    Q_OBJECT
public:
    explicit ComDataPubWorker(int mode = DataOutputClient, QObject *parent = 0);
    ~ComDataPubWorker();
    void    setWorkMode(int mode) {mWorkMode = mode;}
    void    setServerIP(const QString& ip){mServerIP = ip;}
    void    setServerPort(int port){mServerPort = port;}
    void    setTimerInterval(int inter);
    void    setTrackSendPort(int port){
        //cout<<"m_uTrackSendPort"<<port;
        m_uTrackSendPort = port;}
    void    setTrackTopic(QString tic){m_sTrackTopic = tic;}
public slots:
    void slotRecvPubData(const QByteArray& content);
    void slotOutputData();
private:
    QByteArray      mContent;
    int             mWorkMode;
    QTimer          *mOutTimer;
    QString         mServerIP;
    int             mServerPort;
    QThread         mWorkThread;
    QByteArray      mTempContent;

    void *m_pTrackContext;
    void *m_pTrackLisher;
    int  m_uTrackSendPort;
    QString m_sTrackTopic;
    ZmqMonitorThread *m_pTrackMonitorThread;
};

#endif // COMDATAPUBWORKER_H
