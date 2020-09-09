#include "zchxmainwindow.h"
#include "ui_zchxmainwindow.h"
#include "dataserverutils.h"
#include <QThread>
#include <QDateTime>
#include <QProcess>
//#include <QDebug>
#include "profiles.h"
#include "Log.h"
#include <QLabel>
#include <QFileDialog>
#include "zmq.h"
#include "zmq_utils.h"
#include "zmq.hpp"
#include "ais/zchxaisdataserver.h"
#include "ais_radar/zchxradardataserver.h"
#include "ais_radar/zchxanalysisandsendradar.h"
#include "ais/zchxaisdataprocessor.h"
#include "dialog_set.h"
#include <QFileDialog>
#include <QDataStream>
#include <QThread>
#include <QRect>
#include <QPainter>
#include <QPointF>
#include "zchxmainwindow.h"
#include "ui_zchxmainwindow.h"
#include <QThread>
#include <QDateTime>
//#include <QDebug>
#include "profiles.h"
#include <QLabel>
#include <QFileDialog>
#include "common.h"
#include <QMessageBox>
#include "protobuf/protobufdataprocessor.h"
#include "ais_setting.h"
#include <QHostInfo>
#include <QNetworkInterface>
#include <QList>
#include <QNetworkAddressEntry>
#include <QRegExp>
#include <QColorDialog>
#include "fusedatautil.h"

#ifdef Q_OS_WIN
#include "zchxmapmonitorthread.h"
#endif

#define         LOG_LINE_COUNT          50

#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"


bool  output_route_path = false;
zchxMainWindow::zchxMainWindow(QWidget *parent) :
    QMainWindow(parent),
    mTestMapMonitorThread(0),
    ui(new Ui::zchxMainWindow)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle(QStringLiteral("采集器20200908"));

    //this->resize(800, 600);
    //this->resize(1367, 784);
    //this->setMinimumSize(1367, 863);
    initUI();

    ais_index = Utils::Profiles::instance()->value("Ais","AIS_Num").toInt();//AIS个数编号
    radar_index =  Utils::Profiles::instance()->value("Radar","Num").toInt();//radar个数编号
    if(radar_index < 1)
    {
        radar_index=1;
        Utils::Profiles::instance()->setValue("Radar","Num",radar_index);
    }
    if(ais_index < 1)
    {
        ais_index=1;
        Utils::Profiles::instance()->setValue("Ais","AIS_Num",ais_index);
    }
    QAction *ais = new QAction(QIcon(":/image/app.png"), "添加AIS",this);
    ui->Ais_menu->addAction(ais);
    QAction *radar = new QAction(QIcon(":/image/app.png"), "雷达",this);
    ui->Radar_menu->addAction(radar);
    QAction *log = new QAction(QIcon(":/image/app.png"), "显示日志",this);
    ui->Log_menu->addAction(log);
    QAction *help = new QAction(QIcon(":/image/app.png"), "查看帮助",this);
    ui->Help_menu->addAction(help);
