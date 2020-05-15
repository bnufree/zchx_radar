#include "testmainwindow.h"
#include "ui_testmainwindow.h"
#include <QDateTime>
#include "qt/zchxutils.hpp"
#include "testrotatewidget.h"
#include "radar/zchxradardatachange.h"
#include "testmapwatchdogthread.h"

#include "zchxfunction.h"

#define     SERVER          "host"
#define     PORT            "port"
#define     TOPIC           "topic"
#define     CURCOLOR           "cur_color"
#define     HISCOLOR            "history_color"
#define     ID                "id"
#define     SEC_AIS           "Ais"
#define     SEC_AIS_CHART       "AisChart"
#define     SEC_RADAR_RECT     "RadarRect"
#define     SEC_RADAR_TRACK     "RadarTrack"
#define     SEC_RADAR_VIDEO     "RadarVideo"
#define     SEC_RADAR_LIMIT     "RadarLimit"
#define     STATUS           "status"



testMapSettings::testMapSettings(const QString &file)
    :QSettings(file, QSettings::IniFormat)
{
    setIniCodec(QTextCodec::codecForLocale());
    setDefaultValue("Site", ID, 1);
    setDefaultValue(SEC_AIS, SERVER, "127.0.0.1");
    setDefaultValue(SEC_AIS, PORT, "5153");
    setDefaultValue(SEC_AIS, TOPIC, "AIS");
    setDefaultValue(SEC_AIS, STATUS, 0);

    setDefaultValue(SEC_AIS_CHART, SERVER, "127.0.0.1");
    setDefaultValue(SEC_AIS_CHART, PORT, "5153");
    setDefaultValue(SEC_AIS_CHART, TOPIC, "Chart");
    setDefaultValue(SEC_AIS_CHART, STATUS, 0);

    setDefaultValue(SEC_RADAR_TRACK, SERVER, "127.0.0.1");
    setDefaultValue(SEC_RADAR_TRACK, PORT, "5151");
    setDefaultValue(SEC_RADAR_TRACK, TOPIC, "RadarTrack");
    setDefaultValue(SEC_RADAR_TRACK, STATUS, 0);

    setDefaultValue(SEC_RADAR_RECT, SERVER, "127.0.0.1");
    setDefaultValue(SEC_RADAR_RECT, PORT, "5657");
    setDefaultValue(SEC_RADAR_RECT, TOPIC, "RadarRect");
    setDefaultValue(SEC_RADAR_RECT, CURCOLOR, "#ff0000");
    setDefaultValue(SEC_RADAR_RECT, HISCOLOR, "#00ffff");
    setDefaultValue(SEC_RADAR_RECT, STATUS, 0);

    setDefaultValue(SEC_RADAR_VIDEO, SERVER, "127.0.0.1");
    setDefaultValue(SEC_RADAR_VIDEO, PORT, "5689");
    setDefaultValue(SEC_RADAR_VIDEO, TOPIC, "RadarVideo");
    setDefaultValue(SEC_RADAR_VIDEO, STATUS, 0);

    setDefaultValue(SEC_RADAR_LIMIT, SERVER, "127.0.0.1");
    setDefaultValue(SEC_RADAR_LIMIT, PORT, "10086");
    setDefaultValue(SEC_RADAR_LIMIT, TOPIC, "Limit_Area");
    setDefaultValue(SEC_RADAR_LIMIT, STATUS, 0);

}

void testMapSettings::setDefaultValue(const QString &prefix, const QString &key, const QVariant &value)
{
    beginGroup(prefix);
    if(!contains(key))
    {
        setValue(key, value);
    }
    endGroup();
}

void testMapSettings::setUserValue(const QString &prefix, const QString &key, const QVariant &value)
{
    beginGroup(prefix);
    setValue(key, value);
    endGroup();
}

QVariant testMapSettings::getUserValue(const QString &prefix, const QString &key, const QVariant &defaultVal)
{
    QVariant res;
    beginGroup(prefix);
    if(contains(key))
    {
        res = value(key, defaultVal);
    }
    endGroup();
    return res;
}

