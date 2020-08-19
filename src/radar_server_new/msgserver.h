#ifndef MSGSERVER_H
#define MSGSERVER_H

#include <QObject>

class MsgServer : public QObject
{
    Q_OBJECT
public:
    explicit MsgServer(QObject *parent = 0);

signals:

public slots:
};

#endif // MSGSERVER_H