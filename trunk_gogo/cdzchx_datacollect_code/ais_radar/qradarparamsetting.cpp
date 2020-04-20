#include "qradarparamsetting.h"
#include "ui_qradarparamsetting.h"
#include "../profiles.h"
#include <QRegExp>
#include <QFileDialog>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

QRadarParamSetting::QRadarParamSetting(QWidget *parent) :
    QWidget(parent),
    mRadarID(-1),
    mStatusWidget(0),
    ui(new Ui::QRadarParamSetting)
{
    ui->setupUi(this);
    ui->radar_num_lineEdit->setText( Utils::Profiles::instance()->value("Radar_1","Radar_N").toString());
}

QRadarParamSetting::~QRadarParamSetting()
{
    delete ui;
}

void QRadarParamSetting::slotUpdateRealRangeFactor(double range, double factor)
{
    ui->realRangefactor->setText(tr("半径:%1  距离因子:%2").arg(range).arg(factor, 0, 'f', 2));
}
//设置4G还是6G雷达标志
void QRadarParamSetting::setRadarType(const QString& RadarType)
{
    ui->RadarTypeComboBox->setCurrentText(RadarType);
}
QString QRadarParamSetting::RadarType()
{
    return ui->RadarTypeComboBox->currentText();
}


void QRadarParamSetting::setTrackIP(const QString& ip)
{
    ui->trackRecIPLlineEdit->setText(ip);
}

QString QRadarParamSetting::trackIP()
{
    return ui->trackRecIPLlineEdit->text().trimmed();
}

void QRadarParamSetting::setTrackPort(unsigned int port)
{
    ui->trackRecPortSpinBox->setValue(port);
}

int QRadarParamSetting::trackPort()
{
    return ui->trackRecPortSpinBox->value();
}

void QRadarParamSetting::setTrackType(const QString& type)
{
    ui->trackTypeComboBox->setCurrentText(type);
}

QString QRadarParamSetting::trckType()
{
    return ui->trackTypeComboBox->currentText();
}

void QRadarParamSetting::setVideoIP(const QString& ip)
{
    ui->videoRecIPLlineEdit->setText(ip);
}

QString QRadarParamSetting::videoIP()
{
    return ui->videoRecIPLlineEdit->text().trimmed();
}

void QRadarParamSetting::setVideoPort(unsigned int port)
{
    ui->videoRecPortSpinBox->setValue(port);
}

int QRadarParamSetting::videoPort()
{
    return ui->videoRecPortSpinBox->value();
}

void QRadarParamSetting::setTcpPort(int prot)
{
    ui->tcpPort->setValue(prot);
}

int QRadarParamSetting::tcpPort()
{
    return ui->tcpPort->value();
}

void QRadarParamSetting::setVideoType(const QString& type)
{
    ui->videoTypeComboBox->setCurrentText(type);
}

QString QRadarParamSetting::videoType()
{
    //return QString("Lowrance");
    return ui->videoTypeComboBox->currentText();
}

void QRadarParamSetting::setHeartIP(const QString& ip)
{
    ui->heartIPLlineEdit->setText(ip);
}

QString QRadarParamSetting::heartIP()
{
    return ui->heartIPLlineEdit->text().trimmed();
}

void QRadarParamSetting::setHeartPort(unsigned int port)
{
    ui->heartPortSpinBox->setValue(port);
}

int QRadarParamSetting::heartPort()
{
    return ui->heartPortSpinBox->value();
}

void QRadarParamSetting::setHeartInterval(int interval)
{
    ui->heartTimeSpinBox->setValue(interval);
}

int QRadarParamSetting::heartInterval()
{
    return ui->heartTimeSpinBox->value();
}

void QRadarParamSetting::setCenterLat(double lat)
{
    //cout<<"setCenterLon lat"<<lat;
    ui->centreLatDSpinBox->setValue(lat);
}

double QRadarParamSetting::centerLat()
{
    return ui->centreLatDSpinBox->value();
}

void QRadarParamSetting::setCenterLon(double lon)
{
    //cout<<"setCenterLon lon"<<lon;
    ui->centreLonDSpinBox->setValue(lon);
}

double QRadarParamSetting::centerLon()
{
    return ui->centreLonDSpinBox->value();
}

void QRadarParamSetting::setDistance(double dis)
{
    ui->distanceDSpinBox->setValue(dis);
}

double QRadarParamSetting::distance()
{
    return ui->distanceDSpinBox->value();
}

void QRadarParamSetting::setLimit(bool limit)
{
    ui->limitCBox->setChecked(limit);
}

bool QRadarParamSetting::limit()
{
    return ui->limitCBox->isChecked();
}

void QRadarParamSetting::setLimit_zhou(bool limit)
{
    ui->limitCBox_2->setChecked(limit);
}

bool QRadarParamSetting::limit_zhou()
{
    return ui->limitCBox_2->isChecked();
}

void QRadarParamSetting::setLimit_gps(bool limit)
{
    ui->gps_checkBox->setChecked(limit);
}

bool QRadarParamSetting::limit_gps()
{
    return ui->gps_checkBox->isChecked();
}

void QRadarParamSetting::setClearTrackTime(double time)
{
    ui->trackClearTimeSpinBox->setValue(time);
}

