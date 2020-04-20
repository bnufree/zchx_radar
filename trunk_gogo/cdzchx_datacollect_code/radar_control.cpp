#include "radar_control.h"
#include "ui_radar_control.h"
#include "profiles.h"

#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
radar_control::radar_control(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::radar_control)
{
    ui->setupUi(this);
    setWindowTitle("雷达控制");
    mSocket = new QUdpSocket();
    //点播
    QString Control_ip = Utils::Profiles::instance()->value("Radar_Control","Control_ip").toString();
    QString Control_port = Utils::Profiles::instance()->value("Radar_Control","Control_port").toString();
    ui->ip_lineEdit->setText(Control_ip);
    ui->port_lineEdit->setText(Control_port);
    //初始化发送指令
    if(1)
    {
        ba.resize(28);
        ba[0] = 0xFD;
        ba[1] = 0x00;
        ba[2] = 0x1C;
        ba[3] = 0x00;
        //UAP,040
        ba[4] = '\t';
        ba[5] = '0';
        ba[6] = '\b';
        //080
        ba[7] = 0xE8;
        ba[8] = '\f';  //0CE8  3304  设置主电源
        ba[9] = 0x01;
        ba[10] = '0';
        //090
        ba[11] = 0x01;
        ba[12] = 0x03;
        ba[13] = 0x00;
        ba[14] = 0x00;
        ba[15] = 0x00;
        ba[16] = 0x00;
        ba[17] = 0x00;
        ba[18] = 0x00;
        ba[19] = 0x00;
        ba[20] = 0x00;
        ba[21] = 0x00;
        ba[22] = 0x00;
        ba[23] = 0x00;
        ba[24] = 0x00;
        ba[25] = 0x00;
        ba[26] = 0x00;
        ba[27] = 0x00;
    }
    //采用定时器接收雷达控制反馈的信息
//    mReadTimer.setInterval(5000);
//    connect(&mReadTimer, SIGNAL(timeout()), this, SLOT(startProcessSlot()));

//    QHostAddress address(ui->ip_lineEdit->text());
//    quint16 port = ui->port_lineEdit->text().toInt();
    mSocket->bind(QHostAddress(ui->ip_lineEdit->text())
                  ,ui->port_lineEdit->text().toInt());
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(startProcessSlot()));

}

radar_control::~radar_control()
{
    delete ui;
}

void radar_control::startProcessSlot()
{
    QByteArray array;
    array.resize(mSocket->bytesAvailable());//根据可读数据来设置空间大小
    mSocket->readDatagram(array.data(),array.size()); //读取数据
    cout<<"array"<<array.size()<<array;

    QByteArray video_array;
    video_array = array;
    const unsigned char* data = (const unsigned char*)(video_array.constData());
    int len;
    int j = 0;
    while ((j = video_array.indexOf('\xFD', j)) != -1)
    {
        len = data[j+2];
        //cout << "[" << len << "]" ;
        QByteArray send_ba;
        video_array = video_array.mid(j);
        //cout << "[" << ba.size() << "]" ;
        send_ba = video_array.left(len);
        data = (const unsigned char*)(video_array.constData());
        //cout<<"数据"<<send_ba;
        cout<<"雷达控制数据"<<send_ba.size()<<endl<<send_ba;
        if(send_ba.size() != 28)
        {
            cout<<"数据长度不对";
            return;
        }
        cout<<"ba[7]"<<data[7];
        switch (data[7]) {
        //电源
        case 0xE8:
            switch (data[12]) {
            case 0x00:
                cout<<"0 OFF";
                ui->statusip_lineEdit->setText("OFF");
                break;
            case 0x01:
                cout<<"1 ON";
                ui->statusip_lineEdit->setText("ON");
                break;
            default:
                cout<<"2 N/A";
                ui->statusip_lineEdit->setText("N/A");
                break;
            }
            break;
        //PRF
        case 0xE7:
            switch (data[12]) {
            case 0x00:
                cout<<"0 Very Short";
                ui->status_port_lineEdit->setText("Very Short");
                break;
            case 0x01:
                cout<<"1 Short";
                ui->status_port_lineEdit->setText("Short");
                break;
            case 0x02:
                cout<<"2 Medium";
                ui->status_port_lineEdit->setText("Medium");
                break;
            case 0x03:
                cout<<"3 Long";
                ui->status_port_lineEdit->setText("Long");
                break;
            case 0x04:
                cout<<"4 Very Long";
                ui->status_port_lineEdit->setText("Very Long");
                break;
            default:
                ui->status_port_lineEdit->setText("N/A");
                break;
            }
            break;
        default:
            break;
        }
    }
}
//设置雷达PRF
void radar_control::on_send_pushButton_clicked()
{
    //设置PRF
    int PRF = ui->PRF_lineEdit->text().toInt();
    qint64 len;
    switch (PRF) {
    case 0:
        ba[7] = 0xE7;
        ba[12] = 0x00;
        len = mSocket->writeDatagram(ba,QHostAddress(ui->ip_lineEdit->text())
                    ,ui->port_lineEdit->text().toInt());
        if(len < 0)
            cout<<"发送失败";
        break;
    case 1:
        ba[7] = 0xE7;
        ba[12] = 0x01;
        len = mSocket->writeDatagram(ba,QHostAddress(ui->ip_lineEdit->text())
                    ,ui->port_lineEdit->text().toInt());
        if(len < 0)
            cout<<"发送失败";
        break;
    case 2:
        ba[7] = 0xE7;
        ba[12] = 0x02;
        len = mSocket->writeDatagram(ba,QHostAddress(ui->ip_lineEdit->text())
                    ,ui->port_lineEdit->text().toInt());
        if(len < 0)
            cout<<"发送失败";
        break;
    case 3:
        ba[7] = 0xE7;
        ba[12] = 0x03;
        len = mSocket->writeDatagram(ba,QHostAddress(ui->ip_lineEdit->text())
                    ,ui->port_lineEdit->text().toInt());
        if(len < 0)
            cout<<"发送失败";
        break;
    default:
        QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("PRF参数配置有误"));
        break;
    }
}
//保存IP配置
void radar_control::on_save_pushButton_clicked()
{
    //保存
    Utils::Profiles::instance()->setValue("Radar_Control","Control_ip",ui->ip_lineEdit->text());
    Utils::Profiles::instance()->setValue("Radar_Control","Control_port", ui->port_lineEdit->text());
    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("保存成功"));
}
//设置雷达电源
void radar_control::on_power_pushButton_clicked()
{
    //设置雷达电源
    int power = ui->power_lineEdit->text().toInt();
    qint64 len;
    switch (power) {
    case 0:
        cout<<"关闭电源";
        ba[7] = 0xE8;
        ba[12] = 0x00;
        len = mSocket->writeDatagram(ba,QHostAddress(ui->ip_lineEdit->text())
                    ,ui->port_lineEdit->text().toInt());
        if(len < 0) cout<<"发送失败";
        break;
    case 1:
        cout<<"打开电源";
        ba[7] = 0xE8;
        ba[12] = 0x01;
        len = mSocket->writeDatagram(ba,QHostAddress(ui->ip_lineEdit->text())
                    ,ui->port_lineEdit->text().toInt());
        if(len < 0) cout<<"发送失败";
        break;
    default:
        QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("电源参数配置有误"));
        break;
    }
}
