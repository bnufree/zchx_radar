#include "zchxradarinteface.h"
#include "ui_zchxradarinteface.h"
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
#include "myLabel.h"
//#include <Windows.h>
//#include <DbgHelp.h>
#include <QThread>
#include <QDateTime>
#include <QDebug>
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
#include "zchxsimulatethread.h"

#define         LOG_LINE_COUNT          50

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

zchxradarinteface::zchxradarinteface(int ID,QWidget *parent) :
    QMainWindow(parent),
    radarId(ID),
    mReportSimulateThread(0),
    mRadarDataServer(0),
    mAnalysisAndSendRadar(0),
    ui(new Ui::zchxradarinteface)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    ui->import_report_btn->setVisible(true);
    ui->radar_type_combox->addItem(QStringLiteral("BR24雷达"), RADAR_BR24);
    ui->radar_type_combox->addItem(QStringLiteral("3G雷达"), RADAR_3G);
    ui->radar_type_combox->addItem(QStringLiteral("4G雷达"), RADAR_4G);
    ui->radar_type_combox->addItem(QStringLiteral("6G雷达"), RADAR_6G);
    ui->menubar->setStyleSheet(""
    "QMenuBar{" //将菜单栏背景改为白色
    "background-color:white;"
    "}"
    );


    //地图初始化
#ifdef USE_ZCHX_ECDIS
    ui->verticalLayout_4->addWidget((mMap = new QMapWidget(this)));
#else
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->ecdis));
#endif
    //this->resize(1367, 784);
    //this->setMinimumSize(1367, 805);
    initUI();
    m_drawpic = true;//采集器绘制回波标志
    zeor_flag = 0;//雷达是否正常接收回波标志

    str_radar = QString("Radar_%1").arg(radarId);

    txt = Utils::Profiles::instance()->value(str_radar,"Limit_zhou").toBool();//1_初始化打印回波标志
    t_2 = false;//1_初始化打印AIS标志
    t_3= false;//1_初始化打印回波标志
    index = Utils::Profiles::instance()->value("Ais","AIS_Num").toInt();//AIS个数编号
    ui->lineEdit_2->setText(Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toString());
    ui->radius_lineEdit->setText(Utils::Profiles::instance()->value(str_radar,"Radius").toString());

    ui->lineEdit_3->setText(QString::number(0));
    ui->lineEdit_4->setText(QString::number(0));

    ui->minzf_lineEdit->setText(Utils::Profiles::instance()->value(str_radar,"min_amplitude").toString());
    ui->maxzf_lineEdit->setText(Utils::Profiles::instance()->value(str_radar,"max_amplitude").toString());
    ui->angle_lineEdit->setText(Utils::Profiles::instance()->value(str_radar,"historyNum").toString());
    Utils::Profiles::instance()->setDefault("Echo", "Enable", false);
    QString k = Utils::Profiles::instance()->value(str_radar,"RadiusCoefficient").toString();
    ui->k_lineEdit->setText(QString::number(k.toDouble(), 'f', 4));

    //从串口接收GPS数据 ****************************************************************************
    //初始化COM数据接收和解析
//    mComConfigWidget = new ComConfigWidget;
//    mComDataMgr = new ComDataMgr;
//    connect(mComDataMgr, SIGNAL(signalSendRecvedContent(qint64,QString,QString)), \
//            this, SLOT(receiveContent(qint64,QString,QString)));
//    connect(mComDataMgr, SIGNAL(signalSendLogMsg(QString)), this, SLOT(slotRecvWorkerMsg(QString)));
//    //界面参数配置和数据接收线程连接在一起)),
//    connect(mComConfigWidget, SIGNAL(updateSerialPort(QMap<QString,COMDEVPARAM>)),
//            this, SLOT(slotSetComDevParams(QMap<QString,COMDEVPARAM>)));

//    //发送出去
//    PROFILES_INSTANCE->setDefault(SERVER_SETTING_SEC, "GPS_Topic", "GPSData");
//    PROFILES_INSTANCE->setDefault(SERVER_SETTING_SEC, "GPS_Send_Port", 5656);
//    mOutWorker = new ComDataPubWorker();
//    connect(PROTOBUF_DATA, SIGNAL(signalSendComData(QByteArray)), mOutWorker, SLOT(slotRecvPubData(QByteArray)));
//    connect(PROTOBUF_DATA, SIGNAL(signalSendGpsData(double, double)), ui->settingWidget, SLOT(slotGetGpsData(double,double)));

//    mOutWorker->setServerIP(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, SERVER_SETTING_SERVER_IP).toString());

