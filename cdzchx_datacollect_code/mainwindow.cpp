#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dataserverutils.h"
#include <QThread>
#include <QDateTime>
#include <QProcess>
#include <QDebug>
#include "profiles.h"
#include "Log.h"
#include <QLabel>
#include <QFileDialog>
#include "zmq.h"
#include "zmq_utils.h"
#include "zmq.hpp"
#include "ais_radar/zchxaisdataserver.h"
#include "ais_radar/zchxradardataserver.h"
#include "ais_radar/zchxradarechodatachange.h"
#include "ais_radar/zxhcprocessechodata.h"
#include "ais_radar/zchxanalysisandsendradar.h"
#include "ais_radar/zchxaisdataprocessor.h"

#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

class HqTableWidgetItem : public QTableWidgetItem
{
public:
    HqTableWidgetItem(const QString& text, Qt::AlignmentFlag flg = Qt::AlignCenter)
        :QTableWidgetItem(text)
    {
        setTextAlignment(flg);
    }

    ~HqTableWidgetItem()
    {

    }

};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    mpRadarEchoDataChange(0),
    mAisDataProc(0),
    mProcessEchoData(0),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->resize(800, 600);
    initUI();
    txt = false;//1_初始化打印回波标志
    t_2 = false;//1_初始化打印丢包标志
    t_3= false;//1_初始化打印所有扫描线标志

    Utils::Profiles::instance()->setDefault("Echo", "Enable", false);
    //从张邦伟采集器接回波数据
    if(Utils::Profiles::instance()->value("Echo", "Enable", false).toBool())
    {
        mpRadarEchoDataChange = new ZCHXRadarEchoDataChange();
        mpRadarEchoDataChange->updateRadarEchoData();
        mProcessEchoData = new ZXHCProcessEchoData();
        connect(mpRadarEchoDataChange, SIGNAL(sendMsg(Map_RadarVideo)),
                mProcessEchoData, SIGNAL(signalProcess(Map_RadarVideo)));
        connect(mProcessEchoData, SIGNAL(signalSendRecvedContent(qint64,QString,QString)),
                this, SLOT(receiveContent(qint64,QString,QString)));
    }

    //接收模拟器上ais数据
    mAisDataServer = new ZCHXAisDataServer();
    mAisDataProc = new zchxAisDataProcessor;
    connect(mAisDataServer, SIGNAL(signalSendRecvedContent(qint64,QString,QString)),
            this, SLOT(receiveContent(qint64,QString,QString)));    
    connect(mAisDataServer, SIGNAL(signalSocketMsg(QString)),\
            this, SLOT(slotRecvHearMsg(QString)));
    connect(mAisDataServer, SIGNAL(signalSendAisData(QByteArray)), \
            mAisDataProc, SIGNAL(signalRecvAisData(QByteArray)));
    connect(mAisDataProc, SIGNAL(signalClientInout(QString,QString,int,int)),
            this, SLOT(slotUpdateClientTable(QString,QString,int,int)));
    connect(mAisDataProc, SIGNAL(signalSendRecvedContent(qint64,QString,QString)), \
            this, SLOT(receiveContent(qint64,QString,QString)));
    emit mAisDataServer->startProcessSignal();
    emit mAisDataProc->signalInitZmq();

    //接收模拟器上radar数据
    //qDebug()<<"MainWindow thread id :"<<QThread::currentThreadId();
    //Utils::Profiles::instance()->setDefault("Radar","Num",1);
    initRadarCfgInfo();
    mRadarDataServerList.clear();
    foreach(int id, mRadarConfigMap.keys())
    {
        //接收雷达数据
        ZCHXRadarDataServer *pRadarDataServer = new ZCHXRadarDataServer(mRadarConfigMap[id], 0);
        //处理雷达数据并发送
        ZCHXAnalysisAndSendRadar *pAnalysisAndSendRadar = new ZCHXAnalysisAndSendRadar(id);
        emit pRadarDataServer->startProcessSignal();//开启接收
        mRadarDataServerList.append(pRadarDataServer);
        mAnalysisAndSendRadarList.append(pAnalysisAndSendRadar);
        connect(pRadarDataServer, SIGNAL(analysisLowranceRadar(QByteArray,int,int,int)),
                pAnalysisAndSendRadar, SIGNAL(analysisLowranceRadarSignal(QByteArray,int,int,int)));
        connect(pRadarDataServer, SIGNAL(analysisCatRadar(QByteArray,int,int,int,QString)),
                pAnalysisAndSendRadar, SIGNAL(analysisCatRadarSignal(QByteArray,int,int,int,QString)));
        connect(pAnalysisAndSendRadar, SIGNAL(signalSendRecvedContent(qint64,QString,QString)), this, SLOT(receiveContent(qint64,QString,QString)));
        connect(pRadarDataServer, SIGNAL(signalSendRecvedContent(qint64,QString,QString)), this, SLOT(receiveContent(qint64,QString,QString)));
        connect(pRadarDataServer, SIGNAL(signalClientInout(QString,QString,int,int)), this, SLOT(slotUpdateClientTable(QString,QString,int,int)));
        connect(pAnalysisAndSendRadar, SIGNAL(signalClientInout(QString,QString,int,int)), this, SLOT(slotUpdateClientTable(QString,QString,int,int)));
        connect(this, SIGNAL(signalOpenRadar()), pRadarDataServer, SLOT(openRadar()));
        connect(this, SIGNAL(signalcloseRadar()), pRadarDataServer, SLOT(closeRadar()));
        connect(pRadarDataServer, SIGNAL(signalRadarStatusChanged(QList<RadarStatus>, int)), ui->settingWidget, SLOT(slotRecvRadarReportInfo(QList<RadarStatus>,int)));
        connect(pAnalysisAndSendRadar, SIGNAL(signalRadiusFactorUpdated(double,double)), ui->settingWidget, SLOT(slotUpdateRealRangeFactor(double,double)));

        //显示分析页面
        //connect(pAnalysisAndSendRadar, SIGNAL(show_info(QString,double,float)), this, SLOT(show_info_slot(QString,double,float)));//打印txt
        connect(pAnalysisAndSendRadar, SIGNAL(show_video(int,int)), this, SLOT(show_video_slot(int,int)));//打印目标个数
        connect(pAnalysisAndSendRadar, SIGNAL(show_statistics(int,int,int,int,int)), this, SLOT(show_statistics_slot(int,int,int,int,int)));//打印丢包率
        //connect(pAnalysisAndSendRadar, SIGNAL(show_missing_spokes(QString)), this, SLOT(show_missing_spokes_slot(QString)));//打印丢失扫描线信息
        //connect(pAnalysisAndSendRadar, SIGNAL(show_received_spokes(QString)), this, SLOT(show_received_spokes_slot(QString)));//打印所有扫描线信息
        connect(this, SIGNAL(signal_set_penwidth(int)), pAnalysisAndSendRadar, SIGNAL(set_pen_width(int)));
        //实时打印接收到的雷达状态信息
        connect(pRadarDataServer, SIGNAL(signalRadarStatusChanged(QList<RadarStatus>, int)), this, SLOT(slotRecvRadarReportInfo_1(QList<RadarStatus>,int)));

    }
    ui->frame->setVisible(false);
    connect(ui->settingWidget, SIGNAL(signalRangeFactorChanged(double)), this, SLOT(slotRecvRangeFactorChanged(double)));


}

