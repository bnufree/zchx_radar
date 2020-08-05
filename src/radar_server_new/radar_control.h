#ifndef RADAR_CONTROL_H
#define RADAR_CONTROL_H

#include <QWidget>
#include <QThread>
#include <QUdpSocket>
#include <QDialog>
#include <QMessageBox>
#include <QTimer>

namespace Ui {
class radar_control;
}

class radar_control : public QDialog
{
    Q_OBJECT

public:
    explicit radar_control(QWidget *parent = 0);
    ~radar_control();
signals:
    void startProcessSignal();
public slots:
    void startProcessSlot();
private slots:
    void on_send_pushButton_clicked();

    void on_save_pushButton_clicked();

    void on_power_pushButton_clicked();

private:
    Ui::radar_control *ui;
    QUdpSocket *mSocket;
    QByteArray ba;
    //QTimer mReadTimer;
};

#endif // RADAR_CONTROL_H
