#include "ais_setting.h"
#include "ui_ais_setting.h"
#include <QTabWidget>

#include <QPainter>
#include "scrollarea.h"
#include <QMouseEvent>
#include <QScrollBar>
#include <drawaistrack.h>
#include <QMessageBox>
#include <QDebug>
#include "ais/zchxaisdataclient.h"
#include "ais/zchxaisdataserver.h"
#include "ais/zchxaisdataprocessor.h"
#include "fusedatautil.h"


#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

#define     AIS_SEC_PREFIX          "Ais"
#define     AIS_SEND_FREQUENCY      "Ais_Send_Frequency"
#define     AIS_LIMIT_AREA          "Ais_Limit"
#define     AIS_BEIDOU_CONVERT      "Beidou_Limit"
#define     AIS_SEND_PORT           "Ais_Send_Port"
#define     AIS_SEND_TOPIC          "Ais_Topic"
#define     AIS_IS_SERVER           "IsServer"
#define     AIS_LISTEN_PORT         "Server_Port"
#define     AIS_HOST                "IP"
#define     AIS_HOST_PORT           "Port"
#define     AIS_CONNECT_TIMEOUT     "TimeOut"
#define     AIS_DEVICE_NUM          "AIS_Num"
#define     AIS_DEVICE_NAME         "Name"
#define     AIS_DEVICE_LAT          "Lat"
#define     AIS_DEVICE_LON          "Lon"
#define     AIS_CHART_TOPIC         "Chart"
#define     AIS_RADIUS_FIXED        "RadiusFixed"
#define     AIS_RADIUS              "Radius"


