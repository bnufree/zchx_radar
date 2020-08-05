#ifndef AISBASEINFOSETTING_H
#define AISBASEINFOSETTING_H

#include <QWidget>
#include <bitset>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include <QObject>
#include <QThread>
#include "ZCHXAISVessel.pb.h"
#include "zmq.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "ais/ais.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
//#include <boost/regex.hpp>

using std::bitset;
using std::ostream;
using std::string;
using std::vector;
typedef unsigned long u_long;


namespace Ui {
class aisBaseInfoSetting;
}

class aisBaseInfoSetting : public QWidget
{
    Q_OBJECT

public:
    explicit aisBaseInfoSetting(QWidget *parent = 0);
    ~aisBaseInfoSetting();
signals:
    void startProcess();
private slots:
    void on_modifyCommunicationSetup_clicked();
    void workSlot();
    void sendData();
    void create3Str();
    void create4Str();
    void create5Str();
    void create21Str();
    void openPort();
    bool CheckXor(QByteArray data);//校验位检查
    uchar checkxor(QByteArray data);
    void on_type_comboBox_currentIndexChanged(int index);
    void initComboBox(int index);

    void on_tabWidget_currentChanged(int index);
    void saveAllTtpeInfo(int index);//保存到Ini文件
    void initAllTypeInfo();//初始界面数据

private:
    Ui::aisBaseInfoSetting *ui;
    QThread mWorkThread;
    QSerialPort *serial;
    QTimer *mTimer; //定时发送
    string str;
    bool flag;
    QString crc;
    int mSendfrequency;

};

#endif // AISBASEINFOSETTING_H