TestMainWindow::TestMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TestMainWindow),
    m_pEcdisWin(0),
    mDataChange(0),
    mSetting(0)
{
    ui->setupUi(this);
    this->setDockNestingEnabled(false);
    this->setAnimated(false);
    setWindowTitle(QStringLiteral("采集器测试客户端"));
    ui->centralwidget->setLayout(new QVBoxLayout);
    m_pEcdisWin = new qt::MainWindow;
    ui->centralwidget->layout()->addWidget(m_pEcdisWin);





#if 0
    ui->centralwidget->layout()->addWidget(new testRotateWidget(this));
#else


    std::shared_ptr<qt::MapLayer> islandline(new qt::MapLayer(ZCHX::LAYER_ISLAND, ZCHX::TR_LAYER_ISLAND, true));
    m_pEcdisWin->itfAddLayer(islandline);   

    //ais
    std::shared_ptr<qt::MapLayer> pAisLayer(new qt::MapLayer(ZCHX::LAYER_AIS,ZCHX::TR_LAYER_AIS, true));
    m_pEcdisWin->itfAddLayer(pAisLayer);

    std::shared_ptr<qt::MapLayer> pChartLayer(new qt::MapLayer(ZCHX::LAYER_AIS_CHART,ZCHX::TR_LAYER_AIS_CHART, true));
    m_pEcdisWin->itfAddLayer(pChartLayer);

    std::shared_ptr<qt::MapLayer> pAisCurrentLayer(new qt::MapLayer(ZCHX::LAYER_AIS_CURRENT,ZCHX::TR_LAYER_AIS_CURRENT,true));
    m_pEcdisWin->itfAddLayer(pAisCurrentLayer);

    std::shared_ptr<qt::MapLayer> pAisTrackLayer(new qt::MapLayer(ZCHX::LAYER_AIS_TRACK,ZCHX::TR_LAYER_AIS_TRACK,true));
    m_pEcdisWin->itfAddLayer(pAisTrackLayer);

    //雷达
    std::shared_ptr<qt::MapLayer> pRadarLayer(new qt::MapLayer(ZCHX::LAYER_RADAR,ZCHX::TR_LAYER_RADAR, true));
    m_pEcdisWin->itfAddLayer(pRadarLayer);

    std::shared_ptr<qt::MapLayer> pRadarCurrentLayer(new qt::MapLayer(ZCHX::LAYER_RADAR_CURRENT,ZCHX::TR_LAYER_RADAR_CURRENT,true));
    m_pEcdisWin->itfAddLayer(pRadarCurrentLayer);

    std::shared_ptr<qt::MapLayer> pRadarTrackLayer(new qt::MapLayer(ZCHX::LAYER_RADAR_TRACK,ZCHX::TR_LAYER_RADAR_TRACK,true));
    m_pEcdisWin->itfAddLayer(pRadarTrackLayer);


    //雷达回波
    std::shared_ptr<qt::MapLayer> pRadarVideo(new qt::MapLayer(ZCHX::LAYER_RADARVIDEO,ZCHX::TR_LAYER_RADARVIDEO, true));
    m_pEcdisWin->itfAddLayer(pRadarVideo);

    //历史雷达和AIS
    std::shared_ptr<qt::MapLayer> pHistoryAis(new qt::MapLayer(ZCHX::LAYER_HISTORY_AIS,ZCHX::TR_LAYER_HISTORY_AIS,true));
    m_pEcdisWin->itfAddLayer(pHistoryAis);


    //本地标注
    std::shared_ptr<qt::MapLayer> localMarkLayer(new qt::MapLayer(ZCHX::LAYER_LOCALMARK, ZCHX::TR_LAYER_LOCALMARK, true));
    m_pEcdisWin->itfAddLayer(localMarkLayer);


    std::shared_ptr<qt::MapLayer> radar_rect_layer(new qt::MapLayer(ZCHX::LAYER_RADARRECT, ZCHX::TR_LAYER_RADARRECT, true));
    m_pEcdisWin->itfAddLayer(radar_rect_layer);


    std::shared_ptr<qt::MapLayer> pointLayer(new qt::MapLayer(ZCHX::LAYER_POINT,ZCHX::TR_LAYER_POINT, true));
    m_pEcdisWin->itfAddLayer(pointLayer);
    std::shared_ptr<qt::MapLayer> lineLayer(new qt::MapLayer(ZCHX::LAYER_LINE,ZCHX::TR_LAYER_LINE,true));
    m_pEcdisWin->itfAddLayer(lineLayer);
    std::shared_ptr<qt::MapLayer> polygonLayer(new qt::MapLayer(ZCHX::LAYER_POLYGON,ZCHX::TR_LAYER_POLYGON,true));
    m_pEcdisWin->itfAddLayer(polygonLayer);

    m_pEcdisWin->setCtrlFrameVisible(false);
    m_pEcdisWin->itfSetRadarLabelVisible(true);
#endif
    //数据接入
    mDataChange = new ZCHX_RADAR_RECEIVER::ZCHXRadarDataChange(this);
    connect(mDataChange, &ZCHX_RADAR_RECEIVER::ZCHXRadarDataChange::sendConnectionStatus, this, [=](bool sts, const QString& error)
    {
        QString msg = QString("connect to server status:%1 --- %2 --- %3")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                .arg(sts)
                .arg(error);
        this->statusBar()->showMessage(msg);
    });
    connect(mDataChange, &ZCHX_RADAR_RECEIVER::ZCHXRadarDataChange::sendRadarRect, this,
            [=](int site, const QList<ZCHX::Data::ITF_RadarRect>& list){
//            QString msg = QString("Radar Rect:%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
//            this->statusBar()->showMessage(msg);
            m_pEcdisWin->itfSetRadarRect(site, list);
    });
//    connect(change, SIGNAL(sendRadarRect(int,QList<ZCHX::Data::ITF_RadarRect>)),
//            m_pEcdisWin, SLOT(itfSetRadarRect(int,QList<ZCHX::Data::ITF_RadarRect>)));
    connect(mDataChange, SIGNAL(sendRadarPoint(int,QList<ZCHX::Data::ITF_RadarPoint>)),
            m_pEcdisWin, SLOT(itfSetRadarPointData(int,QList<ZCHX::Data::ITF_RadarPoint>)));
    connect(mDataChange, SIGNAL(sendRadarVideo(int,double,double,double,int,int,int,QByteArray,QByteArray)),
            this, SLOT(slotSetRadarVideoWholeData(int,double,double,double,int,int,int,QByteArray,QByteArray)));
    connect(mDataChange, SIGNAL(sendAisDataList(QList<ZCHX::Data::ITF_AIS>)),
            m_pEcdisWin, SLOT(itfSetAisData(QList<ZCHX::Data::ITF_AIS>)));
    connect(mDataChange, SIGNAL(sendLimitDataList(QList<ZCHX::Data::ITF_IslandLine>)),
            m_pEcdisWin, SLOT(itfSetIslandLineData(QList<ZCHX::Data::ITF_IslandLine>)));
    connect(mDataChange, SIGNAL(sendAisChart(ZCHX::Data::ITF_AIS_Chart)),
            m_pEcdisWin, SLOT(itfAppendAisChart(ZCHX::Data::ITF_AIS_Chart)));


#if 0
    QHBoxLayout* hLayout = new QHBoxLayout;
    ((QVBoxLayout*) (ui->centralwidget->layout()))->addLayout(hLayout);
    QCheckBox *point = new QCheckBox(QStringLiteral("雷达目标"));
    point->setChecked(getLayerDisplay(ZCHX::LAYER_RADAR));    
    connect(point, SIGNAL(clicked(bool)), this, SLOT(slotRadarPointLayerDisplay(bool)));
    hLayout->addWidget(point);


    QCheckBox *video = new QCheckBox(QStringLiteral("雷达回波"));
    connect(video, SIGNAL(clicked(bool)), this, SLOT(slotRadarVideoLayerDisplay(bool)));
    video->setChecked(getLayerDisplay(ZCHX::LAYER_RADARVIDEO));
    hLayout->addWidget(video);


    QCheckBox *rect = new QCheckBox(QStringLiteral("雷达余晖"));
    connect(rect, SIGNAL(clicked(bool)), this, SLOT(slotRadarRectLayerDisplay(bool)));
    rect->setChecked(getLayerDisplay(ZCHX::LAYER_RADARRECT));
    hLayout->addWidget(rect);


    QCheckBox *limit_area = new QCheckBox(QStringLiteral("遮蔽区域"));
    connect(limit_area, SIGNAL(clicked(bool)), this, SLOT(slotRadarLimitAreaDisplay(bool)));
    limit_area->setChecked(getLayerDisplay(ZCHX::LAYER_ISLAND));
    hLayout->addWidget(limit_area);

    QCheckBox *ais_chk = new QCheckBox(QStringLiteral("AIS"));
    connect(ais_chk, SIGNAL(clicked(bool)), this, SLOT(slotAisLayerDisplay(bool)));
    ais_chk->setChecked(getLayerDisplay(ZCHX::LAYER_AIS));
    hLayout->addWidget(ais_chk);

#if 0
    QPushButton *btn = new QPushButton(QStringLiteral("测距"));
    hLayout->addWidget(btn);
    connect(btn, &QPushButton::clicked, this, [=](){m_pEcdisWin->itfToolBarMeasureDistance();});

    QPushButton *mark = new QPushButton(QStringLiteral("坐标打印"));
    hLayout->addWidget(mark);
    connect(mark, SIGNAL(clicked(bool)), this, SLOT(slotSelectPointFileDlg()));
#endif
#endif

    //看门狗线程
    testMapWatchDogThread *dog = new testMapWatchDogThread(this);
    dog->start();

    //添加菜单控制
    mMapSourceMenu = ui->menuBar->addMenu(QStringLiteral("地图数据源指定"));
    QStringList mapSourceTitles;
    mapSourceTitles.append(QStringLiteral("本地地图"));
    mapSourceTitles.append("");
    mapSourceTitles.append(QStringLiteral("谷歌在线地图"));
    for(int i=0; i<=2; i+=2)
    {
        QAction* act = mMapSourceMenu->addAction(mapSourceTitles[i]);
        act->setData(i);
        connect(act, SIGNAL(triggered(bool)), this, SLOT(slotSetMapSource()));
        act->setCheckable(true);
    }
    mDataSourceMenu = ui->menuBar->addMenu(QStringLiteral("数据接入设定"));
    //开始设定默认的配置文件
    mSetting = new testMapSettings("test_map_setting.ini");
    for(int i=0; i<Data_Reserved; i++)
    {
        dataSource data;
        data.mId = i;
        QString title, sec;
        bool sts = false;
        if(i == Data_Ais)
        {
            title = QStringLiteral("Ais");
            sec = SEC_AIS;
        } else if(i == Data_Ais_Chart)
        {
            title = QStringLiteral("Ais盲区示意图");
            sec = SEC_AIS_CHART;
        } else if(i == Data_Radar_Track)
        {
            title = QStringLiteral("雷达目标");
            sec = SEC_RADAR_TRACK;
        } else if(i == Data_Radar_Video)
        {
            title = QStringLiteral("雷达回波图像");
            sec = SEC_RADAR_VIDEO;
        } else if(i == Data_radar_Rect)
        {
            title = QStringLiteral("雷达目标余晖");
            sec = SEC_RADAR_RECT;
        } else if(i == Data_Radar_Limit)
        {
            title = QStringLiteral("雷达屏蔽区域");
            sec = SEC_RADAR_LIMIT;
        }
        if(title.isEmpty()) continue;
        sts = mSetting->getUserValue(sec, STATUS, false).toBool();
        data.mStatus = sts;
        data.mTitle = title;

        QAction* act = mDataSourceMenu->addAction(data.mTitle);
        act->setData(QVariant::fromValue(data));
        connect(act, SIGNAL(toggled(bool)), this, SLOT(slotSetDataSource(bool)));
        act->setCheckable(true);
        act->setChecked(data.mStatus);

//        mDataSourceMap[data.mId] = data;
    }

    if(0)
    {
        QList<ZCHX::Data::ITF_LocalMark> marks;
        int k = 0;
        QList<PointData> pnts;
        PointData data;
        data.lat = 22.123456;
        data.lon = 113.123456;
        data.name = QStringLiteral("测我是");
        pnts.append(data);
        foreach (PointData point, pnts) {
            ZCHX::Data::ITF_LocalMark mark;
            mark.ll.lat = point.lat;
            mark.ll.lon = point.lon;
            mark.uuid = 1000;
            mark.name = point.name;
            marks.append(mark);
            k++;
        }
        m_pEcdisWin->itfSetLocalMarkData(marks);
    }

    QAction *btn = new QAction(QStringLiteral("测距"), this);
    ui->menuBar->addAction(QStringLiteral("测距"),this, SLOT(slotTestDistance()));

    QAction *btn2 = new QAction(QStringLiteral("预推区域"), this);
    ui->menuBar->addAction(QStringLiteral("预推区域"),this, SLOT(slotTestPrediction()));


//    ZCHX::Data::ITF_ElePos p1;
//    p1.ll.lat = 21.600171;
//    p1.ll.lon = 111.806594;
//    p1.brush = Qt::red;
//    p1.pen.setColor(Qt::black);
//    p1.radius = 1;
//    p1.name = "P1";

//    ZCHX::Data::ITF_ElePos p2;
//    p2.ll.lat = 21.600676;
//    p2.ll.lon = 111.806412;
//    p2.brush = Qt::red;
//    p2.pen.setColor(Qt::black);
//    p2.radius = 1;
//    p2.name = "P2";

//    ZCHX::Data::ITF_ElePos p3;
//    p3.ll.lat = 21.600342;
//    p3.ll.lon = 111.806775;
//    p3.brush = Qt::red;
//    p3.pen.setColor(Qt::black);
//    p3.radius = 1;
//    p3.name = "P3";

//    m_pEcdisWin->itfAppendPoint(p1);
//    m_pEcdisWin->itfAppendPoint(p2);
//    m_pEcdisWin->itfAppendPoint(p3);
}

