#include "zchxradaraissetting.h"
#include "ui_zchxradaraissetting.h"
#include "../profiles.h"
#include <QMessageBox>
#include "qradarparamsetting.h"
#include "qradarstatussettingwidget.h"
#include <QFile>
#include <QFileDialog>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
ZCHXRadarAisSetting::ZCHXRadarAisSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZCHXRadarAisSetting)
{
    ui->setupUi(this);
    //ui->radarNumComboBox->addItem("2", "2");
    //init();
}

ZCHXRadarAisSetting::~ZCHXRadarAisSetting()
{
    delete ui;
}

void ZCHXRadarAisSetting::init()
{






    ui->groupBox->setVisible(false);//隐藏AIS部分
    //开始动态生成雷达的tab页配置

    //先生成配置widget
    QRadarParamSetting *set = new QRadarParamSetting(this);
    connect(set, SIGNAL(signalRadarConfigChanged(int,int,int)), this, SIGNAL(signalRadarConfigChanged(int,int,int)));
    int i = radarId;
    ui->tabWidget->addTab(set, tr("雷达-%1").arg(i));
    set->setRadarID(i);
    QString str_radar = QString("Radar_%1").arg(i);
    //radar
    int uRadarNum = Utils::Profiles::instance()->value(str_radar,"Num").toInt();
    int uVideoSendPort = Utils::Profiles::instance()->value(str_radar,"video_Send_Port").toInt();
    QString sVideoTopic = Utils::Profiles::instance()->value(str_radar,"video_Topic").toString();
    int uTrackSendPort = Utils::Profiles::instance()->value(str_radar,"Track_Send_Port").toInt();
    QString sTrackTopic = Utils::Profiles::instance()->value(str_radar,"Track_Topic").toString();
    QString uYhSendPort =  Utils::Profiles::instance()->value(str_radar,"Yuhui_Send_Port").toString();
    QString sYhTopic = Utils::Profiles::instance()->value(str_radar,"Yuhui_Topic").toString();

    cout<<"sTrackTopic"<<sTrackTopic;
    ui->trackSendPortSpinBox->setValue(uTrackSendPort);
    ui->trackSendTopicLineEdit->setText(sTrackTopic);
    ui->videoSendPortSpinBox->setValue(uVideoSendPort);
    ui->videoSendTopicLineEdit->setText(sVideoTopic);
    ui->yhportylineEdit->setText(uYhSendPort);
    ui->yhtopiclineEdit->setText(sYhTopic);

    double dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toDouble();
    double dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toDouble();
    int uLoopNum = Utils::Profiles::instance()->value(str_radar,"Loop_Num").toInt();
    bool bLimit = Utils::Profiles::instance()->value(str_radar,"Limit").toBool();
    bool cLimit = Utils::Profiles::instance()->value(str_radar,"Limit_zhou").toBool();
    bool gLimit = Utils::Profiles::instance()->value(str_radar,"Limit_gps").toBool();
    QString sTrackIP = Utils::Profiles::instance()->value(str_radar,"Track_IP").toString();
    int uTrackPort = Utils::Profiles::instance()->value(str_radar,"Track_Port").toInt();
    QString sTrackType = Utils::Profiles::instance()->value(str_radar,"Track_Type").toString();
    QString sVideoIP = Utils::Profiles::instance()->value(str_radar,"Video_IP").toString();
    int uVideoPort = Utils::Profiles::instance()->value(str_radar,"Video_Port").toInt();
    QString sRadarVideoType = Utils::Profiles::instance()->value(str_radar,"Video_Type").toString();
    int uCellNum = Utils::Profiles::instance()->value(str_radar,"Cell_Num").toInt();
    int uLineNum = Utils::Profiles::instance()->value(str_radar,"Line_Num").toInt();
    int uHeading = Utils::Profiles::instance()->value(str_radar,"Heading").toInt();
    int uHeartTime = Utils::Profiles::instance()->value(str_radar,"Heart_Time").toInt();
    QString sHeartIP = Utils::Profiles::instance()->value(str_radar,"Heart_IP").toString();
    int uHeartPort = Utils::Profiles::instance()->value(str_radar,"Heart_Port").toInt();
    double dDis = Utils::Profiles::instance()->value(str_radar,"Distance").toInt();
    int uClearRadarTrackTime = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt();

    QString sLimit_File = Utils::Profiles::instance()->value(str_radar,"Limit_File").toString();
    //ui->limit_fileBox->setCurrentText(sLimet_File);
    //ui->lineEdit->setText(sLimit_File);
    QString sRadarType = Utils::Profiles::instance()->value(str_radar,"Radar_Type").toString();

    QString sControlIP = Utils::Profiles::instance()->value(str_radar,"Report_IP").toString();
    int uControlPort = Utils::Profiles::instance()->value(str_radar,"Report_Port").toInt();
    bool openReport = Utils::Profiles::instance()->value(str_radar, "Report_Open", false).toBool();
    int uTcpPort = Utils::Profiles::instance()->value(str_radar,"Tcp_Port").toInt();

    //开始设定参数
    cout<<"dCentreLat"<<dCentreLat;
    cout<<"dCentreLon"<<dCentreLon;
    set->setTcpPort(uTcpPort);
    set->setRadarType(sRadarType);
    set->setDistance(dDis);
    set->setTrackIP(sTrackIP);
    set->setTrackPort(uTrackPort);
    set->setTrackType(sTrackType);
    set->setVideoIP(sVideoIP);
    set->setVideoPort(uVideoPort);
    set->setVideoType(sRadarVideoType);
    set->setHeartIP(sHeartIP);
    set->setHeartPort(uHeartPort);
    set->setHeartInterval(uHeartTime);
    set->setCenterLat(dCentreLat);
    set->setCenterLon(dCentreLon);
    set->setCellNum(uCellNum);
    set->setLineNum(uLineNum);
    set->setLoopNum(uLoopNum);
    set->setHeading(uHeading);
    set->setLimit(bLimit);
    set->setLimit_gps(gLimit);
    set->setClearTrackTime(uClearRadarTrackTime);
    set->setControlIP(sControlIP);
    set->setControlPort(uControlPort);
    set->setReportOpen(openReport);
    set->setLimit_zhou(cLimit);
    set->setLimitFile(sLimit_File);
    connect(this,SIGNAL(signalUpdateRealRangeFactor(double,double)),set,SLOT(slotUpdateRealRangeFactor(double,double)));
    connect(set,SIGNAL(signalRangeFactorChanged(double)),this,SIGNAL(signalRangeFactorChanged(double)));
    connect(this,SIGNAL(signalGetGpsData(double,double)),set,SLOT(slotGetGpsData(double,double)));

    if(uRadarNum > 0)
    {
        on_radarNumComboBox_activated(0);
    }
}

