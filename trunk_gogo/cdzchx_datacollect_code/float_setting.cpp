#include "float_setting.h"
#include "ui_float_setting.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDebug>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
float_setting::float_setting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::float_setting)
{
    ui->setupUi(this);
    this->setWindowTitle("浮标设置");
    ui->table1->horizontalHeader()->setStretchLastSection(true);// 填充满
    ui->table1->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);//等宽
    ui->table1->setSelectionBehavior(QAbstractItemView::SelectRows);  //整行选中的方式
    ui->table1->setEditTriggers(QAbstractItemView::DoubleClicked);   //双击修改
    ui->table1->setSelectionMode(QAbstractItemView::SingleSelection);  //设置为可以选中单个
    ui->table1->verticalHeader()->setVisible(false);   //隐藏列表头
    ui->table1->setStyleSheet("selection-background-color:rgb(255, 193, 127);"); //设置选中行的背景色
    ui->table1->horizontalHeader()->setStyleSheet("QHeaderView::section{background-color:rgb(188, 220, 244);};");//设置表头颜色

    //遍历读取浮标配置值
    for(int i =0; PROFILES_INSTANCE->value("FloatSet",QString::number(i+1)).isNull() != 1; i++)
    {
        ui->table1->insertRow(i);
        QStringList mStrList = PROFILES_INSTANCE->value("FloatSet",QString::number(i+1)).toStringList();
        cout<<"mStrList";
        cout<<"浮标值"<<mStrList;
        ui->table1->setItem(i,0,new QTableWidgetItem(QString::number(i+1)));
        ui->table1->setItem(i,1,new QTableWidgetItem(mStrList.first()));
        ui->table1->setItem(i,2,new QTableWidgetItem(mStrList.back()));
    }
    //显示距离
    ui->lineEdit->setText(PROFILES_INSTANCE->value("FloatSet","range").toString());
}

float_setting::~float_setting()
{
    delete ui;
}
//添加
void float_setting::on_add_pushButton_clicked()
{
    int cols=ui->table1->columnCount();
    int rows=ui->table1->rowCount();
    qDebug()<<rows;
    ui->table1->insertRow(rows);
    for(int i=0;i<cols;i++)
    {
        ui->table1->setItem(rows,i,new QTableWidgetItem(QString::number(rows+1)));
    }
    ui->table1->selectRow(rows);//选中当前行
    //交替颜色填充行
    for(int i=0; i<rows; i++)
    {
        if(i%2 == 0)
        {
            for(int j=0; j<cols; j++)
            {
                QTableWidgetItem * item = ui->table1->item(i, j);
                item->setBackgroundColor(QColor(188, 220, 244));
            }
        }
    }

}
//删除
void float_setting::on_del_pushButton_clicked()
{
    QTableWidgetItem * item = ui->table1->currentItem();
    if(item==Q_NULLPTR)return;
    cout<<"删除的行号"<<item->row();
    ui->table1->removeRow(item->row());//删除
    int rows=ui->table1->rowCount();
    for(int i = 0; i < rows; i++)
    {
        ui->table1->item(i,0)->setText(QString::number(i+1));
    }
    //交替颜色填充行
    int cols=ui->table1->columnCount();
    for(int i=0; i<rows; i++)
    {
        if(i%2 == 0)
        {
            for(int j=0; j<cols; j++)
            {
                QTableWidgetItem * item = ui->table1->item(i, j);
                item->setBackgroundColor(QColor(188, 220, 244));
            }
        }
        else
        {
            for(int j=0; j<cols; j++)
            {
                QTableWidgetItem * item = ui->table1->item(i, j);
                item->setBackgroundColor(QColor(255, 255, 255));
            }
        }
    }
}
//保存
void float_setting::on_save_pushButton_clicked()
{
    PROFILES_INSTANCE->removeGroup("FloatSet");
    PROFILES_INSTANCE->setValue("FloatSet","range",ui->lineEdit->text());
    fMap.clear();
    int cols=ui->table1->columnCount();
    int rows=ui->table1->rowCount();
    for(int i = 0; i < rows; i++)
    {
        QStringList mList;
        for(int j = 1; j < cols; j++)
        {
           mList.append(ui->table1->item(i, j)->text());
        }
        fMap[i+1] = mList;
        cout<<"mList"<<mList.size()<<mList;
    }
    cout<<"fMap"<<fMap.size()<<fMap;
    int i = 1;
    foreach (QStringList uList, fMap) {
        PROFILES_INSTANCE->setValue("FloatSet",QString::number(i),uList);
        i++;
    }
    emit updateFloatSignal();
}
//修改
//void float_setting::on_mod_pushButton_clicked()
//{
//    emit updateFloatSignal();
//}

//void float_setting::on_sure_pushButton_clicked()
//{
//    PROFILES_INSTANCE->setValue("FloatSet","range",ui->lineEdit->text());
//    emit updateFloatSignal();
//}