void TestMainWindow::slotTestDistance()
{;
    m_pEcdisWin->itfToolBarMeasureDistance();
}

TestMainWindow::~TestMainWindow()
{
    if(m_pEcdisWin) delete m_pEcdisWin;
    delete ui;
}

void TestMainWindow::slotSelectPointFileDlg()
{
    QString file = QFileDialog::getOpenFileName();
    if(file.isEmpty()) return;
    QList<PointData> pnts;
    getLonlatListFromFile(file, pnts);
    qDebug()<<"total mark list:"<<pnts.size();
    //现在将点列打印地图
    static int i=0;

    i++;
    static QList<ZCHX::Data::ITF_LocalMark> marks;
    int k = 0;
    foreach (PointData point, pnts) {
        ZCHX::Data::ITF_LocalMark mark;
        mark.ll.lat = point.lat;
        mark.ll.lon = point.lon;
        mark.uuid = i*100 + k;
        mark.name = point.name;
        marks.append(mark);
        k++;
    }
    m_pEcdisWin->itfSetLocalMarkData(marks);

}

void TestMainWindow::getLonlatListFromFile(const QString &fileName, QList<PointData>& list)
{
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)) return;
    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine());
        QStringList content = line.split(",");
        if(content.size() != 3) continue;
        PointData data;
        data.lat = content[0].toDouble();
        data.lon = content[1].toDouble();
        data.name = content[2];
        list.append(data);
    }
    file.close();
}