//    mOutWorker->setServerPort(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, SERVER_SETTING_SERVER_PORT).toInt());
//    mOutWorker->setTimerInterval(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, SERVER_SETTING_OPLOAD_FREQUENCY).toInt());
//    mOutWorker->setTrackSendPort(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_Send_Port").toInt());
//    mOutWorker->setTrackTopic(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_Topic").toString());
//    connect(mComConfigWidget,SIGNAL(signalUpdataZmq(QString,QString)),this,SLOT(slotUpdateZmq(QString,QString)));
//    //开始数据接收
//    slotSetComDevParams(mComConfigWidget->getComDevParams());
//    PROTOBUF_DATA->startPublish();

    int i = radarId;
    str_radar = QString("Radar_%1").arg(i);
    QString sVideoIP = Utils::Profiles::instance()->value(str_radar,"Video_IP").toString();
    int uVideoPort = Utils::Profiles::instance()->value(str_radar,"Video_Port").toInt();
    QString sHeartIP = Utils::Profiles::instance()->value(str_radar,"Heart_IP").toString();
    int uHeartPort = Utils::Profiles::instance()->value(str_radar,"Heart_Port").toInt();
    QString dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toString();
    QString dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toString();
    QString sControlIP = Utils::Profiles::instance()->value(str_radar,"Report_IP").toString();
    int uControlPort = Utils::Profiles::instance()->value(str_radar,"Report_Port").toInt();
    QString sLimit_File = Utils::Profiles::instance()->value(str_radar,"Limit_File").toString();
    QString m_jupmdis = Utils::Profiles::instance()->value(str_radar,"jump_distance").toString();
    QString radar_num = Utils::Profiles::instance()->value(str_radar,"radar_num").toString();
    Utils::Profiles::instance()->setDefault(str_radar,"track_max_area", "40000");
    Utils::Profiles::instance()->setDefault(str_radar,"track_min_area", "100");
    QString track_max_area = Utils::Profiles::instance()->value(str_radar,"track_max_area").toString();
    QString track_min_area=  Utils::Profiles::instance()->value(str_radar,"track_min_area").toString();


    QString track_radius = Utils::Profiles::instance()->value(str_radar,"track_radius").toString();
    QString track_min_radius =  Utils::Profiles::instance()->value(str_radar,"track_min_radius").toString();
    QString restart_time = Utils::Profiles::instance()->value("Radar","restart_time",1).toString();
    bool mf = Utils::Profiles::instance()->value("Radar","restart_flag").toBool();
    bool mSend = Utils::Profiles::instance()->value(str_radar,"send_dianjian").toBool();
    Utils::Profiles::instance()->setDefault(str_radar, "Direction_Invert", "90");
    QString direction_change = Utils::Profiles::instance()->value(str_radar, "Direction_Invert", "90").toString();
    ui->direction_change_edit->setText(direction_change);

    Utils::Profiles::instance()->setDefault(str_radar, "azimuth_adjustment", 1);
    bool azimuth_adjust = Utils::Profiles::instance()->value(str_radar, "azimuth_adjustment", 1).toBool();
    ui->cog_adjust_chk->setChecked(azimuth_adjust);
    Utils::Profiles::instance()->setDefault(str_radar, "video_cycle_or", 1);
    int video_cycle_or = Utils::Profiles::instance()->value(str_radar, "video_cycle_or", 1).toInt();
    ui->video_or_count->setValue(video_cycle_or);

    Utils::Profiles::instance()->setDefault(str_radar, "native_radius", false);
    bool native_radius = Utils::Profiles::instance()->value(str_radar, "native_radius", false).toBool();
    ui->useNativeRadiusCHK->setChecked(native_radius);

    Utils::Profiles::instance()->setDefault(str_radar, "prediction_width", 20);
    int prediction_width = Utils::Profiles::instance()->value(str_radar, "prediction_width", 20).toInt();
    ui->predictionWidth->setValue(prediction_width);


    ui->send_dj_checkBox->setChecked(mSend);
    ui->restart_checkBox->setChecked(mf);
    ui->restart_lineEdit->setText(restart_time);
    ui->minTradius_lineEdit->setText(QString::number(track_min_radius.toDouble(), 'f', 2) );
    ui->bh_lineEdit->setText(radar_num);
    ui->tradius_lineEdit->setText(QString::number(track_radius.toDouble(), 'f', 2) );
    ui->minAreaEdit->setText(track_min_area);
    ui->maxAreaEdit->setText(track_max_area);
    ui->jump_lineEdit->setText(m_jupmdis);
    ui->lineEdit_7->setText(sLimit_File);
    ui->lineEdit_8->setText( QString::number(dCentreLon.toDouble(), 'f', 6));
    ui->lineEdit_9->setText(QString::number(dCentreLat.toDouble(), 'f', 6));
    ui->videoRecIPLlineEdit->setText(sVideoIP);
    ui->videoRecPortSpinBox->setValue(uVideoPort);
    ui->controlIPLineEdit->setText(sControlIP);
    ui->controlPortSpinBox->setValue(uControlPort);
    ui->heartIPLlineEdit->setText(sHeartIP);
    ui->heartPortSpinBox->setValue(uHeartPort);
    //
    restartTimer = new QTimer();
    connect(restartTimer, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    restartTimer->start(restart_time.toInt() * 60000);
    //接收雷达数据
    mRadarDataServer = new ZCHXRadarDataServer(radarId, 0);
    connect(this, SIGNAL(send_report_signal(QByteArray,int)), mRadarDataServer, SLOT(ProcessReport(QByteArray,int)));
    //处理雷达数据并发送
    ui->settingWidget->setId(radarId);
    mAnalysisAndSendRadar = new ZCHXAnalysisAndSendRadar(radarId,0);
    connect(mRadarDataServer, SIGNAL(signalSendRadarType(int)), mAnalysisAndSendRadar, SLOT(slotSetRadarType(int)));
    emit mRadarDataServer->startProcessSignal();//开启接收

    connect(mAnalysisAndSendRadar,SIGNAL(signalCombineTrackc(zchxTrackPointMap)),this,SIGNAL(signalCombineTrackc(zchxTrackPointMap)));
    connect(mRadarDataServer, SIGNAL(analysisLowranceRadar(QByteArray,int,int,int)),
            mAnalysisAndSendRadar, SIGNAL(analysisLowranceRadarSignal(QByteArray,int,int,int)));
    connect(mRadarDataServer, SIGNAL(analysisCatRadar(QByteArray,int,int,int,QString)),
            mAnalysisAndSendRadar, SIGNAL(analysisCatRadarSignal(QByteArray,int,int,int,QString)));
    connect(mAnalysisAndSendRadar, SIGNAL(signalSendRecvedContent(qint64,QString,QString)), this, SLOT(receiveContent(qint64,QString,QString)));
    connect(mRadarDataServer, SIGNAL(signalSendRecvedContent(qint64,QString,QString)), this, SLOT(receiveContent(qint64,QString,QString)));
    connect(mRadarDataServer, SIGNAL(signalClientInout(QString,QString,int,int)), this, SLOT(slotUpdateClientTable(QString,QString,int,int)));
    connect(mAnalysisAndSendRadar, SIGNAL(signalClientInout(QString,QString,int,int)), this, SLOT(slotUpdateClientTable(QString,QString,int,int)));
    connect(this, SIGNAL(signalOpenRadar()), mRadarDataServer, SLOT(openRadar()));
    connect(this, SIGNAL(signalcloseRadar()), mRadarDataServer, SLOT(closeRadar()));
    connect(mRadarDataServer, SIGNAL(signalRadarStatusChanged(QList<RadarStatus>, int)), ui->settingWidget, SLOT(slotRecvRadarReportInfo(QList<RadarStatus>,int)));
    connect(mAnalysisAndSendRadar, SIGNAL(signalRadiusFactorUpdated(double,double)), ui->settingWidget, SLOT(slotUpdateRealRangeFactor(double,double)));
    connect(mAnalysisAndSendRadar, SIGNAL(signalRadiusFactorUpdated(double,double)), this, SLOT(show_range_slot(double,double)));
    connect(mAnalysisAndSendRadar,SIGNAL(signalCombineVideo(QMap<int, QList<TrackNode>>,int)),this,SIGNAL(signalCombineVideo(QMap<int, QList<TrackNode>>,int)));
    connect(this,SIGNAL(signalSendComVideo(QList<TrackNode>)),mAnalysisAndSendRadar,SLOT(slotDrawCombinVideo(QList<TrackNode>)));
    //显示半径系数,半径参考值
    connect(mAnalysisAndSendRadar,SIGNAL(singalShowRadiusCoefficient(double,double)),this,SLOT(slotShowRadiusCoefficient(double,double)));
    //不融合
    connect(this,SIGNAL(signalSendComTracks(zchxTrackPointMap)),mAnalysisAndSendRadar,SLOT(slotSendComTracks(zchxTrackPointMap)));
    //控制端
    connect(this,SIGNAL(signalSetPix(QPixmap)),mRadarDataServer,SLOT(setPixSlot(QPixmap)));
    connect(mRadarDataServer, SIGNAL(signalRadarConfigChanged(int,int,int)), this, SLOT(slotRadarConfigChanged(int,int,int)));

 // 雷达融合处理
//    connect(this, SIGNAL(signalSendComTracks(Radar_Track_Map)),
//            FuseDataUtil::getInstance(), SLOT(slotSendComTracks(Radar_Track_Map)));
//    connect(FuseDataUtil::getInstance(), SIGNAL(sendComTracks(Radar_Track_Map)),
//            mAnalysisAndSendRadar, SLOT(slotSendComTracks(Radar_Track_Map)));
    //浮标设置
    connect(&mFloat,SIGNAL(updateFloatSignal()),mAnalysisAndSendRadar,SLOT(updateFloatSlot()));
    connect(mRadarDataServer, SIGNAL(joinGropsignal(QString)), this, SLOT(joinGropslot(QString)));
//    connect(mAnalysisAndSendRadar->getVideoProcessor(), SIGNAL(sendProcessTrackInfo(int,int,int)), this, SLOT(slotRecvRadarProcessNum(int,int,int)));

    //实时更新GPS传入的经纬度坐标
    connect(PROTOBUF_DATA, SIGNAL(signalSendGpsData(double, double)),mAnalysisAndSendRadar, SLOT(slotGetGpsData(double,double)));

    //弹窗显示-------雷达配置
    mSet = new Dialog_set(radarId);
    connect(PROTOBUF_DATA, SIGNAL(signalSendGpsData(double, double)),mSet, SIGNAL(signalGetGpsData(double,double)));
    connect(mAnalysisAndSendRadar, SIGNAL(signalRadiusFactorUpdated(double,double)), mSet, SLOT(slotUpdateRealRangeFactor(double,double)));
    connect(mSet, SIGNAL(signalRangeFactorChanged_1(double)), this, SLOT(slotRecvRangeFactorChanged(double)));
    //调用mRadarDataServer析构函数

    connect(mSet, SIGNAL(set_change_signal_1()), this, SLOT(reset_window()));
    connect(this, SIGNAL(send_video_signal(QByteArray,QString,int,int,int)),mRadarDataServer,SLOT(analysisRadar(QByteArray,QString,int,int,int)));

    //显示分析页面
    connect(mAnalysisAndSendRadar, SIGNAL(show_video(int,int)), this, SLOT(show_video_slot(int,int)));//打印目标个数
    connect(mAnalysisAndSendRadar, SIGNAL(show_statistics(int,int,int,int,int)), this, SLOT(show_statistics_slot(int,int,int,int,int)));//打印丢包率
    connect(mAnalysisAndSendRadar, SIGNAL(show_info(QString)),this,SLOT(show_info_slot(QString)));//打印传入周老师库数据
    //设置回波图像素点大小
    connect(this, SIGNAL(signal_set_penwidth(int)), mAnalysisAndSendRadar, SIGNAL(set_pen_width(int)));
    //实时打印接收到的雷达状态信息
    connect(mRadarDataServer, SIGNAL(signalRadarStatusChanged(QList<RadarStatus>, int)),this,SLOT(slotRecvRadarReportInfo_1(QList<RadarStatus>,int)));
    //采集器显示回波图片
    connect(mAnalysisAndSendRadar,SIGNAL(signalRadarVideoAndTargetPixmap(QPixmap,Afterglow)),this,SLOT(setRadarVideoAndTargetPixmap(QPixmap,Afterglow)));
    //打印回波数据
    connect(this, SIGNAL(prtVideoSignal(bool)),mRadarDataServer,SIGNAL(prtVideoSignal_1(bool)));
    //是否显示雷达目标编号
    connect(this,SIGNAL(showTrackNumSignal(bool)),mAnalysisAndSendRadar,SIGNAL(showTrackNumSignal(bool)));

    //回波颜色设置
    connect(this,SIGNAL(colorSetSignal(int,int,int,int,int,int)),mAnalysisAndSendRadar,SIGNAL(colorSetSignal(int,int,int,int,int,int)));
    connect(mRadarDataServer,SIGNAL(colorSetSignal(int,int,int,int,int,int)),this,SIGNAL(colorSetSignal(int,int,int,int,int,int)));
    //ui->frame->setVisible(false);
    //隐藏前3个分页
    //int count = ui->tabWidget->count();
    for(int i = 0; i < 3; i++)
    {
        if(ui->tabWidget->count())
            ui->tabWidget->removeTab(0);
    }
    QAction *log = new QAction(QIcon(":/image/app.png"), "显示日志",this);
    ui->Log_menu->addAction(log);
    QAction *cli = new QAction(QIcon(":/image/app.png"), "显示客户端",this);
    ui->Cli_menu->addAction(cli);
    QAction *help = new QAction(QIcon(":/image/app.png"), "查看帮助",this);
    ui->Help_menu->addAction(help);
    color1 = new QAction(QIcon(":/image/app.png"), "颜色一",this);
    color2 = new QAction(QIcon(":/image/app.png"), "颜色二",this);
    ui->Color_menu->addAction(color1);
    ui->Color_menu->addAction(color2);
    connect(ui->tabWidget->tabBar(),SIGNAL(tabCloseRequested(int)),this,SLOT(removeSubTab(int)));
    //显示距离因子和半径
    connect(ui->settingWidget, SIGNAL(signalRangeFactorChanged(double)), this, SLOT(slotRecvRangeFactorChanged(double)));
    //connect(ui->settingWidget, SIGNAL(set_change_signal()), this, SLOT(reset_window()));

    //弹窗显示数据日志
    mLog = new Dialog_log;
    connect(this, SIGNAL(receiveLogSignal(qint64,QString,QString)),mLog,SLOT(receiveLogSlot(qint64,QString,QString)));
    //弹窗显示客户端
    mCli = new Dialog_cli;
    connect(this, SIGNAL(updateCliSignal(QString,QString,int,int)),mCli,SLOT(slotUpdateClientTable(QString,QString,int,int)));
    //弹窗显示帮助
    mHelo = new dialog_help;
    connect(log,SIGNAL(triggered()),this,SLOT(logButton()));
    connect(cli,SIGNAL(triggered()),this,SLOT(setButton_2()));
    connect(help,SIGNAL(triggered()),this,SLOT(help_clicked()));
    connect(color1,SIGNAL(triggered()),this,SLOT(on_color1Button_clicked()));
    connect(color2,SIGNAL(triggered()),this,SLOT(on_color2Button_clicked()));
    //获取所有网络接口的列表
    ui->mac_comboBox->addItems(getAllIpv4List());
    QString ip_str = Utils::Profiles::instance()->value("Radar_Control","Mac_IP").toString();
    cout<<"ip_str"<<ip_str;
    ui->mac_comboBox->setCurrentText(ip_str);
    //21类AIS数据
    mAisbaseinfosetting = new aisBaseInfoSetting();
    mAisbaseinfosetting->startProcess();
    //回波颜色
    a1 =(Utils::Profiles::instance()->value("Color","color1_R",255).toInt());
    a2 = (Utils::Profiles::instance()->value("Color","color1_G",255).toInt());
    a3 = (Utils::Profiles::instance()->value("Color","color1_B",255).toInt());
    b1 = (Utils::Profiles::instance()->value("Color","color2_R",255).toInt());
    b2 = (Utils::Profiles::instance()->value("Color","color2_G",255).toInt());
    b3 = (Utils::Profiles::instance()->value("Color","color2_B",255).toInt());

    //初始化雷达状态控制相关
    ui->antenna_btn->setType(ANTENNA_HEIGHT);
    ui->antenna_btn->setServer(mRadarDataServer);
    ui->bearing_btn->setType(BEARING_ALIGNMENT);
    ui->bearing_btn->setServer(mRadarDataServer);

    ui->range_btn->setAdjustmode(LARGE_ADJUST_MODE);
    ui->range_btn->setType(RANG);
    ui->range_btn->setServer(mRadarDataServer);
    ui->gain_btn->setType(GAIN);
    ui->gain_btn->setServer(mRadarDataServer);

    ui->rain_culter_btn->setType(RAIN_CLUTTER);
    ui->rain_culter_btn->setServer(mRadarDataServer);

    ui->sea_culter_btn->setType(SEA_CLUTTER);
    ui->sea_culter_btn->setServer(mRadarDataServer);

    ui->sidelobe_btn->setType(SIDE_LOBE_SUPPRESSION);
    ui->sidelobe_btn->setServer(mRadarDataServer);

    ui->scan_seed_btn->setType(SCAN_SPEED);
    ui->scan_seed_btn->setAdjustmode(SMALL_ADJUST_MODE);
    ui->scan_seed_btn->setServer(mRadarDataServer);

    ui->noise_injection_btn->setType(NOISE_REJECTION);
    ui->noise_injection_btn->setAdjustmode(SMALL_ADJUST_MODE);
    ui->noise_injection_btn->setServer(mRadarDataServer);

    ui->local_injection_btn->setType(LOCAL_INTERFERENCE_REJECTION);
    ui->local_injection_btn->setAdjustmode(SMALL_ADJUST_MODE);
    ui->local_injection_btn->setServer(mRadarDataServer);

    ui->target_boost_btn->setType(TARGET_BOOST);
    ui->target_boost_btn->setAdjustmode(SMALL_ADJUST_MODE);
    ui->target_boost_btn->setServer(mRadarDataServer);

    ui->target_separat_btn->setType(TARGET_SEPARATION);
    ui->target_separat_btn->setAdjustmode(SMALL_ADJUST_MODE);
    ui->target_separat_btn->setServer(mRadarDataServer);

    ui->target_expansion_btn->setType(TARGET_EXPANSION);
    ui->target_expansion_btn->setAdjustmode(SMALL_ADJUST_MODE);
    ui->target_expansion_btn->setServer(mRadarDataServer);

    ui->injection_btn->setType(INTERFERENCE_REJECTION);
    ui->injection_btn->setAdjustmode(SMALL_ADJUST_MODE);
    ui->injection_btn->setServer(mRadarDataServer);

    ui->power_statsu_btn->setType(POWER);
    ui->power_statsu_btn->setServer(mRadarDataServer);
    ui->power_statsu_btn->setAdjustmode(SMALL_ADJUST_MODE);
}


