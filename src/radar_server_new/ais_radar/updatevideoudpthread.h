#ifndef UPDATEVIDEOUDPTHREAD_H
#define UPDATEVIDEOUDPTHREAD_H

#include <QThread>
#include <QMap>
#include <QDateTime>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QList>
#include <QPixmap>
#include "zmq.h"
#include "zchxdrawvideorunnable.h"
#include <QThread>
#include "zchxfunction.h"
#include "radarccontroldefines.h"
#include "updatevideoudpthread.h"
#include "zchxradarcommon.h"




class updatevideoudpthread : public QThread
{
    Q_OBJECT
public:
    updatevideoudpthread(QString a = "1",int b = 0,QString c = "2",int d = 1,int e = 1,int f = 1,QObject * parent = 0);
    ~updatevideoudpthread();
    void run();
    void doSomething();
    //bool prt;
private slots:
    //video
    void displayUdpVideoError(QAbstractSocket::SocketError error);
    void updateVideoUdpProgress();
signals:
    void analysisRadar(const QByteArray ,const QString , int ,int ,int );
    void u_signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
    void joinGropsignal(QString);
private:
    QUdpSocket *m_pUdpVideoSocket;
    QString m_sVideoIP;
    int  m_uVideoPort;

    QString m_sRadarVideoType;//cat010-新科，Lowrance-小雷达
    int  m_uCellNum;//一条线上多少个点
    int  m_uLineNum;//一圈多少条线
    int  m_uHeading;//雷达方位

};
#endif // UPDATEVIDEOUDPTHREAD_H