void TestMainWindow::slotRadarPointLayerDisplay(bool sts)
{
    std::shared_ptr<qt::MapLayer> layer = m_pEcdisWin->itfGetLayer(ZCHX::LAYER_RADAR);
    if(layer) layer->setVisible(sts);
    if(mDataChange) mDataChange->slotSetThreadStatus(ZCHX_RADAR_RECEIVER::ZCHX_RECV_RADAR_POINT, sts);
    mSetting->setUserValue(SEC_RADAR_TRACK, STATUS, sts);
}

void TestMainWindow::slotRadarVideoLayerDisplay(bool sts)
{
    std::shared_ptr<qt::MapLayer> layer = m_pEcdisWin->itfGetLayer(ZCHX::LAYER_RADARVIDEO);
    if(layer) layer->setVisible(sts);
    if(mDataChange) mDataChange->slotSetThreadStatus(ZCHX_RADAR_RECEIVER::ZCHX_RECV_RADAR_VIDEO, sts);
    mSetting->setUserValue(SEC_RADAR_VIDEO, STATUS, sts);
}

void TestMainWindow::slotRadarRectLayerDisplay(bool sts)
{
    std::shared_ptr<qt::MapLayer> layer = m_pEcdisWin->itfGetLayer(ZCHX::LAYER_RADARRECT);
    if(layer) layer->setVisible(sts);
    if(mDataChange) mDataChange->slotSetThreadStatus(ZCHX_RADAR_RECEIVER::ZCHX_RECV_RADAR_RECT, sts);
    mSetting->setUserValue(SEC_RADAR_RECT, STATUS, sts);
}