void zchxradarinteface::initUI()
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

    connect(ui->settingWidget, SIGNAL(signalRadarConfigChanged(int,int,int)), this, SLOT(slotRadarConfigChanged(int,int,int)));

    connect(this, SIGNAL(signalRadarConfigChanged(int,int,int)), this, SLOT(slotRadarConfigChanged(int,int,int)));
    //ui->doubleSpinBox->setValue(13.0);

    //初始化雷达控制的列表
     ui->saveRadarBtn->setVisible(false);
     ui->uploadRadarBtn->setVisible(false);
}

void zchxradarinteface::closeEvent(QCloseEvent *)
{
    //mAnalysisAndSendRadarList[0]->closeTT();
    QProcess p(0);
    p.start("cmd", QStringList()<<"/c"<<"taskkill /f /im radar_data_collect_server.exe");
    p.waitForStarted();
    p.waitForFinished();

}

void zchxradarinteface::slotDisplaycurTime()
{
    mTimeLable->setText(DataServerUtils::currentTimeString());
}


void zchxradarinteface::slotRecvWorkerMsg(const QString &msg)
{
    slotRecvHearMsg(msg);
}

void zchxradarinteface::slotRecvHearMsg(QString msg)
{
//    slotInsertLogInfo(msg);
    receiveContent(QDateTime::currentMSecsSinceEpoch(), "NETWORK", msg);
}

