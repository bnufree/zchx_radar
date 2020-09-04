#include "radardataoutputsettings.h"
#include "ui_radardataoutputsettings.h"

RadarDataOutputSettings::RadarDataOutputSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RadarDataOutputSettings)
{
    ui->setupUi(this);
}

RadarDataOutputSettings::~RadarDataOutputSettings()
{
    delete ui;
}
