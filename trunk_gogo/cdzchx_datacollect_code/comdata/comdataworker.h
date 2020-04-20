#ifndef RECIVECOMDATA_H
#define RECIVECOMDATA_H
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSettings>
#include <QTimer>
#include "comdefines.h"

class   ComDataWorker : public QObject
{
    Q_OBJECT
public:
    explicit    ComDataWorker(const COMDEVPARAM& param);
    virtual     ~ComDataWorker();
    bool        open();
    void        stop();
    void        writeData(const QByteArray& bytes);
    void        setQueryMode(bool sts);
    COMDEVPARAM param() const {return mParam;}
signals:
    void        signalReciveComData(const QString& comName, const QString& topic, qint64 time, const QByteArray& recv);
    void        signalReciveWriteMsg(const QString& comName, qint64 time, const QByteArray& recv);
    void        signalSerialPortErrorStr(const QString& msg);

public slots:
    void        slotReadComData();
    void        slotRecvSerialPortErr(QSerialPort::SerialPortError err);
    void        slotRecvWriteInfo(qint64 num);
    void        slotSendQueryCmd();

private:
    QSerialPort                 *mSerialPortPtr;
    bool                        mQueryMode;
    QTimer                      *mQueryTimerPtr;               //定时的查询命令
    //串口参数
    COMDEVPARAM                 mParam;
};

#endif // RECIVECOMDATA_H
