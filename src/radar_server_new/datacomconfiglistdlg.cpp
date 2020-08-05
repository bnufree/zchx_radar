#include "datacomconfiglistdlg.h"
#include "ui_datacomconfiglistdlg.h"
#include "profiles.h"
#include "datacomconfigdlg.h"
#include <QMessageBox>
#include <QDebug>

DataComConfigListDlg::DataComConfigListDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataComConfigListDlg)
{
    ui->setupUi(this);
    //初始化列表
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);  //单击选择一行
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection); //设置只能选择一行，不能多行选中
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);   //设置每行内容不可更改
    ui->tableWidget->setAlternatingRowColors(true);
    init();
}

DataComConfigListDlg::~DataComConfigListDlg()
{
    delete ui;
}

void DataComConfigListDlg::on_addbtn_clicked()
{
    //初始化修改
    DataComConfigDlg dlg;
    if(dlg.exec() == QDialog::Accepted)
    {
        //开始更新
        QStringList res;
        res<<dlg.com()<<dlg.braud()<<QString::number(dlg.check());
        Utils::Profiles::instance()->setValue("COM", dlg.type(), res);
        init();
        ui->tableWidget->setCurrentItem(0);
    } else
    {
        ui->tableWidget->setCurrentItem(0);
    }
    qDebug()<<"cur row:"<<ui->tableWidget->currentRow();

}

void DataComConfigListDlg::on_delbtn_clicked()
{
    int row = ui->tableWidget->currentRow();
    if(row == -1)
    {
        QMessageBox::information(0, "delete configuration", "pls select an item first", QMessageBox::Ok);
        return;
    }
    int ret = QMessageBox::warning(0, "delete configuration", "delete now", QMessageBox::Ok, QMessageBox::Cancel);
    qDebug()<<"ret = "<<ret;
    if(ret = QMessageBox::Ok)
    {
        //删除
        Utils::Profiles::instance()->removeKeys("COM", QStringList(ui->tableWidget->item(row,0)->text()));
        ui->tableWidget->removeRow(row);
        ui->tableWidget->setCurrentItem(0);
    }

}

void DataComConfigListDlg::init()
{
    while (ui->tableWidget->rowCount()) {
        ui->tableWidget->removeRow(0);
    }
    QStringList keys = Utils::Profiles::instance()->subkeys("COM");
    foreach (QString key, keys) {
        QStringList valist = Utils::Profiles::instance()->value("COM", key).toStringList();
        if(valist.length() >= 3)
        {
            int k = 0;
            ui->tableWidget->insertRow(0);
            ui->tableWidget->setItem(0, k++, new MyTableWidgetItem(key));
            ui->tableWidget->setItem(0, k++, new MyTableWidgetItem(valist[0]));
            ui->tableWidget->setItem(0, k++, new MyTableWidgetItem(valist[1]));
            ui->tableWidget->setItem(0, k++, new MyTableWidgetItem(valist[2].toInt() != 0 ? "Yes" : "No"));
            ui->tableWidget->item(0, 0)->setData(Qt::UserRole, valist);
        }
    }
}

void DataComConfigListDlg::on_modbtn_clicked()
{
    int row = ui->tableWidget->currentRow();
    if(row == -1)
    {
        QMessageBox::information(0, "modify configuration", "pls select an item first", QMessageBox::Ok);
        return;
    }
    //初始化修改
    QString type = ui->tableWidget->item(row, 0)->text();
    QStringList list = ui->tableWidget->item(row, 0)->data(Qt::UserRole).toStringList();
    QString name = list[0];
    QString braud = list[1];
    bool chk =list[2].toInt();
    DataComConfigDlg dlg;
    dlg.setParam(type, name, braud, chk);
    if(dlg.exec() == QDialog::Accepted)
    {
        //开始更新
        Utils::Profiles::instance()->removeKeys("COM", QStringList(type));
        QStringList res;
        res<<dlg.com()<<dlg.braud()<<QString::number(dlg.check());
        Utils::Profiles::instance()->setValue("COM", dlg.type(), res);
        init();
        ui->tableWidget->setCurrentItem(0);
    } else
    {
        ui->tableWidget->setCurrentItem(0);
    }
    qDebug()<<"cur row:"<<ui->tableWidget->currentRow();

}
