#include "comconfigwidget.h"
#include "ui_comconfigwidget.h"
#include <QTranslator>
#include <QMessageBox>
#include <QEvent>
#include <QColorDialog>
#include "common.h"
#include <QDebug>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"


ComConfigWidget::ComConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ComConfigWidget)
{
    mInitFlag = false;
    ui->setupUi(this);
    qRegisterMetaType<QMap<QString, COMDEVPARAM>>("const QMap<QString, COMDEVPARAM>&");
    ui->addr1->setVisible(false);
    //Com通讯设置绑定
    mComSetupWidgets[COM_GPS_DEV] = WidgetComSetting(COM_GPS_DEV, ui->txsz_check1, ui->txsz_chnal1, ui->txsz_bit1, ui->addr1,ui->parity_bit1, ui->data_bit1, ui->stop_bit1);
    mComSetupWidgets[COM_WATER_DEV] = WidgetComSetting(COM_WATER_DEV, ui->txsz_check2, ui->txsz_chnal2, ui->txsz_bit2, ui->addr2, ui->parity_bit2, ui->data_bit2, ui->stop_bit2);
    mComSetupWidgets[COM_RDO_DEV] = WidgetComSetting(COM_RDO_DEV, ui->txsz_check3, ui->txsz_chnal3, ui->txsz_bit3, ui->addr3, ui->parity_bit3, ui->data_bit3, ui->stop_bit3);
    mComSetupWidgets[COM_ORP_DEV] = WidgetComSetting(COM_ORP_DEV, ui->txsz_check4, ui->txsz_chnal4, ui->txsz_bit4, ui->addr4, ui->parity_bit4, ui->data_bit4, ui->stop_bit4);
    mComSetupWidgets[COM_DDM_DEV] = WidgetComSetting(COM_DDM_DEV, ui->txsz_check5, ui->txsz_chnal5, ui->txsz_bit5, ui->addr5, ui->parity_bit5, ui->data_bit5, ui->stop_bit5);
    mComSetupWidgets[COM_NHN_DEV] = WidgetComSetting(COM_NHN_DEV, ui->txsz_check6, ui->txsz_chnal6, ui->txsz_bit6, ui->addr6, ui->parity_bit6, ui->data_bit6, ui->stop_bit6);
    mComSetupWidgets[COM_ZS_DEV] = WidgetComSetting(COM_ZS_DEV, ui->txsz_check7, ui->txsz_chnal7, ui->txsz_bit7, ui->addr7,ui->parity_bit7, ui->data_bit7, ui->stop_bit7);

    ui->port_lineEdit->setText(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_Send_Port").toString());
    ui->topic_lineEdit->setText(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_Topic").toString());
    ui->id_lineEdit->setText(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_ID").toString());
    setDefaultParam();
    initComConfigWidget();
    mInitFlag = true;
    this->setWindowTitle("串口通讯");
}

ComConfigWidget::~ComConfigWidget()
{
    delete ui;
}

QStringList ComConfigWidget::getComSaveStringList(bool isStart, const QString name, int baudrate, int parity, int databit, int stopbit, int address)
{
    QStringList list;
    list.append(QString::number(isStart == true? 1: 0));
    list.append(name);
    list.append(QString::number(baudrate));
    list.append(QString::number(parity));
    list.append(QString::number(databit));
    list.append(QString::number(stopbit));
    list.append(QString::number(address));
    return list;
}

void ComConfigWidget::setDefaultParam()
{
    /************************界面参数设定********************************/
    /************************串口设备参数设定********************************/
    PROFILES_INSTANCE->setDefault(COM_DEVICES_SEC,COM_GPS_DEV,getComSaveStringList(false, "COM1", 9600, 0, 8, 1, 6));
    PROFILES_INSTANCE->setDefault(COM_DEVICES_SEC,COM_WATER_DEV,getComSaveStringList(false, "COM2", 9600, 0, 8, 1, 6));
    PROFILES_INSTANCE->setDefault(COM_DEVICES_SEC,COM_RDO_DEV,getComSaveStringList(false, "COM3", 9600, 0, 8, 1, 6));
    PROFILES_INSTANCE->setDefault(COM_DEVICES_SEC,COM_ORP_DEV,getComSaveStringList(false, "COM4", 9600, 0, 8, 1, 6));
    PROFILES_INSTANCE->setDefault(COM_DEVICES_SEC,COM_DDM_DEV,getComSaveStringList(false, "COM5", 9600, 0, 8, 1, 6));
    PROFILES_INSTANCE->setDefault(COM_DEVICES_SEC,COM_NHN_DEV,getComSaveStringList(false, "COM6", 9600, 0, 8, 1, 6));
    PROFILES_INSTANCE->setDefault(COM_DEVICES_SEC,COM_ZS_DEV,getComSaveStringList(false, "COM7", 9600, 0, 8, 1, 6));
}

void ComConfigWidget::initComConfigWidget()
{
    //取得COM串口的配置设定,设定的格式为
    //模块名称=是否启用，串口名，波特率，校验位，数据位，停止位，地址位
    QStringList keys = PROFILES_INSTANCE->subkeys(COM_DEVICES_SEC);
    cout<<"subkeys"<<keys;
    foreach (QString key, keys) {
        QStringList vals = PROFILES_INSTANCE->value(COM_DEVICES_SEC, key, QStringList()).toStringList();
        cout<<"vals"<<vals;
        if(vals.length() >=7)
        {
            COMDEVPARAM param;
            param.mTopic = key;
            param.mStatus = vals[0].toInt();
            param.mName = vals[1];
            param.mBaudRate = vals[2].toInt();
            param.mParity = vals[3].toInt();
            param.mDataBit = vals[4].toInt();
            param.mStopBit = vals[5].toInt();
            param.mDevAddr = vals[6].toInt();
            param.mOpenMode = QIODevice::ReadWrite;
            if(key == COM_GPS_DEV)
            {
                param.mOpenMode = QIODevice::ReadOnly;
            }
            //初始化查询命令
            param.mQueryCmd.resize(8);
            if(key == COM_WATER_DEV)
            {
                param.mQueryCmd[1] = 0x04;
                param.mQueryCmd[2] = 0x75;
                param.mQueryCmd[3] = 0x31;
                param.mQueryCmd[4] = 0x00;
                param.mQueryCmd[5] = 0x05;
                param.mFuncCode = 0x04;
            } else
            {
                param.mQueryCmd[1] = 0x03;
                param.mQueryCmd[2] = 0x00;
                param.mQueryCmd[3] = 0x00;
                param.mQueryCmd[4] = 0x00;
                if(key == COM_ORP_DEV)
                {
                    param.mQueryCmd[5] = 0x02;
                } else
                {
                    param.mQueryCmd[5] = 0x04;
                }
                param.mFuncCode = 0x03;

            }
            param.mDataBitLen = uchar(param.mQueryCmd[5] * 2);
            param.mRetCmdLength = 3 + param.mDataBitLen + 2;
            mDevComSet[key] = param;
        }
    }

    foreach(QString key , mDevComSet.keys()){
        if(mComSetupWidgets.contains(key))
        {
            mComSetupWidgets[key].setCurValue(mDevComSet[key]);
        }
    }
}

void ComConfigWidget::on_modifyCommunicationSetup_clicked()
{
    //检查通道是否有重复
    QList<WidgetComSetting> ws = mComSetupWidgets.values();
    int lenth = ws.size();
    for(int i = 0; i < lenth; ++i){
        QString msg = ws[i].getChineseName();
        if(!ws[i].getFuncStatus()) continue;
        if(ws[i].getDevAddr().toInt() <= 0 ||
                ws[i].getDevAddr().toInt() > 127)
        {
            QMessageBox::warning(this, QStringLiteral("提示"), msg + QStringLiteral("地址设定错误！地址范围1-127"), QMessageBox::Yes);
            return;
        }

        for(size_t j = i+1; j < lenth; ++j){
            if(!ws[j].getFuncStatus()) continue;
            if(ws[i].getComName() == ws[j].getComName()){
                QString msg1 = ws[j].getChineseName();
                QMessageBox::warning(this, QStringLiteral("提示"), msg + QStringLiteral(" 与 ")+msg1+QStringLiteral(" 串口重复,请重新设置!"), QMessageBox::Yes);
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
        param.mDevAddr = w.getDevAddr().toInt();
        param.mStatus = w.getFuncStatus();
        param.mParity = w.getParity();
        param.mStopBit = w.getStopBit();
        param.mDataBit = w.getDataBit();
        if(key == COM_GPS_DEV)
        {
            param.mOpenMode = QIODevice::ReadOnly;
        } else
        {
            param.mOpenMode = QIODevice::ReadWrite;
        }
    }

    //将参数保存
    foreach (COMDEVPARAM param, mDevComSet.values()) {
        PROFILES_INSTANCE->setValue(COM_DEVICES_SEC, param.mTopic,\
                                    getComSaveStringList(param.mStatus, param.mName,param.mBaudRate,\
                                                         param.mParity,param.mDataBit,param.mStopBit,\
                                                         param.mDevAddr)
                                    );

    }
    emit updateSerialPort(mDevComSet);//更新内存中串口数据
    emit signalSendGuiFromJson(toJson(COM_DEVICES_SEC, mDevComSet));

    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("重新设置串口成功"), QMessageBox::Ok);
}

bool ComConfigWidget::IsInit()
{
    return mInitFlag;
}

QMap<QString, COMDEVPARAM> ComConfigWidget::getComDevParams() const
{
    return mDevComSet;
}

QJsonValue ComConfigWidget::toJson(const QString &topic, const QMap<QString, COMDEVPARAM> params)
{
    QJsonObject obj;
    if(params.count() > 0)
    {
        QJsonArray array;
        foreach (QString key, params.keys()) {
            COMDEVPARAM param = params[key];
            QJsonObject obj;            
            obj.insert("checked", param.mStatus);
            obj.insert("name", param.mName);
            obj.insert("baudrate", param.mBaudRate);
            obj.insert("parity", param.mParity);
            obj.insert("databit", param.mDataBit);
            obj.insert("stopbit", param.mStopBit);
            obj.insert("addr", param.mDevAddr);
            obj.insert("topic", param.mTopic);
            array.append(obj);
        }
        obj.insert(topic, array);
    }

    return obj;
}


void ComConfigWidget::fromJson(const QByteArray &content)
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

void ComConfigWidget::updateComSetupParamFromJson(const QJsonArray &array, QMap<QString, COMDEVPARAM> &params)
{
    for(int i=0; i<array.size(); i++)
    {
        QJsonObject wkobj = array[i].toObject();
        QString topic = wkobj.value("topic").toString();
        COMDEVPARAM &param = params[topic];
        param.mTopic = topic;
        param.mBaudRate = wkobj.value("baudrate").toInt();
        param.mName = wkobj.value("name").toString();
        param.mStatus = wkobj.value("checked").toBool();
        param.mDevAddr = wkobj.value("addr").toInt();
        param.mParity = wkobj.value("parity").toInt();
        param.mDataBit = wkobj.value("databit").toInt();
        param.mStopBit = wkobj.value("stopbit").toInt();
    }
}

void ComConfigWidget::updateComWidgetwithComSetupParams(QMap<QString, WidgetComSetting> &group, QMap<QString, COMDEVPARAM> &params)
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

void ComConfigWidget::updateFromJsonObj(const QJsonObject &obj)
{
    //取得当前部分对应的group
    foreach (QString key, obj.keys()) {
        QJsonValue val = obj.value(key);
        if(!val.isArray()) continue;
        QJsonArray array = val.toArray();
        if(key == COM_DEVICES_SEC)
        {
            updateComSetupParamFromJson(array, mDevComSet);
            updateComWidgetwithComSetupParams(mComSetupWidgets, mDevComSet);
            emit updateSerialPort(mDevComSet);
            //将参数保存
            foreach (COMDEVPARAM param, mDevComSet.values()) {
                PROFILES_INSTANCE->setValue(COM_DEVICES_SEC, param.mTopic,\
                                            getComSaveStringList(param.mStatus, param.mName,param.mBaudRate,\
                                                                 param.mParity,param.mDataBit,param.mStopBit,\
                                                                 param.mDevAddr)
                                            );

            }
        }
    }
}



void ComConfigWidget::on_modifyCommunicationSetup_2_clicked()
{
    emit signalUpdataZmq(ui->port_lineEdit->text(),ui->topic_lineEdit->text());
    PROFILES_INSTANCE->setValue(SERVER_SETTING_SEC, "GPS_ID", ui->id_lineEdit->text());
    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("修改成功"), QMessageBox::Ok);
}
