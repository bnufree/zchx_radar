#include "zchxradaroptwidget.h"
#include "ui_zchxradaroptwidget.h"
#include <QDebug>

zchxRadarOptWidget::zchxRadarOptWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::zchxRadarOptWidget)
{
    ui->setupUi(this);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    mIsAutoMode = false;
    mMin = 0;
    mMax = 1;
    mCur = 0;
    mAutoBtnVisible = false;
    setAdjustMode(MIDE_ADJUST_MODE);
}

zchxRadarOptWidget::~zchxRadarOptWidget()
{
    delete ui;
}

void zchxRadarOptWidget::setAdjustMode(int mode)
{
    mAdjusMode = mode;
    ui->btn_1000_plus->setVisible(false);
    ui->btn_100_plus->setVisible(false);
    ui->btn_1000_minus->setVisible(false);
    ui->btn_100_minus->setVisible(false);
    ui->btn_10_minus->setVisible(false);
    ui->btn_minus->setVisible(false);
    ui->btn_10_plus->setVisible(false);
    ui->btn_plus->setVisible(false);
    int row_num = 1;

    if((mAdjusMode & SMALL_ADJUST_MODE) || (mAdjusMode & MIDE_ADJUST_MODE))
    {
        ui->btn_plus->setVisible(true);
        ui->btn_minus->setVisible(true);
        row_num += 2;
    }
    if(mAdjusMode & MIDE_ADJUST_MODE)
    {
        ui->btn_10_minus->setVisible(true);
        ui->btn_10_plus->setVisible(true);
        row_num += 2;
    }

    if(mAdjusMode & LARGE_ADJUST_MODE)
    {
        ui->btn_1000_plus->setVisible(true);
        ui->btn_100_plus->setVisible(true);
        ui->btn_1000_minus->setVisible(true);
        ui->btn_100_minus->setVisible(true);
        row_num += 4;
    }

    int btn_height = ui->btn_1000_minus->height();

    setFixedHeight(btn_height * row_num + ui->verticalLayout->spacing() * (row_num - 1) + 5);

}



void zchxRadarOptWidget::on_btn_auto_clicked()
{
    setAuto(true);
    setServerValue(-1);
}

void zchxRadarOptWidget::on_btn_10_plus_clicked()
{
    setServerValue(mCur + 10);
}

void zchxRadarOptWidget::on_btn_plus_clicked()
{
    setServerValue(mCur + 1);
}

void zchxRadarOptWidget::on_btn_minus_clicked()
{
    setServerValue(mCur - 1);
}

void zchxRadarOptWidget::on_btn_10_minus_clicked()
{
    setServerValue(mCur - 10);
}

void zchxRadarOptWidget::setServerValue(int val)
{
    if(!mIsAutoMode)
    {
        if(val < mMin) val = mMin;
        if(val > mMax) val = mMax;
    }
    qDebug()<<"update server value:"<<val;
    signalConfigChanged(val);
}

void zchxRadarOptWidget::setAuto(bool sts)
{
    mIsAutoMode = sts;
    if(sts)
    {
        ui->btn_auto->setEnabled(false);
    }
}

void zchxRadarOptWidget::setCurrentVal(int val)
{
    if(val != -1 && mAutoBtnVisible)
    {
        ui->btn_auto->setEnabled(true);
        if(mIsAutoMode) mIsAutoMode = false;
    }
    mCur = val;
    if(mCur == -1 &&  mAutoBtnVisible)
    {
        ui->btn_auto->setEnabled(false);
        if(!mIsAutoMode) mIsAutoMode = true;
        mCur = 0;
    }
    qDebug()<<"current opt value:"<<mCur<< "server value:"<<val<<"auto btn sts:"<<mAutoBtnVisible;
}

void zchxRadarOptWidget::setAutoBtnAvailable(bool sts)
{
    ui->btn_auto->setVisible(sts);
    mAutoBtnVisible = sts;
}

void zchxRadarOptWidget::set10BtnAvailale(bool sts)
{
    ui->btn_10_minus->setVisible(sts);
    ui->btn_10_plus->setVisible(sts);
}

void zchxRadarOptWidget::on_btn_close_clicked()
{
    emit signalClose();
}

void zchxRadarOptWidget::setWidth(int w)
{
    setFixedWidth(w);
}

void zchxRadarOptWidget::on_btn_50_minus_clicked()
{
     setServerValue(mCur - 50);
}

void zchxRadarOptWidget::on_btn_100_minus_clicked()
{
     setServerValue(mCur - 100);
}

void zchxRadarOptWidget::on_btn_1000_minus_clicked()
{
     setServerValue(mCur - 1000);
}

void zchxRadarOptWidget::on_btn_50_plus_clicked()
{
     setServerValue(mCur + 50);
}

void zchxRadarOptWidget::on_btn_100_plus_clicked()
{
    setServerValue(mCur + 100);
}

void zchxRadarOptWidget::on_btn_1000_plus_clicked()
{
    setServerValue(mCur + 1000);
}