AIS_Setting::AIS_Setting(int uSourceID,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AIS_Setting),
    mAisCollector(0),
    mAisParser(0),
    mAisFakeThread(0)
{
    ui->setupUi(this);

    //AIS配置初始化
    //ais
    str_ais = QString("%1_%2").arg(AIS_SEC_PREFIX).arg(uSourceID);
    Utils::Profiles::instance()->setDefault(str_ais, AIS_CHART_TOPIC, "Chart");
    Utils::Profiles::instance()->setDefault(str_ais, AIS_RADIUS_FIXED, true);
    Utils::Profiles::instance()->setDefault(str_ais, AIS_RADIUS, "100");
    Utils::Profiles::instance()->setDefault(str_ais, AIS_DEVICE_NAME, "AisStation");

    int frequency =  Utils::Profiles::instance()->value(str_ais, AIS_SEND_FREQUENCY, 10).toInt();
    if (frequency < 10) frequency = 10;
    ui->frequency_lineEdit->setText(QString::number(frequency));

    bool ais_limit = Utils::Profiles::instance()->value(str_ais, AIS_LIMIT_AREA).toBool();
    ui->ais_limite_chk->setChecked(ais_limit);

    bool ais_2_beidou = Utils::Profiles::instance()->value(str_ais, AIS_BEIDOU_CONVERT).toBool();
    ui->ais_2_beidou_chk->setChecked(ais_2_beidou);

    int uAISSendPort = Utils::Profiles::instance()->value(str_ais, AIS_SEND_PORT).toInt();
    QString sAISTopic = Utils::Profiles::instance()->value(str_ais, AIS_SEND_TOPIC).toString();

    bool bServer = Utils::Profiles::instance()->value(str_ais, AIS_IS_SERVER, false).toBool();
    int uServerPort = Utils::Profiles::instance()->value(str_ais, AIS_LISTEN_PORT).toInt();

    QString sIP = Utils::Profiles::instance()->value(str_ais, AIS_HOST).toString();
    int uPort = Utils::Profiles::instance()->value(str_ais, AIS_HOST_PORT).toInt();

    Utils::Profiles::instance()->setDefault(str_ais, AIS_CONNECT_TIMEOUT, 1);
    ui->reconnect_time_edit->setValue(Utils::Profiles::instance()->value(str_ais, AIS_CONNECT_TIMEOUT, 1).toInt());

    ui->ais_device_num->setValue(Utils::Profiles::instance()->value(str_ais, AIS_DEVICE_NUM).toInt());
    int ais_type_index = 0;
    if(bServer) ais_type_index = 1;
    ui->aisTypeBox->setCurrentIndex(ais_type_index);
    ui->listen_port_edit->setText(QString::number(uServerPort));
    ui->host_edit->setText(sIP);
    ui->ais_dev_port_edit->setText(QString::number(uPort));
    on_aisTypeBox_currentIndexChanged(ais_type_index);

    //设定目标的信息名称和经纬度信息
    QString ais_station_name = Utils::Profiles::instance()->value(str_ais, AIS_DEVICE_NAME).toString();
    ui->ais_dev_name->setText(ais_station_name);
    QString ais_lat = Utils::Profiles::instance()->value(str_ais, AIS_DEVICE_LAT).toString();
    QString ais_lon = Utils::Profiles::instance()->value(str_ais, AIS_DEVICE_LON).toString();
    QString chart_topic = Utils::Profiles::instance()->value(str_ais, AIS_CHART_TOPIC).toString();
    ui->ais_dev_lat->setText(ais_lat);
    ui->ais_dev_lon->setText(ais_lon);
    ui->ais_radius_fixed_chk->setChecked(Utils::Profiles::instance()->value(str_ais, AIS_RADIUS_FIXED).toBool());
    ui->fixed_ais_radius->setText(Utils::Profiles::instance()->value(str_ais, AIS_RADIUS).toString());



    ui->aisSendPortSpinBox->setValue(uAISSendPort);
    ui->aisSendTopicLineEdit->setText(sAISTopic);
    ui->chart_topic->setText(chart_topic);
    //初始化经纬度坐标系
    ui->zsjdlineEdit->setText(Utils::Profiles::instance()->value("Ais_Latlon", "LeftLon").toString());
    ui->zswdlineEdit->setText(Utils::Profiles::instance()->value("Ais_Latlon", "LeftLat").toString());
    ui->yxjdlineEdit->setText(Utils::Profiles::instance()->value("Ais_Latlon", "RightLon").toString());
    ui->yxwdlineEdit->setText(Utils::Profiles::instance()->value("Ais_Latlon", "RightLat").toString());
    //自定义数据表格
    ui->table1->setSelectionBehavior(QAbstractItemView::SelectRows);  //整行选中的方式
    ui->table1->setEditTriggers(QAbstractItemView::DoubleClicked);   //双击修改
    ui->table1->setSelectionMode(QAbstractItemView::SingleSelection);  //设置为可以选中单个
    ui->table1->verticalHeader()->setVisible(false);   //隐藏列表头
    ui->table1->setStyleSheet("selection-background-color:rgb(255, 193, 127);"); //设置选中行的背景色
    ui->table1->horizontalHeader()->setStyleSheet("QHeaderView::section{background-color:rgb(188, 220, 244);};");//设置表头颜色
    //轨迹参数表格
    ui->table2->setSelectionBehavior(QAbstractItemView::SelectRows);  //整行选中的方式
    ui->table2->setEditTriggers(QAbstractItemView::DoubleClicked);   //双击修改
    ui->table2->setSelectionMode(QAbstractItemView::SingleSelection);  //设置为可以选中单个
    ui->table2->verticalHeader()->setVisible(false);   //隐藏列表头
    ui->table2->setStyleSheet("selection-background-color:rgb(255, 193, 127);"); //设置选中行的背景色
    ui->table2->horizontalHeader()->setStyleSheet("QHeaderView::section{background-color:rgb(188, 220, 244);};");//设置表头颜色
    //绘制轨迹图
    image = QPixmap(850, 600);
    //drawTrackPixmap();
    //线程画AIS目标图
    //mDrawAisTrack = new drawaistrack;
    //mDrawAisTrack->signalGetPixmap(image, xLevel, yLevel);
    //connect(mDrawAisTrack, SIGNAL(signalSendAisPix(QPixmap)), this, SLOT(slotShowAisPix(QPixmap)));
    //和aislabel交互
    connect(this, SIGNAL(signalCreatBtnClicked()), ui->track_label, SLOT(slotCreatBtnClicked()));
    connect(ui->track_label, SIGNAL(signalAisPolygon(QPolygonF)), this, SLOT(slotDealAisPolygon(QPolygonF)));
    connect(this, SIGNAL(signalRedrawAisTrack(QMap<int,QPolygonF>)), ui->track_label, SLOT(slotRedrawAisTrack(QMap<int,QPolygonF>)));
    //隐藏4个按钮
    ui->add1_pbt->hide();
    ui->add2_pbt->hide();
    ui->del1_pbt->hide();
    ui->del2_pbt->hide();
    //初始化发送AIS模拟数据的定时器
    mWorkTimer.setInterval(frequency);
    connect(&mWorkTimer, SIGNAL(timeout()), this, SLOT(slotSendAisData()));
    mTabTimer.setInterval(1000);
    connect(&mTabTimer, SIGNAL(timeout()), this, SLOT(slotMousePress()));
    mTabTimer.start();
    //初始化轨迹编号及个数
    mNumber = 0;
    mCount = 0;
    mAisCount = 0;
    mMsi = 0;
    //初始化经纬度标签显示
    QString jwd="经纬度 "+ui->zsjdlineEdit->text()+","+ui->zswdlineEdit->text();
    ui->lu_label->setText(jwd);
    jwd = "经纬度 "+ui->yxjdlineEdit->text()+","+ui->yxwdlineEdit->text();
    ui->rd_label->setText(jwd);
    mBeidou = new beidouData(uSourceID);
    mBeidou->starProcess();

    //创建界面对应的ais数据接收器
    initCollector();

    //创建解析器
    initParser();

    //创建解析关联
    if(mAisCollector)
    {
        connect(mAisCollector, SIGNAL(signalSendAisData(QByteArray)), mAisParser, SLOT(slotProcessAisData(QByteArray)));
    }

    //显示北斗数据
    connect(mAisParser,SIGNAL(signalAisToBeidou(double,double)),this,SLOT(aisTobeidou(double,double)));
    //显示AIS目标
    connect(mAisParser, SIGNAL(signalAisTrackData(int,double,double)), this, SLOT(slotDrawAisTrack(int,double,double)));
    //输出AIS自定义模拟数据
    connect(this, SIGNAL(signalSendAislist(ITF_AISList)), mAisParser, SLOT(slotSendAis(ITF_AISList)));

    //融合AIS目标
    connect(mAisParser, SIGNAL(signalSendAisData(ITF_AISList)),
            FuseDataUtil::getInstance(), SLOT(slotReceiveAisData(ITF_AISList)));
    connect(FuseDataUtil::getInstance(), SIGNAL(sendAisData(ITF_AISList)),
            mAisParser, SLOT(sendAisAfterMerge(ITF_AISList)));


}