void TestMainWindow::slotAisLayerDisplay(bool sts)
{
    std::shared_ptr<qt::MapLayer> layer = m_pEcdisWin->itfGetLayer(ZCHX::LAYER_AIS);
    if(layer) layer->setVisible(sts);

    if(mDataChange) mDataChange->slotSetThreadStatus(ZCHX_RADAR_RECEIVER::ZCHX_RECV_AIS, sts);
    mSetting->setUserValue(SEC_AIS, STATUS, sts);
}

void TestMainWindow::slotAisChartLayerDisplay(bool sts)
{
    std::shared_ptr<qt::MapLayer> layer = m_pEcdisWin->itfGetLayer(ZCHX::LAYER_AIS_CHART);
    if(layer) layer->setVisible(sts);

    if(mDataChange) mDataChange->slotSetThreadStatus(ZCHX_RADAR_RECEIVER::ZCHX_RECV_AIS_CHART, sts);
    mSetting->setUserValue(SEC_AIS_CHART, STATUS, sts);
}

void TestMainWindow::slotRadarLimitAreaDisplay(bool sts)
{
    std::shared_ptr<qt::MapLayer> layer = m_pEcdisWin->itfGetLayer(ZCHX::LAYER_ISLAND);
    if(layer) layer->setVisible(sts);
    if(mDataChange) mDataChange->slotSetThreadStatus(ZCHX_RADAR_RECEIVER::ZCHX_RECV_LIMIT_AREA, sts);
    mSetting->setUserValue(SEC_RADAR_LIMIT, STATUS, sts);
}

