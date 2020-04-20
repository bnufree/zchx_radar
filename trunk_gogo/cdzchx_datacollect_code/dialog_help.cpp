#include "dialog_help.h"
#include "ui_dialog_help.h"
#include <QPixmap>

dialog_help::dialog_help(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::dialog_help)
{
    ui->setupUi(this);
    index = 0;
    ChangePicture();
    setWindowTitle("帮助");
}

dialog_help::~dialog_help()
{
    delete ui;
}

void dialog_help::ChangePicture()
{
    QPixmap pix(":/image/1.png");
    switch (index) {
    case 0:
        pix =  QPixmap(":/image/1.png");
        ui->label->setPixmap(pix);
        break;
    case 1:
        pix =  QPixmap(":/image/2.png");
        ui->label->setPixmap(pix);
        break;
    case 2:
        pix =  QPixmap(":/image/3.png");
        ui->label->setPixmap(pix);
        break;
    case 3:
        pix =  QPixmap(":/image/4.png");
        ui->label->setPixmap(pix);
        break;
    case 4:
        pix =  QPixmap(":/image/5.png");
        ui->label->setPixmap(pix);
        break;
    default:
        break;
    }
}

void dialog_help::on_next_pushButton_clicked()
{
    index++;
    if(index > 4)
    {
        index = 4;
        QMessageBox::information(0,QStringLiteral("提示"),QStringLiteral("已经是最后一页了"));
    }
    ChangePicture();
}

void dialog_help::on_front_pushButton_clicked()
{
    index--;
    if(index < 0)
    {
        index = 0;
        QMessageBox::information(0,QStringLiteral("提示"),QStringLiteral("已经是第一页了"));
    }
    ChangePicture();
}