void MainWindow::initRadarCfgInfo()
{
    int  uRadarNum = Utils::Profiles::instance()->value("Radar","Num", 0).toInt();
    if(uRadarNum == 0) return;
    for(int i=1; i<=uRadarNum; i++)
    {
        //从配置文件读取
        QString str_radar = QString("Radar_%1").arg(i);
        ZCHX::Messages::RadarConfig* cfg = new ZCHX::Messages::RadarConfig;
        cfg->setID(i);
        cfg->setName(str_radar);
        cfg->getTSPIConfig()->SetHost(Utils::Profiles::instance()->value(str_radar,"Track_IP").toString().toStdString());
        cfg->getTSPIConfig()->SetPort(Utils::Profiles::instance()->value(str_radar,"Track_Port").toInt());
        cfg->getVideoConfig()->SetHost(Utils::Profiles::instance()->value(str_radar,"Video_IP").toString().toStdString());
        cfg->getVideoConfig()->SetPort(Utils::Profiles::instance()->value(str_radar,"Video_Port").toInt());
        cfg->setSiteLat(Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toDouble());
        cfg->setSiteLon(Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toDouble());
        cfg->setRadarType(Utils::Profiles::instance()->value(str_radar,"Video_Type").toString());
        cfg->setGateCountMax(Utils::Profiles::instance()->value(str_radar,"Cell_Num").toInt());
        cfg->setShaftEncodingMax(Utils::Profiles::instance()->value(str_radar,"Line_Num").toInt() - 1);
        cfg->setLimit(Utils::Profiles::instance()->value(str_radar,"Limit").toBool());
        cfg->setLoopNum(Utils::Profiles::instance()->value(str_radar,"Loop_Num").toInt());
        cfg->setHead(Utils::Profiles::instance()->value(str_radar,"Heading").toInt());
        cfg->setHeartTimeInterval(Utils::Profiles::instance()->value(str_radar,"Heart_Time").toInt());
        cfg->setCmdIP(Utils::Profiles::instance()->value(str_radar,"Heart_IP").toString());
        cfg->setCmdPort(Utils::Profiles::instance()->value(str_radar,"Heart_Port").toInt());
        cfg->setDistance(Utils::Profiles::instance()->value(str_radar,"Distance").toInt());
        cfg->setTrackClearTime(Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt());
        cfg->setReportIP(Utils::Profiles::instance()->value(str_radar,"Report_IP").toString());
        cfg->setReportPort(Utils::Profiles::instance()->value(str_radar,"Report_Port").toInt());
        cfg->setReportOpen(Utils::Profiles::instance()->value(str_radar,"Report_Open").toBool());
        mRadarConfigMap[cfg->getID()] = cfg;
    }
}

