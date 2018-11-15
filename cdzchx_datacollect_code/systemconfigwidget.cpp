#include "systemconfigwidget.h"
#include "ui_systemconfigwidget.h"
#include <QTranslator>
#include <QMessageBox>
#include <QEvent>
#include <QColorDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QDateTime>


SystemConfigWidget::SystemConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SystemConfigWidget)
{
    mInitFlag = false;
    ui->setupUi(this);
    ui->water_sersor_frame->setVisible(false);
    qRegisterMetaType<QMap<QString, COMDEVPARAM>>("const QMap<QString, COMDEVPARAM>&");
    qRegisterMetaType<QMap<QString, DEVANALYPARAM>>("const QMap<QString, DEVANALYPARAM>&");
    //Com通讯设置绑定
    mComSetupWidgets[MSG_SURFACE_HMR] = WidgetComSetting(MSG_SURFACE_HMR, ui->txsz_check0, ui->txsz_chnal0, ui->txsz_bit0, ui->txsz_msg0, ui->parity_bit0, ui->data_bit0, ui->stop_bit0);
    mComSetupWidgets[MSG_GPS] = WidgetComSetting(MSG_GPS, ui->txsz_check1, ui->txsz_chnal1, ui->txsz_bit1, ui->txsz_msg1,ui->parity_bit1, ui->data_bit1, ui->stop_bit1);
    mComSetupWidgets[MSG_USBL] = WidgetComSetting(MSG_USBL, ui->txsz_check2, ui->txsz_chnal2, ui->txsz_bit2, ui->txsz_msg2, ui->parity_bit2, ui->data_bit2, ui->stop_bit2);
    mComSetupWidgets[MSG_UNDER_HMR] = WidgetComSetting(MSG_UNDER_HMR, ui->txsz_check3, ui->txsz_chnal3, ui->txsz_bit3, ui->txsz_msg3, ui->parity_bit3, ui->data_bit3, ui->stop_bit3);
    mComSetupWidgets[MSG_UNDER_4017] = WidgetComSetting(MSG_UNDER_4017, ui->txsz_check4, ui->txsz_chnal4, ui->txsz_bit4, ui->txsz_msg4, ui->parity_bit4, ui->data_bit4, ui->stop_bit4, ui->txsz_sem4, ui->txsz_sem5);
    mComSetupWidgets[MSG_SURFACE_4017] = WidgetComSetting(MSG_SURFACE_4017, ui->txsz_check6, ui->txsz_chnal6, ui->txsz_bit6, ui->txsz_msg6, ui->parity_bit5, ui->data_bit5, ui->stop_bit5, ui->txsz_sem6);
    mComSetupWidgets[MSG_METER_COUNTER] = WidgetComSetting(MSG_METER_COUNTER, ui->txsz_check7, ui->txsz_chnal7, ui->txsz_bit7, ui->txsz_msg7,ui->parity_bit6, ui->data_bit6, ui->stop_bit6, ui->txsz_sem7);
    mComSetupWidgets[MSG_NAVI_DEV] = WidgetComSetting(MSG_NAVI_DEV, ui->txsz_check8, ui->txsz_chnal8, ui->txsz_bit8, ui->txsz_msg8,ui->parity_bit7, ui->data_bit7, ui->stop_bit7);
    mComSetupWidgets[MSG_CABLE_DEV] = WidgetComSetting(MSG_CABLE_DEV, ui->txsz_check9, ui->txsz_chnal9, ui->txsz_bit9, ui->txsz_msg9, ui->parity_bit8, ui->data_bit8, ui->stop_bit8);
    mComSetupWidgets[MSG_DP_DEV] = WidgetComSetting(MSG_DP_DEV, ui->txsz_check10, ui->txsz_chnal10, ui->txsz_bit10, ui->txsz_msg10,ui->parity_bit9, ui->data_bit9, ui->stop_bit9);
    mComSetupWidgets[MSG_DP_UPLOAD_DEV] = WidgetComSetting(MSG_DP_UPLOAD_DEV, ui->txsz_check11, ui->txsz_chnal11, ui->txsz_bit11, 0, ui->parity_bit10, ui->data_bit10, ui->stop_bit10);
    mComSetupWidgets[MSG_TENSION_DEV] = WidgetComSetting(MSG_TENSION_DEV, ui->txsz_check12,ui->txsz_chnal12,ui->txsz_bit12, 0, ui->parity_bit12, ui->data_bit12,ui->stop_bit12);

    //水上4017传感器
    mSurface4017Widgets[PULL_4017_1] = Widget4017(PULL_4017_1, ui->smmn_Pull1_Block, ui->smmn_Pull1_Channel, ui->smmn_Pull1_Offset, ui->smmn_Pull1_offsetCoefficient);
    mSurface4017Widgets[PULL_4017_2] = Widget4017(PULL_4017_2, ui->smmn_Pull2_Block, ui->smmn_Pull2_Channel, ui->smmn_Pull2_Offset, ui->smmn_Pull2_offsetCoefficient);
    mSurface4017Widgets[PULL_4017_3] = Widget4017(PULL_4017_3, ui->smmn_Pull3_Block, ui->smmn_Pull3_Channel, ui->smmn_Pull3_Offset, ui->smmn_Pull3_offsetCoefficient);
    mSurface4017Widgets[LEFT_PUMP_4017] = Widget4017(LEFT_PUMP_4017, ui->smmn_LPump_Block, ui->smmn_LPump_Channel, ui->smmn_LPump_Offset, ui->smmn_LPump_offsetCoefficient);
    mSurface4017Widgets[RIGHT_PUMP_4017] = Widget4017(RIGHT_PUMP_4017, ui->smmn_RPump_Block, ui->smmn_RPump_Channel, ui->smmn_RPump_Offset, ui->smmn_RPump_offsetCoefficient);
    //水下传感器4017-1
    mUnderWater4017_1Widgets[DEPTH_4017] = Widget4017(DEPTH_4017, ui->mn4017_Depth_Block, ui->mn4017_Depth_Channel, ui->mn4017_Depth_Offset, ui->mn4017_Depth_offsetCoefficient);
    mUnderWater4017_1Widgets[BOOTS_ANGLE_4017] = Widget4017(BOOTS_ANGLE_4017, ui->mn4017_BootAngle_Block, ui->mn4017_BootAngle_Channel, ui->mn4017_BootAngle_Offset, ui->mn4017_BootAngle_offsetCoefficient);
    mUnderWater4017_1Widgets[TOUCHDOWN_4017_1] = Widget4017(TOUCHDOWN_4017_1, ui->mn4017_Touchdown1_Block, ui->mn4017_Touchdown1_Channel, ui->mn4017_Touchdown1_Offset, ui->mn4017_Touchdown1_offsetCoefficient);
    mUnderWater4017_1Widgets[TOUCHDOWN_4017_2] = Widget4017(TOUCHDOWN_4017_2, ui->mn4017_Touchdown2_Block, ui->mn4017_Touchdown2_Channel, ui->mn4017_Touchdown2_Offset, ui->mn4017_Touchdown2_offsetCoefficient);
    mUnderWater4017_1Widgets[TOUCHDOWN_4017_3] = Widget4017(TOUCHDOWN_4017_3, ui->mn4017_Touchdown3_Block, ui->mn4017_Touchdown3_Channel, ui->mn4017_Touchdown3_Offset, ui->mn4017_Touchdown3_offsetCoefficient);
    mUnderWater4017_1Widgets[TOUCHDOWN_4017_4] = Widget4017(TOUCHDOWN_4017_4, ui->mn4017_Touchdown4_Block, ui->mn4017_Touchdown4_Channel, ui->mn4017_Touchdown4_Offset, ui->mn4017_Touchdown4_offsetCoefficient);
    //水下传感器4017-2
    mUnderWater4017_2Widgets[PULL_4017_1] = Widget4017(PULL_4017_1, ui->mn40172_pull1_block, ui->mn40172_pull1_channel, ui->mn40172_pull1_offset, ui->mn40172_pull1_offset_coff);
    mUnderWater4017_2Widgets[PULL_4017_2] = Widget4017(PULL_4017_2, ui->mn40172_pull2_block, ui->mn40172_pull2_channel, ui->mn40172_pull2_offset, ui->mn40172_pull2_offset_coff);
    mUnderWater4017_2Widgets[PULL_4017_3] = Widget4017(PULL_4017_3, ui->mn40172_pull3_block, ui->mn40172_pull3_channel, ui->mn40172_pull3_offset, ui->mn40172_pull3_offset_coff);
    mUnderWater4017_2Widgets[LEFT_PUMP_4017] = Widget4017(LEFT_PUMP_4017, ui->mn40172_lpump_block, ui->mn40172_lpump_channel, ui->mn40172_lpump_offset, ui->mn40172_lpump_offset_coff);
    mUnderWater4017_2Widgets[RIGHT_PUMP_4017] = Widget4017(RIGHT_PUMP_4017, ui->mn40172_rpump_block, ui->mn40172_rpump_channel, ui->mn40172_rpump_offset, ui->mn40172_rpump_offset_coff);

    //拖体参数设定绑定
    mTowBodyWidgets[FORWARD_LENGTH] = Widget4017(FORWARD_LENGTH, 0, 0, ui->forward, 0);
    mTowBodyWidgets[BACKWARD_LENGTH] = Widget4017(BACKWARD_LENGTH, 0, 0, ui->backward, 0);
    mTowBodyWidgets[BOOTS_LENGTH] = Widget4017(BOOTS_LENGTH, 0, 0, ui->boots_body_length, 0);
    mTowBodyWidgets[TOW_BODY_RANGE] = Widget4017(TOW_BODY_RANGE, 0, 0, ui->tow_body_range, 0);
    mTowBodyWidgets[BOOTS_HEIGHT] = Widget4017(BOOTS_HEIGHT, 0, 0, ui->boots_body_height, 0);


    //其他的参数设定绑定
    mOtherSettingWidgets[METER_COUNTER_COEFF] = Widget4017(METER_COUNTER_COEFF, 0, 0, ui->MeterStep, 0);
    mOtherSettingWidgets[METER_COUNTER_INIT] = Widget4017(METER_COUNTER_INIT, 0, 0, ui->MeterInitValue, 0);
    //mOtherSettingWidgets[REPORT_STEP] = Widget4017(REPORT_STEP, 0, 0, ui->reportStepSize, 0);
    mOtherSettingWidgets[GPS_LONOFFSET] = Widget4017(GPS_LONOFFSET, 0, 0, ui->gpsLonDeviation, 0);
    mOtherSettingWidgets[GPS_LATOFFSET] = Widget4017(GPS_LATOFFSET, 0, 0, ui->gpsLatDeviation, 0);
    mOtherSettingWidgets[GPS_SHIP_LENGTH] = Widget4017(GPS_SHIP_LENGTH, 0, 0, ui->shipLength, 0);
    //mOtherSettingWidgets[STORAGE_TIME] = Widget4017(STORAGE_TIME, 0, 0, ui->storageTime, 0);
    //mOtherSettingWidgets[DISPLAY_TIME] = Widget4017(DISPLAY_TIME, 0, 0, ui->displayTime, 0);
    mOtherSettingWidgets[SURFACE_HMR_OFFSET] = Widget4017(SURFACE_HMR_OFFSET, 0, 0, ui->surface_hmr_offset, 0);
    mOtherSettingWidgets[UNDER_HMR_OFFSET] = Widget4017(UNDER_HMR_OFFSET, 0, 0, ui->under_hmr_offset, 0);
    mOtherSettingWidgets[TENSION_COEFF] = Widget4017(TENSION_COEFF, 0, 0, ui->tensionCoeff, 0);
    mOtherSettingWidgets[SPEED_TIME_GAP] = Widget4017(SPEED_TIME_GAP, 0, 0, ui->timeGap, 0);
    initSystemConfigWidget();
    mInitFlag = true;
}

