#include "comselectedcombobox.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <QSerialPort>

QComPortSelectedComboBox::QComPortSelectedComboBox(QWidget *parent) : QComboBox(parent)
{
    this->clear();
    //将当前的所有的串口消息更新到列表
    QList<QSerialPortInfo> list = QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo info, list) {
        this->addItem(info.portName());
    }
}

QComBaudrateSelectedComboBox::QComBaudrateSelectedComboBox(QWidget *parent)
{
    this->clear();
    this->addItem(QString::number(QSerialPort::Baud1200), QSerialPort::Baud1200);
    this->addItem(QString::number(QSerialPort::Baud2400), QSerialPort::Baud2400);
    this->addItem(QString::number(QSerialPort::Baud4800), QSerialPort::Baud4800);
    this->addItem(QString::number(QSerialPort::Baud9600), QSerialPort::Baud9600);
    this->addItem(QString::number(QSerialPort::Baud19200), QSerialPort::Baud19200);
    this->addItem(QString::number(QSerialPort::Baud38400), QSerialPort::Baud38400);
    this->addItem(QString::number(QSerialPort::Baud57600), QSerialPort::Baud57600);
    this->addItem(QString::number(QSerialPort::Baud115200), QSerialPort::Baud115200);
}

QComStopBitSelectedComboBox::QComStopBitSelectedComboBox(QWidget *parent)
{
    this->clear();
    this->addItem(QStringLiteral("1位"), QSerialPort::OneStop);
    this->addItem(QStringLiteral("1.5位"), QSerialPort::OneAndHalfStop);
    this->addItem(QStringLiteral("2位"), QSerialPort::TwoStop);
}

QComParityBitSelectedComboBox::QComParityBitSelectedComboBox(QWidget *parent)
{
    this->clear();
    this->addItem(QStringLiteral("无校验"), QSerialPort::NoParity);
    this->addItem(QStringLiteral("奇校验"), QSerialPort::OddParity);
    this->addItem(QStringLiteral("偶校验"), QSerialPort::EvenParity);
    this->addItem(QStringLiteral("0校验"), QSerialPort::SpaceParity);
    this->addItem(QStringLiteral("1校验"), QSerialPort::MarkParity);
}

QComDataBitSelectedComboBox::QComDataBitSelectedComboBox(QWidget *parent)
{
    this->clear();
    this->addItem(QStringLiteral("5"), QSerialPort::Data5);
    this->addItem(QStringLiteral("6"), QSerialPort::Data6);
    this->addItem(QStringLiteral("7"), QSerialPort::Data7);
    this->addItem(QStringLiteral("8"), QSerialPort::Data8);
}
