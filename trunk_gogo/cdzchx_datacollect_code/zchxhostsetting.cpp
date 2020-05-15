#include "zchxhostsetting.h"
#include "ui_zchxhostsetting.h"

zchxHostSetting::zchxHostSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::zchxHostSetting)
{
    ui->setupUi(this);
}

zchxHostSetting::~zchxHostSetting()
{
    delete ui;
}
