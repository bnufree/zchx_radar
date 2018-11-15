#include "qradarparamsetting.h"
#include "ui_qradarparamsetting.h"
#include <QRegExp>

QRadarParamSetting::QRadarParamSetting(QWidget *parent) :
    QWidget(parent),
    mRadarID(-1),
    mStatusWidget(0),
    ui(new Ui::QRadarParamSetting)
{
    ui->setupUi(this);
}

QRadarParamSetting::~QRadarParamSetting()
{
    delete ui;
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
    ui->centreLatDSpinBox->setValue(lat);
}

double QRadarParamSetting::centerLat()
{
    return ui->centreLatDSpinBox->value();
}

void QRadarParamSetting::setCenterLon(double lon)
{
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

void QRadarParamSetting::on_saveBtn_clicked()
{
    emit signalParamSave();
}


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