SystemConfigWidget::~SystemConfigWidget()
{
    delete ui;
}

void SystemConfigWidget::initSystemConfigWidget()
{
    //取得COM串口的配置设定,设定的格式为
    //模块名称=串口名，波特率，消息格式, 是否启用, 消息号1,消息号2
    //dp=COM6,4800,,$GPGLL,1
    QStringList keys = Utils::Profiles::instance()->subkeys(MSG_COM_SET);
    foreach (QString key, keys) {
        QStringList vals = Utils::Profiles::instance()->value(MSG_COM_SET, key, QStringList()).toStringList();
        if(vals.length() >=5)
        {
            COMDEVPARAM param;
            param.mTopic = key;
            param.mName = vals[0];
            param.mBaudRate = vals[1].toInt();
            param.mMessageIdentifier = vals[2];
            param.mStatus = vals[3].toInt();
            param.mOpenMode = QIODevice::ReadOnly;
            if(key == MSG_DP_UPLOAD_DEV)
            {
                param.mOpenMode = QIODevice::ReadWrite;
            }/* else if(key == MSG_TENSION_DEV)
            {
                param.mOpenMode = QIODevice::ReadWrite;
            }*/
            param.mMessageNum1 = vals[4];
            if(vals.length() > 5)
            {
                param.mMessageNum2 = vals[5];
            }
            if(vals.length() > 8)
            {
                param.mParity = vals[6].toInt();
                param.mDataBit = vals[7].toInt();
                param.mStopBit = vals[8].toInt();
            }
                mDevComSet[key] = param;

        }
    }

    foreach(QString key , mDevComSet.keys()){
        if(mComSetupWidgets.contains(key))
        {
            if(key == MSG_GPS)
            {
                COMDEVPARAM param = mDevComSet[key];
                qDebug()<<param.mName<<param.mStatus<<param.mBaudRate<<param.mDataBit<<param.mMessageIdentifier<<param.mParity<<param.mStopBit;
            }
            mComSetupWidgets[key].setCurValue(mDevComSet[key]);
        }
    }
    //水面4017
    //参数名=通道名，误差，误差系数，启用状态
    keys = Utils::Profiles::instance()->subkeys(SURFACE_WATER_4017_SET);
    foreach (QString key, keys) {
        QStringList vals = Utils::Profiles::instance()->value(SURFACE_WATER_4017_SET, key, QStringList()).toStringList();
        if(vals.length() >=4)
        {
            DEVANALYPARAM param;
            param.name = key;
            param.channel = vals[0];
            param.deviation = vals[1].toDouble();
            param.deviationCoefficient = vals[2].toDouble();
            param.sts = vals[3].toInt();
            mSurfaceWater4017Params[key] = param;
        }
    }
    foreach(QString key , mSurfaceWater4017Params.keys()){
        if(mSurface4017Widgets.contains(key))
        {
            mSurface4017Widgets[key].setCurValue(mSurfaceWater4017Params[key]);
        }
    }

    //水下4017-1
    //参数名=通道名，误差，误差系数，启用状态
    keys = Utils::Profiles::instance()->subkeys(UNDER_WATER_4017_1_SET);
    foreach (QString key, keys) {
        QStringList vals = Utils::Profiles::instance()->value(UNDER_WATER_4017_1_SET, key, QStringList()).toStringList();
        if(vals.length() >=4)
        {
            DEVANALYPARAM param;
            param.name = key;
            param.channel = vals[0];
            param.deviation = vals[1].toDouble();
            param.deviationCoefficient = vals[2].toDouble();
            param.sts = vals[3].toInt();
            mUnderWater40171Params[key] = param;
        }
    }
    foreach(QString key , mUnderWater40171Params.keys()){
        if(mUnderWater4017_1Widgets.contains(key))
        {
            mUnderWater4017_1Widgets[key].setCurValue(mUnderWater40171Params[key]);
        }
    }
    //水下4017-2
    //参数名=通道名，误差，误差系数，启用状态
    keys = Utils::Profiles::instance()->subkeys(UNDER_WATER_4017_2_SET);
    foreach (QString key, keys) {
        QStringList vals = Utils::Profiles::instance()->value(UNDER_WATER_4017_2_SET, key, QStringList()).toStringList();
        if(vals.length() >=4)
        {
            DEVANALYPARAM param;
            param.name = key;
            param.channel = vals[0];
            param.deviation = vals[1].toDouble();
            param.deviationCoefficient = vals[2].toDouble();
            param.sts = vals[3].toInt();
            mUnderWater40172Params[key] = param;
        }
    }
    foreach(QString key , mUnderWater40172Params.keys()){
        if(mUnderWater4017_2Widgets.contains(key))
        {
            mUnderWater4017_2Widgets[key].setCurValue(mUnderWater40172Params[key]);
        }
    }

    //其他的参数设定
    keys = Utils::Profiles::instance()->subkeys(OTHER_PARAM_SET);
    foreach (QString key, keys) {
        QString vals = Utils::Profiles::instance()->value(OTHER_PARAM_SET, key, QString()).toString();
        DEVANALYPARAM param;
        param.name = key;
        param.deviation = vals.toDouble();
        mOtherSetParams[key] = param;
    }
    if(!mOtherSetParams.contains(TENSION_COEFF))
    {
        DEVANALYPARAM param;
        param.name = TENSION_COEFF;
        param.deviation = 1.00;
        mOtherSetParams[TENSION_COEFF] = param;
    }
    if(!mOtherSetParams.contains(GPS_LATOFFSET))
    {
        DEVANALYPARAM param;
        param.name = GPS_LATOFFSET;
        param.deviation = 0.00;
        mOtherSetParams[GPS_LATOFFSET] = param;
    }

    if(!mOtherSetParams.contains(GPS_LONOFFSET))
    {
        DEVANALYPARAM param;
        param.name = GPS_LONOFFSET;
        param.deviation = 0.00;
        mOtherSetParams[GPS_LONOFFSET] = param;
    }
    if(!mOtherSetParams.contains(GPS_SHIP_AHEAD))
    {
        DEVANALYPARAM param;
        param.name = GPS_SHIP_AHEAD;
        param.deviation = 1.0;
        mOtherSetParams[GPS_SHIP_AHEAD] = param;
    }

    if(!mOtherSetParams.contains(GPS_SHIP_LENGTH))
    {
        DEVANALYPARAM param;
        param.name = GPS_SHIP_LENGTH;
        param.deviation = 0.00;
        mOtherSetParams[GPS_SHIP_LENGTH] = param;
    }
    if(!mOtherSetParams.contains(SPEED_TIME_GAP))
    {
        DEVANALYPARAM param;
        param.name = SPEED_TIME_GAP;
        param.deviation = 3.00;
        mOtherSetParams[SPEED_TIME_GAP] = param;
    }

    foreach(QString key , mOtherSetParams.keys()){
        if(mOtherSettingWidgets.contains(key))
        {
            mOtherSettingWidgets[key].setCurValue(mOtherSetParams[key]);
        }
    }
    {
        DEVANALYPARAM param = mOtherSetParams[GPS_SHIP_AHEAD];
        if(param.deviation > 0)
        {
            ui->ship_head_pos_cbx->setCurrentIndex(0);
        } else
        {
            ui->ship_head_pos_cbx->setCurrentIndex(1);
        }

    }

    //拖体参数设定
    keys = Utils::Profiles::instance()->subkeys(TOW_BODY_SET);
    foreach (QString key, keys) {
        QString vals = Utils::Profiles::instance()->value(TOW_BODY_SET, key, QString()).toString();
        DEVANALYPARAM param;
        param.name = key;
        param.deviation = vals.toDouble();
        mTowBodySetParams[key] = param;
    }
    foreach(QString key , mTowBodySetParams.keys()){
        if(mTowBodyWidgets.contains(key))
        {
            mTowBodyWidgets[key].setCurValue(mTowBodySetParams[key]);
        }
    }
}

