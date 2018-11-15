#include "zchxradaraissetting.h"
#include "ui_zchxradaraissetting.h"
#include "../profiles.h"
#include <QMessageBox>
#include "qradarparamsetting.h"
#include "qradarstatussettingwidget.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
ZCHXRadarAisSetting::ZCHXRadarAisSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ZCHXRadarAisSetting)
{
    ui->setupUi(this);
    init();
}

ZCHXRadarAisSetting::~ZCHXRadarAisSetting()
{
    delete ui;
}

void ZCHXRadarAisSetting::init()
{
    //ais
    int uAISSendPort = Utils::Profiles::instance()->value("Ais","Ais_Send_Port").toInt();
    QString sAISTopic = Utils::Profiles::instance()->value("Ais","Ais_Topic").toString();
    bool bServer = Utils::Profiles::instance()->value("Ais","IsServer").toBool();
    int uServerPort = Utils::Profiles::instance()->value("Ais","Server_Port").toInt();
    QString sIP = Utils::Profiles::instance()->value("Ais","IP").toString();
    int uPort = Utils::Profiles::instance()->value("Ais","Port").toInt();
    Utils::Profiles::instance()->setDefault("Ais", "TimeOut", 1);
    ui->aisTimeoutSpinBox->setValue(Utils::Profiles::instance()->value("Ais", "TimeOut", 1).toInt());
    if(bServer)//服务端
    {
        ui->aisTypeBox->setCurrentIndex(1);
        ui->aisRecIPLineEdit->clear();
        ui->aisRecIPLineEdit->setEnabled(false);
        ui->aisRecPortSpinBox->setValue(uServerPort);
    }
    else//客户端
    {
        ui->aisRecIPLineEdit->setEnabled(true);
        ui->aisTypeBox->setCurrentIndex(0);
        ui->aisRecIPLineEdit->setText(sIP);
        ui->aisRecPortSpinBox->setValue(uPort);
    }
    ui->aisSendPortSpinBox->setValue(uAISSendPort);
    ui->aisSendTopicLineEdit->setText(sAISTopic);

    //radar
    int  uRadarNum = Utils::Profiles::instance()->value("Radar","Num").toInt();
    int uVideoSendPort = Utils::Profiles::instance()->value("Radar","video_Send_Port").toInt();
    QString sVideoTopic = Utils::Profiles::instance()->value("Radar","video_Topic").toString();
    int uTrackSendPort = Utils::Profiles::instance()->value("Radar","Track_Send_Port").toInt();
    QString sTrackTopic = Utils::Profiles::instance()->value("Radar","Track_Topic").toString();
    ui->trackSendPortSpinBox->setValue(uTrackSendPort);
    ui->trackSendTopicLineEdit->setText(sTrackTopic);
    ui->videoSendPortSpinBox->setValue(uVideoSendPort);
    ui->videoSendTopicLineEdit->setText(sVideoTopic);
    //开始动态生成雷达的tab页配置
    for(int i=0; i<uRadarNum; i++)
    {
        //先生成配置widget
        QRadarParamSetting *set = new QRadarParamSetting(this);
        connect(set, SIGNAL(signalRadarConfigChanged(int,int,int)), this, SIGNAL(signalRadarConfigChanged(int,int,int)));
        ui->tabWidget->addTab(set, tr("雷达-%1").arg(i+1));
        set->setRadarID(i+1);
        QString str_radar = QString("Radar_%1").arg(i+1);
        double dCentreLat = Utils::Profiles::instance()->value(str_radar,"Centre_Lat").toDouble();
        double dCentreLon = Utils::Profiles::instance()->value(str_radar,"Centre_Lon").toDouble();
        int uLoopNum = Utils::Profiles::instance()->value(str_radar,"Loop_Num").toInt();
        bool bLimit = Utils::Profiles::instance()->value(str_radar,"Limit").toBool();
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

        int uVideoSendPort = Utils::Profiles::instance()->value(str_radar,"video_Send_Port").toInt();
        QString sVideoTopic = Utils::Profiles::instance()->value(str_radar,"video_Topic").toString();
        int uTrackSendPort = Utils::Profiles::instance()->value(str_radar,"Track_Send_Port").toInt();
        QString sTrackTopic = Utils::Profiles::instance()->value(str_radar,"Track_Topic").toString();
        ui->trackSendPortSpinBox->setValue(uTrackSendPort);
        ui->trackSendTopicLineEdit->setText(sTrackTopic);
        ui->videoSendPortSpinBox->setValue(uVideoSendPort);
        ui->videoSendTopicLineEdit->setText(sVideoTopic);
        QString sLimet_File = Utils::Profiles::instance()->value(str_radar,"Limet_File").toString();
        ui->limit_fileBox->setCurrentText(sLimet_File);

        QString sControlIP = Utils::Profiles::instance()->value(str_radar,"Report_IP").toString();
        int uControlPort = Utils::Profiles::instance()->value(str_radar,"Report_Port").toInt();
        bool openReport = Utils::Profiles::instance()->value(str_radar, "Report_Open", false).toBool();
        //开始设定参数
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
        set->setClearTrackTime(uClearRadarTrackTime);
        set->setControlIP(sControlIP);
        set->setControlPort(uControlPort);
        set->setReportOpen(openReport);
        if(openReport)
        {
            QRadarStatusSettingWidget* statusW = new QRadarStatusSettingWidget(i+1, this);
            connect(statusW, SIGNAL(signalRadarConfigChanged(int,int,int)), \
                    set, SIGNAL(signalRadarConfigChanged(int,int,int)));
            ui->tabWidget->addTab(statusW, tr("雷达-%1状态").arg(i+1));
            set->setStatusWidget(statusW);
        }
    }
    if(uRadarNum > 0)
    {
        ui->radarNumComboBox->setCurrentIndex(0);
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
    //ais save
    int uAisIndex = ui->aisTypeBox->currentIndex();
    if(uAisIndex == 1)//服务端
    {
        Utils::Profiles::instance()->setValue("Ais","IsServer",true);
        QString sIP = ui->aisRecIPLineEdit->text();
        Utils::Profiles::instance()->setValue("Ais","IP",sIP);
        int uServerPort = ui->aisRecPortSpinBox->value();
        Utils::Profiles::instance()->setValue("Ais","Server_Port",uServerPort);
    }
    else//客户端
    {
        Utils::Profiles::instance()->setValue("Ais","IsServer",false);
        QString sIP = ui->aisRecIPLineEdit->text();
        Utils::Profiles::instance()->setValue("Ais","IP",sIP);
        int uPort = ui->aisRecPortSpinBox->value();
        Utils::Profiles::instance()->setValue("Ais","Port",uPort);
    }
    Utils::Profiles::instance()->setValue("Ais", "TimeOut", ui->aisTimeoutSpinBox->value());
    int uAISSendPort = ui->aisSendPortSpinBox->value();
    Utils::Profiles::instance()->setValue("Ais","Ais_Send_Port",uAISSendPort);
    QString sAISTopic = ui->aisSendTopicLineEdit->text();
    Utils::Profiles::instance()->setValue("Ais","Ais_Topic",sAISTopic);
    //radar save
    Utils::Profiles::instance()->setValue("Radar","Num",1);
    int uVideoSendPort = ui->videoSendPortSpinBox->value();
    Utils::Profiles::instance()->setValue("Radar","video_Send_Port",uVideoSendPort);
    QString sVideoTopic = ui->videoSendTopicLineEdit->text();
    Utils::Profiles::instance()->setValue("Radar","video_Topic",sVideoTopic);
    int uTrackSendPort = ui->trackSendPortSpinBox->value();
    Utils::Profiles::instance()->setValue("Radar","Track_Send_Port",uTrackSendPort);
    QString sTrackTopic = ui->trackSendTopicLineEdit->text();
    Utils::Profiles::instance()->setValue("Radar","Track_Topic",sTrackTopic);

    for(int i=0; i<ui->tabWidget->count(); i++)
    {
        QRadarParamSetting *set = qobject_cast<QRadarParamSetting*>(ui->tabWidget->widget(i));
        if(!set) continue;
        QString str_radar = QString("Radar_%1").arg(i+1);
        Utils::Profiles::instance()->setValue(str_radar,"Distance",set->distance());
        Utils::Profiles::instance()->setValue(str_radar,"Centre_Lat",set->centerLat());
        Utils::Profiles::instance()->setValue(str_radar,"Centre_Lon",set->centerLon());
        Utils::Profiles::instance()->setValue(str_radar,"Loop_Num",set->loopNum());
        Utils::Profiles::instance()->setValue(str_radar,"Limit",set->limit());
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

        int uVideoSendPort = ui->videoSendPortSpinBox->value();
        Utils::Profiles::instance()->setValue(str_radar,"video_Send_Port",uVideoSendPort);
        QString sVideoTopic = ui->videoSendTopicLineEdit->text();
        Utils::Profiles::instance()->setValue(str_radar,"video_Topic",sVideoTopic);
        int uTrackSendPort = ui->trackSendPortSpinBox->value();
        Utils::Profiles::instance()->setValue(str_radar,"Track_Send_Port",uTrackSendPort);
        QString sTrackTopic = ui->trackSendTopicLineEdit->text();
        Utils::Profiles::instance()->setValue(str_radar,"Track_Topic",sTrackTopic); // /radar_offset.json
        QString sLimet_File = ui->limit_fileBox->currentText();
        Utils::Profiles::instance()->setValue(str_radar,"Limet_File",sLimet_File);

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
    }

    QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("配置保存成功，请重新启动软件！"));

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

void ZCHXRadarAisSetting::on_limit_fileBox_currentIndexChanged(int index)
{

}

void ZCHXRadarAisSetting::on_range_factor_valueChanged(double arg1)
{
    emit signalRangeFactorChanged(arg1);
}

void ZCHXRadarAisSetting::slotUpdateRealRangeFactor(double range, double factor)
{
    ui->realRangefactor->setText(tr("半径:%1  距离因子:%2").arg(range).arg(factor, 0, 'f', 2));
}
