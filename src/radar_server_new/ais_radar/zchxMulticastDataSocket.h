#ifndef ZCHXMULTICASTDATASOCKET_H
#define ZCHXMULTICASTDATASOCKET_H

#include <QObject>
#include <QUdpSocket>

enum DataRecvMode{
    ModeAsyncRecv = 0, //默认是异步接收
    ModeSyncRecv, //同步接收
};

class zchxMulticastDataScoket : public QObject
{
    Q_OBJECT
public:
    explicit zchxMulticastDataScoket(const QString& host,
                                     int port,
                                     const QString& tag,
                                     bool join,
                                     int data_size = 0,
                                     int mode = ModeAsyncRecv,
                                     QObject *parent = 0);
    QUdpSocket* socket() {return mSocket;}
    void startRecv(); //同步接收处理
    virtual void processRecvData(const QByteArray& data);
    bool isFine() const {return mInit;}
private:
    void init();

signals:
    void signalRecvMulticastData(qint64 utc, const QString& tag, const QString& dataSize);
    void signalSendRecvData(const QByteArray& data);
public slots:
    virtual void slotReadyReadMulticastData();
    virtual void slotDisplayUdpReportError(QAbstractSocket::SocketError);
    void    slotWriteData(const QByteArray& data);

private:
    QString     mHost;
    int         mPort;
    QString     mTag;           //标记功能
    int         mMode;          //soket工作时是同步进行还是异步进行
    QByteArray  mRecvData;      //接收到的数据
    int         mDataSize;      //每一个数据的大小.如果是0就来多少转发多少.如果不为0,则接收到指定的大小后再转发
    bool        mInit;          //初始化是否完成
    bool        mIsJoin;        //是否加入组播
    QUdpSocket  *mSocket;
};

#endif // ZCHXMULTICASTDATASOCKET_H
