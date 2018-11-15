#ifndef QRADARSTATUSSETTINGWIDGET_H
#define QRADARSTATUSSETTINGWIDGET_H

#include <QWidget>
#include "radarccontroldefines.h"

namespace Ui {
class QRadarStatusSettingWidget;
}

class QRadarStatusSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QRadarStatusSettingWidget(int radarID, QWidget *parent = 0);
    ~QRadarStatusSettingWidget();
    void setRadarReportSeting(const QList<RadarStatus> &report);
signals:
    void signalRadarConfigChanged(int radarID, int type, int value);
public slots:
    void slotValueChanged(int val);

private:
    Ui::QRadarStatusSettingWidget *ui;
    int mRadarID;
};

#endif // QRADARSTATUSSETTINGWIDGET_H