void AIS_Setting::initCollector()
{
    if(ui->aisTypeBox->currentIndex() == 0)
    {
        //客户端模式
        QString host = ui->host_edit->text().trimmed();
        int port = ui->ais_dev_port_edit->text().toInt();
        int time_out_secs = ui->reconnect_time_edit->value() * 60;
        if(time_out_secs > 0 && port > 0 && !host.isEmpty())
        {
            mAisCollector = new zchxAisDataClient(host, port, time_out_secs, this);
        }
    } else
    {
        //服务端模式
        int port = ui->listen_port_edit->text().toInt();
        if(port > 0)
        {
            mAisCollector = new zchxAisDataServer(port);
        }
    }

}

void AIS_Setting::initParser()
{
    zchxAisDataProcessParam param;
    param.mAis2Beidou = ui->ais_2_beidou_chk->isChecked();
    param.mAisDataTopic = ui->aisSendTopicLineEdit->text().trimmed();
    param.mAisLimitChk = ui->ais_limite_chk->isChecked();

    QPointF start(ui->zsjdlineEdit->text().toDouble(), ui->yxwdlineEdit->text().toDouble());
    QPointF end(ui->yxjdlineEdit->text().toDouble(), ui->zswdlineEdit->text().toDouble());
    param.mAisLimitRect.setBottomLeft(start);
    param.mAisLimitRect.setTopRight(end);
    if(!(ui->ais_dev_lat->text().trimmed().isEmpty() || ui->ais_dev_lon->text().trimmed().isEmpty()))
    {
        param.mCenterLat = ui->ais_dev_lat->text().toDouble();
        param.mCenterLon = ui->ais_dev_lon->text().toDouble();
        param.mIsCenterInit = true;
    }
    param.mChartTopic = ui->chart_topic->text().trimmed();
    param.mDataSendPort = ui->aisSendPortSpinBox->value();
    param.mStationId = ui->ais_device_num->value();
    param.mStationName = ui->ais_dev_name->text().trimmed();
    param.mAisFixRadius = ui->ais_radius_fixed_chk->isChecked();
    param.mRadius = qRound(ui->fixed_ais_radius->text().trimmed().toDouble()) * 1000;

    mAisParser = new zchxAisDataProcessor(param);
}