//水上参数的保存
void SystemConfigWidget::on_modifyAboveWaterSensorCfg_clicked()
{
    //检查通道是否有重复
    QList<Widget4017> ws = mSurface4017Widgets.values();
    int lenth = ws.size();
    for(int i = 0; i < lenth; ++i){
        QString msg = ws[i].getChineseName();
        for(size_t j = i+1; j < lenth; ++j){
            if(ws[i].getChannelName() == ws[j].getChannelName()){
                QString msg1 = ws[j].getChineseName();
                QMessageBox::warning(this, QStringLiteral("提示"), msg + QStringLiteral(" 与 ")+msg1+QStringLiteral(" 频道重复,请重新设置!"), QMessageBox::Cancel);
                return;
            }
        }
    }

    //检查所有的选项，然后保存到对应的列表中
    foreach (QString key, mSurface4017Widgets.keys()) {
        DEVANALYPARAM& param = mSurfaceWater4017Params[key];
        Widget4017 w = mSurface4017Widgets[key];
        param.name = key;
        param.chineseName = w.getChineseName();
        param.channel = w.getChannelName();
        param.deviation = w.getOffset();
        param.deviationCoefficient = w.getOffsetCoeff();
        param.sts = w.getFuncStatus();
    }

    //将参数保存
    foreach (DEVANALYPARAM param, mSurfaceWater4017Params.values()) {
        Utils::Profiles::instance()->setValue(
                    SURFACE_WATER_4017_SET,
                    param.name,
                    QStringList()<<param.channel<<QString::number(param.deviation)
                    <<QString::number(param.deviationCoefficient)<<QString::number(param.sts)
                    );

    }

    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("水面模拟传感器设置成功"), QMessageBox::Ok);
    emit signalUpdateSurface4017Setting(mSurfaceWater4017Params);
    QJsonDocument doc;
    doc.setObject(toJson(SURFACE_WATER_4017_SET, mSurfaceWater4017Params).toObject());
    emit signalSendGuiFromJson(doc.toJson());
}

