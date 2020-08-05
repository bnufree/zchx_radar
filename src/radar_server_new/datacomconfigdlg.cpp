#include "datacomconfigdlg.h"
#include "ui_datacomconfigdlg.h"

DataComConfigDlg::DataComConfigDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataComConfigDlg)
{
    ui->setupUi(this);
}

DataComConfigDlg::~DataComConfigDlg()
{
    delete ui;
}

void DataComConfigDlg::on_cancelbtn_clicked()
{
    return reject();
}

void DataComConfigDlg::on_okbtn_clicked()
{
    return accept();
}

void DataComConfigDlg::setParam(const QString &type, const QString &com, const QString &braud, bool chk)
{
    ui->comtype->setText(type);
    ui->comname->setText(com);
    ui->combraud->setText(braud);
    ui->startchk->setCheckState(chk==true? Qt::Checked : Qt::Unchecked);
}

QString DataComConfigDlg::braud()
{
    return ui->combraud->text();
}

QString DataComConfigDlg::type()
{
    return ui->comtype->text();
}

QString DataComConfigDlg::com()
{
    return ui->comname->text();
}

bool DataComConfigDlg::check()
{
    return ui->startchk->isChecked();
}