void zchxradarinteface::slotInsertLogInfo(const QString &msg)
{

}

void zchxradarinteface::slotUpdateVirtualIpString(const QString &msg)
{
    mVirtualIpWorkingLbl->setText(msg);
    mVirtualIpWorkingLbl->setStyleSheet("color:red;font-weight:bold;font-size:18;");
}

zchxradarinteface::~zchxradarinteface()
{
    if(mRadarDataServer)
    {
        mRadarDataServer->deleteLater();
    }
    if(mAnalysisAndSendRadar)
    {
        mAnalysisAndSendRadar->deleteLater();
    }
    delete mLog;//日志弹窗
    delete mSet;//雷达配置
    delete mCli;
    delete ui;
}

void zchxradarinteface::receiveContent(qint64 time, const QString& name, const QString& content)
{
    ui->listWidget->insertItem(0, QString("%1---%2---%3").arg(DataServerUtils::time2String(time, true)).arg(name).arg(content));
    if(ui->listWidget->count() > 100)
    {
        QListWidgetItem* item = ui->listWidget->takeItem(99);
        delete item;
    }
    emit receiveLogSignal(time, name, content);
}

void zchxradarinteface::slotUpdateClientTable(const QString& ip, const QString& name, int port, int inout)
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
            cout<<"断开"<<sIpPort;
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
    emit updateCliSignal(ip, name, port, inout);//更新客户端信号
}
//打开雷达
void zchxradarinteface::on_openRadarBtn_clicked()
{
    emit signalOpenRadar();
}
//关闭雷达
void zchxradarinteface::on_closeRadarBtn_clicked()
{
    emit signalcloseRadar();
}