void SystemConfigWidget::on_modifyUnderWaterSensorCfg_clicked()
{
    //检查通道是否有重复
    QList<Widget4017> ws = mUnderWater4017_1Widgets.values();
    int lenth = ws.size();
    for(int i = 0; i < lenth; ++i){
        QString msg = ws[i].getChineseName();
        for(size_t j = i+1; j < lenth; ++j){
            if(ws[i].getChannelName() == ws[j].getChannelName()){
                QString msg1 = ws[j].getChineseName();
                QMessageBox::warning(this, QStringLiteral("提示"), msg + QStringLiteral(" 与 ")+msg1+QStringLiteral(" 频道重复,请重新设置!"), QMessageBox::Cancel);
                return;
            }
        }
    }

    //检查所有的选项，然后保存到对应的列表中
    foreach (QString key, mUnderWater4017_1Widgets.keys()) {
        DEVANALYPARAM& param = mUnderWater40171Params[key];
        Widget4017 w = mUnderWater4017_1Widgets[key];
        param.name = key;
        param.chineseName = w.getChannelName();
        param.channel = w.getChannelName();
        param.deviation = w.getOffset();
        param.deviationCoefficient = w.getOffsetCoeff();
        param.sts = w.getFuncStatus();
    }

    //将参数保存
    foreach (DEVANALYPARAM param, mUnderWater40171Params.values()) {
        //参数名=通道名，误差，误差系数，启用状态
        Utils::Profiles::instance()->setValue(
                    UNDER_WATER_4017_1_SET,
                    param.name,
                    QStringList()<<param.channel<<QString::number(param.deviation)
                    <<QString::number(param.deviationCoefficient)<<QString::number(param.sts)
                    );

    }

    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("水下模拟传感器4017-1设置成功"), QMessageBox::Ok);
    emit signalUpdateUndetWater4017_1Setting(mUnderWater40171Params);

    QJsonDocument doc;
    doc.setObject(toJson(UNDER_WATER_4017_1_SET, mUnderWater40171Params).toObject());
    emit signalSendGuiFromJson(doc.toJson());
}