//    QAction *publish = new QAction(QIcon(":/image/app.png"), "数据状态",this);
//    connect(publish, SIGNAL(triggered(bool)), this, SLOT(slotShowDataStatus()));
//    ui->publish_menu->addAction(publish);

    connect(ui->tabWidget->tabBar(),SIGNAL(tabCloseRequested(int)),this,SLOT(removeSubTab(int)));

    mHelo = new dialog_help;

    connect(log,SIGNAL(triggered()),this,SLOT(logButton()));
    connect(ais,SIGNAL(triggered()),this,SLOT(ais_clicked()));
    connect(radar,SIGNAL(triggered()),this,SLOT(radar_clicked()));
    connect(help,SIGNAL(triggered()),this,SLOT(help_clicked()));

    //初始化雷达界面
    int radarMum =Utils::Profiles::instance()->value("Radar","Num").toInt();
    for(int i = 0; i < radarMum; i++)
    {
        zchxradarinteface *w = new zchxradarinteface(i+1);
        connect(w, SIGNAL(signalSendPortStartStatus(int,int,QString)), this, SLOT(slotRecvPortStartStatus(int,int,QString)));
        zchxradarintefaceList.append(w);
        ui->tabWidget->addTab(w,"雷达-"+QString::number(i+1));
        connect(w, SIGNAL(signalRestart()), this, SLOT(restartMe()));
    }
    connect(zchxradarintefaceList.first(),SIGNAL(signalCombineTrackc(zchxTrackPointMap)),this,SLOT(soltDealTacks1(zchxTrackPointMap)));
    connect(zchxradarintefaceList.last(),SIGNAL(signalCombineTrackc(zchxTrackPointMap)),this,SLOT(soltDealTacks2(zchxTrackPointMap)));
    connect(this,SIGNAL(signalSendComTracks(zchxTrackPointMap)),zchxradarintefaceList.first(),SIGNAL(signalSendComTracks(zchxTrackPointMap)));
    connect(zchxradarintefaceList.first(),SIGNAL(signalCombineVideo(QMap<int, QList<TrackNode>>,int)),this,SLOT(combineVideo(QMap<int, QList<TrackNode>>,int)));
    connect(zchxradarintefaceList.last(),SIGNAL(signalCombineVideo(QMap<int, QList<TrackNode>>,int)),this,SLOT(combineVideo(QMap<int, QList<TrackNode>>,int)));
    connect(this,SIGNAL(signalSendComVideo(QList<TrackNode>)),zchxradarintefaceList.first(),SIGNAL(signalSendComVideo(QList<TrackNode>)));

    //初始化AIS
    aisInit();

    //初始化融合单例
    FuseDataUtil::getInstance();

    //弹窗显示数据日志
    mLog = new Dialog_log;
    connect(this, SIGNAL(receiveLogSignal(qint64,QString,QString)),mLog,SLOT(receiveLogSlot(qint64,QString,QString)));

#ifdef Q_OS_WIN
    //启动客户端
    Utils::Profiles::instance()->setDefault("Ecdis","Enable", 0);
    bool ecdis = Utils::Profiles::instance()->value("Ecdis","Enable", 0).toBool();
    if(ecdis)
    {
        mTestMapMonitorThread = new zchxMapMonitorThread("zchxMapTest.exe", this);
//        mTestMapProcess->start("zchxMapTestLoader.exe", QStringList()<<"zchxMapTest.exe");
        mTestMapMonitorThread->start();
    } else
    {
        mTestMapMonitorThread = 0;
    }
#endif


//    {
//        double start_lat = 30.123456, start_lon = 113.123456;
//        double end_lat = 30.123456, end_lon = 113.123556;
//        zchxTargetPredictionLine line(start_lat, start_lon, end_lat, end_lon, 200, Prediction_Area_Rectangle);
//        double gis_dis = getDisDeg(start_lat, start_lon, end_lat, end_lon);
//        double mercator_length = line.length();
//        qDebug()<<"gis_dis:"<<gis_dis<<" mercator length:"<<mercator_length;
//        if(!line.isValid())
//        {
//            qDebug()<<"mercator line is invalid";
//        }
//        Mercator now = latlonToMercator(30.123856, 113.123456);
//        //计算点到预估位置的距离
//        double distance_pt = now.distanceToPoint(start_lat, start_lon);
//        double distance_line = line.distanceToMe(now);
//        qDebug()<<"point with distance(pt, pl)"<<distance_pt<<distance_line;
//        //检查是否在连线的范围内
//        if(!line.isPointIn(now, 200, Prediction_Area_Rectangle))
//        {
//            qDebug()<<"point is not in check area";
//        } else
//        {
//            qDebug()<<"point is in check area";
//        }
//    }
}

void zchxMainWindow::slotShowDataStatus()
{
    if(mDataPortSts.size() == 0) return;
}

void zchxMainWindow::initUI()
{

    QIcon icon = QIcon(":/image/app.png");
    if(icon.isNull())
    {
        cout<<"icon image not found!!!!!!!!!!!!!!!!!!";
    } else
    {
        cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<endl;
    }
    this->setWindowIcon(icon);
    ui->tabWidget->setCurrentIndex(0);//显示的页面

}

void zchxMainWindow::closeEvent(QCloseEvent *)
{
    if(mTestMapMonitorThread)
    {
        delete mTestMapMonitorThread;
        mTestMapMonitorThread = 0;
    }
    //mAnalysisAndSendRadarList[0]->closeTT();
    QProcess p(0);
    p.start("cmd", QStringList()<<"/c"<<QString("taskkill /f /im %1").arg(QApplication::applicationName()));
    p.waitForStarted();
    p.waitForFinished();

}

