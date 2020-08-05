#ifndef COMSENDTREAD_H
#define COMSENDTREAD_H
#include <QThread>
#include <QSerialPort>


class ComSendTread : public QThread
{
    Q_OBJECT
public:
   explicit ComSendTread(QString comName,int comBaud);
   virtual ~ComSendTread();
   bool sendData( QString data);

   QSerialPort *serialport() const;

private:
   QString m_comName;
   int     m_comBaud;
    QSerialPort *m_serialport;
};

#endif // COMSENDTREAD_H
