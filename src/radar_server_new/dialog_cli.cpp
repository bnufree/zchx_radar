#include "dialog_cli.h"
#include "ui_dialog_cli.h"
#include <qdebug.h>
#include "dataserverutils.h"

class HqTableWidgetItem : public QTableWidgetItem
{
public:
    HqTableWidgetItem(const QString& text, Qt::AlignmentFlag flg = Qt::AlignCenter)
        :QTableWidgetItem(text)
    {
        setTextAlignment(flg);
    }

    ~HqTableWidgetItem()
    {

    }

};
Dialog_cli::Dialog_cli(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_cli)
{
    ui->setupUi(this);
    this->setWindowTitle("客户端信息");
    ui->tableWidget->horizontalHeader()->setDefaultSectionSize(120);//设置列的宽度
    //ui->tableWidget->setColumnWidth(1, 60);//第一列的宽度
    ui->tableWidget->setColumnWidth(2, 60);
    ui->tableWidget->setColumnWidth(3, 60);
    ui->tableWidget->setColumnWidth(4, 150);
    ui->tableWidget->setColumnWidth(5, 150);
    ui->tableWidget->setColumnHidden(0, true);
}

Dialog_cli::~Dialog_cli()
{
    delete ui;
}
void Dialog_cli::slotUpdateClientTable(const QString& ip, const QString& name, int port, int inout)
{
    QString sIpPort = ip+QString::number(port);
    if(inout == 1)
    {
        //客户端连上
        if(!mClientList.contains(sIpPort))
        {
            qDebug()<<"连接"<<sIpPort;
            mClientList.append(sIpPort);
            //添加到第一行
            ui->tableWidget->insertRow(0);
            ui->tableWidget->setItem(0, 0, new HqTableWidgetItem(name));
            ui->tableWidget->setItem(0, 1, new HqTableWidgetItem(ip));
            ui->tableWidget->setItem(0, 2, new HqTableWidgetItem(QString::number(port)));
            ui->tableWidget->setItem(0, 3, new HqTableWidgetItem(QStringLiteral("已连接")));
            ui->tableWidget->setItem(0, 4, new HqTableWidgetItem(DataServerUtils::currentTimeString()));
            ui->tableWidget->setItem(0, 5, new HqTableWidgetItem("-"));
        }
    }
    else
    {
        //客户端离开
        if(mClientList.contains(sIpPort))
        {
            mClientList.removeOne(sIpPort);
            //从表格删除
            qDebug()<<"断开"<<sIpPort;
            for(int i=0; i<ui->tableWidget->rowCount(); i++)
            {
                QTableWidgetItem *item = ui->tableWidget->item(i, 1);
                QTableWidgetItem *portItem = ui->tableWidget->item(i, 2);

                if(item && portItem)
                {
                    QString str = item->text()+portItem->text();
                    if(sIpPort == str)
                    {
                        ui->tableWidget->item(i,3)->setText(QStringLiteral("已断开"));
                        ui->tableWidget->item(i,5)->setText(DataServerUtils::currentTimeString());
                    }
                }
            }
        }
    }
}