void SystemConfigWidget::on_modifyTowedParam_clicked()
{
    //检查所有的选项，然后保存到对应的列表中
    foreach (QString key, mTowBodyWidgets.keys()) {
        DEVANALYPARAM& param = mTowBodySetParams[key];
        Widget4017 w = mTowBodyWidgets[key];
        param.name = key;
        param.deviation = w.getOffset();
    }

    //将参数保存
    foreach (DEVANALYPARAM param, mTowBodySetParams.values()) {
        Utils::Profiles::instance()->setValue(TOW_BODY_SET, param.name, param.deviation);
    }

    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("拖体参数设置成功"), QMessageBox::Ok);
    emit signalUpdateTowBodySetting(mTowBodySetParams);
    QJsonDocument doc;
    doc.setObject(toJson(TOW_BODY_SET, mTowBodySetParams).toObject());
    emit signalSendGuiFromJson(doc.toJson());
}


void SystemConfigWidget::on_modifyOtherSetup_clicked()
{
    //检查所有的选项，然后保存到对应的列表中
    foreach (QString key, mOtherSettingWidgets.keys()) {
        DEVANALYPARAM& param = mOtherSetParams[key];
        Widget4017 w = mOtherSettingWidgets[key];
        param.name = key;
        param.deviation = w.getOffset();
    }
    //船舶位置保存
    DEVANALYPARAM& param = mOtherSetParams[GPS_SHIP_AHEAD];
    param.name = GPS_SHIP_AHEAD;
    param.deviation = ui->ship_head_pos_cbx->currentIndex() == 0 ? 1.0 : 0.0;

    //将参数保存
    foreach (DEVANALYPARAM param, mOtherSetParams.values()) {
        Utils::Profiles::instance()->setValue(OTHER_PARAM_SET, param.name, param.deviation);
    }
    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("其他参数设置成功"), QMessageBox::Ok);
    emit signalUpdateOtherSetting(mOtherSetParams);
    QJsonDocument doc;
    doc.setObject(toJson(OTHER_PARAM_SET, mOtherSetParams).toObject());
    emit signalSendGuiFromJson(doc.toJson());
}


