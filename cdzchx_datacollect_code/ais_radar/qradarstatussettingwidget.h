#ifndef QRADARSTATUSSETTINGWIDGET_H
#define QRADARSTATUSSETTINGWIDGET_H

#include <QWidget>
#include "radarccontroldefines.h"
#include <QMap>

namespace Ui {
class QRadarStatusSettingWidget;
}

class QSpinBox;

class QRadarStatusSettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QRadarStatusSettingWidget(int radarID, QWidget *parent = 0);
    ~QRadarStatusSettingWidget();
    void setRadarReportSeting(const QList<RadarStatus> &report);
signals:
    void signalRadarConfigChanged(int type, int value);
public slots:
    void slotValueChanged(int val);
    void slotValueChangedFromServer(int type, int value);

private:
    Ui::QRadarStatusSettingWidget *ui;
    int mRadarID;
    QMap<int, QSpinBox*>     mValueSpinBoxMap;
};

#endif // QRADARSTATUSSETTINGWIDGET_H