void MainWindow::initUI()
{
    this->setWindowTitle(QStringLiteral("采集器"));
    QIcon icon = QIcon(":/image/app.png");
    if(icon.isNull())
    {
        qDebug()<<"icon image not found!!!!!!!!!!!!!!!!!!";
    } else
    {
        qDebug()<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<endl;
    }
    this->setWindowIcon(icon);
    ui->tabWidget->setCurrentIndex(0);//显示的页面
    ui->tableWidget->horizontalHeader()->setDefaultSectionSize(120);//设置列的宽度
    //ui->tableWidget->setColumnWidth(1, 60);//第一列的宽度
    ui->tableWidget->setColumnWidth(2, 60);
    ui->tableWidget->setColumnWidth(3, 60);
    ui->tableWidget->setColumnWidth(4, 150);
    ui->tableWidget->setColumnWidth(5, 150);
    ui->tableWidget->setColumnHidden(0, true);
    //ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mTimeLable = new QLabel(this);
    mVirtualIpWorkingLbl = new QLabel(this);
    mVirtualIpWorkingLbl->setStyleSheet("color:red;font-weight:bold;font-size:18;");
    ui->horizontalLayout->addItem(new QSpacerItem(40,20, QSizePolicy::Expanding, QSizePolicy::Preferred));
    ui->horizontalLayout->addWidget(mVirtualIpWorkingLbl);
    ui->horizontalLayout->addItem(new QSpacerItem(40,20, QSizePolicy::Expanding, QSizePolicy::Preferred));
    ui->horizontalLayout->addWidget(mTimeLable);
//    QTimer *updateTimer = new QTimer(this);
//    updateTimer->setInterval(1);
//    connect(updateTimer, SIGNAL(timeout()), this, SLOT(slotDisplaycurTime()));
//    updateTimer->start();
    connect(ui->settingWidget, SIGNAL(signalRadarConfigChanged(int,int,int)), this, SLOT(slotRadarConfigChanged(int,int,int)));

}

void MainWindow::closeEvent(QCloseEvent *)
{
    mAnalysisAndSendRadarList[0]->closeTT();
    QProcess p(0);
    p.start("cmd", QStringList()<<"/c"<<"taskkill /f /im Data_Collect_Server.exe");
    p.waitForStarted();
    p.waitForFinished();

}

void MainWindow::slotDisplaycurTime()
{
    mTimeLable->setText(DataServerUtils::currentTimeString());
}

