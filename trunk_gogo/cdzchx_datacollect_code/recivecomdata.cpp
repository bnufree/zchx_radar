#include "recivecomdata.h"
#include <QDebug>
#include <QDateTime>
#include "Log.h"

ReciveComData::ReciveComData(const ReciveCom &s)
    :m_serialport(0)
{
    mSendCommandTimer = 0;
    mQueryMode = false;
    mQueryCmd = QByteArray();
    m_reciveCom = s;
    m_serialport = new QSerialPort;
    m_serialport->setPortName(m_reciveCom.name);

    qDebug()<<"name:"<<m_reciveCom.name;
    qDebug()<<"baudRate"<<m_reciveCom.baudRate;
    qDebug()<<"dataBits:"<<m_reciveCom.dataBits;
    qDebug()<<"parity:"<<m_reciveCom.parity;
    qDebug()<<"stopBits:"<<m_reciveCom.stopBits;
    qDebug()<<"flowControl:"<<m_reciveCom.flowControl;
    connect(m_serialport,SIGNAL(readyRead()),this,SLOT(slotReadComData()));
    connect(m_serialport,SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(slotRecvSerialPortErr(QSerialPort::SerialPortError)));
    connect(m_serialport, SIGNAL(bytesWritten(qint64)), this, SLOT(slotRecvWriteInfo(qint64)));
}

ReciveComData::ReciveComData(const QString &topic, const QString &comName, int baudRate, int parity, int databit, int stopbit)
{
    mSendCommandTimer = 0;
    mQueryMode = false;
    mQueryCmd = QByteArray();
    ReciveCom s;
    s.topic = topic;
    s.name = comName;
    s.baudRate = baudRate;
    s.dataBits = (QSerialPort::DataBits)databit;
    s.parity = (QSerialPort::Parity)parity;
    s.stopBits = (QSerialPort::StopBits)stopbit;
    s.flowControl = QSerialPort::NoFlowControl;
    m_reciveCom = s;
    m_serialport = new QSerialPort;
    m_serialport->setPortName(m_reciveCom.name);

    qDebug()<<"name:"<<m_reciveCom.name;
    qDebug()<<"baudRate"<<m_reciveCom.baudRate;
    qDebug()<<"dataBits:"<<m_reciveCom.dataBits;
    qDebug()<<"parity:"<<m_reciveCom.parity;
    qDebug()<<"stopBits:"<<m_reciveCom.stopBits;
    qDebug()<<"flowControl:"<<m_reciveCom.flowControl;
    connect(m_serialport,SIGNAL(readyRead()),this,SLOT(slotReadComData()));
    connect(m_serialport,SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(slotRecvSerialPortErr(QSerialPort::SerialPortError)));
    connect(m_serialport, SIGNAL(bytesWritten(qint64)), this, SLOT(slotRecvWriteInfo(qint64)));
}

QIODevice::OpenMode ReciveComData::openMode()
{
    return m_serialport->openMode();
}

bool ReciveComData::open(QIODevice::OpenMode mode,  const QByteArray& msg)
{
    mOpemMode = mode;
    mQueryCmd = msg;
    if(!m_serialport) return false;

    if (!m_serialport->isOpen())
    {
        if(!m_serialport->open(mode))
        {
            return false;
        }
    } else
    {
        m_serialport->close();
        if(!m_serialport->open(mode))
        {
            return false;
        }
    }
    m_serialport->setBaudRate(m_reciveCom.baudRate);
    m_serialport->setDataBits(m_reciveCom.dataBits);
    m_serialport->setParity(m_reciveCom.parity);
    m_serialport->setStopBits(m_reciveCom.stopBits);
    m_serialport->setFlowControl(m_reciveCom.flowControl);
    m_serialport->setRequestToSend(true);
    m_serialport->setDataTerminalReady(true);
    if(mode == QIODevice::ReadWrite && mQueryCmd.length() > 0)
    {
        setQueryMode(true);
    }
    return true;
}

void ReciveComData::stop()
{
    if(m_serialport && m_serialport->isOpen())
    {
        //m_serialport->clear();
        //m_serialport->close();
    }
    if(mSendCommandTimer)
    {
        mSendCommandTimer->stop();
        mSendCommandTimer->deleteLater();
    }
    mSendCommandTimer = 0;
    mQueryMode = false;
    mQueryCmd = QByteArray();
}

