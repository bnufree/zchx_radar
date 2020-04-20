#ifndef COMCONFIGWIDGET_H
#define COMCONFIGWIDGET_H

#include <QDialog>
#include "profiles.h"
#include "comdefines.h"

class QJsonValue;

namespace Ui {
class ComConfigWidget;
}
class ComConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ComConfigWidget(QWidget *parent = 0);
    ~ComConfigWidget();

    void initComConfigWidget();
    bool IsInit();
    void setDefaultParam();
    QMap<QString, COMDEVPARAM>  getComDevParams() const;
    QJsonValue toJson(const QString& topic, const QMap<QString, COMDEVPARAM> params);
    void       updateFromJsonObj(const QJsonObject& obj);
    void       updateComSetupParamFromJson(const QJsonArray& array, QMap<QString, COMDEVPARAM>& params);
    void       updateComWidgetwithComSetupParams(QMap<QString, WidgetComSetting>& group, QMap<QString, COMDEVPARAM>& params);
private:
    QStringList     getComSaveStringList(bool isStart, const QString name, int baudrate,\
                                         int parity, int databit, int stopbit, int address);

public slots:
    void       fromJson(const QByteArray& content);

signals:
    void updateSerialPort(const QMap<QString, COMDEVPARAM>& params);//更新内存中串口数据
    void signalImportSettingWithError(const QString& error);
    void signalSendGuiFromJson(const QJsonValue& content);
    void signalUpdataZmq(QString,QString);

private slots:
    void on_modifyCommunicationSetup_clicked();
    void on_modifyCommunicationSetup_2_clicked();

private:
    Ui::ComConfigWidget *ui;
    //这里是每一项设定对应的参数
    QMap<QString, COMDEVPARAM>      mDevComSet;                     //串口设置
    QMap<QString, WidgetComSetting> mComSetupWidgets;
    bool                        mInitFlag;
};
#endif // COMCONFIGWIDGET_H