void MainWindow::toolAction(QMenu *menu, QToolBar *toolBar)
{
//    QAction *userAct = new QAction(QStringLiteral("&账户管理"), this);
//    menu->addAction(userAct);
//    connect(userAct, SIGNAL(triggered()), this, SLOT(onPower()));

//    QAction *configAct = new QAction(QStringLiteral("&配置管理"), this);
//    menu->addAction(configAct);
//    connect(configAct, SIGNAL(triggered()), this, SLOT(onConfig()));

//    QAction *installAct = new QAction(QStringLiteral("&安装管理"), this);
//    menu->addAction(installAct);

//    QAction *interfaceAct = new QAction(QStringLiteral("&外部接口"), this);
//    menu->addAction(interfaceAct);
//    connect(interfaceAct, SIGNAL(triggered()), this, SLOT(onOutInter()));

//    QAction *protocalAct = new QAction(QStringLiteral("&协议配置"), this);
//    menu->addAction(protocalAct);
//    connect(protocalAct, SIGNAL(triggered()), this, SLOT(onProtocal()));
}

void MainWindow::onPower()
{

}

void MainWindow::onConfig()
{

}

void MainWindow::onOutInter()
{

}

void MainWindow::onProtocal()
{
}



void MainWindow::slotRecvWorkerMsg(const QString &msg)
{
    slotRecvHearMsg(msg);
}

void MainWindow::slotRecvHearMsg(QString msg)
{
//    slotInsertLogInfo(msg);
    receiveContent(QDateTime::currentMSecsSinceEpoch(), "NETWORK", msg);
}

void MainWindow::slotInsertLogInfo(const QString &msg)
{
//    ui->loglistwidget->insertItem(0, DataServerUtils::currentTimeString(false) + " " + msg);
//    if(ui->loglistwidget->count() > 100)
//    {
//        QListWidgetItem *item =  ui->loglistwidget->takeItem(99);
//        if(item) delete item;
//    }
//    LOG(LOG_RTM, "%s", msg.toUtf8().data());
}

void MainWindow::slotUpdateVirtualIpString(const QString &msg)
{
    mVirtualIpWorkingLbl->setText(msg);
    mVirtualIpWorkingLbl->setStyleSheet("color:red;font-weight:bold;font-size:18;");
}



MainWindow::~MainWindow()
{
    if(mpRadarEchoDataChange)
    {
        delete mpRadarEchoDataChange;
        mpRadarEchoDataChange = NULL;
    }
    if(mAisDataServer)
    {
        delete mAisDataServer;
        mAisDataServer = NULL;
    }
    if(mAisDataProc)
    {
        delete mAisDataProc;
        mAisDataProc = 0;
    }
    if(mRadarDataServerList.size()>0)
    {
        for(int i = 0;i<mRadarDataServerList.size();i++)
        {
            if(mRadarDataServerList[i] != NULL)
            {
                delete mRadarDataServerList[i];
                mRadarDataServerList[i] = NULL;
            }
        }
        mRadarDataServerList.clear();
    }
    if(mAnalysisAndSendRadarList.size()>0)
    {
        for(int i = 0;i<mAnalysisAndSendRadarList.size();i++)
        {
            if(mAnalysisAndSendRadarList[i] != NULL)
            {
                delete mAnalysisAndSendRadarList[i];
                mAnalysisAndSendRadarList[i] = NULL;
            }
        }
        mAnalysisAndSendRadarList.clear();
    }
    delete ui;
}




void MainWindow::receiveContent(qint64 time, const QString& name, const QString& content)
{
    ui->listWidget->insertItem(0, QString("%1---%2---%3").arg(DataServerUtils::time2String(time, true)).arg(name).arg(content));
    if(ui->listWidget->count() > 100)
    {
        QListWidgetItem* item = ui->listWidget->takeItem(99);
        delete item;
    }
}

void MainWindow::on_setip_clicked()
{
//    QString ip = "192.168.8.120";
//    QDateTime time = QDateTime::currentDateTime();
//    time.time().start();
//    DataServerUtils::setVirtualIp(ip);
//    while(!DataServerUtils::isVirtualIpExist(ip))
//    {
//        DataServerUtils::delVirtualIp(ip);
//        QThread::msleep(500);
//    }

    //ui->label->setText(QString("set total time:%1 ms").arg(time.time().elapsed()));

}

