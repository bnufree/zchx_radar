#ifndef MAINPROCESS_H
#define MAINPROCESS_H

#include <QObject>
#include "msgserver.h"
#include <QJsonDocument>

#define MainProc        MainProcess::instance()

class MainProcess : public QObject
{
    Q_OBJECT
private:
    explicit MainProcess(QObject *parent = 0);
private:
    void    initConfig();
public:
    static MainProcess* instance();
    void    start();
    bool    isStart() const {return mStartFlag;}

signals:

public slots:
    void    slotSendSocketServerMsg(QTcpSocket* socket);
private:
    static  MainProcess* m_pInstance;
    bool    mStartFlag;
    MsgServer       *mMsgServer;
    QJsonDocument       mCfgDoc;


};

#endif // MAINPROCESS_H
