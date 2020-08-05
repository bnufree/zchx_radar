#ifndef AIS_SETTING_H
#define AIS_SETTING_H

#include <QWidget>
#include "up_ais_pthread.h"
#include <QThread>
#include <QFile>
#include <QString>
#include <QDialog>
#include <QFileDialog>
//#include <QDebug>
#include "profiles.h"
#include <QPainter>
#include <QPixmap>
#include <QWheelEvent>
#include <QPaintEvent> //用于绘画事件
#include <QtGui> //引入用到的控件
#include <QTimer>
#include <drawaistrack.h>
#include "./ais_radar/zchxfunction.h"
#include "ais/ais.h"
#include "beidoudata.h"
#include "ZCHXAISVessel.pb.h"


typedef com::zhichenhaixin::proto::AIS ITF_AIS;
typedef com::zhichenhaixin::proto::AISList ITF_AISList;

namespace Ui {
class AIS_Setting;
}

class zchxAisDataCollector;
class zchxAisDataProcessor;
class up_ais_pthread;

class AIS_Setting : public QWidget
{
    Q_OBJECT

public:
    explicit AIS_Setting(int uSourceID = 1,QWidget *parent = 0);
    ~AIS_Setting();
signals:
    void newAisClassSignal();
    void signalCreatBtnClicked();
    void signalSendAislist(ITF_AISList objAisList);
    void signalRedrawAisTrack(QMap<int, QPolygonF>);
    void signalShowBeidou();
private:
    void initCollector();
    void initParser();
private slots:
    void on_add1_pbt_clicked();

    void on_del1_pbt_clicked();

    void on_add2_pbt_clicked();

    void on_del2_pbt_clicked();

    void on_save_pushbotton_clicked();

    void drawTrackPixmap();

    void on_save_pushbotton_2_clicked();

    void slotDrawAisTrack(int a, double b, double c);

    void wheelEvent(QWheelEvent *event);//重写滚轮事件

    void slotShowAisPix(QPixmap);

    void on_creatl_pushbotton_clicked();

    void slotDealAisPolygon(QPolygonF);//解析模拟数据

    void showAisTrackData(ITF_AIS ais);

    void on_clear_pushbotton_clicked();//清楚按钮

    void on_set_pbt_clicked();//保存按钮

    void on_send_pbt_clicked();//发送按钮

    void slotSendAisData();//定时发送AIS模拟数据

    void slotMousePress();		//--鼠标按下事件

    void analysisAisTrackData(int a);//--解析AIS经纬度点容器

    void on_del_pbt_clicked();

    void on_frequency_pushButton_clicked();

    void on_limit_checkBox_clicked(bool checked);

    void on_checkBox_clicked(bool checked);

    void on_beidou_pushButton_clicked();

    void aisTobeidou(double,double);

    void on_aisTypeBox_currentIndexChanged(int index);

    void on_load_file_btn_clicked();

    void on_frequency_btn_clicked();

private:
    Ui::AIS_Setting *ui;
    //up_ais_pthread *up_ais;
    QPixmap image;//画板
    int xLevel = 850, yLevel = 600, aLevel = 1;
    drawaistrack *mDrawAisTrack;
    QTimer mWorkTimer;
    QTimer mTabTimer;
    QMap<int,int> mTabMap;
    int mNumber, mCount;//记录表格中的AIS数据
    int mAisCount;
    int mMsi;
    QMap<int, QPolygonF> mPolyMap;
    QMap<int, QPolygonF> mPoyMap;
    QMap<int, QList<ITF_AIS>> mAisMap;
    QList<ITF_AIS> mAisList;
    QString str_ais;
    beidouData *mBeidou;

    //界面对应的数据接收器和解析器
    zchxAisDataCollector*       mAisCollector;
    zchxAisDataProcessor*       mAisParser;
    //模拟数据线程
    up_ais_pthread*             mAisFakeThread;
};

#endif // AIS_SETTING_H