void TestMainWindow::slotTimerout()
{
    //开始构造数据
    ZCHX::Data::ITF_RadarRect rect;
    rect.blockColor = Qt::yellow;
    rect.blockEdgeColor = Qt::darkGray;
    rect.HisBlockColor.setRgb(0,78,183);
    rect.rectNumber = 1005;
    ZCHX::Data::LatLon ll[] = {
        {22.212637,113.122392},
        {22.213532,113.122477},
        {22.211724,113.121148},
        {22.212614,113.121283},
        {22.211247,113.120504},
        {22.212129,113.120689},
        {22.210730,113.119860},
        {22.211602,113.120095},
        {22.210134,113.119174},
        {22.210993,113.119458},
        {22.209618,113.118358},
        {22.210462,113.118690},
        {22.208744,113.117243},
        {22.209571,113.117622},
        {22.207750,113.115784},
        {22.208557,113.116209},
        {22.206837,113.114281},
        {22.207623,113.114751},
        {22.205923,113.112908},
        {22.206685,113.113422},
        {22.204930,113.111621},
        {22.205666,113.112178},
        {22.203698,113.110076},
        {22.204406,113.110673},
        {22.202824,113.109089},
        {22.203502,113.109726},
        {22.202228,113.107973},
        {22.202874,113.108647},
        {22.201314,113.106557},
        {22.201927,113.107267},
        {22.200440,113.105827},
        {22.201017,113.106570},
        {22.199605,113.104497},
        {22.200146,113.105272},
        {22.198612,113.103510},
        {22.199114,113.104314},
        {22.197738,113.102995},
        {22.198201,113.103827},
        {22.196903,113.102480},
        {22.197325,113.103337},
        {22.196109,113.102094},
        {22.196489,113.102973},
        {22.195314,113.101922},
        {22.195651,113.102822},
        {22.194082,113.101536},
        {22.194374,113.102453},
        {22.192373,113.101021},
        {22.192621,113.101954},
        {22.191976,113.100334},
        {22.192178,113.101279},
        {22.190228,113.098102},
        {22.190384,113.099057},
        {22.189632,113.096858},
        {22.189741,113.097821},
        {22.188797,113.095742},
        {22.188860,113.096710},
        {22.188042,113.094626},
        {22.188058,113.095596},
        {22.187247,113.093296},
        {22.187216,113.094266}
    };
    int size = sizeof(ll) / sizeof(ZCHX::Data::LatLon);
//    qDebug()<<"size = "<<size;
    static int index = 0;
    index++;
    for(int i=0; i<size; i=i+2)
    {
//        qDebug()<<"ll"<<ll[i].lat<<ll[i+1].lon;
        ZCHX::Data::ITF_RadarRectDef his;
        his.rectNumber = 1005;
        his.startlatitude = ll[i].lat;
        his.startlongitude = ll[i].lon;
        his.endlatitude = ll[i+1].lat;
        his.endlongitude = ll[i+1].lon;
        his.centerlatitude = (his.startlatitude + his.endlatitude)/2;
        his.centerlongitude = (his.startlongitude + his.endlongitude) / 2;
        his.isRealData= true;
        his.angle = ZCHX::Utils::calcAzimuth(his.startlongitude, his.startlatitude, his.endlongitude, his.endlatitude);
        his.timeOfDay = QDateTime::currentDateTime().toTime_t();
        rect.rects.prepend(his);
//        if(i >= index) break;
    }
    rect.current = rect.rects.first();
    rect.rects.removeFirst();;

    QList<ZCHX::Data::ITF_RadarRect> list;
    list.append(rect);
    m_pEcdisWin->itfSetRadarRect(2, list);
//    m_pEcdisWin->setMapCenter(rect.current.centerlatitude, rect.current.centerlongitude);
    if(index >= size) index = 0;


}

void TestMainWindow::slotSetRadarVideoWholeData(int siteID, double lon, double lat, double dis, int type, int loop, int curIndex, const QByteArray &objPixmap, const QByteArray &prePixMap)
{
    m_pEcdisWin->itfSetRadarVideoData(siteID, lon, lat, dis, type, loop);
    m_pEcdisWin->itfSetRadarVideoPixmap(siteID, curIndex,objPixmap, prePixMap);
}

