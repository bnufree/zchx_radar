#ifndef QDATATCPRECEIVER_H
#define QDATATCPRECEIVER_H

#include <QObject>
#include <QTcpSocket>;

class QDataTcpReceiver : public QObject
{
    Q_OBJECT
public:
    explicit QDataTcpReceiver(QObject *parent = 0);
    ~QDataTcpReceiver();
signals:
    void signalRecvContent(qint64 time, const QString& topic, const QString& content);
public slots:
    void slotReadContent();
    void slotRecvError(QAbstractSocket::SocketError err);
private:
    QTcpSocket      *mSocket;
};

#endif // QDATATCPRECEIVER_H
