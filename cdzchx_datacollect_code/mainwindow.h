#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "dataserverutils.h"

#include "ais_radar/zchxaisdataserver.h"
#include "ais_radar/zchxradardataserver.h"
#include "ais_radar/zchxradarechodatachange.h"
#include "ais_radar/zxhcprocessechodata.h"
#include "ais_radar/zchxanalysisandsendradar.h"
#include "ais_radar/zchxaisdataprocessor.h"
#include "side_car_parse/Messages/RadarConfig.h"
#include "side_car_parse/Messages/VideoConfig.h"
#include "side_car_parse/Messages/TSPIConfig.h"

class QLabel;
class ZCHXAisDataServer;
class ZCHXRadarDataServer;
class ZCHXRadarEchoDataChange;
class ZXHCProcessEchoData;
class ZCHXAnalysisAndSendRadar;
class zchxAisDataProcessor;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void  initUI();
    void  initRadarCfgInfo();

signals:
    void signalOpenRadar();
    void signalcloseRadar();
    void signal_set_penwidth(int);//设置笔宽度信号

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
//    void show_info_slot(QString,double,float );//1_显示分析,打印txt
    void show_video_slot(int, int);//1_显示目标个数
    void show_statistics_slot(int, int, int, int, int);//1_打印统计丢包率
//    void show_missing_spokes_slot(QString);//1_打印丢失扫描线信息
//    void show_received_spokes_slot(QString);//1_打印所有扫描线信息
    void slotRecvRadarReportInfo_1(QList<RadarStatus>,int);//1_实时打印接收到的雷达状态信息
    void slotRecvRangeFactorChanged(double factor);
private slots:    
    void toolAction(QMenu *menu, QToolBar *toolBar);
    void onPower();
    void onConfig();
    void onOutInter();
    void onProtocal();
    void on_setip_clicked();

    void on_deleteip_clicked();

    void on_openRadarBtn_clicked();

    void on_closeRadarBtn_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_7_clicked();

private:
    Ui::MainWindow *ui;
    QLabel          *mTimeLable;
    QLabel          *mVirtualIpWorkingLbl;
    QStringList       mClientList;
    int index;
    void            *mCtx;
    bool txt;//1_打印txt标志
    bool t_2;//1_打印丢包情况
    bool t_3;//1_打印丢失扫描线信息

    ZCHXAisDataServer *mAisDataServer;
    zchxAisDataProcessor *mAisDataProc;
    QList<ZCHXRadarDataServer *> mRadarDataServerList;
    QList<ZCHXAnalysisAndSendRadar*> mAnalysisAndSendRadarList;

    ZCHXRadarEchoDataChange *mpRadarEchoDataChange;


    ZXHCProcessEchoData *mProcessEchoData;
    QMap<int, ZCHX::Messages::RadarConfig*> mRadarConfigMap;
};

#endif // MAINWINDOW_H