void zchxradarinteface::on_uploadRadarBtn_clicked()
{

//    int radarID = radarId;
//    bool ok;
//    // 雷达电源
//    int type = INFOTYPE::POWER;
//    int value = ui->power_status_cbx->currentData().toInt();
//    emit signalRadarConfigChanged(radarID, type, value);

//    // 扫描速度
//    type = INFOTYPE::SCAN_SPEED;
//    value = ui->scan_speed_cbx->currentData().toInt();
//    emit signalRadarConfigChanged(radarID, type, value);

    // 天线高度
//    type = INFOTYPE::ANTENNA_HEIGHT;
//    value = ui->Antenna_lineEdit->text().toInt(&ok);
//    if (ok)
//    {
//        emit signalRadarConfigChanged(radarID, type, value);
//    }

    // 方位校准
//    type = INFOTYPE::BEARING_ALIGNMENT;
//    value = ui->Bearing_lineEdit->text().toInt(&ok);
//    if (ok)
//    {
//        emit signalRadarConfigChanged(radarID, type, value);
//    }

//    // 半径
//    type = INFOTYPE::RANG;
//    value = ui->Rang_lineEdit->text().toInt(&ok);
//    if (ok)
//    {
//        if(mUpRad != value)
//        {
//            mUpRad = value;
//            Utils::Profiles::instance()->setValue(str_radar,"Radar_up_radius", value);
//        }
//        emit signalRadarConfigChanged(radarID, type, value);
//    }

//    // 增益
//    type = INFOTYPE::GAIN;
//    value = ui->Gain_lineEdit->text().toInt(&ok);
//    if (ok)
//    {
//        emit signalRadarConfigChanged(radarID, type, value);
//    }

//    // 海杂波
//    type = INFOTYPE::SEA_CLUTTER;
//    value = ui->Sea_lineEdit->text().toInt(&ok);
//    if (ok)
//    {
//        emit signalRadarConfigChanged(radarID, type, value);
//    }

//    // 雨杂波
//    type = INFOTYPE::RAIN_CLUTTER;
//    value = ui->Rain_lineEdit->text().toInt(&ok);
//    if (ok)
//    {
//        emit signalRadarConfigChanged(radarID, type, value);
//    }

    // 噪声抑制
//    type = INFOTYPE::NOISE_REJECTION;
//    value = ui->noise_injection_cbx->currentData().toInt();
//    emit signalRadarConfigChanged(radarID, type, value);

//    // 旁瓣抑制
//    type = INFOTYPE::SIDE_LOBE_SUPPRESSION;
//    value = ui->Side_lineEdit->text().toInt(&ok);
//    if (ok)
//    {
//        emit signalRadarConfigChanged(radarID, type, value);
//    }

//    // 抗干扰
//    type = INFOTYPE::INTERFERENCE_REJECTION;
//    value = ui->injection_cbx->currentData().toInt();
//    emit signalRadarConfigChanged(radarID, type, value);

//    // 本地抗干扰
//    type = INFOTYPE::LOCAL_INTERFERENCE_REJECTION;
//    value = ui->local_injection_cbx->currentData().toInt();
//    emit signalRadarConfigChanged(radarID, type, value);

//    // 目标分离
//    type = INFOTYPE::TARGET_SEPARATION;
//    value = ui->target_separation_cbx->currentData().toInt();
//    emit signalRadarConfigChanged(radarID, type, value);

//    // 目标扩展
//    type = INFOTYPE::TARGET_EXPANSION;
//    value = ui->target_expansion_cbx->currentData().toInt();
//    emit signalRadarConfigChanged(radarID, type, value);

//    // 目标推进
//    type = INFOTYPE::TARGET_BOOST;
//    value = ui->target_boost_cbx->currentData().toInt();
//    emit signalRadarConfigChanged(radarID, type, value);
}