bool TestMainWindow::getLayerDisplay(const QString &layerName)
{
    std::shared_ptr<qt::MapLayer> layer = m_pEcdisWin->itfGetLayer(layerName);
    if(layer) return layer->visible();
    return false;
}

void TestMainWindow::slotSetMapSource()
{
    QAction *act = qobject_cast<QAction*> (sender());
    if(!act) return;
    QList<QAction*> actList = mMapSourceMenu->actions();
    foreach (QAction* temp, actList) {
        if(temp == act) temp->setChecked(true);
        else temp->setChecked(false);
    }

    int mode = act->data().toInt();
    if(m_pEcdisWin) m_pEcdisWin->setEcdisMapSource(ZCHX::TILE_SOURCE(mode));
}

void TestMainWindow::slotSetDataSource(bool sts)
{
    QAction *act = qobject_cast<QAction*> (sender());
    if(!act) return;
    dataSource data = act->data().value<dataSource>();
    data.mStatus = sts;
    act->setData(QVariant::fromValue(data));
    qDebug()<<data.mId<<data.mTitle<<data.mStatus;

    //根据不同的ID进行状态更新
    int i = data.mId;
    if(i == Data_Ais)
    {
        ZCHX_RADAR_RECEIVER::ZCHX_Radar_Setting_Param param;
        param.m_sIP = mSetting->getUserValue(SEC_AIS, SERVER).toString();
        param.m_sPort = mSetting->getUserValue(SEC_AIS, PORT).toString();
        param.m_sTopicList.append(mSetting->getUserValue(SEC_AIS, TOPIC).toString());
        param.m_sSiteID = 1;
        mDataChange->appendAis(param);
        slotAisLayerDisplay(sts);

    } else if(i == Data_Ais_Chart)
    {
        ZCHX_RADAR_RECEIVER::ZCHX_Radar_Setting_Param param;
        param.m_sIP = mSetting->getUserValue(SEC_AIS_CHART, SERVER).toString();
        param.m_sPort = mSetting->getUserValue(SEC_AIS_CHART, PORT).toString();
        param.m_sTopicList.append(mSetting->getUserValue(SEC_AIS_CHART, TOPIC).toString());
        param.m_sSiteID = 1;
        mDataChange->appendAisChart(param);
        slotAisChartLayerDisplay(sts);

    } else if(i == Data_Radar_Track)
    {
        ZCHX_RADAR_RECEIVER::ZCHX_Radar_Setting_Param param;
        param.m_sIP = mSetting->getUserValue(SEC_RADAR_TRACK, SERVER).toString();
        param.m_sPort = mSetting->getUserValue(SEC_RADAR_TRACK, PORT).toString();
        param.m_sTopicList.append(mSetting->getUserValue(SEC_RADAR_TRACK, TOPIC).toString());
        param.m_sSiteID = 1;
        mDataChange->appendRadarPoint(param);
        slotRadarPointLayerDisplay(sts);

    } else if(i == Data_Radar_Video)
    {
        ZCHX_RADAR_RECEIVER::ZCHX_Radar_Setting_Param param;
        param.m_sIP = mSetting->getUserValue(SEC_RADAR_VIDEO, SERVER).toString();
        param.m_sPort = mSetting->getUserValue(SEC_RADAR_VIDEO, PORT).toString();
        param.m_sTopicList.append(mSetting->getUserValue(SEC_RADAR_VIDEO, TOPIC).toString());
        param.m_sSiteID = 1;

        mDataChange->appendRadarVideo(param);
        slotRadarVideoLayerDisplay(sts);

    } else if(i == Data_radar_Rect)
    {
        //先将线程添加进去
        ZCHX_RADAR_RECEIVER::ZCHX_RadarRect_Param param;
        param.mSetting.m_sIP = mSetting->getUserValue(SEC_RADAR_RECT, SERVER).toString();
        param.mSetting.m_sPort = mSetting->getUserValue(SEC_RADAR_RECT, PORT).toString();
        param.mSetting.m_sTopicList.append(mSetting->getUserValue(SEC_RADAR_RECT, TOPIC).toString());
        param.mSetting.m_sSiteID = 1;
        param.m_sCurColor = mSetting->getUserValue(SEC_RADAR_RECT, CURCOLOR).toString();
        param.m_sEdgeColor = "#c0c0c0";
        param.m_sHistoryColor = "#0000ff";
        param.m_sHistoryBackgroundColor = mSetting->getUserValue(SEC_RADAR_RECT, HISCOLOR).toString();
        mDataChange->appendRadarRect(param);
        slotRadarRectLayerDisplay(sts);

    } else if(i == Data_Radar_Limit)
    {
        ZCHX_RADAR_RECEIVER::ZCHX_Radar_Setting_Param param;
        param.m_sIP = mSetting->getUserValue(SEC_RADAR_LIMIT, SERVER).toString();
        param.m_sPort = mSetting->getUserValue(SEC_RADAR_LIMIT, PORT).toString();
        param.m_sTopicList.append(mSetting->getUserValue(SEC_RADAR_LIMIT, TOPIC).toString());
        param.m_sSiteID = 1;
        mDataChange->appendLimit(param);
        slotRadarLimitAreaDisplay(sts);
    }
}

