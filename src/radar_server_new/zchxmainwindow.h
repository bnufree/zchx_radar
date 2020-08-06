#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "dataserverutils.h"

#include "aisbaseinfosetting.h"
#include "ais/zchxaisdataserver.h"
#include "ais_radar/zchxradardataserver.h"
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
//#include "side_car_parse/Messages/RadarConfig.h"
//#include "side_car_parse/Messages/VideoConfig.h"
//#include "side_car_parse/Messages/TSPIConfig.h"
#include "zchxradarinteface.h"
#ifdef USE_ZCHX_ECDIS
#include "qmapwidget.h"
#endif

class QLabel;
class ZCHXAisDataServer;
class ZCHXRadarDataServer;
class ZCHXRadarEchoDataChange;
class ZXHCProcessEchoData;
class ZCHXAnalysisAndSendRadar;
class zchxAisDataProcessor;
namespace Ui {
class zchxMainWindow;
}

//namespace qt {
//class MainWindow;
//}
class QProcess;
class zchxMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit zchxMainWindow(QWidget *parent = 0);
    ~zchxMainWindow();
    void  initUI();

signals:
    void signalSendComTracks(const zchxTrackPointMap&);
    void signalSendComVideo(QList<TrackNode>);
    void prtAisSignal(bool);//打印ais信号
    void receiveLogSignal(qint64 time, const QString& name, const QString& content);//更新数据日志信号
    void updateCliSignal(const QString& ip, const QString& name, int port, int inout);//更新客户端信号
    void send_ais_signal(QByteArray);//导入AIS数据
protected:
    void closeEvent(QCloseEvent *);
private:


public slots:
    void receiveContent(qint64 time, const QString& name, const QString& content);
    void slotRecvHearMsg(QString msg);
    void slotUpdateClientTable(const QString& ip, const QString& name, int port, int inout);
    void removeSubTab(int);
    void ais_clicked();//AIS菜单按下
    void radar_clicked();//雷达菜单按下
    void help_clicked();//帮助菜单按下
    void logButton();
    void newAisClassSlot();//新生成AIS解析对象
    void soltDealTacks1(const zchxTrackPointMap&);
    void soltDealTacks2(const zchxTrackPointMap&);
    void combineTracks();
    void combineVideo(QMap<int, QList<TrackNode>>,int);
    void slotShowDataStatus();
    void slotRecvPortStartStatus(int port, int sts, const QString &topic);
private slots:
    void aisInit();
    void restartMe();

private:
    Ui::zchxMainWindow *ui;
    QLabel          *mTimeLable;
    QLabel          *mVirtualIpWorkingLbl;
    QStringList       mClientList;
    Dialog_log *mLog;//日志弹窗
    dialog_help *mHelo;//帮助
    int ais_index;
    int radar_index;
    QList<zchxradarinteface*> zchxradarintefaceList;
    zchxTrackPointMap t1,t2,t3;//目标集合1,2,3
    QMap<int,QMap<int, QList<TrackNode>>> mVideoMap;
    QThread*           mTestMapMonitorThread;
    QMap<int, QString>  mDataPortSts;
};

#endif // MAINWINDOW_H
