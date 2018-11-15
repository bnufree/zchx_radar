#ifndef QRADARPARAMSETTING_H
#define QRADARPARAMSETTING_H

#include <QWidget>
#include "radarccontroldefines.h"
#include "qradarstatussettingwidget.h"

namespace Ui {
class QRadarParamSetting;
}


class QRadarParamSetting : public QWidget
{
    Q_OBJECT

public:
    explicit QRadarParamSetting(QWidget *parent = 0);
    ~QRadarParamSetting();
    void setTrackIP(const QString& ip);
    QString trackIP();
    void setTrackPort(unsigned int port);
    int trackPort();
    void setTrackType(const QString& type);
    QString trckType();
    void setVideoIP(const QString& ip);
    QString videoIP();
    void setVideoPort(unsigned int port);
    int videoPort();
    void setVideoType(const QString& type);
    QString videoType();
    void setHeartIP(const QString& ip);
    QString heartIP();
    void setHeartPort(unsigned int port);
    int heartPort();
    void setHeartInterval(int interval);
    int heartInterval();
    void setCenterLat(double lat);
    double centerLat();
    void setCenterLon(double lon);
    double centerLon();
    void setDistance(double dis);
    double distance();
    void setLimit(bool limit);
    bool limit();
    void setClearTrackTime(double time);
    double clearTrackTime();
    void setHeading(double head);
    double heading();
    void setLoopNum(int num);
    int loopNum();
    void setLineNum(int num);
    int lineNum();
    void setCellNum(int num);
    int cellNum();
    void setRadarReportSeting(const QList<RadarStatus>& report);
    void setRadarID(int id);
    int  getRadarID();
    void setControlIP(const QString& ip);
    QString controlIP();
    void setControlPort(unsigned int port);
    int controlPort();
    void setStatusWidget(QRadarStatusSettingWidget* w);
    QRadarStatusSettingWidget* statusWidget();
    void setReportOpen(bool open);
    bool reportOpen();

signals:
    void signalParamSave();
    void signalRadarConfigChanged(int radarID, int type, int value);

private slots:
    void on_saveBtn_clicked();
    //void slotValueChanged(int val);

    void on_controlIPLineEdit_textChanged(const QString &arg1);

    void on_controlPortSpinBox_valueChanged(const QString &arg1);

private:
    Ui::QRadarParamSetting *ui;
    int mRadarID;
    QRadarStatusSettingWidget*  mStatusWidget;
};

#endif // QRADARPARAMSETTING_H
