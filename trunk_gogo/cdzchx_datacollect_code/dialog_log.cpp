#include "dialog_log.h"
#include "ui_dialog_log.h"
#include "dataserverutils.h"

Dialog_log::Dialog_log(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_log)
{
    ui->setupUi(this);
    this->setWindowTitle("数据日志");
}

Dialog_log::~Dialog_log()
{
    delete ui;
}

void Dialog_log::receiveLogSlot(qint64 time, const QString& name, const QString& content)
{
    ui->listWidget->insertItem(0, QString("%1---%2---%3").arg(DataServerUtils::time2String(time, true)).arg(name).arg(content));
    if(ui->listWidget->count() > 100)
    {
        QListWidgetItem* item = ui->listWidget->takeItem(99);
        delete item;
    }
}
