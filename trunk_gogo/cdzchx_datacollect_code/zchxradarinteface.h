#ifndef ZCHXRADARINTEFACE_H
#define ZCHXRADARINTEFACE_H
#include <QMainWindow>
#include "dataserverutils.h"

#include "aisbaseinfosetting.h"
#include "ais/zchxaisdataserver.h"
#include "ais_radar/zchxradardataserver.h"
#include "ais_radar/zchxradarechodatachange.h"
#include "ais_radar/zxhcprocessechodata.h"
#include "ais_radar/zchxanalysisandsendradar.h"
#include "ais/zchxaisdataprocessor.h"
#include "dialog_log.h"
#include "ais_radar/zchxradaraissetting.h"
#include "ui_zchxradaraissetting.h"
#include "dialog_set.h"
#include "dialog_cli.h"
#include "up_video_pthread.h"
#include "up_ais_pthread.h"
#include<QMouseEvent>
#include<QWheelEvent>
#include "myLabel.h"
#include "comdata/comdatamgr.h"
#include "dataout/comdatapubworker.h"
#include "comdata/comconfigwidget.h"
#include "ui_comconfigwidget.h"
#include "ais_setting.h"
#include "radar_control.h"
#include "dialog_help.h"
#include "float_setting.h"


class QLabel;
class ZCHXRadarDataServer;
class ZCHXRadarEchoDataChange;
class ZXHCProcessEchoData;
class ZCHXAnalysisAndSendRadar;
class zchxSimulateThread;

namespace Ui {
class zchxradarinteface;
}

//namespace qt {
//class MainWindow;
//}

class zchxradarinteface : public QMainWindow
{
    Q_OBJECT

public:
    explicit zchxradarinteface(int ID,QWidget *parent = 0);
    ~zchxradarinteface();
    void  initUI();
    bool reset;
 //   void wheelEvent(QWheelEvent*event);//重写滚轮事件

signals:
    void signalOpenRadar();
    void signalcloseRadar();
    void prtVideoSignal(bool);//打印回波信号
    void prtAisSignal(bool);//打印ais信号
    void signal_set_penwidth(int);//设置笔宽度信号
    void signalRadarConfigChanged(int, int, int);
    void receiveLogSignal(qint64 time, const QString& name, const QString& content);//更新数据日志信号
    void updateCliSignal(const QString& ip, const QString& name, int port, int inout);//更新客户端信号
    void send_video_signal(QByteArray,QString,int,int,int);//导入回波数据
    void send_report_signal(QByteArray, int);//导入回波数据
    void send_ais_signal(QByteArray);//导入AIS数据
    void send_res_signal();//删除雷达对象信号
    void showTrackNumSignal(bool);//显示目标编号
    void colorSetSignal(int,int,int,int,int,int);//回波颜色设置
    void signalCombineTrackc(const zchxTrackPointMap&);
    void signalSendComTracks(const zchxTrackPointMap&);
    void signalCombineVideo(QMap<int, QList<TrackNode>>,int);
    void signalSendComVideo(QList<TrackNode>);
    void signalSetPix(QPixmap);
    void signalRestart();
protected:
    void closeEvent(QCloseEvent *);
private:


public slots:
    void receiveContent(qint64 time, const QString& name, const QString& content);
    void slotRecvHearMsg(QString msg);
    void slotRecvWorkerMsg(const QString& msg);
    void slotDisplaycurTime();
    void slotInsertLogInfo(const QString& msg);
    void slotUpdateClientTable(const QString& ip, const QString& name, int port, int inout);
    void slotUpdateVirtualIpString(const QString& msg);
    void slotRadarConfigChanged(int radarID, int type, int value);
    void show_video_slot(int, int);//1_显示目标个数
    void show_statistics_slot(int, int, int, int, int);//1_打印统计丢包率
    void slotRecvRadarReportInfo_1(QList<RadarStatus>,int);//1_实时打印接收到的雷达状态信息
    void slotRecvRangeFactorChanged(double factor);
    void setRadarVideoAndTargetPixmap(const QPixmap &videoPixmap,const Afterglow &dataAfterglow);
    void reset_window();//重新初始化
    void show_info_slot(QString);//显示传入周老师库的数据
    void slotSetComDevParams(const QMap<QString,COMDEVPARAM>& param);//开始串口数据接收
    void slotUpdateZmq(QString,QString);
    void removeSubTab(int);
    void help_clicked();//帮助菜单按下
    void joinGropslot(QString);//判断加入组播是否成功
    void slotRecvRadarProcessNum(int extract, int correct, int track);
    void slotShowRadiusCoefficient(double,double);
    void handleTimeout();
private slots:
    //void on_setip_clicked();