void zchxradarinteface::slotRadarConfigChanged(int radarID, int type, int value)
{
    if(!mRadarDataServer) return;
    if(mRadarDataServer->sourceID() != radarID) return;
    mRadarDataServer->setControlValue((INFOTYPE)type, value);
}
//1_打印统计丢包率
void zchxradarinteface::show_statistics_slot(int a, int b, int c, int d, int e)
{
    static int count = 0;
    if(a == 0)
    {
        count++;
        //cout<<"count"<<count;
    }
    else
    {
        count = 0;
    }
    if(zeor_flag == a && count > 200)
    {
        //cout<<"需要打开雷达";
        //emit signalOpenRadar();
        count = 0;
        //QThread::msleep(200);
    }
    else
    {
        //cout<<"雷达接收正常";
//        cout<<"丢包率"<<a<<c;
    }
    zeor_flag = a;
    QString str_packets = QString("%1/%2").arg(a).arg(b);
    ui->lineEdit_5->setText(str_packets);
    QString str_spoles = QString("%1/%2/%3").arg(c).arg(d).arg(e);
    ui->lineEdit_6->setText(str_spoles);
    zeor_flag = a;
}
//界面显示目标个数
void zchxradarinteface::show_video_slot(int a, int b)
{
    ui->lineEdit_3->setText(QString::number(a));
    ui->lineEdit_4->setText(QString::number(b));
}
//打印传入周老师库的数据
void zchxradarinteface::show_info_slot(QString str)
{
        QString info_r =  str+"\n";
        static int name_num = 1;
        //cout<<"标志打印"<<prt;
        if(txt == true)
        {
            QDir dir;
            dir.cd("../");  //进入某文件夹
            if(!dir.exists("传入周老师库的数据"))//判断需要创建的文件夹是否存在
            {
                dir.mkdir("传入周老师库的数据"); //创建文件夹
            }
            QString file_name ="../传入周老师库的数据/传入周老师库的数据_" + QString::number(name_num) + ".txt";
            QFile file(file_name);//创建文件对象
            bool isOk = file.open(QIODevice::Text |QIODevice::WriteOnly |QIODevice::Append);
            int a = file.size()/1024;
            if(a > 30720) //当文本大于30M时 新建另一个文本写入数据
            {
                name_num++;
            }
            if(false == isOk)
            {
                cout <<"打开文件失败";
                return;
            }
            if(true == isOk)
            {
                file.write(info_r.toStdString().data());
            }
            file.close();
        }
}
//网卡选择
void zchxradarinteface::on_save_restart_btn_clicked()
{
    QString ip = ui->mac_comboBox->currentText();
    cout<<"网卡选择"<<ip;
    Utils::Profiles::instance()->setValue("Radar_Control","Mac_IP",ip);
    int i = radarId;
    str_radar = QString("Radar_%1").arg(i);
    QString sVideoIP = ui->videoRecIPLlineEdit->text();
    Utils::Profiles::instance()->setValue(str_radar,"Video_IP",sVideoIP);
    QString uVideoPort = ui->videoRecPortSpinBox->text();
    Utils::Profiles::instance()->setValue(str_radar,"Video_Port",uVideoPort);
    QString sHeartIP = ui->heartIPLlineEdit->text();
    Utils::Profiles::instance()->setValue(str_radar,"Heart_IP",sHeartIP);
    QString uHeartPort = ui->heartPortSpinBox->text();
    Utils::Profiles::instance()->setValue(str_radar,"Heart_Port",uHeartPort);
    QString dCentreLat = ui->lineEdit_9->text();
    Utils::Profiles::instance()->setValue(str_radar,"Centre_Lat",dCentreLat);
    QString dCentreLon = ui->lineEdit_8->text();
    Utils::Profiles::instance()->setValue(str_radar,"Centre_Lon",dCentreLon);
    QString sControlIP = ui->controlIPLineEdit->text();
    Utils::Profiles::instance()->setValue(str_radar,"Report_IP",sControlIP);
    QString uControlPort = ui->controlPortSpinBox->text();
    Utils::Profiles::instance()->setValue(str_radar,"Report_Port",uControlPort);
    QString sLimit_File =ui->lineEdit_7->text();
    Utils::Profiles::instance()->setValue(str_radar,"Limit_File",sLimit_File);

    int ret1 = QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("配置修改成功"));
    cout<<"ret1"<<ret1;
    if(ret1 == 1024) reset_window();
}

void zchxradarinteface::on_pushButton_clicked()
{

}

void zchxradarinteface::on_pushButton_4_clicked()
{
    t_2 = false;
    emit prtAisSignal(t_2);
}
//打印回波数据
void zchxradarinteface::on_pushButton_5_clicked()
{
    t_3 = true;
    emit prtVideoSignal(t_3);
}

void zchxradarinteface::on_pushButton_6_clicked()
{
    t_3 = false;
    emit prtVideoSignal(t_3);
}
//设置限制文件
void zchxradarinteface::on_pushButton_7_clicked()
{
//    int penwidth = ui->lineEdit_7->text().toInt();
//    emit signal_set_penwidth(penwidth);
    QString anterior_file_name = ui->lineEdit_7->text();
    QString file_name = QFileDialog::getOpenFileName(NULL,"选择区域文件","./","*.json");
    //cout<<"限制文件"<<file_name;
    QString pathName;
    QRegExp na("(\/)(\\w)+(\\.)(json)"); //初始化名称结果
    QString name("");
    if(na.indexIn(file_name) != -1)
    {
        //匹配成功
        name = na.cap(0);
        cout<<"name"<<name;
    }
    cout<<"打印区域文件地址";
    pathName = "."+name;
    cout<<"地址pathName"<< pathName;
    if(pathName == ".")
    {
        cout<<"打开失败";
        //anterior_file_name = Utils::Profiles::instance()->value(str_radar,"Limit_File").toString();
        ui->lineEdit_7->setText(anterior_file_name);
    }
    else
    {
        ui->lineEdit_7->setText(pathName);
    }
    Utils::Profiles::instance()->setValue(str_radar,"Limit_File", pathName);
}
//1_实时打印接收到的雷达状态信息
void zchxradarinteface::slotRecvRadarReportInfo_1(QList<RadarStatus> radarStatusList,int val)
{
   // cout<<"显示当前雷达状态";
    foreach (RadarStatus element, radarStatusList) {
        int elelmentID = element.getId();// 消息类型
        int min = element.getMin();// value可设置的最小值
        int max = element.getMax();// value可设置的最大值
        int value = element.getValue();// 当前值
        int unit = element.getUnit(); // 值类型
        QString str = RadarStatus::getTypeString(element.getId());
//        cout<<"elelmentID"<<elelmentID;
//        cout<<"min"<<min;
//        cout<<"max"<<max;
//        cout<<"value"<<value;
//        cout<<"unit"<<unit;
//        cout<<"str"<<str;
        if(elelmentID == 1)
            ui->power_statsu_btn->setValue(value);
        if(elelmentID == 2)
            ui->scan_seed_btn->setValue(value);
        if(elelmentID == 3)
            ui->antenna_btn->setValue(value);
        if(elelmentID == 4)
            ui->bearing_btn->setValue(value);
        if(elelmentID == 5)
        {
            ui->range_btn->setValue(value);
            Utils::Profiles::instance()->setValue(str_radar,"Radar_up_radius", value);
        }
        if(elelmentID == 6)
            ui->gain_btn->setValue(value);
        if(elelmentID == 7)
            ui->sea_culter_btn->setValue(value);
        if(elelmentID == 8)
            ui->rain_culter_btn->setValue(value);
        if(elelmentID == 9)
            ui->noise_injection_btn->setValue(value);
        if(elelmentID == 10)
            ui->sidelobe_btn->setValue(value);
        if(elelmentID == 11)
            ui->injection_btn->setValue(value);
        if(elelmentID == 12)
            ui->local_injection_btn->setValue(value);
        if(elelmentID == 13)
            ui->target_expansion_btn->setValue(value);
        if(elelmentID == 14)
            ui->target_boost_btn->setValue(value);
        if(elelmentID == 15)
            ui->target_separat_btn->setValue(value);
    }
}

void zchxradarinteface::slotRecvRangeFactorChanged(double factor)
{
    cout<<"距离因子改变了"<<factor;
//    foreach (ZCHXAnalysisAndSendRadar * server, mAnalysisAndSendRadarList) {
//        if(server) server->setRangeFactor(factor);
//    }
}