void zchxMainWindow::receiveContent(qint64 time, const QString& name, const 
                                    QString& content)
{
//    ui->listWidget->insertItem(0, QString("%1---%2---%3").arg(DataServerUtils::time2String(time, true)).arg(name).arg(content));
//    if(ui->listWidget->count() > 100)
//    {
//        QListWidgetItem* item = ui->listWidget->takeItem(99);
//        delete item;
//    }
    emit receiveLogSignal(time, name, content);
}

void zchxMainWindow::slotRecvHearMsg(QString msg)
{
//    slotInsertLogInfo(msg);
    receiveContent(QDateTime::currentMSecsSinceEpoch(), "NETWORK", msg);
}

void zchxMainWindow::slotUpdateClientTable(const QString& ip, const QString& name, int port, int inout)
{
    QString sIpPort = ip+QString::number(port);
    if(inout == 1)
    {
        //客户端连上
        if(!mClientList.contains(sIpPort))
        {
            cout<<"连接"<<sIpPort;
            mClientList.append(sIpPort);
            //添加到第一行
//            ui->tableWidget->insertRow(0);
//            ui->tableWidget->setItem(0, 0, new HqTableWidgetItem(name));
//            ui->tableWidget->setItem(0, 1, new HqTableWidgetItem(ip));
//            ui->tableWidget->setItem(0, 2, new HqTableWidgetItem(QString::number(port)));
//            ui->tableWidget->setItem(0, 3, new HqTableWidgetItem(QStringLiteral("已连接")));
//            ui->tableWidget->setItem(0, 4, new HqTableWidgetItem(DataServerUtils::currentTimeString()));
//            ui->tableWidget->setItem(0, 5, new HqTableWidgetItem("-"));
        }
    } else
    {
        //客户端离开
        if(mClientList.contains(sIpPort))
        {
            mClientList.removeOne(sIpPort);
            //从表格删除
            cout<<"断开"<<sIpPort;
//            for(int i=0; i<ui->tableWidget->rowCount(); i++)
//            {
//                QTableWidgetItem *item = ui->tableWidget->item(i, 1);
//                QTableWidgetItem *portItem = ui->tableWidget->item(i, 2);

//                if(item && portItem)
//                {
//                    QString str = item->text()+portItem->text();
//                    if(sIpPort == str)
//                    {
//                        ui->tableWidget->item(i,3)->setText(QStringLiteral("已断开"));
//                        ui->tableWidget->item(i,5)->setText(DataServerUtils::currentTimeString());
//                    }
//                }
//            }
        }
    }
    emit updateCliSignal(ip, name, port, inout);//更新客户端信号
}

zchxMainWindow::~zchxMainWindow()
{

}

void zchxMainWindow::combineVideo(QMap<int, QList<TrackNode>> map,int num)
{
    cout<<"num"<<num;
    mVideoMap[num] = map;
    QList<TrackNode> combinVideoList;
    QMap<int,QMap<int, QList<TrackNode>>>::iterator mIterator;
    QMap<int, QList<TrackNode>>::iterator mIterator2;
    for(mIterator = mVideoMap.begin(); mIterator != mVideoMap.end(); ++mIterator)
    {
        for(mIterator2 = mIterator.value().begin(); mIterator2 != mIterator.value().end(); ++mIterator2)
        {
            QList<TrackNode> mList = mIterator2.value();
            for(int j = 0; j < mList.size(); j++)
            {
                TrackNode mTrackNode = mList[j];
                combinVideoList.append(mTrackNode);
            }
        }
    }
    signalSendComVideo(combinVideoList);
}


void zchxMainWindow::soltDealTacks1(const zchxTrackPointMap& m1)
{
    //cout<<"m1"<<m1.size();
    t1 = m1;
    combineTracks();
}

void zchxMainWindow::soltDealTacks2(const zchxTrackPointMap& m2)
{
    //cout<<"m2"<<m2.size();
    t2 = m2;
}