void SystemConfigWidget::on_modifyCommunicationSetup_clicked()
{
    //检查通道是否有重复
    QList<WidgetComSetting> ws = mComSetupWidgets.values();
    int lenth = ws.size();
    for(int i = 0; i < lenth; ++i){
        if(!ws[i].getFuncStatus()) continue;
        QString msg = ws[i].getChineseName();
        for(size_t j = i+1; j < lenth; ++j){
            if(!ws[j].getFuncStatus()) continue;
            if(ws[i].getComName() == ws[j].getComName()){
                QString msg1 = ws[j].getChineseName();
                QMessageBox::warning(this, QStringLiteral("提示"), msg + QStringLiteral(" 与 ")+msg1+QStringLiteral(" 通道重复,请重新设置!"), QMessageBox::Cancel);
                return;
            }
        }
    }

    //检查所有的选项，然后保存到对应的列表中
    foreach (QString key, mComSetupWidgets.keys()) {
        COMDEVPARAM& param = mDevComSet[key];
        WidgetComSetting w = mComSetupWidgets[key];
        param.mTopic = key;
        param.mName = w.getComName();
        param.mBaudRate = w.getBaudrate();
        param.mMessageNum1 = w.getMsgNum1();
        param.mMessageNum2 = w.getMsgNum2();
        param.mMessageIdentifier = w.getMsgHead();
        param.mStatus = w.getFuncStatus();
        param.mParity = w.getParity();
        param.mStopBit = w.getStopBit();
        param.mDataBit = w.getDataBit();
        param.mOpenMode = QIODevice::ReadOnly;
        if(key == MSG_DP_UPLOAD_DEV)
        {
            param.mOpenMode = QIODevice::ReadWrite;
        } /*else if(key == MSG_TENSION_DEV)
        {
            param.mOpenMode = QIODevice::ReadWrite;
        }*/

        qDebug()<<key<<param.mParity<<param.mDataBit<<param.mStopBit;
    }

    //将参数保存
    foreach (COMDEVPARAM param, mDevComSet.values()) {
        //模块名称=串口名，波特率，消息号，消息格式, 是否启用，
        //dp=COM6,4800,,$GPGLL,1
        Utils::Profiles::instance()->setValue(
                    MSG_COM_SET,
                    param.mTopic,
                    QStringList()<<param.mName<<QString::number(param.mBaudRate)
                    <<param.mMessageIdentifier<<QString::number(param.mStatus)<<param.mMessageNum1<<param.mMessageNum2
                    <<QString::number(param.mParity)<<QString::number(param.mDataBit)<<QString::number(param.mStopBit)
                    );

    }
    emit updateSerialPort(mDevComSet);//更新内存中串口数据

    QJsonDocument doc;
    doc.setObject(toJson(MSG_COM_SET, mDevComSet).toObject());
    //emit signalSendGuiFromJson(doc.toJson());

    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("重新设置串口成功"), QMessageBox::Ok);
}

void SystemConfigWidget::on_modifyUnderWaterSensorCfg_2_clicked()
{
    //检查通道是否有重复
    QList<Widget4017> ws = mUnderWater4017_2Widgets.values();
    int lenth = ws.size();
    for(int i = 0; i < lenth; ++i){
        QString msg = ws[i].getChineseName();
        for(size_t j = i+1; j < lenth; ++j){
            if(ws[i].getChannelName() == ws[j].getChannelName()){
                QString msg1 = ws[j].getChineseName();
                QMessageBox::warning(this, QStringLiteral("提示"), msg + QStringLiteral(" 与 ")+msg1+QStringLiteral(" 频道重复,请重新设置!"), QMessageBox::Cancel);
                return;
            }
        }
    }

    //检查所有的选项，然后保存到对应的列表中
    foreach (QString key, mUnderWater4017_2Widgets.keys()) {
        DEVANALYPARAM& param = mUnderWater40172Params[key];
        Widget4017 w = mUnderWater4017_2Widgets[key];
        param.name = key;
        param.chineseName = w.getChannelName();
        param.channel = w.getChannelName();
        param.deviation = w.getOffset();
        param.deviationCoefficient = w.getOffsetCoeff();
        param.sts = w.getFuncStatus();
    }

    //将参数保存
    foreach (DEVANALYPARAM param, mUnderWater40172Params.values()) {
        //参数名=通道名，误差，误差系数，启用状态
        Utils::Profiles::instance()->setValue(
                    UNDER_WATER_4017_2_SET,
                    param.name,
                    QStringList()<<param.channel<<QString::number(param.deviation)
                    <<QString::number(param.deviationCoefficient)<<QString::number(param.sts)
                    );

    }

    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("水下模拟传感器4017-2设置成功"), QMessageBox::Ok);
    emit signalUpdateUndetWater4017_2Setting(mUnderWater40172Params);

    QJsonDocument doc;
    doc.setObject(toJson(UNDER_WATER_4017_2_SET, mUnderWater40172Params).toObject());
    emit signalSendGuiFromJson(doc.toJson());
}