void ZCHXRadarAisSetting::on_radarNumComboBox_activated(int index)
{
    return;
    for(int i=0; i<ui->tabWidget->count(); i++)
    {
        if(i == index)
        {
            ui->tabWidget->setTabEnabled(i, true);
        } else
        {
            ui->tabWidget->setTabEnabled(i, false);
        }
    }
}

void ZCHXRadarAisSetting::on_saveBtn_clicked()
{


    int i = radarId - 1;
    QRadarParamSetting *set = qobject_cast<QRadarParamSetting*>(ui->tabWidget->widget(0));
    if(!set)
    {
        cout<<"!set"<<i;
        return;
    }
    QString str_radar = QString("Radar_%1").arg(i+1);
    Utils::Profiles::instance()->setValue(str_radar,"Tcp_Port",set->tcpPort());
    Utils::Profiles::instance()->setValue(str_radar,"Radar_Type",set->RadarType());
    Utils::Profiles::instance()->setValue(str_radar,"Distance",set->distance());
    Utils::Profiles::instance()->setValue(str_radar,"Centre_Lat",set->centerLat());
    Utils::Profiles::instance()->setValue(str_radar,"Centre_Lon",set->centerLon());
    Utils::Profiles::instance()->setValue(str_radar,"Loop_Num",set->loopNum());
    Utils::Profiles::instance()->setValue(str_radar,"Limit",set->limit());
    Utils::Profiles::instance()->setValue(str_radar,"Limit_zhou",set->limit_zhou());
    Utils::Profiles::instance()->setValue(str_radar,"Limit_gps",set->limit_gps());
    Utils::Profiles::instance()->setValue(str_radar,"ClearTrack_Time",set->clearTrackTime());
    Utils::Profiles::instance()->setValue(str_radar,"Track_IP",set->trackIP());
    Utils::Profiles::instance()->setValue(str_radar,"Track_Port",set->trackPort());
    Utils::Profiles::instance()->setValue(str_radar,"Track_Type",set->trckType());
    Utils::Profiles::instance()->setValue(str_radar,"Video_IP",set->videoIP());
    Utils::Profiles::instance()->setValue(str_radar,"Video_Port",set->videoPort());
    Utils::Profiles::instance()->setValue(str_radar,"Video_Type",set->videoType());
    Utils::Profiles::instance()->setValue(str_radar,"Cell_Num",set->cellNum());
    Utils::Profiles::instance()->setValue(str_radar,"Line_Num",set->lineNum());
    Utils::Profiles::instance()->setValue(str_radar,"Heading",set->heading());
    Utils::Profiles::instance()->setValue(str_radar,"Heart_Time",set->heartInterval());
    Utils::Profiles::instance()->setValue(str_radar,"Heart_IP",set->heartIP());
    Utils::Profiles::instance()->setValue(str_radar,"Heart_Port",set->heartPort());
    Utils::Profiles::instance()->setValue(str_radar,"Report_IP",set->controlIP());
    Utils::Profiles::instance()->setValue(str_radar,"Report_Port",set->controlPort());
    Utils::Profiles::instance()->setValue(str_radar,"Report_Open", set->reportOpen());
    Utils::Profiles::instance()->setValue(str_radar,"Limit_File", set->limitFile());


    int uVideoSendPort = ui->videoSendPortSpinBox->value();
    Utils::Profiles::instance()->setValue(str_radar,"video_Send_Port",uVideoSendPort);
    QString sVideoTopic = ui->videoSendTopicLineEdit->text();
    Utils::Profiles::instance()->setValue(str_radar,"video_Topic",sVideoTopic);
    int uTrackSendPort = ui->trackSendPortSpinBox->value();
    Utils::Profiles::instance()->setValue(str_radar,"Track_Send_Port",uTrackSendPort);
    QString sTrackTopic = ui->trackSendTopicLineEdit->text();
    Utils::Profiles::instance()->setValue(str_radar,"Track_Topic",sTrackTopic); // /radar_offset.json
    int uYhSendPort = ui->yhportylineEdit->text().toInt();
    Utils::Profiles::instance()->setValue(str_radar,"Yuhui_Send_Port",uYhSendPort);
    QString sYhTopic = ui->yhtopiclineEdit->text();
    Utils::Profiles::instance()->setValue(str_radar,"Yuhui_Topic",sYhTopic);

    if(set->reportOpen())
    {
        //开始设定默认的控制参数值
        QString str_radar_cmd = QString("Radar_Command_%1").arg(i+1);
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(POWER,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"1");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(SCAN_SPEED,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"1");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(ANTENNA_HEIGHT,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"30");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(BEARING_ALIGNMENT,STR_MODE_ENG),\
                                                QStringList()<<"-180"<<"180");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(RANG,STR_MODE_ENG),\
                                                QStringList()<<"-50"<<"72707");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(GAIN,STR_MODE_ENG),\
                                                QStringList()<<"-1"<<"100");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(SEA_CLUTTER,STR_MODE_ENG),\
                                                QStringList()<<"-1"<<"100");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(RAIN_CLUTTER,STR_MODE_ENG),\
                                                QStringList()<<"-1"<<"100");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(NOISE_REJECTION,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"2");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(SIDE_LOBE_SUPPRESSION,STR_MODE_ENG),\
                                                QStringList()<<"-1"<<"100");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(INTERFERENCE_REJECTION,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"3");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(LOCAL_INTERFERENCE_REJECTION,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"3");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(TARGET_EXPANSION,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"1");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(TARGET_BOOST,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"2");
        Utils::Profiles::instance()->setDefault(str_radar_cmd,\
                                                RadarStatus::getTypeString(TARGET_SEPARATION,STR_MODE_ENG),\
                                                QStringList()<<"0"<<"3");
    }

    cout<<"set_change_signal信号";
    int ret1 = QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("配置修改成功"));
    cout<<"ret1"<<ret1;
    if(ret1 == 1024)
        emit set_change_signal();


}