void MainWindow::on_deleteip_clicked()
{
//    QString ip = "192.168.8.120";
//    QDateTime time = QDateTime::currentDateTime();
//    time.time().start();
//    DataServerUtils::delVirtualIp(ip);
//    while(DataServerUtils::isVirtualIpExist(ip))
//    {
//        QThread::msleep(500);
//    }

    //ui->label->setText(QString("del total time:%1 ms").arg(time.time().elapsed()));
}

void MainWindow::slotUpdateClientTable(const QString& ip, const QString& name, int port, int inout)
{
    QString sIpPort = ip+QString::number(port);
    if(inout == 1)
    {
        //客户端连上
        if(!mClientList.contains(sIpPort))
        {
            qDebug()<<"连接"<<sIpPort;
            mClientList.append(sIpPort);
            //添加到第一行
            ui->tableWidget->insertRow(0);
            ui->tableWidget->setItem(0, 0, new HqTableWidgetItem(name));
            ui->tableWidget->setItem(0, 1, new HqTableWidgetItem(ip));
            ui->tableWidget->setItem(0, 2, new HqTableWidgetItem(QString::number(port)));
            ui->tableWidget->setItem(0, 3, new HqTableWidgetItem(QStringLiteral("已连接")));
            ui->tableWidget->setItem(0, 4, new HqTableWidgetItem(DataServerUtils::currentTimeString()));
            ui->tableWidget->setItem(0, 5, new HqTableWidgetItem("-"));
        }
    } else
    {
        //客户端离开
        if(mClientList.contains(sIpPort))
        {
            mClientList.removeOne(sIpPort);
            //从表格删除
            qDebug()<<"断开"<<sIpPort;
            for(int i=0; i<ui->tableWidget->rowCount(); i++)
            {
                QTableWidgetItem *item = ui->tableWidget->item(i, 1);
                QTableWidgetItem *portItem = ui->tableWidget->item(i, 2);

                if(item && portItem)
                {
                    QString str = item->text()+portItem->text();
                    if(sIpPort == str)
                    {
                        ui->tableWidget->item(i,3)->setText(QStringLiteral("已断开"));
                        ui->tableWidget->item(i,5)->setText(DataServerUtils::currentTimeString());
                    }
                }
            }
        }
    }

}


void MainWindow::on_openRadarBtn_clicked()
{
    emit signalOpenRadar();
}

void MainWindow::on_closeRadarBtn_clicked()
{
    emit signalcloseRadar();
}

void MainWindow::slotRadarConfigChanged(int radarID, int type, int value)
{
    foreach (ZCHXRadarDataServer* server, mRadarDataServerList) {
        if(radarID == server->sourceID()){
            server->setControlValue((INFOTYPE)type, value);
        }
    }
}

//1_打印所有扫描线信息
//void MainWindow::show_received_spokes_slot(QString str)
//{
//    QString info_r = str+"\n\n";
//    if(true == t_3)
//    {
//        ui->textEdit_2->setText(info_r);
//        //cout<<"扫描线信息";
//        QFile file("../所有扫描线信息.txt");//创建文件对象
//        bool isOk = file.open(QIODevice::Text |QIODevice::WriteOnly |QIODevice::Append);
//        if(false == isOk)
//        {
//            cout <<"打开文件失败";
//            return;
//        }
//        if(true == isOk)
//        {
//            file.write(info_r.toStdString().data());
//        }
//        file.close();
//    }
//}
////1_打印丢失扫描线信息
//void MainWindow::show_missing_spokes_slot(QString str)
//{
//    QString info_r = str+"\n\n";
//    if(true == t_3)
//    {
//        ui->textEdit_2->setText(info_r);
//        cout<<"丢失扫描线信息";
//        QFile file("../丢失扫描线信息.txt");//创建文件对象
//        bool isOk = file.open(QIODevice::Text |QIODevice::WriteOnly |QIODevice::Append);
//        if(false == isOk)
//        {
//            cout <<"打开文件失败";
//            return;
//        }
//        if(true == isOk)
//        {
//            file.write(info_r.toStdString().data());
//        }
//        file.close();
//    }
//}
//1_打印统计丢包率
void MainWindow::show_statistics_slot(int a, int b, int c, int d, int e)
{

    QString str_packets = QString("%1/%2").arg(a).arg(b);
    ui->lineEdit_5->setText(str_packets);
    QString str_spoles = QString("%1/%2/%3").arg(c).arg(d).arg(e);
    ui->lineEdit_6->setText(str_spoles);
}

