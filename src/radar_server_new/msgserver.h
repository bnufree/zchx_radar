#ifndef MSGSERVER_H
#define MSGSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class MsgServer : public QObject
{
    Q_OBJECT
public:
    explicit MsgServer(QObject *parent = 0);

signals:
    void    signalSendServerParamMsgToSocket(QTcpSocket* socket);
public slots:
    void    slotNewConnection();
    void    slotAcceptError(QAbstractSocket::SocketError socketError);
    void    slotReadClientContent();
    void    slotClientDisconnect();

private:
    QTcpServer*     mServer;
    int             mPort;
    QList<QTcpSocket*>      mClientList;
};

#endif // MSGSERVER_H