AIS_Setting::~AIS_Setting()
{
    delete ui;
    delete mDrawAisTrack;
}
//1添加,删除表格记录
void AIS_Setting::on_add1_pbt_clicked()
{
    int cols=ui->table1->columnCount();
    int rows=ui->table1->rowCount();
    qDebug()<<rows;
    ui->table1->insertRow(rows);
    for(int i=0;i<cols;i++)
    {
        ui->table1->setItem(rows,i,new QTableWidgetItem("data"+QString::number(rows)));
    }
    ui->table1->selectRow(rows);
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
//2
void AIS_Setting::on_del1_pbt_clicked()
{
    QTableWidgetItem * item = ui->table1->currentItem();
        if(item==Q_NULLPTR)return;
        ui->table1->removeRow(item->row());
}
//3
void AIS_Setting::on_add2_pbt_clicked()
{
    int cols=ui->table2->columnCount();
    int rows=ui->table2->rowCount();
    qDebug()<<rows;
    ui->table2->insertRow(rows);
    for(int i=0;i<cols;i++)
    {
        ui->table2->setItem(rows,i,new QTableWidgetItem("param"+QString::number(rows)));
    }
    ui->table2->selectRow(rows);
    //交替颜色填充行
    for(int i=0; i<rows; i++)
    {
        if(i%2 == 0)
        {
            for(int j=0; j<cols; j++)
            {
                QTableWidgetItem * item = ui->table2->item(i, j);
                item->setBackgroundColor(QColor(188, 220, 244));
            }
        }
    }
}
//4
void AIS_Setting::on_del2_pbt_clicked()
{
    QTableWidgetItem * item = ui->table2->currentItem();
        if(item==Q_NULLPTR)return;
        ui->table2->removeRow(item->row());
}

//--保存AIS接入,输出配置
void AIS_Setting::on_save_pushbotton_clicked()
{
    //ais save
    int uAisIndex = ui->aisTypeBox->currentIndex();
    bool isServer = (uAisIndex == 1 ? true : false);
    Utils::Profiles::instance()->setValue(str_ais, AIS_LISTEN_PORT, ui->listen_port_edit->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_IS_SERVER, isServer);
    Utils::Profiles::instance()->setValue(str_ais, AIS_HOST, ui->host_edit->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_HOST_PORT, ui->ais_dev_port_edit->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_CONNECT_TIMEOUT, ui->reconnect_time_edit->value());
    Utils::Profiles::instance()->setValue(str_ais,AIS_SEND_PORT, ui->aisSendPortSpinBox->value());
    Utils::Profiles::instance()->setValue(str_ais,AIS_SEND_TOPIC, ui->aisSendTopicLineEdit->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_DEVICE_NUM, ui->ais_device_num->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_DEVICE_NAME, ui->ais_dev_name->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_DEVICE_LAT, ui->ais_dev_lat->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_DEVICE_LON, ui->ais_dev_lon->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_CHART_TOPIC, ui->chart_topic->text());
    Utils::Profiles::instance()->setValue(str_ais, AIS_RADIUS_FIXED, ui->ais_radius_fixed_chk->isChecked());
    Utils::Profiles::instance()->setValue(str_ais, AIS_RADIUS, ui->fixed_ais_radius->text().trimmed());


    //重新生成AIS对象信号
    cout<<"set_change_signal信号";
    int ret1 = QMessageBox::information(0,QStringLiteral("信息"),QStringLiteral("配置修改成功"));
    cout<<"ret1"<<ret1;
    if(ret1 == 1024)
        emit newAisClassSignal();
}
//--建立坐标初始化
void AIS_Setting::drawTrackPixmap()
{
    image.fill(Qt::gray);
    QPainter painter(&image);// 画的设备
    QPen pen;
    pen.setWidth(2); //设置宽度
    pen.setBrush(Qt::black); //设置颜色
    painter.setPen(pen); //选中画笔
    painter.drawRect(0, 0,ui->track_label->width(),ui->track_label->height());
    int i;
    for(i = 0;i<ui->track_label->width();i+=50)//x 轴
    {
         painter.drawLine(i+50,ui->track_label->height(),i+50,0); //绘制x轴上的点
         painter.drawText(i,ui->track_label->height()-2,QString::number(i)); //绘制文本

    }
    for(i=0;i<ui->track_label->height();i+=50)//y 轴
    {
         painter.drawLine(0,i,ui->track_label->width(),i); //绘制y轴上的点
         painter.drawText(0,i,QString::number(ui->track_label->height()-i)); //绘制文本
    }
    //画图
    if(&image == NULL)
        return;
    QPixmap pixmap_1 = image.scaled(ui->track_label->width(), ui->track_label->height(), Qt::KeepAspectRatio);
    ui->track_label->setPixmap(pixmap_1);

}
//--保存坐标系配置
void AIS_Setting::on_save_pushbotton_2_clicked()
{
    double mLuLon = ui->zsjdlineEdit->text().toDouble();
    double mLuLat = ui->zswdlineEdit->text().toDouble();
    double mRdLon = ui->yxjdlineEdit->text().toDouble();
    double mRdLat = ui->yxwdlineEdit->text().toDouble();
    Utils::Profiles::instance()->setValue("Ais_Latlon", "LeftLon", mLuLon);
    Utils::Profiles::instance()->setValue("Ais_Latlon", "LeftLat", mLuLat);
    Utils::Profiles::instance()->setValue("Ais_Latlon", "RightLon", mRdLon);
    Utils::Profiles::instance()->setValue("Ais_Latlon", "RightLat", mRdLat);
    //显示经纬度在坐标系
    QString jwd="经纬度 "+ui->zsjdlineEdit->text()+","+ui->zswdlineEdit->text();
    ui->lu_label->setText(jwd);
    jwd = "经纬度 "+ui->yxjdlineEdit->text()+","+ui->yxwdlineEdit->text();
    ui->rd_label->setText(jwd);

}
//--画实时动态AIS轨迹图
void AIS_Setting::slotDrawAisTrack(int a, double lon, double lat)
{
    //cout<<"开始绘图";
    //mDrawAisTrack->signalDrawAisTrack(a, lon, lat);

}
//--重新鼠标滚轮事件
void AIS_Setting::wheelEvent(QWheelEvent *event)
{
//    if(event->delta()>0)
//    {//如果滚轮往上滚
//       aLevel++;
//       cout<<"放大信号";
//    }
//    else
//    {//如果滚轮往下滚
//        if(aLevel > 1)
//        {
//            aLevel--;
//        }
//        cout<<"缩小信号";
//    }
//    //ui->track_label->clear();
//    xLevel = 850 * aLevel;
//    yLevel = 600 * aLevel;
//    ui->scale_spinBox->setValue(aLevel);
//    image = QPixmap(850 * aLevel, 600 * aLevel);
//    ui->track_label->resize(850 * aLevel, 600 * aLevel);
    //mDrawAisTrack->signalGetPixmap(image, xLevel, yLevel);
}
//--显示图片
void AIS_Setting::slotShowAisPix(QPixmap pix1)
{
    ui->track_label->setPixmap(pix1);
}
//--生成数据按钮按下
void AIS_Setting::on_creatl_pushbotton_clicked()
{
    mMsi = 0;
    foreach (int k, mAisMap.keys()) {
        if(k == mMsi)
        {
            cout<<"keys"<<k;
            mMsi++;
        }
        else
        {
            break;
        }
    }
    cout<<"最新mMsi"<<mMsi<<mAisMap.keys();
    emit signalCreatBtnClicked();

}
//--解析模拟数据
void AIS_Setting::slotDealAisPolygon(QPolygonF poy)
{
    double mLuLon = Utils::Profiles::instance()->value("Ais_Latlon", "LeftLon").toDouble();
    double mLuLat = Utils::Profiles::instance()->value("Ais_Latlon", "LeftLat").toDouble();
    double mRdLon = Utils::Profiles::instance()->value("Ais_Latlon", "RightLon").toDouble();
    double mRdLat = Utils::Profiles::instance()->value("Ais_Latlon", "RightLat").toDouble();
    LatLong startLatLong(mLuLon,mLuLat);
    double posx = 0, posy = 0;
    getDxDy(startLatLong, mRdLat, mRdLon, posx, posy);
    QPointF pos_1(posy,-posx);
    double kx = pos_1.x() / xLevel;
    double ky = pos_1.y() / yLevel;

    QPolygonF yel_pot_2;
    foreach (QPointF p, poy) {
        //平面坐标算经纬度
        double  posx=  -( p.y() * ky);
        double  posy= p.x() * kx;
        double  dLat = 0, dLon = 0;
        getNewLatLong(startLatLong, dLat, dLon, posx, posy);
        //cout<<"dLat, dLon, posx, posy"<<dLat<<dLon<<posx<<posy;
        QPointF p1;
        p1.setX(dLat);
        p1.setY(dLon);
        yel_pot_2 << p1;
        cout<<"坐标"<<p<<"经纬度"<<p1;
    }
    mPolyMap[mMsi] = yel_pot_2;//将每次的经纬度坐标记录下来
    mPoyMap[mMsi] = poy;//将每次的平面坐标记录下来

//    foreach (int k, mPolyMap.keys()) {
//    }
    //cout<<"mPolyMap[mMsi].size()"<<mPolyMap[mMsi].size();
    //analysisAisTrackData(mMsi,poy);
    int rows2=ui->table2->rowCount();
    //qDebug()<<rows2;
    for(int i=0;i<rows2;i++)
    {
        ui->table2->removeRow(0);
    }
    if(mPolyMap[mMsi].size() > 1)//不止一个点的时候
        {
            for(int i=0; i<mPolyMap[mMsi].size() - 1; i++)
            {
                //距离
                double distance = getDisDeg(mPolyMap[mMsi][i].x(), mPolyMap[mMsi][i].y(), mPolyMap[mMsi][i+1].x(), mPolyMap[mMsi][i+1].y());
                //速度
                double speed = distance / 3 *(3600.0/1852.0);//换算成节每秒
                //角度计算
                float angle;
                int len_x = poy[i+1].x() - poy[i].x();
                int len_y = (600-poy[i+1].y()) - (600-poy[i].y());
                //double tan_yx = (std::abs(len_y)) / (std::abs(len_x));
                double aby = std::abs(len_y);
                double abx = std::abs(len_x);
                double tan_yx = aby/abx;
                //cout<<"tan_yx"<<tan_yx;
                if(len_y > 0 && len_x < 0)
                {
                    angle =270 + atan(tan_yx)*180/M_PI;
                     cout<<"distance"<<distance<<"angle"<<angle;
                }
                else if (len_y > 0 && len_x > 0)
                {
                    angle =90- atan(tan_yx)*180/M_PI;
                     cout<<"distance"<<distance<<"angle"<<angle;
                }
                else if(len_y < 0 && len_x < 0)
                {
                    angle =180+90- atan(tan_yx)*180/M_PI;
                     //cout<<"distance"<<distance<<"angle"<<angle;
                }
                else if(len_y < 0 && len_x > 0)
                {
                    angle = 90 + atan(tan_yx)*180/M_PI;
                    // cout<<"distance"<<distance<<"angle"<<angle;
                }
                else if(len_x == 0 && len_y > 0)
                {
                    angle = 0;
                    // cout<<"distance"<<distance<<"angle"<<angle;
                }
                else if(len_x == 0 && len_y < 0)
                {
                    angle = 180;
                     //cout<<"distance"<<distance<<"angle"<<angle;
                }
                else if(len_y == 0 && len_x > 0)
                {
                    angle = 90;
                     cout<<"distance"<<distance<<"angle"<<angle;
                }
                else if(len_y == 0 && len_x < 0)
                {
                    angle = 270;
                     //cout<<"distance"<<distance<<"angle"<<angle;
                }
                ITF_AIS ais;
                //必须先初始化，否则Java订阅端会出错！
                com::zhichenhaixin::proto::VesselInfo vesselInfo;
                vesselInfo.set_utc(0);
                vesselInfo.set_mmsi(413521680+mMsi);
                vesselInfo.set_id("AIS_A__413521680");
                vesselInfo.set_country("---");
                vesselInfo.set_shiptype("---");
                QString name = "FENGSHUN" + QString::number(mMsi);
                vesselInfo.set_shipname(name.toStdString());
                vesselInfo.set_cargotype(70);
                vesselInfo.set_tobow(0);
                vesselInfo.set_tostern(0);
                vesselInfo.set_toport(0);
                vesselInfo.set_tostarboard(0);
                vesselInfo.set_shiplength(97);
                vesselInfo.set_shipwidth(14);
                vesselInfo.set_imo(0);
                vesselInfo.set_callsign("BVBT5");
                vesselInfo.set_vendorid("---");
                vesselInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(0));
                vesselInfo.set_eta("---");
                vesselInfo.set_draught(5.8);
                vesselInfo.set_dest("---");
                ais.mutable_vesselinfo()->CopyFrom(vesselInfo);//初始化
                ais.set_flag(0);
                QString sSourceID = "0";
                ais.set_sourceid(sSourceID.toLatin1().constData());
                //构造AIS数据类型,
                com::zhichenhaixin::proto::VesselTrack vesselTrack;
                vesselTrack.set_lat(mPolyMap[mMsi][i].x());//纬度
                vesselTrack.set_lon(mPolyMap[mMsi][i].y());//经度
                vesselTrack.set_heading(angle);//航艏向
                vesselTrack.set_sog(speed);//速度
                ais.mutable_vesseltrack()->CopyFrom(vesselTrack);
                //显示到表格中并发送
                showAisTrackData(ais);
                mAisMap[mMsi] << ais;
                //第一个点画静态数据,每一条轨迹用一行记录
                if(i == 0)
                {
                    //自定义数据
                    int cols=ui->table1->columnCount();
                    int rows=ui->table1->rowCount();
                    ui->table1->insertRow(rows);
                    //填充静态数据
                    //ui->table1->setItem(rows,0,new QTableWidgetItem(QString::number(rows+1)));
                    ui->table1->setItem(rows,0,new QTableWidgetItem(QString::number(mMsi+1)));
                    ui->table1->setItem(rows,1,new QTableWidgetItem(QString(ais.vesselinfo().shipname().data())));
                    ui->table1->setItem(rows,2,new QTableWidgetItem(QString::number(ais.vesselinfo().mmsi())));
                    ui->table1->setItem(rows,3,new QTableWidgetItem(QString::number(ais.vesselinfo().imo())));
                    ui->table1->setItem(rows,4,new QTableWidgetItem(QString(ais.vesselinfo().callsign().data())));
                    ui->table1->setItem(rows,5,new QTableWidgetItem(QString::number(ais.vesselinfo().cargotype())));
                    ui->table1->setItem(rows,6,new QTableWidgetItem(QString::number(ais.vesselinfo().shiplength())));
                    ui->table1->setItem(rows,7,new QTableWidgetItem(QString::number(ais.vesselinfo().shipwidth())));
                    ui->table1->setItem(rows,8,new QTableWidgetItem(QString::number(ais.vesselinfo().draught())));
                    //选中该行
                    ui->table1->selectRow(rows);
                    //交替颜色填充行,文字居中
                    for(int i=0; i<rows; i++)
                    {
                        if(i%2 == 0)
                        {
                            for(int j=0; j<cols; j++)
                            {
                                QTableWidgetItem * item = ui->table1->item(i, j);
                                item->setBackgroundColor(QColor(188, 220, 244));
                                item->setTextAlignment(Qt::AlignCenter);
                            }
                        }
                    }
                    for(int i=0; i<rows+1; i++)
                    {
                        for(int j=0; j<cols; j++)
                        {
                            QTableWidgetItem * item = ui->table1->item(i, j);
                            item->setTextAlignment(Qt::AlignCenter);
                        }
                    }
                    ui->track_label->drawTrackName(QString::number(ais.vesselinfo().mmsi()));
                }
                //最后一个点
                if(i == mPolyMap[mMsi].size() - 2)
                {
                   vesselTrack.set_lat(mPolyMap[mMsi][i+1].x());//纬度
                   vesselTrack.set_lon(mPolyMap[mMsi][i+1].y());//经度
                   vesselTrack.set_heading(0.0);//航艏向
                   vesselTrack.set_sog(speed);//速度
                   ais.mutable_vesseltrack()->CopyFrom(vesselTrack);
                   showAisTrackData(ais);
                   mAisMap[mMsi] << ais;
                }
            }
        }
    int rows=ui->table2->rowCount();
    mTabMap[mMsi] = rows;
    cout<<"map的大小"<<mTabMap.size()<<mTabMap<<"mAisMap[mMsi]"<<mAisMap[mMsi].size();

}
//--解析AIS经纬度点容器
void  AIS_Setting::analysisAisTrackData(int aa)
{
    QTableWidgetItem * item = ui->table1->currentItem();
    //mNumber = item->row();
    if(item==Q_NULLPTR) return;
    mNumber = ui->table1->item(item->row(),0)->text().toInt() - 1;
    //清楚动态数据掉原来的记录
    int rows2=ui->table2->rowCount();
    qDebug()<<rows2;
    for(int i=0;i<rows2;i++)
    {
        ui->table2->removeRow(0);
    }
    if(mPolyMap[aa].size() > 1)
        {
            for(int i=0; i<mPolyMap[aa].size() - 1; i++)
            {
                ITF_AIS ais;
                //显示到表格中并发送
                ais = mAisMap[mNumber][i];
                showAisTrackData(ais);
                //最后一个点
                if(i == mPolyMap[aa].size() - 2)
                {
                   ais = mAisMap[mNumber][i+1];
                   showAisTrackData(ais);
                }
            }
        }
}
//--表格2动态生成模拟数据
void AIS_Setting::showAisTrackData(ITF_AIS ais)
{
   //轨迹参数
   int cols=ui->table2->columnCount();
   int rows=ui->table2->rowCount();
   ui->table2->insertRow(rows);
   //填充动态数据
   //cout<<str_ais<<ais.vesseltrack().heading()<<"rows"<<rows<<"cols"<<cols;
   ui->table2->setItem(rows,0,new QTableWidgetItem(QString::number(rows+1)));
   ui->table2->setItem(rows,1,new QTableWidgetItem(QString::number(ais.vesseltrack().lon())));
   ui->table2->setItem(rows,2,new QTableWidgetItem(QString::number(ais.vesseltrack().lat())));
   ui->table2->setItem(rows,3,new QTableWidgetItem(QString::number(ais.vesseltrack().sog())));
   ui->table2->setItem(rows,4,new QTableWidgetItem(QString::number(ais.vesseltrack().heading())));
   ui->table2->setItem(rows,5,new QTableWidgetItem(QString::number(ais.vesseltrack().heading())));
   //选中该行
   cout<<"ais.vesseltrack().lon()"<<ais.vesseltrack().lon();
   ui->table2->selectRow(rows);
   //交替颜色填充行,文字居中
   for(int i=0; i<rows; i++)
   {
       if(i%2 == 0)
       {
           for(int j=0; j<cols; j++)
           {
               QTableWidgetItem * item = ui->table2->item(i, j);
               item->setBackgroundColor(QColor(188, 220, 244));
               item->setTextAlignment(Qt::AlignCenter);
           }
       }
   }
   for(int i=0; i<rows+1; i++)
   {
           for(int j=0; j<cols; j++)
           {
               QTableWidgetItem * item = ui->table2->item(i, j);
               item->setTextAlignment(Qt::AlignCenter);
           }
   }

   ui->track_label->drawTrackName(QString::number(ais.vesselinfo().mmsi()));
}
//--清楚按钮按下
void AIS_Setting::on_clear_pushbotton_clicked()
{
    //emit signalCreatBtnClicked();
    //重绘坐标系
    ui->track_label->drawTrackPixmap();
    //清空以前残留的轨迹点
    ui->track_label->delThePoy();
    //表格数据清空
    //table1
    int rows=ui->table1->rowCount();
    //qDebug()<<rows;
    for(int i=0;i<rows;i++)
    {
        //cout<<"rows"<<rows<<i;
        ui->table1->removeRow(0);
    }
    rows=ui->table2->rowCount();
    for(int i=0;i<rows;i++)
    {
        ui->table2->removeRow(0);
    }
    //QMap清空
    mTabMap.clear();
    mPolyMap.clear();
    mPoyMap.clear();
    mAisMap.clear();
    mNumber = 0;
    mCount = 0;
    mAisCount = 0;
    mMsi = 0;
    mWorkTimer.stop();
}
//--修改数据按钮按下
void AIS_Setting::on_set_pbt_clicked()
{
    mAisList.clear();
    //先停止发送
    mWorkTimer.stop();
    //取出当前的记录
    QTableWidgetItem * item = ui->table1->currentItem();
    //mNumber = item->row();
    if(item==Q_NULLPTR) return;
    mNumber = ui->table1->item(item->row(),0)->text().toInt() - 1;
    QTableWidgetItem * item2 = ui->table2->currentItem();
    mCount = ui->table2->columnCount();
    //QTableWidgetItem * item3 = ui->table2->rowCount();
    int rows2 = ui->table2->rowCount();
    ITF_AIS ais;
    //静态数据
    //必须先初始化，否则Java订阅端会出错！
    com::zhichenhaixin::proto::VesselInfo vesselInfo;
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    vesselInfo.set_utc(utc);
    vesselInfo.set_shipname(ui->table1->item(mNumber, 1)->text().toStdString());
    vesselInfo.set_mmsi(ui->table1->item(mNumber, 2)->text().toInt());
    vesselInfo.set_imo(ui->table1->item(mNumber, 3)->text().toInt());
    vesselInfo.set_callsign(ui->table1->item(mNumber, 4)->text().toStdString());
    vesselInfo.set_cargotype(ui->table1->item(mNumber, 5)->text().toInt());
    vesselInfo.set_shiplength(ui->table1->item(mNumber, 6)->text().toInt());
    vesselInfo.set_shipwidth(ui->table1->item(mNumber, 7)->text().toInt());
    vesselInfo.set_draught(ui->table1->item(mNumber, 8)->text().toFloat());
    vesselInfo.set_id("AIS_A__"+ui->table1->item(mNumber, 2)->text().toStdString());
    vesselInfo.set_country("---");
    vesselInfo.set_shiptype("---");
    vesselInfo.set_tobow(0);
    vesselInfo.set_tostern(0);
    vesselInfo.set_toport(0);
    vesselInfo.set_tostarboard(0);
    vesselInfo.set_vendorid("---");
    vesselInfo.set_fixtype(static_cast<com::zhichenhaixin::proto::EPFD>(0));
    vesselInfo.set_eta("---");
    vesselInfo.set_dest("---");

    ais.mutable_vesselinfo()->CopyFrom(vesselInfo);//初始化
    ais.set_flag(0);
    QString sSourceID = "0";
    ais.set_sourceid(sSourceID.toLatin1().constData());
    cout<<"------------------------------------------------------------------------------------------------";
    //动态数据
    for(int i = 0; i<rows2; i++)
    {
        //QTableWidgetItem * item = ui->table2->item(mNumber, j);
        //cout<<"("<<k<<","<<j<<")"<<"内容"<<item->text();
        //构造AIS数据类型,
        com::zhichenhaixin::proto::VesselTrack vesselTrack;
        QString ssource = Utils::Profiles::instance()->value("Radar_1","Radar_N").toString();
        qint64 utc = QDateTime::currentMSecsSinceEpoch();
        vesselTrack.set_lat(ui->table2->item(i, 2)->text().toDouble());//纬度
        vesselTrack.set_lon(ui->table2->item(i, 1)->text().toDouble());//经度
        vesselTrack.set_sog(ui->table2->item(i, 3)->text().toFloat());//速度
        vesselTrack.set_cog(ui->table2->item(i, 4)->text().toFloat());//对地航向
        vesselTrack.set_heading(ui->table2->item(i, 5)->text().toFloat());//航艏向
        vesselTrack.set_navstatus(static_cast<com::zhichenhaixin::proto::NAVI_STATUS>(15));//航行状态
        vesselTrack.set_mmsi(ui->table1->item(mNumber, 2)->text().toInt());//用户识别码
        vesselTrack.set_id(QString("AIS_A__"+ui->table1->item(mNumber, 2)->text()).toStdString());//唯一识别码("AIS_"+shipType+"__"+用户识别码)
        vesselTrack.set_shiptype("A");//船种
//        vesselTrack.set_sourceid(ssource.toLatin1().constData());//设置AIS编号
        vesselTrack.set_rot(0.0);//转向率
        vesselTrack.set_utc(utc);//时间
        ais.mutable_vesseltrack()->CopyFrom(vesselTrack);
//        qDebug()<<"船舶动态实例信息如下:" << "  \n"
//               << "用户识别码: " << ais.vesseltrack().mmsi() << "  \n"
//               << "唯一识别码: " << QString::fromStdString(ais.vesseltrack().id()) << "  \n"
//               << "船舶种类: " << QString::fromStdString(ais.vesseltrack().shiptype()) << "  \n"
//               << "船舶航行状态: " << int(ais.vesseltrack().navstatus()) << "  \n"
//               << "船舶转向率: " << ais.vesseltrack().rot() << " \n"
//               << "对地航速: " << ais.vesseltrack().sog() << " \n"
//               << "经度: " << ais.vesseltrack().lon() << " \n"
//               << "纬度: " << ais.vesseltrack().lat() << " \n"
//               << "对地航向: " << ais.vesseltrack().cog() << " \n"
//               << "船艏向: " << ais.vesseltrack().heading() << " \n"
//               << "时间标记: " << ais.vesseltrack().utc() << " \n"
//               << "基站编号: " << QString::fromStdString(ais.vesseltrack().sourceid()) << " \n";
        //每次添加进去修改的所有AIS模拟数据
        mAisList << ais;
    }
    mAisMap[mNumber] = mAisList;
}
//--发送AIS数据按钮按下
void AIS_Setting::on_send_pbt_clicked()
{
    mWorkTimer.start();
}
//--定时发送AIS模拟数据
void AIS_Setting::slotSendAisData()
{
    qRegisterMetaType<ITF_AISList>("ITF_AISList");
    //本次数据的所有的ais信息
    cout<<"发送模拟AIS数据"<<mAisMap.size();
    ITF_AISList objAisList;
    //轨迹参数

    //构建数据
    foreach(int k, mTabMap.keys())
    {
        if( mAisCount < mTabMap[k])
        {
            cout<<"k"<<k<< mTabMap[k]<<mAisCount;
            ITF_AIS ais;
            ais = mAisMap[k][mAisCount];
            ITF_AIS * temp = objAisList.add_ais();
            temp->CopyFrom(ais);
        }
    }
     mAisCount++;
    //发送AIS模拟数据
    if(objAisList.ais_size() > 0) {
        cout<<"objAisList.ais_size()"<<objAisList.ais_size();
        emit signalSendAislist(objAisList);
    }
    else
    {
        cout<<"停止发送";
        mWorkTimer.stop();
    }
}
//--鼠标按下事件
void AIS_Setting::slotMousePress()
{
    QTableWidgetItem * item = ui->table1->currentItem();
        if(item==Q_NULLPTR)return;
    static int row = item->row();
    mNumber = ui->table1->item(item->row(),0)->text().toInt() - 1;
    if(row != mNumber)
    {
        row = mNumber;
        analysisAisTrackData(mNumber);
    }
    //cout<<"这是第几行"<<item->row();
}
//--删除按钮按下
void AIS_Setting::on_del_pbt_clicked()
{
    //取出当前的记录
    QTableWidgetItem * item = ui->table1->currentItem();
    if(item==Q_NULLPTR) return;
    if(ui->table1->rowCount() == 1)
    {
        //ui->table2->clear();
        int i = ui->table2->rowCount();
        while(i)
        {
            i--;
            ui->table2->removeRow(i);
        }
    }
    mNumber = ui->table1->item(item->row(),0)->text().toInt() - 1;
    mAisMap.remove(mNumber);
    mPolyMap.remove(mNumber);
    mPoyMap.remove(mNumber);
    mTabMap.remove(mNumber);
    ui->table1->removeRow(item->row());
    cout<<"mNumber"<<mNumber<<"Map"<<mAisMap.keys()<<mPolyMap.keys()<<mPoyMap.keys()<<mTabMap.keys();

    slotMousePress();
    emit signalRedrawAisTrack(mPoyMap);
}

