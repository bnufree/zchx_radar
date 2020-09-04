#ifndef RADARDATAOUTPUTSETTINGS_H
#define RADARDATAOUTPUTSETTINGS_H

#include <QWidget>

namespace Ui {
class RadarDataOutputSettings;
}

class RadarDataOutputSettings : public QWidget
{
    Q_OBJECT

public:
    explicit RadarDataOutputSettings(QWidget *parent = 0);
    ~RadarDataOutputSettings();

private:
    Ui::RadarDataOutputSettings *ui;
};

#endif // RADARDATAOUTPUTSETTINGS_H
