#include "dialog_set.h"
#include "ui_dialog_set.h"

Dialog_set::Dialog_set(int id,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_set)
{
    ui->setupUi(this);
    this->setWindowTitle("数据配置");
    ui->settingWidget->setId(id);
    connect(ui->settingWidget,SIGNAL(signalRangeFactorChanged(double)),this,SIGNAL(signalRangeFactorChanged_1(double)));
    connect(ui->settingWidget, SIGNAL(set_change_signal()), this, SIGNAL(set_change_signal_1())); 
    connect(this, SIGNAL(signalGetGpsData(double, double)), ui->settingWidget, SLOT(slotGetGpsData(double,double)));

}

Dialog_set::~Dialog_set()
{
    delete ui;
}

void Dialog_set::slotUpdateRealRangeFactor(double range, double factor)
{
    ui->settingWidget->slotUpdateRealRangeFactor(range,factor);
    //ui->re_2->setText(tr("半径:%1  距离因子:%2").arg(range).arg(factor, 0, 'f', 2));
}