bool SystemConfigWidget::IsInit()
{
    return mInitFlag;
}

QMap<QString, COMDEVPARAM> SystemConfigWidget::getComDevParams() const
{
    return mDevComSet;
}

QMap<QString, DEVANALYPARAM> SystemConfigWidget::getSurface4017Param()
{
    return mSurfaceWater4017Params;
}

QMap<QString, DEVANALYPARAM> SystemConfigWidget::getUnderWater40171Param()
{
    return mUnderWater40171Params;
}

QMap<QString, DEVANALYPARAM> SystemConfigWidget::getUnderWater40172Param()
{
    return mUnderWater40172Params;
}

QMap<QString, DEVANALYPARAM> SystemConfigWidget::getTowbodyParam()
{
    return mTowBodySetParams;
}

QMap<QString, DEVANALYPARAM> SystemConfigWidget::getOtherSetParam()
{
    return mOtherSetParams;
}

COMDEVPARAM SystemConfigWidget::getUploadDpParam()
{
    return mDevComSet[MSG_DP_UPLOAD_DEV];
}

QJsonValue SystemConfigWidget::toJson(const QString &topic, const QMap<QString, COMDEVPARAM> params)
{
    QJsonObject obj;
    if(params.count() > 0)
    {
        QJsonArray array;
        foreach (QString key, params.keys()) {
            COMDEVPARAM param = params[key];
            QJsonObject obj;
            obj.insert("name", param.mName);
            obj.insert("baudrate", param.mBaudRate);
            obj.insert("msg", param.mMessageIdentifier);
            obj.insert("msgnum1", param.mMessageNum1);
            obj.insert("topic", param.mTopic);
            obj.insert("msgnum2", param.mMessageNum2);
            obj.insert("checked", param.mStatus);
            array.append(obj);
        }
        obj.insert(topic, array);
    }

    return obj;
}

QJsonValue SystemConfigWidget::toJson(const QString &topic, const QMap<QString, DEVANALYPARAM> params)
{
    QJsonObject obj;
    if(params.count() > 0)
    {
        QJsonArray array;
        foreach (QString key, params.keys()) {
            DEVANALYPARAM param = params[key];
            QJsonObject obj;
            obj.insert("channel", param.channel);
            obj.insert("offset", param.deviation);
            obj.insert("coeff", param.deviationCoefficient);
            obj.insert("topic", key);
            array.append(obj);
        }
        obj.insert(topic, array);
    }
    return obj;
}

void SystemConfigWidget::fromJson(const QByteArray &content)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(content, &err);
    qDebug()<<QDateTime::currentDateTime()<< "recv from server:"<<doc;
    if(err.error != QJsonParseError::NoError) return;
    if(doc.isObject())
    {
        //只更新单个部分
        updateFromJsonObj(doc.object());
    }

    if(doc.isArray())
    {
        //更新了多个部分的参数
        QJsonArray array = doc.array();
        for(int i=0; i<array.size(); i++)
        {
            updateFromJsonObj(array[i].toObject());
        }
    }


}

void SystemConfigWidget::updateDevAnalyParamFromJson(const QJsonArray &array, QMap<QString, DEVANALYPARAM> &params)
{
    for(int i=0; i<array.size(); i++)
    {
        QJsonObject wkobj = array[i].toObject();
        QString name = wkobj.value("topic").toString();
        DEVANALYPARAM &param = params[name];
        param.name = name;
        param.channel = wkobj.value("channel").toString();
        param.deviation = wkobj.value("offset").toDouble();
        param.deviationCoefficient = wkobj.value("coeff").toDouble();
    }
}

void SystemConfigWidget::updateWidgetwithDevParams(QMap<QString, Widget4017>& group, QMap<QString, DEVANALYPARAM> &params)
{
    //更新界面
    foreach(QString key , params.keys()){
        if(group.contains(key))
        {
            group[key].setCurValue(params[key]);
        }
    }
}

void SystemConfigWidget::updateComSetupParamFromJson(const QJsonArray &array, QMap<QString, COMDEVPARAM> &params)
{
    for(int i=0; i<array.size(); i++)
    {
        QJsonObject wkobj = array[i].toObject();
        QString topic = wkobj.value("topic").toString();
        COMDEVPARAM &param = params[topic];
        param.mTopic = topic;
        param.mBaudRate = wkobj.value("baudrate").toInt();
        param.mName = wkobj.value("name").toString();
        param.mMessageIdentifier = wkobj.value("msg").toString();
        param.mStatus = wkobj.value("checked").toBool();
        param.mMessageNum1 = wkobj.value("msgnum1").toInt();
        param.mMessageNum2 = wkobj.value("msgnum2").toInt();

        qDebug()<<"wkobj:"<<wkobj<<" status:"<<param.mStatus<<" src:"<<wkobj.value("checked")<<param.mName;
    }
}