//界面显示目标个数
void MainWindow::show_video_slot(int a, int b)
{
    ui->lineEdit_3->setText(QString::number(a));
    ui->lineEdit_4->setText(QString::number(b));
}

//界面显示半径
//void MainWindow::show_info_slot(QString info,double d,float f)
//{
//    QString info_r = info+"\n";
//    ui->lineEdit->setText(QString::number(d));
//    ui->lineEdit_2->setText(QString::number(f));
//    if(true == txt)
//    {
//        ui->textEdit->setText(info);
//        QFile file("../test.txt");//创建文件对象
//        bool isOk = file.open(QIODevice::Text |QIODevice::WriteOnly |QIODevice::Append);
//        if(false == isOk)
//        {
//            cout <<"打开文件失败";
//            return;
//        }
//        if(true == isOk)
//        {
//            file.write(info_r.toStdString().data());
//        }
//        file.close();
//    }
//}
//以下3个都为打印标志
void MainWindow::on_pushButton_clicked()
{
    txt = true;
}

void MainWindow::on_pushButton_2_clicked()
{
    txt = false;
}

void MainWindow::on_pushButton_3_clicked()
{
    t_2 = true;
}

void MainWindow::on_pushButton_4_clicked()
{
    t_2 = false;
}

void MainWindow::on_pushButton_5_clicked()
{
    t_3 = true;
}
void MainWindow::on_pushButton_6_clicked()
{
    t_3 = false;
}
//设置画笔宽度信号
void MainWindow::on_pushButton_7_clicked()
{
    int penwidth = ui->lineEdit_7->text().toInt();
    emit signal_set_penwidth(penwidth);
}
//1_实时打印接收到的雷达状态信息
void MainWindow::slotRecvRadarReportInfo_1(QList<RadarStatus> radarStatusList,int val)
{
//    cout<<"显示当前雷达状态";
    foreach (RadarStatus element, radarStatusList) {
        int elelmentID = element.id;// 消息类型
        int min = element.min;// value可设置的最小值
        int max = element.max;// value可设置的最大值
        int value = element.value;// 当前值
        int unit = element.unit; // 值类型
        QString str = RadarStatus::getTypeString(element.id);
//        cout<<"elelmentID"<<elelmentID;
//        cout<<"min"<<min;
//        cout<<"max"<<max;
//        cout<<"value"<<value;
//        cout<<"unit"<<unit;
//        cout<<"str"<<str;
        if(elelmentID == 1)
            ui->Power_lineEdit->setText(QString::number(value));
        if(elelmentID == 2)
            ui->speed_lineEdit->setText(QString::number(value));
        if(elelmentID == 3)
            ui->Antenna_lineEdit->setText(QString::number(value));
        if(elelmentID == 4)
            ui->Bearing_lineEdit->setText(QString::number(value));
        if(elelmentID == 5)
            ui->Rang_lineEdit->setText(QString::number(value));
        if(elelmentID == 6)
            ui->Gain_lineEdit->setText(QString::number(value));
        if(elelmentID == 7)
            ui->Sea_lineEdit->setText(QString::number(value));
        if(elelmentID == 8)
            ui->Rain_lineEdit->setText(QString::number(value));
        if(elelmentID == 9)
            ui->Noise_lineEdit->setText(QString::number(value));
        if(elelmentID == 10)
            ui->Side_lineEdit->setText(QString::number(value));
        if(elelmentID == 11)
            ui->Interference_lineEdit->setText(QString::number(value));
        if(elelmentID == 12)
            ui->Local_lineEdit->setText(QString::number(value));
        if(elelmentID == 13)
            ui->expansion_lineEdit->setText(QString::number(value));
        if(elelmentID == 14)
            ui->boost_lineEdit->setText(QString::number(value));
        if(elelmentID == 15)
            ui->separation_lineEdit->setText(QString::number(value));
    }
}

void MainWindow::slotRecvRangeFactorChanged(double factor)
{
    foreach (ZCHXAnalysisAndSendRadar * server, mAnalysisAndSendRadarList) {
        if(server) server->setRangeFactor(factor);
    }
}