ReciveComData::~ReciveComData()
{
    qDebug()<<__FILE__<<__FUNCTION__<<__LINE__;
    if(m_serialport) m_serialport->deleteLater();
    qDebug()<<__FILE__<<__FUNCTION__<<__LINE__;
}

void ReciveComData::slotRecvSerialPortErr(QSerialPort::SerialPortError err)
{
    qDebug()<<__FILE__<<__FUNCTION__<<err;
    QString errmsg;
    switch (err) {
    case QSerialPort::DeviceNotFoundError:
        errmsg = QString("'%1' device not found").arg(m_reciveCom.name);
        break;
    case QSerialPort::PermissionError:
        errmsg = QString("'%1' permission error").arg(m_reciveCom.name);
        break;
    case QSerialPort::OpenError:
        errmsg = QString("'%1' opened error").arg(m_reciveCom.name);
        break;
    case QSerialPort::ParityError:
        errmsg = QString("'%1' parity error").arg(m_reciveCom.name);
        break;
    case QSerialPort::FramingError:
        errmsg = QString("'%1' framing error").arg(m_reciveCom.name);
        break;
    case QSerialPort::BreakConditionError:
        errmsg = QString("'%1' break condition error").arg(m_reciveCom.name);
        break;
    case QSerialPort::WriteError:
        errmsg = QString("'%1' write error").arg(m_reciveCom.name);
        break;
    case QSerialPort::ReadError:
        errmsg = QString("'%1' read error").arg(m_reciveCom.name);
        break;
    case QSerialPort::ResourceError:
        errmsg = QString("'%1' resource error").arg(m_reciveCom.name);
        break;
    case QSerialPort::UnsupportedOperationError:
        errmsg = QString("'%1' unsupport operation error").arg(m_reciveCom.name);
        break;
    case QSerialPort::UnknownError:
        errmsg = QString("'%1' unknown error").arg(m_reciveCom.name);
        break;
    case QSerialPort::TimeoutError:
        errmsg = QString("'%1' timeout error").arg(m_reciveCom.name);
        break;
    case QSerialPort::NotOpenError:
        errmsg = QString("'%1' not opened error").arg(m_reciveCom.name);
        break;
    default:
        break;
    }

    if(errmsg.length())
    {
        emit signalSerialPortErrorStr(errmsg);
    }



}

void ReciveComData::setReciveCom(const ReciveCom &reciveCom)
{
    m_reciveCom = reciveCom;
}


void ReciveComData::slotReadComData()
{

    emit signalReciveComData(m_serialport->portName(), m_reciveCom.topic, QDateTime::currentMSecsSinceEpoch(), m_serialport->readAll());

}

void ReciveComData::writeData(const QByteArray &bytes)
{
    m_serialport->write(bytes);
    m_serialport->waitForBytesWritten(3000);
}

void ReciveComData::slotRecvWriteInfo(qint64 num)
{
    QString msg = tr("写入串口信息长度:%1").arg(num);
    emit signalReciveComData(m_serialport->portName(), m_reciveCom.topic, QDateTime::currentMSecsSinceEpoch(), msg.toUtf8());
}

QString ReciveComData::getComName()
{
    return m_reciveCom.name;
}

int     ReciveComData::getBaudRate()
{
    return m_reciveCom.baudRate;
}

QString ReciveComData::getTopic()
{
    return m_reciveCom.topic;
}

int ReciveComData::getParity()
{
    return m_reciveCom.parity;
}

int ReciveComData::getDataBit()
{
    return m_reciveCom.dataBits;
}

int ReciveComData::getStopBit()
{
    return m_reciveCom.stopBits;
}

void ReciveComData::setQueryMode(bool sts)
{
    mQueryMode = sts;
    if(mQueryMode)
    {
        slotSendQueryCmd();
        mSendCommandTimer = new QTimer(0);
        mSendCommandTimer->setInterval(3000);
        connect(mSendCommandTimer, SIGNAL(timeout()), this, SLOT(slotSendQueryCmd()));
        mSendCommandTimer->start();
    }
}

void ReciveComData::slotSendQueryCmd()
{
    if(mQueryMode && mQueryCmd.length() > 0)
    {
        writeData(mQueryCmd);
    }
}
