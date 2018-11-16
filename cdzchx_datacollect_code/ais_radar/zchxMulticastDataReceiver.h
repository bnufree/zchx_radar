#ifndef ZCHXMULTICASTDATARECEIVER_H
#define ZCHXMULTICASTDATARECEIVER_H

#include <QObject>
#include <QUdpSocket>

enum DataRecvMode{
    ModeAsyncRecv = 0, //默认是异步接收
    ModeSyncRecv, //同步接收
};

class zchxMulticastDataReceiver : public QObject
{
    Q_OBJECT
public:
    explicit zchxMulticastDataReceiver(const QString& host, int port, const QString& tag, int data_size = 0, int mode = ModeAsyncRecv, QObject *parent = 0);
    QUdpSocket* socket() {return mSocket;}
    void startRecv();
private:
    void init();

signals:
    void signalRecvMulticastData(qint64 utc, const QString& tag, int dataSize);
    void signalSendRecvData(const QByteArray& data);

public slots:
    virtual void slotReadyReadMulticastData();
    virtual void slotDisplayUdpReportError(QAbstractSocket::SocketError);

private:
    QString     mHost;
    int         mPort;
    QString     mTag;           //标记功能
    int         mMode;          //soket工作时是同步进行还是异步进行
    QByteArray  mRecvData;      //接收到的数据
    int         mDataSize;      //每一个数据的大小.如果是0就来多少转发多少.如果不为0,则接收到指定的大小后再转发
    bool        mInit;          //初始化是否完成
    QUdpSocket  *mSocket;
};

#endif // ZCHXMULTICASTDATARECEIVER_H
