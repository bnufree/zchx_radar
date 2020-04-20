#ifndef SERIALPORT_H
#define SERIALPORT_H
#include<QSerialPort>
#include <QWidget>
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

namespace Ui {
class serialport;
}

class serialport : public QWidget
{
    Q_OBJECT

public:
    explicit serialport(QWidget *parent = 0);
    ~serialport();
private slots:
    void on_OpenSerialButton_clicked();

    void ReadData();

    void on_SendButton_clicked();

private:
    Ui::serialport *ui;
    QSerialPort *serial;

};

#endif // SERIALPORT_H