void ZCHXRadarAisSetting::on_aisTypeBox_currentIndexChanged(int index)
{
    if(index == 0)
    {
        QString sIP = Utils::Profiles::instance()->value("Ais","IP").toString();
        int uPort = Utils::Profiles::instance()->value("Ais","Port").toInt();
        ui->aisRecIPLineEdit->setEnabled(true);
        ui->aisRecIPLineEdit->setText(sIP);
        ui->aisRecPortSpinBox->setValue(uPort);
    }
    else
    {
        int uServerPort = Utils::Profiles::instance()->value("Ais","Server_Port").toInt();
        ui->aisRecIPLineEdit->clear();
        ui->aisRecIPLineEdit->setEnabled(false);
        ui->aisRecPortSpinBox->setValue(uServerPort);
    }
}

void ZCHXRadarAisSetting::slotRecvRadarReportInfo(const QList<RadarStatus> &info, int radarID)
{
    //cout<<"雷达状态改变了!!!";
    for(int i=0; i<ui->tabWidget->count(); i++)
    {
        QWidget* w = ui->tabWidget->widget(i);
        if(w && w->metaObject()->className() == "QRadarParamSetting")
        {
            QRadarParamSetting *set = qobject_cast<QRadarParamSetting*>(w);
            if(!set) continue;
            if(set->getRadarID() != radarID) continue;
            set->setRadarReportSeting(info);
            break;
        }
    }
}

//void ZCHXRadarAisSetting::on_limit_fileBox_currentIndexChanged(int index)
//{

//}


void ZCHXRadarAisSetting::slotUpdateRealRangeFactor(double range, double factor)
{
    //ui->realRangefactor->setText(tr("半径:%1  距离因子:%2").arg(range).arg(factor, 0, 'f', 2));
    emit signalUpdateRealRangeFactor(range, factor);
}

void ZCHXRadarAisSetting::slotGetGpsData(double lat, double lon)//从串口接入GPS经纬度
{
    //cout<<"从串口接入GPS经纬度";
    emit signalGetGpsData(lat, lon);
}
void ZCHXRadarAisSetting::setId(int ID)
{
    radarId = ID;
    init();
}
