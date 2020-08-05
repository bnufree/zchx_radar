#ifndef ZCHXRADARAISSETTING_H
#define ZCHXRADARAISSETTING_H

#include <QWidget>
#include "radarccontroldefines.h"
#include "qradarparamsetting.h"

namespace Ui {
class ZCHXRadarAisSetting;
}

class ZCHXRadarAisSetting : public QWidget
{
    Q_OBJECT

public:
    explicit ZCHXRadarAisSetting(QWidget *parent = 0);
    ~ZCHXRadarAisSetting();

    void init();
private:
    int radarId;
signals:
    void signalRadarConfigChanged(int radarID, int type, int value);
    void signalRangeFactorChanged(double val);
    void set_change_signal();
    void signalUpdateRealRangeFactor(double, double);
    void signalGetGpsData(double, double);
public slots:
    void slotRecvRadarReportInfo(const QList<RadarStatus>& info, int radarID);
    void slotUpdateRealRangeFactor(double range, double factor);

    void slotGetGpsData(double lat, double lon);//从串口接入GPS经纬度
    void setId(int ID);

private slots:
    void on_radarNumComboBox_activated(int index);

    void on_saveBtn_clicked();

    void on_aisTypeBox_currentIndexChanged(int index);




private:
    Ui::ZCHXRadarAisSetting *ui;
};

#endif // ZCHXRADARAISSETTING_H