void TestMainWindow::slotTestPrediction()
{
    static double angle = 0;
    //测试目标的预测区域
    Latlon start(21.600171, 111.806594);
    Latlon end(21.600271, 111.805647);
    double cog = Mercator::angle(start.lat, start.lon, end.lat, end.lon);

    //计算目标的运动位置
#if 1
    double cur_max_speed = 5.0;
    double est_distance = cur_max_speed * 4;
    double est_lat = 0.0, est_lon = 0.0;
    ZCHX::Utils::distbear_to_latlon(end.lat, end.lon, est_distance, cog, est_lat, est_lon);
    Latlon next(est_lat, est_lon);

#else
    double est_lat = 0, est_lon = 0;
    QGeoCoordinate test(start.lat, start.lon);
    QGeoCoordinate result = test.atDistanceAndAzimuth(200, angle);
    Latlon end(result.latitude(),  result.longitude());
#endif
    zchxTargetPredictionLine prediction(end, next, 40, Prediction_Area_Rectangle);

    ZCHX::Data::ITF_ElePos p1;
    p1.ll.lat = start.lat;
    p1.ll.lon = start.lon;
    p1.brush = Qt::red;
    p1.pen.setColor(Qt::black);
    p1.radius = 3;
    p1.name = "P1";

    ZCHX::Data::ITF_ElePos p2;
    p2.ll.lat = end.lat;
    p2.ll.lon = end.lon;
    p2.brush = Qt::red;
    p2.pen.setColor(Qt::black);
    p2.radius = 3;
    p2.name = "P2";

    ZCHX::Data::ITF_ElePos p3;
    p3.ll.lat = next.lat;
    p3.ll.lon = next.lon;
    p3.brush = Qt::red;
    p3.pen.setColor(Qt::black);
    p3.radius = 3;
    p3.name = "P3";

    QList<ZCHX::Data::ITF_ElePos> poslist;
    poslist<<p1<<p2<<p3;
    m_pEcdisWin->itfAppendPoint(poslist);

    double len1 = ZCHX::Utils::getDistanceDeg(start.lat, start.lon, end.lat, end.lon);
    double len2 = ZCHX::Utils::getDistanceDeg(end.lat, end.lon, next.lat, next.lon);

    qDebug()<<"len:"<<len1<<len2;

    QList<ZCHX::Data::ITF_EleLine> linelist;

    ZCHX::Data::ITF_EleLine line;
    line.brush = Qt::transparent;
    line.pen.setColor(Qt::red);
    line.ll1 = p1.ll;
    line.ll2 = p2.ll;
    line.name = "P1--P2";
    linelist.append(line);
    //m_pEcdisWin->itfAppendLine(line);

    ZCHX::Data::ITF_EleLine line2;
    line2.brush = Qt::transparent;
    line2.pen.setColor(Qt::red);
    line2.ll1 = p2.ll;
    line2.ll2 = p3.ll;
    line2.name = "P2--P3";
    linelist.append(line2);
    m_pEcdisWin->itfAppendLine(linelist);

    QList<ZCHX::Data::ITF_ElePolygon> polylist;

    ZCHX::Data::ITF_ElePolygon poly;
    poly.fillColor = Qt::transparent;
    poly.lineColor = Qt::green;
    QList<Latlon> list =  prediction.getPredictionArea();
    for(int i=0; i<list.size(); i++)
    {
        poly.path.append(ZCHX::Data::LatLon(list[i].lat, list[i].lon));
    }
    poly.name = "PolyArea";
    polylist.append(poly);
    m_pEcdisWin->itfAppendPolygon(polylist);

    angle += 30;
}
