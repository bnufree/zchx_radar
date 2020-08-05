#include "comsendtread.h"
#include <QDebug>

ComSendTread::ComSendTread(QString comName, int comBaud)
{
    m_comName = comName;
    m_comBaud = comBaud;
    m_serialport = new QSerialPort;
    m_serialport->setPortName(m_comName);
    if(m_serialport->open(QIODevice::ReadWrite))
    {
        qDebug()<<"com:"<<m_comName<<m_comBaud;
        m_serialport->setBaudRate(m_comBaud);
        m_serialport->setDataBits(QSerialPort::Data8);
        m_serialport->setParity(QSerialPort::NoParity);
        m_serialport->setStopBits(QSerialPort::OneStop);
        m_serialport->setFlowControl(QSerialPort::NoFlowControl);
        m_serialport->setRequestToSend(true);
        m_serialport->setDataTerminalReady(true);
    }
}

ComSendTread::~ComSendTread()
{
    if(m_serialport)
    {
        m_serialport->close();
        delete m_serialport;
        m_serialport = NULL;
    }
}

bool ComSendTread::sendData(QString data)
{
    qDebug()<<"send:"<<data;
    m_serialport->write(data.toLatin1());
    return m_serialport->waitForBytesWritten(1000);
}

QSerialPort *ComSendTread::serialport() const
{
    return m_serialport;
}