void zchxradarinteface::setRadarVideoAndTargetPixmap(const QPixmap &videoPixmap,const Afterglow &dataAfterglow)
{
    //cout<<"主界面开始画图";
    QPixmap pixmap = videoPixmap;
    //cout<<"pixmap.scaled"<<pixmap.size();
    if(pixmap.size() == QSize(0,0))
    {
        cout<<"图片太小了";
        return;
    }
    emit signalSetPix(videoPixmap);
    QPixmap pixmap_1 = pixmap.scaled(ui->videoImageLabel->width(), ui->videoImageLabel->height(), Qt::KeepAspectRatio);
    //cout<<"是否为空";
    if(m_drawpic == false)
        ui->videoImageLabel->clear();
    else
        ui->videoImageLabel->setPixmap(pixmap_1);
}

//帮助菜单按下
void zchxradarinteface::help_clicked()
{
    mHelo->show();
}
//弹窗显示日志
void zchxradarinteface::logButton()
{
    mLog->exec();
}
//弹窗显示雷达配置
void zchxradarinteface::on_setButton_clicked()
{
    mSet->exec();
}
//弹窗显示客户端信息
void zchxradarinteface::setButton_2()
{
    mCli->exec();
}
//重新启动采集器
void zchxradarinteface::reset_window()
{
    emit signalRestart();
#if 0
    cout<<"重新启动采集器";
    QString program = QApplication::applicationFilePath();
        QStringList arguments = QApplication::arguments();
        QString workingDirectory = QDir::currentPath();
        QProcess::startDetached(program, arguments, workingDirectory);
        QApplication::exit();
#endif

}
//导入回波文件
void zchxradarinteface::on_logButton_3_clicked()
{
    //QString file_name = QFileDialog::getOpenFileName(NULL,"导入回波文件","../回波数据/","*");
    QString file_path = QFileDialog::getExistingDirectory(this,"请选择文件夹...","./");
    if(file_path.isEmpty())
    {
        return;
    }else{
        qDebug() << file_path << endl;
    }
    int radar_type = ui->radar_type_combox->currentData().toInt();
    if(mAnalysisAndSendRadar) mAnalysisAndSendRadar->slotSetRadarType(radar_type);
    //delete up_video;
    up_video = new up_video_pthread("track",file_path,0);
    up_video->start();
    connect(up_video,SIGNAL(send_video_signal(QByteArray,QString,int,int,int)),
            this,SIGNAL(send_video_signal(QByteArray,QString,int,int,int)));

}
//打开绘图
void zchxradarinteface::on_draw_pushButton_clicked()
{
    switch(m_drawpic)
    {
        case true:
            cout<<"关闭";
            m_drawpic = false;
            ui->draw_pushButton->setText("打开绘图");
            Utils::Profiles::instance()->setValue(str_radar,"draw_pic", m_drawpic);
            ui->videoImageLabel->clear();
            break;
        case false:
            m_drawpic = true;
            cout<<"打开";
            ui->draw_pushButton->setText("关闭绘图");
            Utils::Profiles::instance()->setValue(str_radar,"draw_pic", m_drawpic);
            break;
    }
}
//设置目标距离
void zchxradarinteface::on_pushButton_9_clicked()
{
    cout<<"设置目标距离"<<ui->radius_lineEdit->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"Radius", ui->radius_lineEdit->text().toInt());

}

//显示串口通讯
void zchxradarinteface::on_serial_pushButton_clicked()
{
    //mComConfigWidget->show();
}
//开始接收串口数据
void zchxradarinteface::slotSetComDevParams(const QMap<QString, COMDEVPARAM> &param)
{
    //开始接收串口数据
    mComDataMgr->setComDevParams(param);
}
//配置GPS输出zmq
void zchxradarinteface::slotUpdateZmq(QString port, QString topic)
{
    PROFILES_INSTANCE->setValue(SERVER_SETTING_SEC, "GPS_Send_Port", port.toInt());
    PROFILES_INSTANCE->setValue(SERVER_SETTING_SEC, "GPS_Topic", topic);

    mOutWorker->setTrackSendPort( port.toInt());
    mOutWorker->setTrackTopic(topic);
}
//删除显示页
void zchxradarinteface::removeSubTab(int dex)
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
    if(name == "A")
    {
        index--;
        Utils::Profiles::instance()->setValue("Ais","AIS_Num",index);
    }
}

void zchxradarinteface::on_xinke_pushButton_clicked()
{
    /*QString file_name = QFileDialog::getOpenFileName(NULL,"导入目标文件","../回波数据/","*");
    ui->videoDataPath_lineEdit->setText(file_name);
    //delete up_video;
    up_video = new up_video_pthread("track",file_name,0);
    up_video->start();
    connect(up_video,SIGNAL(send_video_signal(QByteArray,QString,int,int,int)),
            this,SIGNAL(send_video_signal(QByteArray,QString,int,int,int)));*/
}
//雷达控制,助航设备信息
void zchxradarinteface::on_control_pushButton_clicked()
{
    //mControl->exec();
    mAisbaseinfosetting->show();
}

void zchxradarinteface::joinGropslot(QString str)//判断加入组播是否成功
{
    if(str == "s")
    {
        QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("加入组播成功"));
    }
    if(str == "f")
    {
        QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("加入组播失败"));
    }
}


void zchxradarinteface::on_radar_parse_setting_clicked()
{

}

void zchxradarinteface::on_threshold_type_currentIndexChanged(int index)
{


}

void zchxradarinteface::slotRecvRadarProcessNum(int extract, int correct, int track)
{

}

//设置清理时间
//void zchxradarinteface::on_push_Button_clicked()
//{
//    int time_1 = ui->lineEdit_2->text().toInt();
//    Utils::Profiles::instance()->setValue(str_radar,"ClearTrack_Time", time_1);
//}

void zchxradarinteface::show_range_slot(double r, double factor)
{

    ui->label->setText(tr("半径:%1  距离因子:%2").arg(r).arg(factor, 0, 'f', 2));
}

void zchxradarinteface::on_shownum_pushButton_clicked()
{
    QString str = ui->shownum_pushButton->text();
    if(str == "显示编号")
    {
        ui->shownum_pushButton->setText("隐藏编号");
        emit showTrackNumSignal(true);
    }
    else
    {
        ui->shownum_pushButton->setText("显示编号");
        emit showTrackNumSignal(false);
    }
}
//缓存点迹
//void zchxradarinteface::on_angle_push_Button_clicked()
//{
//    int angle = ui->angle_lineEdit->text().toInt();
//    Utils::Profiles::instance()->setValue(str_radar,"historyNum", angle);
//    int time_1 = ui->lineEdit_2->text().toInt();
//    Utils::Profiles::instance()->setValue(str_radar,"ClearTrack_Time", time_1);
//}

