#ifndef SYSTEMCONFIGWIDGET_H
#define SYSTEMCONFIGWIDGET_H

#include <QDialog>
#include "profiles.h"
#include "systemconfigsettingdefines.h"

class QJsonValue;

namespace Ui {
class SystemConfigWidget;
}
class SystemConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SystemConfigWidget(QWidget *parent = 0);
    ~SystemConfigWidget();

    void initSystemConfigWidget();
    bool IsInit();
    QMap<QString, COMDEVPARAM>  getComDevParams() const;
    QMap<QString, DEVANALYPARAM>    getSurface4017Param();
    QMap<QString, DEVANALYPARAM>    getUnderWater40171Param();
    QMap<QString, DEVANALYPARAM>    getUnderWater40172Param();
    QMap<QString, DEVANALYPARAM>    getTowbodyParam();
    QMap<QString, DEVANALYPARAM>    getOtherSetParam();
    COMDEVPARAM getUploadDpParam();

    QJsonValue toJson(const QString& topic, const QMap<QString, DEVANALYPARAM> params);
    QJsonValue toJson(const QString& topic, const QMap<QString, COMDEVPARAM> params);

    void       updateFromJsonObj(const QJsonObject& obj);
    void       updateDevAnalyParamFromJson(const QJsonArray& array, QMap<QString, DEVANALYPARAM>& params);
    void       updateWidgetwithDevParams(QMap<QString, Widget4017>& group, QMap<QString, DEVANALYPARAM>& params);
    void       updateComSetupParamFromJson(const QJsonArray& array, QMap<QString, COMDEVPARAM>& params);
    void       updateComWidgetwithComSetupParams(QMap<QString, WidgetComSetting>& group, QMap<QString, COMDEVPARAM>& params);

public slots:
    void       fromJson(const QByteArray& content);

signals:
    void updateSerialPort(const QMap<QString, COMDEVPARAM>& params);//更新内存中串口数据
    void signalImportSettingWithError(const QString& error);
    void signalUpdateSurface4017Setting(const QMap<QString, DEVANALYPARAM>& params);
    void signalUpdateUndetWater4017_1Setting(const QMap<QString, DEVANALYPARAM>& params);
    void signalUpdateUndetWater4017_2Setting(const QMap<QString, DEVANALYPARAM>& params);
    void signalUpdateTowBodySetting(const QMap<QString, DEVANALYPARAM>& params);
    void signalUpdateUnderWaterThresholdSetting(const QMap<QString, DEVANALYPARAM>& params);
    void signalUpdateSurfaceWaterThresholdSetting(const QMap<QString, DEVANALYPARAM>& params);
    void signalUpdateOtherSetting(const QMap<QString, DEVANALYPARAM>& params);
    void signalSendGuiFromJson(const QByteArray& content);

private slots:
    void on_modifyCommunicationSetup_clicked();

    void on_modifyAboveWaterSensorCfg_clicked();

    void on_modifyUnderWaterSensorCfg_clicked();

    void on_modifyTowedParam_clicked();

    void on_modifyOtherSetup_clicked();

    void on_modifyUnderWaterSensorCfg_2_clicked();

private:
    Ui::SystemConfigWidget *ui;
    //这里是每一项设定对应的参数
    QMap<QString, COMDEVPARAM>      mDevComSet;                     //串口设置
    QMap<QString, DEVANALYPARAM>    mSurfaceWater4017Params;      //水上4017
    QMap<QString, DEVANALYPARAM>    mUnderWater40171Params;     //水下4017_1
    QMap<QString, DEVANALYPARAM>    mUnderWater40172Params;     //水下4017_2
    QMap<QString, DEVANALYPARAM>    mOtherSetParams;            //其他参数
//    QMap<QString, DEVANALYPARAM>    mSurfaceThresholdParams;                //水上传感器阀值
//    QMap<QString, DEVANALYPARAM>    mUnderWaterThresholdParams;             //水下传感器阀值
    QMap<QString, DEVANALYPARAM>    mTowBodySetParams;           //拖体参数设置

    QMap<QString, Widget4017>   mSurface4017Widgets;
    QMap<QString, Widget4017>   mUnderWater4017_1Widgets;
    QMap<QString, Widget4017>   mUnderWater4017_2Widgets;
    QMap<QString, WidgetComSetting> mComSetupWidgets;
    QMap<QString, Widget4017>   mTowBodyWidgets;
    QMap<QString, Widget4017>   mOtherSettingWidgets;
    bool                        mInitFlag;
};
#endif // SYSTEMCONFIGWIDGET_H
