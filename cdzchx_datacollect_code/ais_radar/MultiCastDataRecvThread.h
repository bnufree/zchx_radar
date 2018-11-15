#ifndef MULTICASTDATARECVTHREAD_H
#define MULTICASTDATARECVTHREAD_H

#include <QThread>
#include <QUdpSocket>

class MultiCastDataRecvThread : public QThread
{
    Q_OBJECT
public:
    MultiCastDataRecvThread(QString a = "1",int b = 0,QString c = "2",int d = 1,int e = 1,int f = 1,QObject * parent = 0);
    void run();
    void doSomething();
private slots:
    //video
    void displayUdpVideoError(QAbstractSocket::SocketError error);
    void updateVideoUdpProgress();
signals:
    void analysisRadar(const QByteArray ,const QString , int ,int ,int );
    void u_signalSendRecvedContent(qint64 time, const QString& name, const QString& content);
private:
    QUdpSocket *m_pUdpVideoSocket;
    QString     m_sVideoIP;
    int         m_uVideoPort;

    QString     m_sRadarVideoType;//cat010-新科，Lowrance-小雷达
    int  m_uCellNum;//一条线上多少个点
    int  m_uLineNum;//一圈多少条线
    int  m_uHeading;//雷达方位
    QByteArray      mRecvContent;
};
#endif // MULTICASTDATARECVTHREAD_H