double QRadarParamSetting::clearTrackTime()
{
    return ui->trackClearTimeSpinBox->value();
}

void QRadarParamSetting::setHeading(double head)
{
    ui->headingSpinBox->setValue(head);
}

double QRadarParamSetting::heading()
{
    return ui->headingSpinBox->value();
}

void QRadarParamSetting::setLoopNum(int num)
{
    ui->loopNumSpinBox->setValue(num);
}

int QRadarParamSetting::loopNum()
{
    return ui->loopNumSpinBox->value();
}
void QRadarParamSetting::setLineNum(int num)
{
    ui->lineNumSpinBox->setValue(num);
}

int QRadarParamSetting::lineNum()
{
    return ui->lineNumSpinBox->value();
}

void QRadarParamSetting::setCellNum(int num)
{
    ui->cellNumSpinBox->setValue(num);
}

int QRadarParamSetting::cellNum()
{
    return ui->cellNumSpinBox->value();
}

//void QRadarParamSetting::on_saveBtn_clicked()
//{
//    emit signalParamSave();
//}


void QRadarParamSetting::setRadarReportSeting(const QList<RadarStatus> &report)
{
    if(mStatusWidget)
    {
        mStatusWidget->setRadarReportSeting(report);
    }
}

//void QRadarParamSetting::slotValueChanged(int val)
//{
//    QSpinBox *box = qobject_cast<QSpinBox*> (sender());
//    if(!box) return;
//    QString name= box->objectName();
//    QRegExp num("[0-9]{1,}");
//    if(num.indexIn(name) >= 0)
//    {
//        int type = num.cap().toInt();
//        emit signalRadarConfigChanged(mRadarID, type , val);
//    }

//}

void QRadarParamSetting::setRadarID(int id)
{
    mRadarID = id;
}

int QRadarParamSetting::getRadarID()
{
    return mRadarID;
}

void QRadarParamSetting::setControlIP(const QString& ip)
{
    ui->controlIPLineEdit->setText(ip);
}

QString QRadarParamSetting::controlIP()
{
    return ui->controlIPLineEdit->text().trimmed();
}

void  QRadarParamSetting::setLimitFile(QString str)
{
    ui->limitfile_lineEdit->setText(str);
}

QString  QRadarParamSetting::limitFile()
{
    return ui->limitfile_lineEdit->text();
}

void QRadarParamSetting::setControlPort(unsigned int port)
{
    ui->controlPortSpinBox->setValue(port);
}

int QRadarParamSetting::controlPort()
{
    return ui->controlPortSpinBox->value();
}

void QRadarParamSetting::on_controlIPLineEdit_textChanged(const QString &arg1)
{

}

void QRadarParamSetting::on_controlPortSpinBox_valueChanged(const QString &arg1)
{

}

void QRadarParamSetting::setStatusWidget(QRadarStatusSettingWidget *w)
{
    mStatusWidget = w;
}

QRadarStatusSettingWidget* QRadarParamSetting::statusWidget()
{
    return mStatusWidget;
}

void QRadarParamSetting::setReportOpen(bool open)
{
    ui->statusCheckBox->setChecked(open);
}

bool QRadarParamSetting::reportOpen()
{
    return ui->statusCheckBox->isChecked();
}

void QRadarParamSetting::on_videoTypeComboBox_currentIndexChanged(const QString &arg1)
{
    emit signalVideoTypeChanged(arg1);
}

void QRadarParamSetting::on_radar_num_pushButton_clicked()
{
    if( ui->radar_num_lineEdit->text().toInt() != NULL )
    Utils::Profiles::instance()->setValue("Radar_1","Radar_N", ui->radar_num_lineEdit->text().toInt());
}

void QRadarParamSetting::on_pushButton_clicked()
{
    QString anterior_file_name = ui->limitfile_lineEdit->text();
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
        //anterior_file_name = Utils::Profiles::instance()->value("Radar_1","Limit_File").toString();
        ui->limitfile_lineEdit->setText(anterior_file_name);
    }
    else
    {
        ui->limitfile_lineEdit->setText(pathName);
    }

    //ui->pushButton->setText(file_name);
}

void QRadarParamSetting::on_range_factor_valueChanged(double arg1)
{
    emit signalRangeFactorChanged(arg1);
}

void QRadarParamSetting::slotGetGpsData(double lat, double lon)
{
    //cout<<"lat"<<lat<<"lon"<<lon;
    if(ui->gps_checkBox->isChecked())
    {
        ui->centreLatDSpinBox->setEnabled(0);
        ui->centreLonDSpinBox->setEnabled(0);
        setCenterLat(lat);
        setCenterLon(lon);
    }
    else
    {
        ui->centreLatDSpinBox->setEnabled(1);
        ui->centreLonDSpinBox->setEnabled(1);
    }
}

void QRadarParamSetting::on_gps_checkBox_clicked()
{
    if(ui->gps_checkBox->isChecked())
    {
        ui->centreLatDSpinBox->setEnabled(0);
        ui->centreLonDSpinBox->setEnabled(0);
    }
    else
    {
        ui->centreLatDSpinBox->setEnabled(1);
        ui->centreLonDSpinBox->setEnabled(1);
    }
}
