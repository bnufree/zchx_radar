#ifndef RECIVECOMDATA_H
#define RECIVECOMDATA_H
#include <QThread>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSettings>
#include <QTimer>

typedef struct tagReciveCom{
    QString topic;
    QString name;
    int     baudRate;
    QSerialPort::Direction direction;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity   parity;
    QSerialPort::StopBits stopBits;
    QSerialPort::FlowControl flowControl;

}ReciveCom;

class ReciveComData : public QThread
{
    Q_OBJECT
public:
    explicit  ReciveComData(const ReciveCom &);
    explicit  ReciveComData(const QString& topic, const QString& comName, int baudRate, int parity, int databit, int stopbit);
    virtual ~ReciveComData();
    void setReciveCom(const ReciveCom &reciveCom);
    bool open(QIODevice::OpenMode mode = QIODevice::ReadOnly, const QByteArray& msg = QByteArray());
    void stop();
    QString getComName();
    int  getBaudRate();
    int  getParity();
    int  getDataBit();
    int  getStopBit();
    QString getTopic();
    void  writeData(const QByteArray& bytes);
    QIODevice::OpenMode openMode();
    void    setQueryMode(bool sts);
signals:
    void signalReciveComData(const QString& comName, const QString& topic, qint64 time, const QByteArray& recv);
    void signalSerialPortErrorStr(const QString& msg);

public slots:
    void slotReadComData();
    void slotRecvSerialPortErr(QSerialPort::SerialPortError err);
    void slotRecvWriteInfo(qint64 num);
    void slotSendQueryCmd();

private:
    ReciveCom m_reciveCom;
    QSerialPort *m_serialport;
    //QString    mMsg;
    int        mOpemMode;
    QTimer      *mSendCommandTimer;
    bool       mQueryMode;
    QByteArray  mQueryCmd;
};

#endif // RECIVECOMDATA_H