void zchxMainWindow::combineTracks()
{
#if 0
    //cout<<"t2"<<t2.size();
    //if(t1.size()<1)return;
    t3.clear();
    t3 = t1;
    int distance = Utils::Profiles::instance()->value("Radar_1","Radius").toInt();
    //cout<<"t1t2"<<t1.size()<<t2.size();
    foreach (zchxTrackPoint pnt2, t2) {
        bool flag = 0;
        foreach (zchxTrackPoint pnt1, t1) {
            double dis = getDisDeg(pnt1.wgs84poslat(),pnt1.wgs84poslong(), pnt2.wgs84poslat(), pnt2.wgs84poslong());
            if(dis <= distance)
            {
                flag = 1;
                break;
            }
         }
        if(!flag)
        {
            t3[pnt2.tracknumber()] = pnt2;
        }
    }
    //cout<<"t1"<<t1.size()<<"t2"<<t2.size()<<"t3"<<t3.size();
    signalSendComTracks(t3);
#endif
}

//删除显示页
void zchxMainWindow::removeSubTab(int dex)
{
    cout<<"页面编号index"<<dex;
    QString result = ui->tabWidget->tabText(dex);
    ui->tabWidget->removeTab(dex);
    cout<<result;
    QRegExp na("(\\w)"); //初始化名称结果
    QString name("");
    if(na.indexIn(result) != -1)
    {
        //匹配成功
        name = na.cap(0);
    }
    cout<<"name"<<name;
    if(name == "雷")
    {
        radar_index--;
        Utils::Profiles::instance()->setValue("Radar","Num",radar_index);
    }
    if(name == "A")
    {
        ais_index--;
        Utils::Profiles::instance()->setValue("Ais","AIS_Num",ais_index);
    }
}

//AIS菜单按下
void zchxMainWindow::ais_clicked()
{
    ais_index++;
    Utils::Profiles::instance()->setValue("Ais","AIS_Num",ais_index);
    AIS_Setting *mAisSetting = new AIS_Setting(ais_index);
    ui->tabWidget->insertTab(ui->tabWidget->count(), mAisSetting,"AIS-"+QString::number(ais_index));
    //重新生成AIS解析对象
    connect(mAisSetting,SIGNAL(newAisClassSignal()),this,SLOT(newAisClassSlot()));
}

//雷达菜单按下
void zchxMainWindow::radar_clicked()
{
    int num =  Utils::Profiles::instance()->value("Radar","Num").toInt();//radar个数编号
    cout<<"当前有几个雷达界面"<<num;
    num++;
    radar_index = num;
    Utils::Profiles::instance()->setValue("Radar","Num",radar_index);
    zchxradarinteface *w = new zchxradarinteface(num);
    zchxradarintefaceList.append(w);
    ui->tabWidget->addTab(w,"雷达-"+QString::number(num));
}

void zchxMainWindow::aisInit()
{
//    mAisDataProc = new zchxAisDataProcessor(0);
//    emit mAisDataProc->signalInitZmq();
    for(int i = 0; i < ais_index; i++)
    {
        AIS_Setting *mAisSetting = new AIS_Setting(i+1);
        ui->tabWidget->addTab(mAisSetting,"AIS-"+QString::number(i+1));
        //重新生成AIS解析对象
        connect(mAisSetting,SIGNAL(newAisClassSignal()),this,SLOT(restartMe()));
    }
}

//帮助菜单按下
void zchxMainWindow::help_clicked()
{
    mHelo->show();
}

//弹窗显示日志
void zchxMainWindow::logButton()
{
    mLog->exec();
}

//保存析构老对象新生成AIS解析对象
void zchxMainWindow::newAisClassSlot()
{
    cout<<"重新启动采集器";
    QString program = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    QString workingDirectory = QDir::currentPath();
    QProcess::startDetached(program, arguments, workingDirectory);
    QApplication::exit();
}

void zchxMainWindow::restartMe()
{
    if(mTestMapMonitorThread)
    {
        delete mTestMapMonitorThread;
        mTestMapMonitorThread = 0;
    }

    cout<<"重新启动采集器";
    QString program = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    QString workingDirectory = QDir::currentPath();
    QProcess::startDetached(program, arguments, workingDirectory);
    QApplication::exit();
}

void zchxMainWindow::slotRecvPortStartStatus(int port, int sts, const QString &topic)
{
    qDebug()<<"recv port stats:"<<port<<sts<<topic;
    QString result = QString("Port: %1 Status: %2  Tpoic:%3").arg(port).arg(sts == true ? "ON" : "OFF").arg(topic);
    mDataPortSts[port] = result;
    ui->publish_menu->clear();
    foreach (QString val , mDataPortSts.values()) {
        ui->publish_menu->addAction(new QAction(val, this));
    }
}