   // void on_deleteip_clicked();

    void on_openRadarBtn_clicked();

    void on_closeRadarBtn_clicked();

    void on_pushButton_clicked();


    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_7_clicked();

    void on_uploadRadarBtn_clicked();

    void logButton();

    void on_setButton_clicked();

    void setButton_2();

    void on_logButton_3_clicked();

    //void logButton_2_clicked();

    void on_draw_pushButton_clicked();

    void on_pushButton_9_clicked();

    void on_serial_pushButton_clicked();

    void on_xinke_pushButton_clicked();

    void on_control_pushButton_clicked();

    //void on_doubleSpinBox_valueChanged(double arg1);

    void on_radar_parse_setting_clicked();

    void on_threshold_type_currentIndexChanged(int index);

    //void on_push_Button_clicked();

    void show_range_slot(double, double);

    void on_shownum_pushButton_clicked();

    //void on_angle_push_Button_clicked();

    void on_setfubiao_pushButton_clicked();

    void on_color_2_pushButton_clicked();

    void on_color1Button_clicked();
    void on_color2Button_clicked();


    //void on_jump_pushButton_clicked();


    void on_k_pushButton_clicked();

    void on_restart_pushButton_clicked();

    void on_saveRadarBtn_clicked();


    void on_update_setting_btn_clicked();

    void on_save_restart_btn_clicked();

    void on_import_report_btn_clicked();

private:
    Ui::zchxradarinteface *ui;
    QLabel          *mTimeLable;
    QLabel          *mVirtualIpWorkingLbl;
    QStringList       mClientList;
    int index; //AIS个数的编号
    int zeor_flag;
    void            *mCtx;

    bool txt;//1_打印txt标志
    bool t_2;//1_打印丢包情况
    bool t_3;//1_打印丢失扫描线信息
    bool m_drawpic;//采集器绘制回波图标志

    up_video_pthread *up_video;//本地回波数据导入
    Dialog_log *mLog;//日志弹窗
    Dialog_set *mSet;//雷达配置
    Dialog_cli *mCli;//客户端
    dialog_help *mHelo;//帮助
    float_setting mFloat;//浮标
    ZCHXRadarDataServer* mRadarDataServer;
    ZCHXAnalysisAndSendRadar* mAnalysisAndSendRadar;
    QList<AIS_Setting*> mAisSettingList;

    ZCHXRadarEchoDataChange *mpRadarEchoDataChange;
    ZXHCProcessEchoData *mProcessEchoData;
    //串口类
    ComDataMgr      *mComDataMgr;
    ComDataPubWorker* mOutWorker;
    ComConfigWidget *mComConfigWidget;
    //21类AIS助航消息
    aisBaseInfoSetting *mAisbaseinfosetting;
    QAction *color1 ;
    QAction *color2 ;
    int a1,a2,a3,b1,b2,b3;//回波颜色
    int radarId;//雷达编号
    QString str_radar;
    int mUpRad = 0;
    QTimer *restartTimer; //重新打开雷达定时器

    //雷达状态报告模拟
    zchxSimulateThread*   mReportSimulateThread;
};

#endif // MAINWINDOW_H