//浮标设置
void zchxradarinteface::on_setfubiao_pushButton_clicked()
{
    mFloat.show();
}

//颜色一
void zchxradarinteface::on_color1Button_clicked()
{
    QColorDialog color;//调出颜色选择器对话框
    QColor c = color.getRgba();
    QString backgroud = QString("background-color: rgb(%1, %2, %3);").arg(c.red()).arg(c.green()).arg(c.blue());
    cout<<"backgroud"<<backgroud;
    //c.red(), c.green(), c.blue()是分别对应的rgb值
    a1 = c.red();
    a2 = c.green();
    a3 = c.blue();
    cout<<"rgb2"<<b1<<b2<<b3;
    Utils::Profiles::instance()->setValue("Color","color1_R", a1);
    Utils::Profiles::instance()->setValue("Color","color1_G", a2);
    Utils::Profiles::instance()->setValue("Color","color1_B", a3);
    emit colorSetSignal(a1,a2,a3,b1,b2,b3);
}

//颜色二
void zchxradarinteface::on_color2Button_clicked()
{
    QColorDialog color;//调出颜色选择器对话框
    QColor c = color.getRgba();
    QString backgroud = QString("background-color: rgb(%1, %2, %3);").arg(c.red()).arg(c.green()).arg(c.blue());
    cout<<"backgroud"<<backgroud;
    //c.red(), c.green(), c.blue()是分别对应的rgb值
    b1 = c.red();
    b2 = c.green();
    b3 = c.blue();
    Utils::Profiles::instance()->setValue("Color","color2_R", b1);
    Utils::Profiles::instance()->setValue("Color","color2_G", b2);
    Utils::Profiles::instance()->setValue("Color","color2_B", b3);
    emit colorSetSignal(a1,a2,a3,b1,b2,b3);
}

//确定颜色
void zchxradarinteface::on_color_2_pushButton_clicked()
{
    cout<<"设置颜色";
}

//设置
void zchxradarinteface::on_update_setting_btn_clicked()
{
    int angle = ui->angle_lineEdit->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"historyNum", angle);
    int time_1 = ui->lineEdit_2->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"ClearTrack_Time", time_1);
    int jump_distance = ui->jump_lineEdit->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"jump_distance", jump_distance);
    int radar_num = ui->bh_lineEdit->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"radar_num", radar_num);
    double track_radius = ui->tradius_lineEdit->text().toDouble();
    Utils::Profiles::instance()->setValue(str_radar,"track_radius", track_radius);
    double minTradius = ui->minTradius_lineEdit->text().toDouble();
    Utils::Profiles::instance()->setValue(str_radar,"track_min_radius", minTradius);
    cout<<"设置目标距离"<<ui->radius_lineEdit->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"Radius", ui->radius_lineEdit->text().toInt());
    int min_amplitude = ui->minzf_lineEdit->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"min_amplitude", min_amplitude);
    int max_amplitude = ui->maxzf_lineEdit->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"max_amplitude", max_amplitude);
    cout<<"设置半径系数:"<<ui->k_lineEdit->text().toDouble();
    Utils::Profiles::instance()->setValue(str_radar,"RadiusCoefficient", ui->k_lineEdit->text().toDouble());
    bool send = ui->send_dj_checkBox->isChecked();
    Utils::Profiles::instance()->setValue(str_radar,"send_dianjian", send);

    Utils::Profiles::instance()->setValue(str_radar,"track_min_area", ui->minAreaEdit->text());
    Utils::Profiles::instance()->setValue(str_radar,"track_max_area", ui->maxAreaEdit->text());

    Utils::Profiles::instance()->setValue(str_radar, "Direction_Invert", ui->direction_change_edit->text());
    Utils::Profiles::instance()->setValue(str_radar, "azimuth_adjustment", ui->cog_adjust_chk->isChecked());
    Utils::Profiles::instance()->setValue(str_radar, "video_cycle_or", ui->video_or_count->value());
    Utils::Profiles::instance()->setValue(str_radar, "native_radius", ui->useNativeRadiusCHK->isChecked());    
    Utils::Profiles::instance()->setValue(str_radar, "prediction_width", ui->predictionWidth->value());



    int ret1 = QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("配置修改成功"));
    cout<<"ret1"<<ret1;
    if(ret1 == 1024) reset_window();
}

void zchxradarinteface::on_k_pushButton_clicked()
{
    cout<<"设置半径系数:"<<ui->k_lineEdit->text().toDouble();
    Utils::Profiles::instance()->setValue(str_radar,"RadiusCoefficient", ui->k_lineEdit->text().toDouble());
}

void zchxradarinteface::slotShowRadiusCoefficient(double Radius,double Coefficient)
{
    //cout<<"slotShowRadiusCoefficient";
    QString str = QString("参考半径:%1,系数:%2").arg(Radius).arg(Coefficient);
    ui->k_label->setText(str);
}

void zchxradarinteface::on_restart_pushButton_clicked()
{
    QString str = ui->restart_lineEdit->text();
    Utils::Profiles::instance()->setValue("Radar","restart_time", str);
    bool mf = ui->restart_checkBox->isChecked();
    Utils::Profiles::instance()->setValue("Radar","restart_flag", mf);
}

void zchxradarinteface::handleTimeout()
{
    int power = ui->power_statsu_btn->getValue();
    bool mf = ui->restart_checkBox->isChecked();
    if(1 != power && mf)
    {
        cout<<"打开雷达";
        signalOpenRadar();
    }
}

void zchxradarinteface::on_saveRadarBtn_clicked()
{

}

void zchxradarinteface::on_import_report_btn_clicked()
{
    if(mReportSimulateThread) mReportSimulateThread->setCancel(true);
    //QString file_name = QFileDialog::getOpenFileName(NULL,"导入回波文件","../回波数据/","*");
    QString file_path = QFileDialog::getExistingDirectory(this,"请选择文件夹...","./");
    if(file_path.isEmpty()) return;

    mReportSimulateThread = new zchxSimulateThread(file_path);
    connect(mReportSimulateThread, SIGNAL(signalSendContents(QByteArray,int)),
            this, SIGNAL(send_report_signal(QByteArray,int)));
    connect(mReportSimulateThread, SIGNAL(finished()), mReportSimulateThread, SLOT(deleteLater()));
    mReportSimulateThread->start();

}