void AIS_Setting::on_frequency_pushButton_clicked()
{
    int frequency = ui->frequency_lineEdit->text().toInt();
    if(frequency < 10)
        frequency = 10;
    Utils::Profiles::instance()->setValue("Ais","Ais_Send_Frequency",frequency);
}

void AIS_Setting::on_limit_checkBox_clicked(bool checked)
{
    cout<<"checked"<<checked;
    Utils::Profiles::instance()->setValue("Ais","Ais_Limit",checked);
}

void AIS_Setting::on_checkBox_clicked(bool checked)
{
    cout<<"checked"<<checked;
    Utils::Profiles::instance()->setValue("Ais","Beidou_Limit",checked);
}

void AIS_Setting::on_beidou_pushButton_clicked()
{
    mBeidou->show();
}

void AIS_Setting::aisTobeidou(double lon,double lat)
{
    mBeidou->aisToBeidou(lon,lat);
}

void AIS_Setting::on_aisTypeBox_currentIndexChanged(int index)
{
    bool client_mode = true;
    if(index == 1) client_mode = false;
    ui->listen_label->setVisible(!client_mode);
    ui->listen_port_edit->setVisible(!client_mode);
    ui->host_label->setVisible(client_mode);
    ui->host_edit->setVisible(client_mode);
    ui->host_port_label->setVisible(client_mode);
    ui->ais_dev_port_edit->setVisible(client_mode);
    ui->reconnect_time_edit->setVisible(client_mode);
    ui->host_connect_label->setVisible(client_mode);
    ui->minute_label->setVisible(client_mode);
}

void AIS_Setting::on_load_file_btn_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(NULL,"导入AIS文件","../AIS数据/","*");
    if(file_name.isEmpty()) return;
    ui->ais_lineEdit->setText(file_name);
    if(mAisFakeThread)
    {
        //结束正在运行的线程
        mAisFakeThread->setIsOver(true);
        mAisFakeThread->deleteLater();
    }
    //加载ais文件
    mAisFakeThread = new up_ais_pthread(file_name,  ui->frequency_lineEdit->text().toInt());
    if(mAisParser)
    {
        connect(mAisFakeThread,SIGNAL(send_ais_signal(QByteArray)), mAisParser, SLOT(slotProcessAisData(QByteArray)));
    }
    mAisFakeThread->start();
}

void AIS_Setting::on_frequency_btn_clicked()
{
    int frequency = ui->frequency_lineEdit->text().toInt();
    if(frequency < 10) frequency = 10;
    Utils::Profiles::instance()->setValue(str_ais , AIS_SEND_FREQUENCY, frequency);
    if(mAisFakeThread) mAisFakeThread->setInterval(frequency);
}
