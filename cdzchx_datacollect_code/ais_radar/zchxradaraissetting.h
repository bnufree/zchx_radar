#ifndef ZCHXRADARAISSETTING_H
#define ZCHXRADARAISSETTING_H

#include <QWidget>
#include "radarccontroldefines.h"

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

signals:
    void signalRadarConfigChanged(int radarID, int type, int value);
    void signalRangeFactorChanged(double val);
public slots:
    void slotRecvRadarReportInfo(const QList<RadarStatus>& info, int radarID);
    void slotUpdateRealRangeFactor(double range, double factor);

private slots:
    void on_radarNumComboBox_activated(int index);

    void on_saveBtn_clicked();

    void on_aisTypeBox_currentIndexChanged(int index);

    void on_limit_fileBox_currentIndexChanged(int index);

    void on_range_factor_valueChanged(double arg1);

private:
    Ui::ZCHXRadarAisSetting *ui;
};

#endif // ZCHXRADARAISSETTING_H
