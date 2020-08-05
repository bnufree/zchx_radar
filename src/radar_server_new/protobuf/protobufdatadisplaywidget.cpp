#include "protobufdatadisplaywidget.h"
#include "ui_protobufdatadisplaywidget.h"
#include <QDateTime>

#define     DATETIME2STR(val)      (QDateTime::fromMSecsSinceEpoch(val).toString("yyyy-MM-dd hh:mm:ss"))
#define     STS2STR(val)    (val == true ? tr("启用") : tr("未启用"))

ProtobufDataDisplayWidget::ProtobufDataDisplayWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProtobufDataDisplayWidget)
{
    ui->setupUi(this);
}

ProtobufDataDisplayWidget::~ProtobufDataDisplayWidget()
{
    delete ui;
}

void ProtobufDataDisplayWidget::display(const DevInfo &info)
{
    ui->site_id->setText(info.site_id().data());
    ui->utc_time->setText(DATETIME2STR(info.cur_utc_time()));
    ui->gps_time->setText(DATETIME2STR(info.gps_info().ship_update_time()));
    ui->gps_sts->setText(STS2STR(info.gps_info().sts()));
    ui->gps_val->setText(QString("%1, %2, %3, %4, %5")\
                         .arg(info.gps_info().ship_lon(), 0, 'f', 6)\
                         .arg(info.gps_info().ship_lat(), 0, 'f', 6)\
                         .arg(info.gps_info().ship_speed(), 0, 'f', 2)\
                         .arg(info.gps_info().ship_head(), 0, 'f', 2)\
                         .arg(info.gps_info().ship_course(), 0, 'f', 2)\
                         );
    ui->zs_time->setText(DATETIME2STR(info.zs_info().time()));
    ui->zs_sts->setText(STS2STR(info.zs_info().sts()));
    ui->zs_val->setText(QString("%1, %2").arg(info.zs_info().zs(), 0, 'f', 2).arg(info.zs_info().temp(), 0, 'f', 2));

    ui->rdo_time->setText(DATETIME2STR(info.rdo_info().time()));
    ui->rdo_sts->setText(STS2STR(info.rdo_info().sts()));
    ui->rdo_val->setText(QString("%1, %2").arg(info.rdo_info().rdo(), 0, 'f', 2).arg(info.rdo_info().temp(), 0, 'f', 2));

    ui->ddm_time->setText(DATETIME2STR(info.ddm_info().time()));
    ui->ddm_sts->setText(STS2STR(info.ddm_info().sts()));
    ui->ddm_val->setText(QString("%1, %2").arg(info.ddm_info().ddm(), 0, 'f', 2).arg(info.ddm_info().temp(), 0, 'f', 2));

    ui->nhn_time->setText(DATETIME2STR(info.nhn_info().time()));
    ui->nhn_sts->setText(STS2STR(info.nhn_info().sts()));
    ui->nhn_val->setText(QString("%1, %2").arg(info.nhn_info().nhn(), 0, 'f', 2).arg(info.nhn_info().temp(), 0, 'f', 2));

    ui->wl_time->setText(DATETIME2STR(info.wl_info().time()));
    ui->wl_sts->setText(STS2STR(info.wl_info().sts()));
    ui->wl_val->setText(QString("%1, %2, %3")\
                        .arg(info.wl_info().press(), 0, 'f', 0)\
                        .arg(info.wl_info().temp(), 0, 'f', 2)\
                        .arg(info.wl_info().lvl(), 0, 'f', 0));

    ui->orp_time->setText(DATETIME2STR(info.orp_info().time()));
    ui->orp_sts->setText(STS2STR(info.orp_info().sts()));
    ui->orp_val->setText(QString("%1").arg(info.orp_info().orp(), 0, 'f', 2));

}
