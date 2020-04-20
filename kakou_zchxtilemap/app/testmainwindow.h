#ifndef TESTMAINWINDOW_H
#define TESTMAINWINDOW_H

#include <QMainWindow>
#include "qt/mainwindow.h"
#include "qt/map_layer/zchxMapLayer.h"

namespace Ui {
class TestMainWindow;
}

struct PointData
{
    double lat;
    double lon;
    QString name;
};

enum DataInput{
    Data_Ais = 0,
    Data_Ais_Chart,
    Data_Radar_Track,
    Data_Radar_Video,
    Data_radar_Rect,
    Data_Radar_Limit,
    Data_Reserved,
};

struct dataSource
{
    int         mId;
    QString     mTitle;
    bool        mStatus;
};

Q_DECLARE_METATYPE(dataSource)

namespace ZCHX_RADAR_RECEIVER {
    class ZCHXRadarDataChange;
}

class testMapSettings : public QSettings
{
public:
    testMapSettings(const QString& file);
    void setDefaultValue(const QString& prefix, const QString& key, const QVariant& value);
    void setUserValue(const QString& prefix, const QString& key, const QVariant& value);
    QVariant getUserValue(const QString& prefix, const QString& key, const QVariant& value = QVariant());
};


class TestMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TestMainWindow(QWidget *parent = 0);
    ~TestMainWindow();

public slots:
    void slotTimerout();
    void slotSetRadarVideoWholeData(int siteID, double lon, double lat, double dis, int type, int loop, int curIndex, const QByteArray &objPixmap, const QByteArray &prePixMap);
    void slotRadarPointLayerDisplay(bool sts);
    void slotRadarVideoLayerDisplay(bool sts);
    void slotSelectPointFileDlg();
    void slotRadarRectLayerDisplay(bool sts);
    void slotRadarLimitAreaDisplay(bool sts);
    void slotAisLayerDisplay(bool sts);
    void slotAisChartLayerDisplay(bool sts);
    void slotSetMapSource();
    void slotSetDataSource(bool sts);
private:
    void getLonlatListFromFile(const QString& file, QList<PointData>& list);
    bool getLayerDisplay(const QString& layer);

private:
    Ui::TestMainWindow *ui;
    qt::MainWindow  *m_pEcdisWin;
    QTimer *mTestTimer;
    ZCHX_RADAR_RECEIVER::ZCHXRadarDataChange* mDataChange;
    testMapSettings       *mSetting;
    QMenu*                  mMapSourceMenu;
    QMenu*                  mDataSourceMenu;
//    QMap<int, dataSource>       mDataSourceMap;
};

#endif // TESTMAINWINDOW_H
