#include "radartestwindow.h"
#include "ui_radartestwindow.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>

RadarTestWindow::RadarTestWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RadarTestWindow)
{
    ui->setupUi(this);
    m_pEcdisWin = new MainWindow_1;
    ui->centralwidget->layout()->addWidget(m_pEcdisWin);


    if(!m_pEcdisWin) return;
    //显示地图
    m_pEcdisWin->setMapCenterAndZoom(20, 113, 10);
    //添加地图的图层控制
    //添加AIS图层
    addMapLayer(ZCHX::LAYER_AIS,ZCHX::TR_LAYER_AIS,true);
    addMapLayer(ZCHX::LAYER_RADAR,ZCHX::TR_LAYER_RADAR,true);
    addMapLayer(ZCHX::LAYER_RADAR_CURRENT,ZCHX::TR_LAYER_RADAR_CURRENT,true);
    connect(m_pEcdisWin, SIGNAL(itfSignalSendCurPos(double, double)), this, SLOT(slotRecvCurPos(double,double)));
    qDebug()<<" map load finished!!";
    RadarRecvThread* th = new RadarRecvThread(this);
    connect(th, SIGNAL(signalSendTrackPointList(TrackPointList)), this, SLOT(slotRecvRadarList(TrackPointList)));
    th->start();
    QTimer *test = new QTimer;
    test->setInterval(3000);
    connect(test, SIGNAL(timeout()), this, SLOT(slotTimeOut()));
    //test->start();
    label = new QLabel(this);

    ui->statusbar->setLayout(new QHBoxLayout());
    ui->statusbar->layout()->addWidget(label);
    ui->statusbar->addWidget(label);
    QPushButton* disBtn = new QPushButton(tr("测距"), this);
    ui->statusbar->addWidget(disBtn);
    connect(disBtn, SIGNAL(clicked(bool)), this, SLOT(slotDistanceMeasureEnable(bool)));

    QPushButton* moveBtn = new QPushButton(tr("移动"), this);
    ui->statusbar->addWidget(moveBtn);
    connect(moveBtn, SIGNAL(clicked(bool)), this, SLOT(slotMove()));
}

void RadarTestWindow::slotRecvCurPos(double lat, double lon)
{
    label->setText(QString("%1, %2").arg(QString::number(lon, 'f', 10)).arg(QString::number(lat, 'f', 10)));
}

RadarTestWindow::~RadarTestWindow()
{
    delete ui;
}


void RadarTestWindow::addMapLayer(const QString &name, const QString &displayName, bool visible)
{
    if(m_pEcdisWin)
    {
        std::shared_ptr<qt::MapLayer> layer(new qt::MapLayer(name,displayName,visible));
        m_pEcdisWin->itfAddLayer(layer);
    }
}

void RadarTestWindow::slotTimeOut()
{
//    TrackPointList list;
//    for(int i=0; i<10; i++)
//    {
//        TrackPoint* point = list.add_tracks();
//        point->set_tracknumber(1000+i);
//        point->set_wgs84poslong(110 + 0.512435 * i * 0.01);
//        point->set_wgs84poslat(20 + 0.123456 * i *0.01);
//        point->set_cog(1.0);
//        point->set_sog(2.0);
//    }

//    slotRecvRadarList(list);
}

void RadarTestWindow::slotRecvRadarList(const TrackPointList &dataList)
{
    QList<ZCHX::Data::ITF_RadarPoint> list;
    for(int i=0; i<dataList.size(); i++)
    {
        ZCHX::Data::tagITF_RadarPoint item;
        TrackPoint data = dataList[i];
        std::vector<std::pair<double,double>> path = {
            std::pair<double,double>(data.wgs84poslat(),data.wgs84poslong())
        };
        item.lat = data.wgs84poslat();
        item.lon = data.wgs84poslong();
        item.path = path;
        item.trackNumber = data.tracknumber();
        item.cog = data.cog();
        item.sog = data.sog();
        item.status = 0;
        item.warnStatusColor = Qt::red;
        //qDebug()<<QString("%1, %2").arg(QString::number(item.lon, 'f', 10)).arg(QString::number(item.lat, 'f', 10));

        list.append(item);
    }
    m_pEcdisWin->itfSetRadarPointData(list);

}

void RadarTestWindow::slotDistanceMeasureEnable(bool sts)
{
    m_pEcdisWin->itfToolBarMeasureDistance();
}

void RadarTestWindow::slotMove()
{
    m_pEcdisWin->itfToolBarRoam();
}