void SystemConfigWidget::updateComWidgetwithComSetupParams(QMap<QString, WidgetComSetting> &group, QMap<QString, COMDEVPARAM> &params)
{
    //更新界面
    foreach(QString key , params.keys()){
        qDebug()<<"key:"<<key<<" status:"<<params[key].mStatus;
        if(group.contains(key))
        {
            group[key].setCurValue(params[key]);
        }
    }
}

void SystemConfigWidget::updateFromJsonObj(const QJsonObject &obj)
{
    //取得当前部分对应的group
    foreach (QString key, obj.keys()) {
        QJsonValue val = obj.value(key);
        if(!val.isArray()) continue;
        QJsonArray array = val.toArray();
        if(key == SURFACE_WATER_4017_SET)
        {
            updateDevAnalyParamFromJson(array, mSurfaceWater4017Params);
            updateWidgetwithDevParams(mSurface4017Widgets, mSurfaceWater4017Params);
            emit signalUpdateSurface4017Setting(mSurfaceWater4017Params);
            //将参数保存
            foreach (DEVANALYPARAM param, mSurfaceWater4017Params.values()) {
                Utils::Profiles::instance()->setValue(
                            SURFACE_WATER_4017_SET,
                            param.name,
                            QStringList()<<param.channel<<QString::number(param.deviation)
                            <<QString::number(param.deviationCoefficient)<<QString::number(param.sts)
                            );

            }
        } else if(key == UNDER_WATER_4017_1_SET)
        {
            updateDevAnalyParamFromJson(array, mUnderWater40171Params);
            updateWidgetwithDevParams(mUnderWater4017_1Widgets, mUnderWater40171Params);
            emit signalUpdateUndetWater4017_1Setting(mUnderWater40171Params);
            //将参数保存
            foreach (DEVANALYPARAM param, mUnderWater40171Params.values()) {
                //参数名=通道名，误差，误差系数，启用状态
                Utils::Profiles::instance()->setValue(
                            UNDER_WATER_4017_1_SET,
                            param.name,
                            QStringList()<<param.channel<<QString::number(param.deviation)
                            <<QString::number(param.deviationCoefficient)<<QString::number(param.sts)
                            );

            }

        } else if(key == UNDER_WATER_4017_2_SET)
        {
            updateDevAnalyParamFromJson(array, mUnderWater40172Params);
            updateWidgetwithDevParams(mUnderWater4017_2Widgets, mUnderWater40172Params);
            emit signalUpdateUndetWater4017_2Setting(mUnderWater40172Params);
            //将参数保存
            foreach (DEVANALYPARAM param, mUnderWater40172Params.values()) {
                //参数名=通道名，误差，误差系数，启用状态
                Utils::Profiles::instance()->setValue(
                            UNDER_WATER_4017_2_SET,
                            param.name,
                            QStringList()<<param.channel<<QString::number(param.deviation)
                            <<QString::number(param.deviationCoefficient)<<QString::number(param.sts)
                            );

            }
        } else if(key == TOW_BODY_SET)
        {
            updateDevAnalyParamFromJson(array, mTowBodySetParams);
            updateWidgetwithDevParams(mTowBodyWidgets, mTowBodySetParams);
            emit signalUpdateTowBodySetting(mTowBodySetParams);
            //将参数保存
            foreach (DEVANALYPARAM param, mTowBodySetParams.values()) {
                Utils::Profiles::instance()->setValue(TOW_BODY_SET, param.name, param.deviation);
            }
        } else if(key == OTHER_PARAM_SET)
        {
            updateDevAnalyParamFromJson(array, mOtherSetParams);
            updateWidgetwithDevParams(mOtherSettingWidgets, mOtherSetParams);
            emit signalUpdateOtherSetting(mOtherSetParams);
            //将参数保存
            foreach (DEVANALYPARAM param, mTowBodySetParams.values()) {
                Utils::Profiles::instance()->setValue(TOW_BODY_SET, param.name, param.deviation);
            }
        } else if(key == MSG_COM_SET)
        {
            updateComSetupParamFromJson(array, mDevComSet);
            updateComWidgetwithComSetupParams(mComSetupWidgets, mDevComSet);
            emit updateSerialPort(mDevComSet);
            //将参数保存
            foreach (COMDEVPARAM param, mDevComSet.values()) {
                //模块名称=串口名，波特率，消息号，消息格式, 是否启用，
                //dp=COM6,4800,,$GPGLL,1
                Utils::Profiles::instance()->setValue(
                            MSG_COM_SET,
                            param.mTopic,
                            QStringList()<<param.mName<<QString::number(param.mBaudRate)
                            <<param.mMessageIdentifier<<QString::number(param.mStatus)<<param.mMessageNum1<<param.mMessageNum2
                            );

            }
        }
    }
}


