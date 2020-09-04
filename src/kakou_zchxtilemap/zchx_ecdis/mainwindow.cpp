#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zchxmapthread.h"
#include "map_layer/zchxmaplayermgr.h"
#include "data_manager/zchxdatamgrfactory.h"
#include "zchxroutedatamgr.h"
#include "zchxshipplandatamgr.h"
#include <QDebug>
#include <QLabel>
#include <QHBoxLayout>
#include <QSpacerItem>

namespace qt {
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mMapWidget(0)
{
    ui->setupUi(this);
    QString style = QString("background-color:%1;").arg(Profiles::instance()->value(MAP_INDEX, MAP_BACK_GROUND).toString());
    qDebug()<<"style:"<<style;
    ui->ecdis_frame->setStyleSheet(style);
    QPixmapCache::setCacheLimit(1);
//    ui->statusBar->setVisible(false);
    ui->source->addItem(tr("local map"), ZCHX::TILE_TMS_LOCAL_MAP);
    ui->source->addItem(tr("local image"), ZCHX::TILE_TMS_LOCAL_IMG);
    ui->source->addItem(tr("google map online"), ZCHX::TILE_GOOGLE_ONLINE);
    ui->progressBar->setVisible(false);
    ui->download->setVisible(false);
    ui->load_frame->setVisible(false);
    ui->pos_frame->setVisible(false);
    ui->ctrl_frame->setVisible(false);    

    m_dataMgrFactory = new zchxDataMgrFactory();

    mMapWidget = new zchxMapWidget(m_dataMgrFactory, this);
    ui->ecdis_frame->layout()->addWidget(mMapWidget);

    m_mapLayerMgr = new MapLayerMgr();
    m_mapLayerMgr->setDrawWidget(mMapWidget);
    m_mapLayerMgr->loadEcdisLayers();

    mMapWidget->setLayerMgr(m_mapLayerMgr);

    connect(mMapWidget, SIGNAL(signalDisplayCurPos(double,double)), this, SLOT(slotUpdateCurrentPos(double,double)));
    connect(mMapWidget, SIGNAL(signalSendNewMap(double,double,int)), this, SLOT(slotDisplayMapCenterAndZoom(double,double,int)));
    initSignalConnect();
}

MainWindow::~MainWindow()
{
    if (m_mapLayerMgr != NULL)
    {
        delete m_mapLayerMgr;
        m_mapLayerMgr = NULL;
    }

    if (m_dataMgrFactory != NULL)
    {
        delete m_dataMgrFactory;
        m_dataMgrFactory = NULL;
    }

    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    //loadEcdis();
    emit itfSignalIsEcdisResize();
}

//void MainWindow::loadEcdis()
//{
//    if(!mStartLoad)
//    {
//        mStartLoad = true;
//    }
//    if(mMapWidget) mMapWidget->setGeometry(0, 0, ui->ecdis_frame->size().width(), ui->ecdis_frame->size().height());
//}



void MainWindow::on_load_clicked()
{
    //QRect rect =  ui->ecdis->geometry();
    double lon = ui->lon->text().toDouble();
    double lat = ui->lat->text().toDouble();
    int zoom = ui->zoom->text().toInt();
    if(mMapWidget) mMapWidget->setCenterAndZoom(ZCHX::Data::LatLon(lat, lon), zoom);
}

void MainWindow::slotUpdateCurrentPos(double lon, double lat)
{
    ui->pos_label->setText(QString("%1, %2").arg(lon, 0, 'f', 6).arg(lat, 0, 'f', 6));
    emit itfSignalSendCurPos(lat, lon);
}

void MainWindow::slotDisplayMapCenterAndZoom(double lon, double lat, int zoom)
{
    ui->lat->setText(FLOAT_STRING(lat, 6));
    ui->lon->setText(FLOAT_STRING(lon, 6));
    ui->zoom->setText(INT_STRING(zoom));
}

void MainWindow::on_download_clicked()
{

}

void MainWindow::slotUpdateDownloadProgress(int val)
{
    static int num = 1;
    ui->progressBar->setValue(num++);
    if(num == 23)
    {
        num = 1;
    }
}

void MainWindow::on_source_activated(const QString &arg1)
{
}

void MainWindow::on_source_currentIndexChanged(int index)
{
    if(mMapWidget) mMapWidget->setSource(ui->source->currentData().toInt());
}

void MainWindow::on_image_num_clicked(bool checked)
{
    if(mMapWidget) mMapWidget->setImgNumberVisible(checked);
}

void MainWindow::initSignalConnect()
{
    connect(mMapWidget, SIGNAL(signalCreateRouteLINE(ZCHX::Data::RouteLine)),this,SIGNAL(itfCreateRouteLine(ZCHX::Data::RouteLine)));
    connect(mMapWidget, SIGNAL(signalCancelRouteLine()), this, SIGNAL(itfCancelAddRouteLine()));
    connect(mMapWidget, SIGNAL(signalCancelChangeRouteLine()), this, SIGNAL(itfCancelChangeRouteLine()));
    connect(mMapWidget, SIGNAL(signalCancelChangeShipPlanLine()), this, SIGNAL(itfCancelChangeShipPlanLine()));
    connect(mMapWidget, SIGNAL(signalNoRouteLineSelected()),this,SIGNAL(itfNoRouteLineSelected()));
    connect(mMapWidget, SIGNAL(signalNoShipPlanSelected()), this, SIGNAL(itfNoShipPlanLineSelected()));
    connect(mMapWidget, SIGNAL(signalCreateRadarFeatureZONE(ZCHX::Data::ITF_RadarFeaturesZone)),this,SIGNAL(itfCreateRadarFeatureZONE(ZCHX::Data::ITF_RadarFeaturesZone)));
    connect(mMapWidget, SIGNAL(signalCreateShipPlanLine(ZCHX::Data::ShipPlanLine)),this,SIGNAL(itfCreateShipPlanLine(ZCHX::Data::ShipPlanLine)));
    connect(mMapWidget, SIGNAL(signalIsSelected4TrackRadarOrbit(ZCHX::Data::ITF_AIS,bool)), this,SIGNAL(itfSignalIsSelected4TrackAis(ZCHX::Data::ITF_AIS,bool)));
    connect(mMapWidget, SIGNAL(signalIsEcdisScales(double)), this, SIGNAL(itfSignalIsEcdisScales(double)));
    connect(mMapWidget, SIGNAL(signalIsEcdisDoubleClickCoordinate(double, double)), this, SIGNAL(itfSignalIsEcdisDoubleClickCoordinate(double, double)));
    connect(mMapWidget, SIGNAL(signalCreateWarringZONE(ZCHX::Data::ITF_WarringZone)),this,SIGNAL(itfCreateWarringZONE(ZCHX::Data::ITF_WarringZone)));
    connect(mMapWidget, SIGNAL(signalCreateWarringLINE(ZCHX::Data::ITF_WarringZone)),this,SIGNAL(itfCreateWarringLine(ZCHX::Data::ITF_WarringZone)));
    connect(mMapWidget, SIGNAL(signalCreateIslandLINE(ZCHX::Data::ITF_IslandLine)),this,SIGNAL(itfCreateIslandLine(ZCHX::Data::ITF_IslandLine)));
    connect(mMapWidget, SIGNAL(signalCreateLocalMark(ZCHX::Data::ITF_LocalMark)), this,SIGNAL(itfCreateLocalMark(ZCHX::Data::ITF_LocalMark)));
    connect(mMapWidget, SIGNAL(signalSendCurPos(double,double)), this, SLOT(onSignalSendCurPos(double,double)), Qt::QueuedConnection);
    connect(mMapWidget, SIGNAL(signalIsSelected4CameraRod(ZCHX::Data::ITF_CameraRod)),this, SIGNAL(itfSignalIsSelected4CameraRod(ZCHX::Data::ITF_CameraRod)));
    connect(mMapWidget, SIGNAL(signalIsSelected4IslandLine(ZCHX::Data::ITF_IslandLine)),this,SIGNAL(itfSignalIsSelected4IslandLine(ZCHX::Data::ITF_IslandLine)));
    connect(mMapWidget, SIGNAL(signalIsSelected4LocalMark(ZCHX::Data::ITF_LocalMark)),this,SIGNAL(itfSignalIsSelected4LocalMark(ZCHX::Data::ITF_LocalMark)));
    connect(mMapWidget, SIGNAL(signalIsSelected4PastrolStation(ZCHX::Data::ITF_PastrolStation)),this,SIGNAL(itfSignalIsSelected4PastrolStation(ZCHX::Data::ITF_PastrolStation)));
    connect(mMapWidget, SIGNAL(signalIsSelected4RadarArea(ZCHX::Data::ITF_RadarArea)), this,SIGNAL(itfSignalIsSelected4RadarArea(ZCHX::Data::ITF_RadarArea)));
    connect(mMapWidget, SIGNAL(signalIsSelected4Ais(ZCHX::Data::ITF_AIS,bool,bool)), this,SIGNAL(itfSignalIsSelected4Ais(ZCHX::Data::ITF_AIS,bool,bool)));
    connect(mMapWidget, SIGNAL(signalIsSelected4RadarPoint(ZCHX::Data::ITF_RadarPoint,bool)), this,SIGNAL(itfSignalIsSelected4RadarPoint(ZCHX::Data::ITF_RadarPoint,bool)));
    connect(mMapWidget, SIGNAL(signalIsSelected4WarringZone(ZCHX::Data::ITF_WarringZone)),this,SIGNAL(itfSignalIsSelected4WarringZone(ZCHX::Data::ITF_WarringZone)));
    connect(mMapWidget, SIGNAL(signalIsSelected4WarringLine(ZCHX::Data::ITF_WarringZone)),this,SIGNAL(itfSignalIsSelected4WarringLine(ZCHX::Data::ITF_WarringZone)));
    connect(mMapWidget, SIGNAL(signalSendPointNealyCamera(int,double,double)),this,SIGNAL(itfSendPointNealyCamera(int,double,double)));
    connect(mMapWidget, SIGNAL(signalIsSelected4CameraVideoWarn(ZCHX::Data::ITF_CameraVideoWarn)), this, SIGNAL(itfSignalIsSelected4CameraVideoWarn(ZCHX::Data::ITF_CameraVideoWarn)));
    connect(mMapWidget, SIGNAL(signalIsSelected4CameraDev(ZCHX::Data::ITF_CameraRod)), this, SIGNAL(itfSignalIsSelected4CameraDev(ZCHX::Data::ITF_CameraDev)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4CameraDev(ZCHX::Data::ITF_CameraDev)), this, SIGNAL(signalIsDoubleClicked4CameraDev(ZCHX::Data::ITF_CameraDev)));

    connect(mMapWidget, SIGNAL(signalCreateCoastDataLINE(ZCHX::Data::ITF_CoastData)),this,SIGNAL(itfCreateCoastDataLine(ZCHX::Data::ITF_CoastData)));
    connect(mMapWidget, SIGNAL(signalCreateSeabedPipeLineLINE(ZCHX::Data::ITF_SeabedPipeLine)),this,SIGNAL(itfCreateSeabedPipeLineLine(ZCHX::Data::ITF_SeabedPipeLine)));
    connect(mMapWidget, SIGNAL(signalCreateStructurePoint(ZCHX::Data::ITF_Structure)),this,SIGNAL(itfCreateStructurePoint(ZCHX::Data::ITF_Structure)));
    connect(mMapWidget, SIGNAL(signalCreateAreaNetZone(ZCHX::Data::ITF_AreaNet)),this,SIGNAL(itfCreateAreaNetZone(ZCHX::Data::ITF_AreaNet)));

    connect(mMapWidget, SIGNAL(signalCreateChannelZone(ZCHX::Data::ITF_Channel)),this,SIGNAL(itfCreateChannelZone(ZCHX::Data::ITF_Channel)));
    connect(mMapWidget, SIGNAL(signalIsSelected4ChannelZone(ZCHX::Data::ITF_Channel)),this,SIGNAL(itfSignalIsSelected4ChannelZone(ZCHX::Data::ITF_Channel)));

    connect(mMapWidget, SIGNAL(signalCreateMooringZone(ZCHX::Data::ITF_Mooring)),this,SIGNAL(itfCreateMooringZone(ZCHX::Data::ITF_Mooring)));
    connect(mMapWidget, SIGNAL(signalIsSelected4MooringZone(ZCHX::Data::ITF_Mooring)),this,SIGNAL(itfSignalIsSelected4MooringZone(ZCHX::Data::ITF_Mooring)));

    connect(mMapWidget, SIGNAL(signalCreateCardMouthZone(ZCHX::Data::ITF_CardMouth)),this,SIGNAL(itfCreateCardMouthZone(ZCHX::Data::ITF_CardMouth)));
    connect(mMapWidget, SIGNAL(signalIsSelected4CardMouthZone(ZCHX::Data::ITF_CardMouth)),this,SIGNAL(itfSignalIsSelected4CardMouthZone(ZCHX::Data::ITF_CardMouth)));

    connect(mMapWidget, SIGNAL(signalCreateStatistcLineZone(ZCHX::Data::ITF_StatistcLine)),this,SIGNAL(itfCreateStatistcLineZone(ZCHX::Data::ITF_StatistcLine)));
    connect(mMapWidget, SIGNAL(signalIsSelected4StatistcLineZone(ZCHX::Data::ITF_StatistcLine)),this,SIGNAL(itfSignalIsSelected4StatistcLineZone(ZCHX::Data::ITF_StatistcLine)));

    connect(mMapWidget, SIGNAL(signalSelectPlayZone(std::vector<std::pair<double, double>>)),
            this,SIGNAL(itfSelectPlayZone(std::vector<std::pair<double, double>>)));
    connect(mMapWidget, SIGNAL(signalDrawRadarFilterArea(std::vector<std::pair<double,double> >)),
            this, SIGNAL(itfSelectRadarFilterArea(std::vector<std::pair<double,double> >)));

    connect(mMapWidget, SIGNAL(signalAddFleet(const ZCHX::Data::ITF_AIS&)), this, SIGNAL(itfAddFleet(const ZCHX::Data::ITF_AIS&)));
    connect(mMapWidget, SIGNAL(signalRemoveFleet(const ZCHX::Data::ITF_AIS&)), this, SIGNAL(itfRemoveFleet(const ZCHX::Data::ITF_AIS&)));

    //其他目标的双击信号的关联
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4Ais(ZCHX::Data::ITF_AIS)), this, SIGNAL(signalIsDoubleClicked4Ais(ZCHX::Data::ITF_AIS)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4RadarPoint(ZCHX::Data::ITF_RadarPoint)), this, SIGNAL(signalIsDoubleClicked4RadarPoint(ZCHX::Data::ITF_RadarPoint)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4RadarArea(ZCHX::Data::ITF_RadarArea)), this, SIGNAL(signalIsDoubleClicked4RadarArea(ZCHX::Data::ITF_RadarArea)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4CameraRod(ZCHX::Data::ITF_CameraRod)), this, SIGNAL(signalIsDoubleClicked4CameraRod(ZCHX::Data::ITF_CameraRod)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4IslandLine(ZCHX::Data::ITF_IslandLine)), this, SIGNAL(signalIsDoubleClicked4IslandLine(ZCHX::Data::ITF_IslandLine)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4LocalMark(ZCHX::Data::ITF_LocalMark)), this, SIGNAL(signalIsDoubleClicked4LocalMark(ZCHX::Data::ITF_LocalMark)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4PastrolStation(ZCHX::Data::ITF_PastrolStation)), this, SIGNAL(signalIsDoubleClicked4PastrolStation(ZCHX::Data::ITF_PastrolStation)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4WarringLine(ZCHX::Data::ITF_WarringZone)), this, SIGNAL(signalIsDoubleClicked4WarringLine(ZCHX::Data::ITF_WarringZone)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4WarringZone(ZCHX::Data::ITF_WarringZone)), this, SIGNAL(signalIsDoubleClicked4WarringZone(ZCHX::Data::ITF_WarringZone)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4CameraVideoWarn(ZCHX::Data::ITF_CameraVideoWarn)), this, SIGNAL(signalIsDoubleClicked4CameraVideoWarn(ZCHX::Data::ITF_CameraVideoWarn)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4ChannelZone(ZCHX::Data::ITF_Channel)), this, SIGNAL(signalIsDoubleClicked4ChannelZone(ZCHX::Data::ITF_Channel)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4MooringZone(ZCHX::Data::ITF_Mooring)), this, SIGNAL(signalIsDoubleClicked4MooringZone(ZCHX::Data::ITF_Mooring)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4CardMouthZone(ZCHX::Data::ITF_CardMouth)), this, SIGNAL(signalIsDoubleClicked4CardMouthZone(ZCHX::Data::ITF_CardMouth)));
    connect(mMapWidget, SIGNAL(signalIsDoubleClicked4StatistcLineZone(ZCHX::Data::ITF_StatistcLine)), this, SIGNAL(signalIsDoubleClicked4StatistcLineZone(ZCHX::Data::ITF_StatistcLine)));
    connect(mMapWidget,SIGNAL(signalIsSelected4RouteLine(ZCHX::Data::RouteLine)),this,SIGNAL(itfSignalIsSelected4RouteLine(ZCHX::Data::RouteLine)));
    connect(mMapWidget,SIGNAL(signalDeleteRoutePoint(ZCHX::Data::RouteLine,int)),this,SIGNAL(itfSignalDeleteRoutePoint(ZCHX::Data::RouteLine,int)));
    connect(mMapWidget,SIGNAL(signalModfiyRoutePoint(ZCHX::Data::RouteLine,int)),this,SIGNAL(itfSignalModfiyRoutePoint(ZCHX::Data::RouteLine,int)));
    connect(mMapWidget,SIGNAL(signalChangeRouteLine(ZCHX::Data::RouteLine)),this,SIGNAL(itfSignalChangeRouteLine(ZCHX::Data::RouteLine)));
    connect(mMapWidget,SIGNAL(signalIsEcdisCameraTrackTarget(ZCHX::Data::ITF_CameraTrackTarget)), this, SIGNAL(itfSignalIsEcdisCameraTrackTarget(ZCHX::Data::ITF_CameraTrackTarget)));

    connect(mMapWidget,SIGNAL(signalIsSelected4ShipPlanLine(ZCHX::Data::ShipPlanLine)),this,SIGNAL(itfSignalIsSelected4ShipPlanLine(ZCHX::Data::ShipPlanLine)));
    connect(mMapWidget,SIGNAL(signalDeleteShipPlanPoint(ZCHX::Data::ShipPlanLine,int)),this,SIGNAL(itfSignalDeleteShipPlanPoint(ZCHX::Data::ShipPlanLine,int)));
    connect(mMapWidget,SIGNAL(signalModfiyShipPlanPoint(ZCHX::Data::ShipPlanLine,int)),this,SIGNAL(itfSignalModfiyShipPlanPoint(ZCHX::Data::ShipPlanLine,int)));
    connect(mMapWidget,SIGNAL(signalChangeShipPlanLine(ZCHX::Data::ShipPlanLine)),this,SIGNAL(itfSignalChangeShipPlanLine(ZCHX::Data::ShipPlanLine)));

    connect(mMapWidget,SIGNAL(signalOpenCameraDisplayWin()),this,SIGNAL(itfSignalOpenCameraDisplayWin()));
    connect(mMapWidget,SIGNAL(signalScreenShot()),this,SIGNAL(itfSignalScreenShot()));
    connect(mMapWidget,SIGNAL(signalSendSeletedAISTarget(QList<ZCHX::Data::ITF_AIS>)),this,SIGNAL(itfSignalSendSeletedAISTarget(QList<ZCHX::Data::ITF_AIS>)));
    connect(mMapWidget,SIGNAL(signalSendPictureInPictureTarget(int, QString)),this,SIGNAL(itfSignalSendPictureInPictureTarget(int, QString)));
    connect(mMapWidget,SIGNAL(signalCreateShipSimutionLine(std::vector<std::pair<double,double> >)),this,SIGNAL(itfSignalCreateShipSimutionLine(std::vector<std::pair<double,double> >)));
    connect(mMapWidget,SIGNAL(signalSendLocalMarkPos(double,double)),this,SIGNAL(itfSignalSendLocalMarkPos(double,double)));
    connect(mMapWidget,SIGNAL(signalSendReferencePos(double,double)),this,SIGNAL(itfSignalSendReferencePos(double,double)));
    connect(mMapWidget,SIGNAL(signalShowLocationMarkOrReferencePos(bool)),this,SIGNAL(itfSignalShowLocationMarkOrReferencePos(bool)));
    connect(mMapWidget,SIGNAL(signalInvokeHotSpot(ZCHX::Data::ITF_CloudHotSpot)),this,SIGNAL(itfSignalInvokeHotSpot(ZCHX::Data::ITF_CloudHotSpot)));
    connect(mMapWidget,SIGNAL(signalAddShipExtrapolation(QString, bool)), this,SIGNAL(itfSignalAddShipExtrapolation(QString, bool)));
    connect(mMapWidget,SIGNAL(signalSendHistoryTrail(QString, bool)), this,SIGNAL(itfSignalSendHistoryTrail(QString, bool)));
    connect(mMapWidget,SIGNAL(signalSendRealTimeTrail(QString, bool)), this,SIGNAL(itfSignalSendRealTimeTrail(QString, bool)));
    connect(mMapWidget,SIGNAL(signalIsSelectedSpecialRoutePoint(ZCHX::Data::SpecialRoutePoint)),this, SIGNAL(itfSignalIsSelectedSpecialRoutePoint(ZCHX::Data::SpecialRoutePoint)));
    connect(mMapWidget,SIGNAL(signalCreateFlowLine(ZCHX::Data::CustomFlowLine)),this,SIGNAL(itfSignalCreateFlowLine(ZCHX::Data::CustomFlowLine)));
    connect(mMapWidget,SIGNAL(signalCreateBlackOrWhiteList(QString,int,QString)),this,SIGNAL(itfSignalCreateBlackOrWhiteList(QString, int, QString)));
    connect(mMapWidget,SIGNAL(signalCancelBlackOrWhiteList(QString,int)),this,SIGNAL(itfSignalCancelBlackOrWhiteList(QString, int)));
    connect(mMapWidget,SIGNAL(signalCreateCPATrack(QString)),this,SIGNAL(itfSignalCreateCPATrack(QString)));
    connect(mMapWidget,SIGNAL(signalLoading()),this, SIGNAL(itfSignalLoading()));
    connect(mMapWidget,SIGNAL(signalLoadOver()),this, SIGNAL(itfSignalLoadOver()));
    connect(mMapWidget,SIGNAL(signalUpdateWarringZoneOrLineState(int,int)),this, SIGNAL(itfSignalUpdateWarringZoneOrLineState(int, int)));
    connect(mMapWidget,SIGNAL(signalUpdateChannelZoneState(int,int)),this, SIGNAL(itfSignalUpdateChannelZoneState(int, int)));
    connect(mMapWidget,SIGNAL(signalUpdateMooringZoneState(int,int)),this, SIGNAL(itfSignalUpdateMooringZoneState(int, int)));
    connect(mMapWidget,SIGNAL(signalUpdateCardMouthZoneState(int,int)),this, SIGNAL(itfSignalUpdateCardMouthZoneState(int, int)));
    connect(mMapWidget,SIGNAL(signalUpdateStatistcLineZoneState(int,int)),this, SIGNAL(itfSignalUpdateStatistcLineZoneState(int, int)));
    connect(mMapWidget,SIGNAL(signalMapIsRoaming()),this, SIGNAL(itfSignalMapIsRoaming()));
    connect(mMapWidget,SIGNAL(signalSendCameraNetGrid(ZCHX::Data::ITF_NetGrid)), this, SIGNAL(itfSignalSendCameraNetGrid(ZCHX::Data::ITF_NetGrid)));
    connect(mMapWidget,SIGNAL(signalSendPTZLocation(double, double)),this,SIGNAL(itfSignalSendPTZLocation(double, double)));
    connect(mMapWidget,SIGNAL(sigElementHoverChanged(qt::Element*)), this, SIGNAL(sigLayerElementHoverChanged(qt::Element*)));
    connect(mMapWidget,SIGNAL(sigElementSelectionChanged(qt::Element*)), this, SIGNAL(sigLayerElementSelectionChanged(qt::Element*)));
    connect(mMapWidget,SIGNAL(signalPlayDirectVhfText(QString)),this,SIGNAL(itfSignalPlayDirectVhfText(QString)));
}

void MainWindow::itfSetAisData(const QList<ZCHX::Data::ITF_AIS> &data)
{
    m_dataMgrFactory->getAisDataMgr()->setAisData(data);
}

bool MainWindow::itfSetSingleAisData(QString id, const QList<ZCHX::Data::ITF_AIS> &data, bool force)
{
    return m_dataMgrFactory->getAisDataMgr()->setSingleAisData(id, data, force);
}

void MainWindow::itfRemoveAisHistoryData(QString id)
{
    m_dataMgrFactory->getAisDataMgr()->removeAisHistoryData(id);
}

void MainWindow::itfSetHistoryAisData(const QList<ZCHX::Data::ITF_AIS> &data)
{
    m_dataMgrFactory->getAisDataMgr()->setHistoryAisData(data);
}

void MainWindow::itfSetClearHistoryData(bool states)
{
    m_dataMgrFactory->getAisDataMgr()->setClearHistoryData(states);
    m_dataMgrFactory->getRadarDataMgr()->setClearHistoryData(states);
}

void MainWindow::itfSetConsAisData(const ZCHX::Data::ITF_AIS &data)
{
    m_dataMgrFactory->getAisDataMgr()->setConsAisData(data);
}

void MainWindow::itfSetRadarEchoData(const QMap<QDateTime, ZCHX::Data::ITF_RadarEchoMap> &data)
{
    //if(mMapWidget) mMapWidget->setRadarEchoData(data);
}
void MainWindow::itfSetRadarPointData(int radarSiteId, const QList<ZCHX::Data::ITF_RadarPoint> &data)
{
    m_dataMgrFactory->getRadarDataMgr()->setRadarPointData(radarSiteId, data);
}

void MainWindow::itfSetHistoryRadarPointData(const QList<ZCHX::Data::ITF_RadarPoint> &data)
{
    m_dataMgrFactory->getRadarDataMgr()->setHistoryRadarPointData(data);
}

void MainWindow::itfSetRadarAreaData(const QList<ZCHX::Data::ITF_RadarArea> &data)
{
    std::vector<RadarAreaElement> list;
    for(int i=0; i < data.count(); ++i)
    {
        //RadarAreaElement item(data.at(i));
        //list.push_back(item);
    }
    m_dataMgrFactory->getRadarDataMgr()->setRadarAreaData(list);
}

void MainWindow::itfSetCameraRodData(const QList<ZCHX::Data::ITF_CameraRod> &data)
{
    m_dataMgrFactory->getRodDataMgr()->setRodData(data);
}

void MainWindow::itfSetCameraDevData(const QList<ZCHX::Data::ITF_CameraDev> &data, ZCHX::Data::ITF_CameraDev::ITF_CAMERA_PARENT parent)
{
    if(!mMapWidget) return;
    if(parent == ZCHX::Data::ITF_CameraDev::PARENT_ROD){
        m_dataMgrFactory->getRodDataMgr()->updateCamera(data);
    } else if(parent == ZCHX::Data::ITF_CameraDev::PARENT_AIS) {
        m_dataMgrFactory->getAisDataMgr()->updateCamera(data);
    } else if(parent == ZCHX::Data::ITF_CameraDev::PARENT_NONE) {
        m_dataMgrFactory->getCameraDataMgr()->setCameraDevData(data);
    }
}

void MainWindow::itfSetAisCameraDevData(const QList<ZCHX::Data::ITF_CameraDev> &data)
{
    itfSetCameraDevData(data, ZCHX::Data::ITF_CameraDev::PARENT_AIS);
}

void MainWindow::itfSetPastrolStation(const QList<ZCHX::Data::ITF_PastrolStation> &data)
{
    m_dataMgrFactory->getPastrolStationMgr()->setData(data);
}

void MainWindow::itfSetAisSite(const QList<ZCHX::Data::ITF_AisSite> &data)
{
    m_dataMgrFactory->getAisSiteMgr()->setAisSiteDevData(data);
}

void MainWindow::itfSetAidtoNavigation(const QList<ZCHX::Data::ITF_AidtoNavigation> &data)
{
    m_dataMgrFactory->getAidtoNavigationMgr()->setAidtoNavigationDevData(data);
}

void MainWindow::itfSetRadarSite(const QList<ZCHX::Data::ITF_RadarSite> &data)
{
    m_dataMgrFactory->getRadarSiteMgr()->setRadarSiteDevData(data);
}

void MainWindow::itfSetWarringZoneData(const QList<ZCHX::Data::ITF_WarringZone> &data)
{
    m_dataMgrFactory->getWarningZoneMgr()->setData(data);
}

void MainWindow::itfSetShipSiumtionData(const std::vector<std::pair<double, double> > &data)
{
    //if(mMapWidget) mMapWidget->setShipSiumtionData(data);
}

void MainWindow::itfSetAISDataToSiumtion(const QList<ZCHX::Data::ITF_AIS> &data)
{
    //if(mMapWidget) mMapWidget->setAisDataToSiumtion(data);
}

void MainWindow::itfSetCoastData(const QList<ZCHX::Data::ITF_CoastData> &data)
{
    m_dataMgrFactory->getCoastMgr()->setData(data);
}

void MainWindow::itfImportCoastData(const std::vector<std::pair<double, double> > & data)
{
    m_dataMgrFactory->getCoastMgr()->importData(data);
}

void MainWindow::itfSetSeabedPipeLine(const QList<ZCHX::Data::ITF_SeabedPipeLine> &data)
{
    m_dataMgrFactory->getSeabedPiplineMgr()->setData(data);
}

void MainWindow::itfImportSeabedPipeLine(const std::vector<std::pair<double, double> > & data)
{
    m_dataMgrFactory->getSeabedPiplineMgr()->importData(data);
}

void MainWindow::itfSetStructure(const QList<ZCHX::Data::ITF_Structure> &data)
{
    m_dataMgrFactory->getStructureMgr()->setData(data);
}

void MainWindow::itfImportStructure(const std::vector<std::pair<double, double> > &data)
{
    m_dataMgrFactory->getStructureMgr()->importData(data);
}

void MainWindow::itfSetAreaNet(const QList<ZCHX::Data::ITF_AreaNet> &data)
{
    m_dataMgrFactory->getAreanetMgr()->setData(data);
}

void MainWindow::itfImportAreaNet(const std::vector<std::pair<double, double> > &data)
{
    m_dataMgrFactory->getAreanetMgr()->importData(data);
}

void MainWindow::itfSetChannel(const QList<ZCHX::Data::ITF_Channel> &data)
{
    m_dataMgrFactory->getChannelMgr()->setData(data);
}

void MainWindow::itfImportChannel(const std::vector<std::pair<double, double> > &data)
{
    m_dataMgrFactory->getChannelMgr()->importData(data);
}

void MainWindow::itfSelectChannelLine(int channelId, const ZCHX::Data::ITF_ChannelLine & line)
{
    m_dataMgrFactory->getChannelMgr()->selectChannelLine(channelId, line);
}

void MainWindow::itfCancelChannelLine(int channelId)
{
    m_dataMgrFactory->getChannelMgr()->cancelChannelLine(channelId);
}

void MainWindow::itfSetMooring(const QList<ZCHX::Data::ITF_Mooring> &data)
{
    m_dataMgrFactory->getMooringMgr()->setData(data);
}

void MainWindow::itfImportMooring(const std::vector<std::pair<double, double> > &data)
{
    m_dataMgrFactory->getMooringMgr()->importData(data);
}

void MainWindow::itfSetCardMouth(const QList<ZCHX::Data::ITF_CardMouth> &data)
{
    m_dataMgrFactory->getCardmouthMgr()->setData(data);
}

void MainWindow::itfImportCardMouth(const std::vector<std::pair<double, double> > &data)
{
    m_dataMgrFactory->getCardmouthMgr()->importData(data);
}

void MainWindow::itfSetStatistcLine(const QList<ZCHX::Data::ITF_StatistcLine> &data)
{
    m_dataMgrFactory->getStatistcLineMgr()->setData(data);
}

void MainWindow::itfImportStatistcLine(const std::vector<std::pair<double, double> > &data)
{
    m_dataMgrFactory->getStatistcLineMgr()->importData(data);
}

void MainWindow::itfSetFleet(const QList<ZCHX::Data::ITF_Fleet> &data)
{
    QMap<QString, ZCHX::Data::ITF_Fleet> fleetMap;
    for(int i = 0; i < data.size(); ++i)
    {
        fleetMap.insert(data[i].aisId, data[i]);
    }
    if(mMapWidget) mMapWidget->setFleet(fleetMap);
}

void MainWindow::itfSetNaviMark(const QList<ZCHX::Data::ITF_NaviMark> &data)
{
    m_dataMgrFactory->getNaviMarkDataMgr()->setNaviMarkDevData(data);
}

void MainWindow::itfSetShipAlarmAscendMap(const QMap<QString, ZCHX::Data::ITF_ShipAlarmAscend> &shipAlarmAscendMap)
{
    m_dataMgrFactory->getShipAlarmAscendMgr()->setData(shipAlarmAscendMap.values());
}

void MainWindow::itfUpdateWarringZone(const QList<ZCHX::Data::ITF_WarringZone> &data)
{
    m_dataMgrFactory->getWarningZoneMgr()->updateData(data);
}

void MainWindow::itfUpdateWarringZone(const ZCHX::Data::ITF_WarringZone &zone)
{
    m_dataMgrFactory->getWarningZoneMgr()->updateData(zone);
}

void MainWindow::removeWarrningZone(const ZCHX::Data::ITF_WarringZone &zone)
{
    m_dataMgrFactory->getWarningZoneMgr()->removeData(zone);
}

QList<ZCHX::Data::ITF_WarringZone> MainWindow::getAllWarrningZone() const
{
    return m_dataMgrFactory->getWarningZoneMgr()->getData();
}

void MainWindow::itfSetIslandLineData(const QList<ZCHX::Data::ITF_IslandLine> &data)
{
    m_dataMgrFactory->getIslandlineMgr()->setData(data);
}

void MainWindow::itfSetLocalMarkData(const QList<ZCHX::Data::ITF_LocalMark> &data)
{
    m_dataMgrFactory->getLocalmarkMgr()->setData(data);
}

void MainWindow::itfSetCameraGdyData(const QList<ZCHX::Data::ITF_CameraDev> &data)
{
    m_dataMgrFactory->getCameraDataMgr()->setCameraDevData(data);
}

void MainWindow::itfSetCameraPlanData(const QList<ZCHX::Data::ITF_CameraDev> &data)
{
    m_dataMgrFactory->getCameraDataMgr()->setCameraDevData(data);
}

void MainWindow::itfSetDangerousCircleData(const QList<ZCHX::Data::ITF_DangerousCircle> &data)
{
    m_dataMgrFactory->getDangerousMgr()->setData(data);
}

void MainWindow::itfSetDangerousCircleRange(const double range)
{
    //每一个危险圈设定半径  这里取消
    //if(mMapWidget) mMapWidget->setDangerousCircleRange(range);
}

void MainWindow::itfSetRadarFeatureZoneDagta(const QList<ZCHX::Data::ITF_RadarFeaturesZone> &data)
{
    m_dataMgrFactory->getRadarFeatureZoneMgr()->setData(data);
}

void MainWindow::itfSetRouteLineData(const QList<ZCHX::Data::RouteLine> &data)
{
    std::vector<RouteLine> list;
    for(int i=0; i< data.count(); ++i)
    {
        ZCHX::Data::RouteLine tmp = data.at(i);
        RouteLine item(tmp);
        list.push_back(item);

    }
    if(mMapWidget) mMapWidget->getRouteDataMgr()->setRouteLineData(list);
}

void MainWindow::itfSetALLSpecialRouteLineData(const QList<ZCHX::Data::SpecialRouteLine> &data)
{
    std::vector<SpecialRouteLine> list;
    for(int i=0; i< data.count(); ++i)
    {
        ZCHX::Data::SpecialRouteLine tmp = data.at(i);
        SpecialRouteLine item(tmp);
        list.push_back(item);
    }
    if(mMapWidget) mMapWidget->getRouteDataMgr()->setALLSpecialRouteLineData(list);

}

void MainWindow::itfSetSpacilRouteLineData(const QList<ZCHX::Data::SpecialRouteLine> &data)
{
    std::vector<SpecialRouteLine> list;
    for(int i=0; i< data.count(); ++i)
    {
        ZCHX::Data::SpecialRouteLine tmp = data.at(i);
        SpecialRouteLine item(tmp);
        list.push_back(item);

    }
    if(mMapWidget) mMapWidget->getRouteDataMgr()->setSpacilRouteLineData(list);
}

void MainWindow::itfSetPickUpSpacilRoutePoint(const ZCHX::Data::SpecialRoutePoint &data)
{
    if(mMapWidget) mMapWidget->getRouteDataMgr()->setPickUpSpacilRoutePoint(data);
}

void MainWindow::itfSetHistoryRouteLineData(const QList<ZCHX::Data::RouteLine> &data)
{
    std::vector<RouteLine> list;
    for(int i=0; i< data.count(); ++i)
    {
        ZCHX::Data::RouteLine tmp = data.at(i);
        RouteLine item(tmp);
        list.push_back(item);

    }
    if(mMapWidget) mMapWidget->getRouteDataMgr()->setHistoryRouteLineData(list);
}

void MainWindow::itfSetRouteDefaultCableData(const ZCHX::Data::CableAssembly &data, int uSlack)
{
    if(mMapWidget) mMapWidget->getRouteDataMgr()->setRouteDefaultCableData(data,uSlack);
}

void MainWindow::itfSetRouteConfigDisplay(int uRouteID, bool bShowPoint,bool bShowType,bool bShowLine,bool bShowCable, bool bShowIndex)
{
    //if(mMapWidget) mMapWidget->setRouteConfigDisplay(uRouteID, bShowPoint, bShowType, bShowLine, bShowCable, bShowIndex);
}

void MainWindow::itfSetRouteDisplayFlag(int uRouteID, int uDisplayFlag)
{
    //if(mMapWidget) mMapWidget->setRouteDisplayFlag(uRouteID,uDisplayFlag);
}

void MainWindow::itfCloseCurRouteLineData()
{
    //if(mMapWidget) mMapWidget->closeCurRouteLineData();
}

void MainWindow::itfDeleteRouteLineDataFromProjectID(const int uProjectID)
{
    //if(mMapWidget) mMapWidget->deleteRouteLineDataFromProjectID(uProjectID);
}

void MainWindow::itfSetShipPlanLineData(const QList<ZCHX::Data::ShipPlanLine> &data)
{
    std::vector<ShipPlanLine> list;
    for(int i=0; i< data.count(); ++i)
    {
        ZCHX::Data::ShipPlanLine tmp = data.at(i);
        ShipPlanLine item(tmp);
        list.push_back(item);

    }
    if(mMapWidget) mMapWidget->getShipPlanDataMgr()->setShipPlanLineData(list);
}

void MainWindow::itfSetAllProjectShipPlanLineData(const QList<ZCHX::Data::ShipPlanLine> &data)
{
    std::vector<ShipPlanLine> list;
    for(int i=0; i< data.count(); ++i)
    {
        ZCHX::Data::ShipPlanLine tmp = data.at(i);
        ShipPlanLine item(tmp);
        list.push_back(item);
    }

    if(mMapWidget) mMapWidget->getShipPlanDataMgr()->setAllProjecrShipPlanLineData(list);
}

void MainWindow::itfSetShipPlanProjectID(const int ProjectID)
{
    //路由的时候已经传入工程ID了
}

void MainWindow::itfSetRouteLinePickupLocation(const int uRouteID, const double dLat, const double dLon)
{
    //if(mMapWidget) mMapWidget->setRouteLinePickupLocation(uRouteID,dLat,dLon);
}

void MainWindow::itfSetRouteLinePickupInfor(const double dKp, bool bShowLength, const double dLength, bool bShowSlope, const double dSlope)
{
    //if(mMapWidget) mMapWidget->setRouteLinePickupInfor(dKp,bShowLength,dLength,bShowSlope,dSlope);
}

void MainWindow::itfChangeCurrentRouteLine(const ZCHX::Data::RouteLine &data, int uRouteID)
{
    //if(mMapWidget) mMapWidget->changeCurrentRouteLine(data,uRouteID);
}

void MainWindow::itfChangeRoutePathAssessStatus(int uAssessStatus, int uRouteID)
{
    //if(mMapWidget) mMapWidget->changeRoutePathAssessStatus(uAssessStatus,uRouteID);
}

void MainWindow::itfSetSimulateData(int uSimulateDataID, int uSimulateFlag)
{
    //if(mMapWidget) mMapWidget->setSimulateData(uSimulateDataID,uSimulateFlag);
}

void MainWindow::itfSetSimulateLocation(float fCurSimulateKP)
{
    //if(mMapWidget) mMapWidget->setSimulateLocation(fCurSimulateKP);
}

void MainWindow::itfSetCameraObservationZoneData(const QList<ZCHX::Data::ITF_CameraView> &data)
{
    m_dataMgrFactory->getCameraViewMgr()->setData(data);
}

void MainWindow::itfSetRadarVideoData(int radarSiteId, double dCentreLon,double dCentreLat,double dDistance, const QByteArray &objPixmap)
{
    m_dataMgrFactory->getRadarVideoMgr()->setRadarVideoData(radarSiteId, dCentreLon,dCentreLat,dDistance,objPixmap);
}

void MainWindow::itfSetRadarRect(int radarSiteId, QList<ZCHX::Data::ITF_RadarRect> rectList)
{
    m_dataMgrFactory->getRadarRectMgr()->setRadarRect(radarSiteId, rectList);
}

//void MainWindow::itfSetMultibeamData(const QList<ZCHX::Data::ITF_Multibeam> &data, const double dMinLon, const double dMinLat, const double dMaxLon, const double dMaxLat, const double dMinX, const double dMinY, const double dMinZ, const double dMaxX, const double dMaxY, const double dMaxZ)
//{
//    std::vector<Multibeam> list;
//    for(int i=0; i< data.count(); ++i)
//    {
//        ZCHX::Data::ITF_Multibeam tmp = data.at(i);
//        Multibeam item(tmp);
//        list.push_back(item);
//    }

//    if(mMapWidget) mMapWidget->setMultibeamData(list,dMinLon,dMinLat,dMaxLon,dMaxLat,dMinX,dMinY,dMinZ,dMaxX,dMaxY,dMaxZ);
//}

void MainWindow::itfSetMultibeamDataByImg(int iRouteID, const QPixmap &samplePixmap, const QPixmap &objPixmap, double dMinLon, double dMinLat, double dMaxLon, double dMaxLat)
{
    //if(mMapWidget) mMapWidget->setMultibeamDataByImg(iRouteID, samplePixmap, objPixmap, dMinLon, dMinLat, dMaxLon, dMaxLat);
}

void MainWindow::itfSetAllProjectRouteLineData(const QList<ZCHX::Data::RouteLine> &data)
{

    std::vector<RouteLine> list;
    for(int i=0; i< data.count(); ++i)
    {
        ZCHX::Data::RouteLine tmp = data.at(i);
        RouteLine item(tmp);
        list.push_back(item);

    }

    if(mMapWidget) mMapWidget->getRouteDataMgr()->setAllProjectRouteLineData(list);

}

void MainWindow::itfSetRouteCrossData(const QList<ZCHX::Data::ITF_RouteCross> &data)
{
    std::vector<RouteCross> list;
    for(int i=0; i< data.count(); ++i)
    {
        ZCHX::Data::ITF_RouteCross tmp = data.at(i);
        RouteCross item(tmp);
        list.push_back(item);

    }

    if(mMapWidget) mMapWidget->getRouteDataMgr()->setRouteCrossData(list);
}
void MainWindow::itfSetCameraVideoWarnData(const QList<ZCHX::Data::ITF_VideoTarget> &data)
{
    m_dataMgrFactory->getVideoDataMgr()->setData(data);
}

void MainWindow::itfAppendElementItem(const ZCHX::Data::ITF_EleEllipse &item)
{
    //if(mMapWidget) mMapWidget->appendElementItem(item);
}

void MainWindow::itfAppendElementItem(const ZCHX::Data::ITF_EleTriangle &item)
{
    //if(mMapWidget) mMapWidget->appendElementItem(item);
}

void MainWindow::itfAppendElementItem(const ZCHX::Data::ITF_EleLine &item)
{
    //if(mMapWidget) mMapWidget->appendElementItem(item);
}

void MainWindow::itfAppendElementItem(const ZCHX::Data::ITF_EleRect &item)
{
    //if(mMapWidget) mMapWidget->appendElementItem(item);
}

void MainWindow::itfRemoveElementItem(Element *pitem)
{
    //if(mMapWidget) mMapWidget->removeElementItem(pitem);
}

void MainWindow::itfClearElementData()
{
    //if(mMapWidget) mMapWidget->clearElementData();
}

void MainWindow::itfSetPickUpRadarInfo(qint32 tracknumber)
{
    m_dataMgrFactory->getRadarDataMgr()->setPickUpRadarPoint(QString::number(tracknumber));
    //if(mMapWidget) mMapWidget->SetPickUpRadarInfo(tracknumber);
}

void MainWindow::itfSetPickUpAisInfo(QString id)
{
    m_dataMgrFactory->getAisDataMgr()->SetPickUpAisInfo(id);
}

void MainWindow::itfSetPickUpPosition(QString id)
{
    //if(mMapWidget) mMapWidget->SetPickUpPosition(id);
}

void MainWindow::itfSetPickUpRouteCross(const int id)
{
    //if(mMapWidget) mMapWidget->SetPickUpRouteCross(id);
}

//void MainWindow::itfSetShipSimulationExtrapolationDatas(const QList<ZCHX::Data::ITF_AIS> &DataList)
//{
//    //if(mMapWidget) mMapWidget->setShipSimulationExtrapolationDatas(DataList);
//}

void MainWindow::itfUpdateShipSimulationExtrapolationData(const QString &id, int time)
{
    m_dataMgrFactory->getAisDataMgr()->updateExtrapolationTime(id, time);
}

void MainWindow::itfAppendShipSimulationExtrapolationData(const QString &id, int time)
{
    m_dataMgrFactory->getAisDataMgr()->appendExtrapolationList(QStringList()<<id, false);
    m_dataMgrFactory->getAisDataMgr()->updateExtrapolationTime(id, time);
}

void MainWindow::itfDeleteShipSimulationExtrapolationData(const QString &id)
{
    m_dataMgrFactory->getAisDataMgr()->removeExtrapolation(id);
}

ZCHX::Data::ExtrapolateList MainWindow::itfGetShipSimulationExtrapolationData()
{
    return m_dataMgrFactory->getAisDataMgr()->getExtrapolationList();
}

void MainWindow::itfSetRealTimeShipTrailDatas(const QMap<QString, QList<ZCHX::Data::ITF_AIS> > &DataList)
{
    QMap<QString, QList<ZCHX::Data::ITF_AIS> >::const_iterator it = DataList.begin();

    m_dataMgrFactory->getAisDataMgr()->clearRealtimeTailTrackList();
    m_dataMgrFactory->getAisDataMgr()->appendRealtimeTailTrackList(DataList.keys(), false);

    for(; it != DataList.end(); it++)
    {
        m_dataMgrFactory->getAisDataMgr()->setRealtimeTailTrack(it.key(), it.value());
    }
}



void MainWindow::itfSetRouteHistogramData(const int RouteId, const QMap<int, QList<int> > &DataList)
{
    //if(mMapWidget) mMapWidget->setRouteHistogramData(RouteId,DataList);
}

void MainWindow::itfSetRealTimeFlowLineHistogramData(const QMap<int, QList<int> > &DataList)
{
    //if(mMapWidget) mMapWidget->setRealTimeFlowLineHistogramData(DataList);
}

void MainWindow::itfSetInTheAreaOfTrackData(const QList<QList<ZCHX::Data::LatLon> > &data)
{
    //if(mMapWidget) mMapWidget->setInTheAreaOfTrackData(data);
}

void MainWindow::itfSetRolePrivilegeList(const QList<int> &list)
{

    //if(mMapWidget) mMapWidget->setRolePrivilegeList(list);
}
void MainWindow::itfSetIsHideMap(bool b)
{
    if(mMapWidget) mMapWidget->setIsHideMap(b);
}

void MainWindow::itfSetNavigateionging(bool b)
{
    if(mMapWidget) mMapWidget->setIsNavigation(b);
}

void MainWindow::itfSetIsOpenMeet(bool b)
{
    if(mMapWidget) mMapWidget->setIsOpenMeet(b);
}

void MainWindow::itfSetPuickupType(const ZCHX::Data::ECDIS_PICKUP_TYPEs &type)
{
    if(mMapWidget) mMapWidget->setCurPickupType(type);
}

void MainWindow::itfUpdateCameraDevStatus(uint cameid, uint st)
{
    //if(mMapWidget) mMapWidget->zchxUpdateCameraDevStatus(cameid, st);
}
void MainWindow::itfSetRadarTrackPoint(bool b)
{
    if(mMapWidget) mMapWidget->setEnableRadarTrackPoint(b);
}

void MainWindow::itfSetEnableCameraTargerTrack(bool b)
{
    if(mMapWidget) mMapWidget->setEnableCameracombinedTrack(b);
}

void MainWindow::itfSetEnableRouteHistogram(bool b)
{
    if(mMapWidget) mMapWidget->setEnableRouteHistogram(b);
}

void MainWindow::itfSetEnableShipTag(int tag)
{
    m_dataMgrFactory->getAisDataMgr()->SetEnableShipTag(tag);
}

void MainWindow::itfSetRadarLabelVisible(bool showRadarLabel)
{
    if(mMapWidget)
    {
        mMapWidget->SetRadarLabelVisible(showRadarLabel);
    }

    m_dataMgrFactory->getRadarDataMgr()->SetRadarLabelVisible(showRadarLabel);
    m_dataMgrFactory->getRadarRectMgr()->SetRadarLabelVisible(showRadarLabel);
}

void MainWindow::itfSetRadarDisplayInfo(bool showRadarLabel, int targetSizeIndex, int traceLenIndex, int continueTimeIndex)
{
    if(mMapWidget)
    {
        mMapWidget->setRadarDisplayInfo(showRadarLabel, targetSizeIndex, traceLenIndex, continueTimeIndex);
    }

    m_dataMgrFactory->getRadarDataMgr()->SetRadarDisplayInfo(showRadarLabel, targetSizeIndex, traceLenIndex, continueTimeIndex);
    m_dataMgrFactory->getRadarRectMgr()->SetRadarDisplayInfo(showRadarLabel, targetSizeIndex, traceLenIndex, continueTimeIndex);
}

void MainWindow::itfSetEnableTooBarVisable(bool visible)
{
    //d->m_toolbar->setVisible(visible);
}

void MainWindow::itfsetPluginUseModel(const ZCHX::Data::ECDIS_PLUGIN_USE_MODEL &mod)
{
    if(mMapWidget) mMapWidget->setCurPluginUserModel(mod);
}

void MainWindow::itfsetSelectedShip()
{
    mMapWidget->setETool2DrawTrackTarget();
}

void MainWindow::itfsetSelectedAllTrack()
{
    mMapWidget->setETool2DrawCameraTrackTarget();
}

void MainWindow::itfsetSelectedRouteOrCross()
{
    mMapWidget->setETool2DrawRouteOrCross();
}

double MainWindow::itfzchxUtilToolAngle4north()
{
    return mMapWidget->zchxUtilToolAngle4north();
}

void MainWindow::itfzchxUtilToolSetAngle4north(double ang)
{
//    qDebug()<<"MainWindow::itfzchxUtilToolSetAngle4north 当前角度:" << ang;
    if(mMapWidget) mMapWidget->zchxUtilToolSetAngle4north(ang);
}

bool MainWindow::itfzchxUtilToolPoint4CurWindow(double lat, double lon, QPointF &p)
{
    return  mMapWidget->zchxUtilToolPoint4CurWindow(lat,lon,p);
}

bool MainWindow::itfzchxUtilToolLL4CurPoint(const QPointF &p, double &lat, double &lon)
{
    //    QPoint tmp(p.x(),p.y());
    //    QPoint zxl = mapFromParent(tmp);
    //    qDebug()<<"zxl"<<zxl;
    return mMapWidget->zchxUtilToolLL4CurPoint(p,lat,lon);
}

uint MainWindow::itfzchxUtilToolCurZoom()
{
    return mMapWidget->zchxUtilToolCurZoom();
}


void MainWindow::setMapUnit(const MapUnit &unit)
{
    if(mMapWidget) mMapWidget->setMapUnit(unit);
}

void MainWindow::setIsShowStatusBar(bool bShow)
{
    statusBar()->setVisible(bShow);
}

Element* MainWindow::getCurrentSelectedElement()
{
    return mMapWidget->getCurrentSelectedElement();
}

void MainWindow::itfChannelZoneData4id(int uuid, ZCHX::Data::tagITF_Channel &info, bool &ok)
{
    ok = m_dataMgrFactory->getChannelMgr()->data4id(uuid, info);
}

void MainWindow::itfMooringZoneData4id(int uuid, ZCHX::Data::tagITF_Mooring &info, bool &ok)
{
    ok = m_dataMgrFactory->getMooringMgr()->data4id(uuid, info);
}

void MainWindow::itfCardMouthZoneData4id(int uuid, ZCHX::Data::tagITF_CardMouth &info, bool &ok)
{
    ok = m_dataMgrFactory->getCardmouthMgr()->data4id(uuid, info);
}

void MainWindow::itfStatistcLineZoneData4id(int uuid, ZCHX::Data::tagITF_StatistcLine &info, bool &ok)
{
    ok = m_dataMgrFactory->getStatistcLineMgr()->data4id(uuid, info);
}

bool MainWindow::getIsAtiAdaptor() const
{
    return true;
   // return mMapWidget->getIsAtiAdaptor();
}

void MainWindow::setIsAtiAdaptor(bool isAtiAdaptor)
{
    //if(mMapWidget) mMapWidget->setIsAtiAdaptor(isAtiAdaptor);
}

void MainWindow::setUseRightKey(bool bUseRightKey)
{
    if(mMapWidget) mMapWidget->setUseRightKey(bUseRightKey);
}



void MainWindow::closeEvent(QCloseEvent * e)
{
    e->accept();
}
//zxl
/**
 * @brief MainWindow::OnMeasureAreaAction
 * @anthor:zxl
 * area measurement
 */
void MainWindow::OnMeasureAreaAction()
{
     mMapWidget->setETool2DrawArea();
}

/**
 * @brief MainWindow::OnDirAngleAction
 * @anthor:zxl
 * draw direct angle
 */
void MainWindow::OnDirAngleAction()
{
    mMapWidget->setETool2DrawDirAngle();
}

void MainWindow::OnDistantcAction()
{
    mMapWidget->setETool2DrawDistance();
}

void MainWindow::OnExitEToolAction()
{
    mMapWidget->releaseDrawStatus();
}

void MainWindow::OnWarringZONEAction()
{
    mMapWidget->setETool2DrawZONE();
}

void MainWindow::OnWarringZONESelected()
{
    mMapWidget->setETool2SelectZONE();
}

void MainWindow::OnWarringZONEmove()
{
    mMapWidget->setETool2moveZONE();
}

void MainWindow::OnWarringZONEmoveCtrlPoint()
{
    mMapWidget->setETool2ctrlZONE();
}

void MainWindow::OnWarringZONEDelCtrlPoint()
{
    mMapWidget->setETool2delCtrlZONE();
}

void MainWindow::OnWarringZONEAddCtrlPoint()
{
    mMapWidget->setETool2addCtrlZONE();
}

void MainWindow::OnCoastDataLineAdd()
{
    mMapWidget->setETool2Draw4CoastDataLine();
}

void MainWindow::OnSeabedPipeLineLineAdd()
{
    mMapWidget->setETool2Draw4SeabedPipeLineLine();
}

void MainWindow::OnStructurePointAdd()
{
    mMapWidget->setETool2Draw4StructurePoint();
}

void MainWindow::OnAreaNetZoneAdd()
{
    mMapWidget->setETool2Draw4AreaNetZone();
}

void MainWindow::OnChannelAreaAdd()
{
    mMapWidget->setETool2Draw4ChannelArea();
}

void MainWindow::OnChannelZONESelected()
{
    mMapWidget->setETool2SelectChannel();
}

void MainWindow::OnChannelZONEMove()
{
    mMapWidget->setETool2moveChannel();
}

void MainWindow::OnChannelZONEMoveCtrlPoint()
{
    mMapWidget->setETool2ctrlChannel();
}

void MainWindow::OnChannelZONEDelCtrlPoint()
{
    mMapWidget->setETool2delCtrlChannel();
}

void MainWindow::OnChannelZONEAddCtrlPoint()
{
    mMapWidget->setETool2addCtrlChannel();
}

void MainWindow::OnMooringAreaAdd()
{
    mMapWidget->setETool2Draw4MooringArea();
}

void MainWindow::OnMooringZONESelected()
{
    mMapWidget->setETool2SelectMooring();
}

void MainWindow::OnMooringZONEMove()
{
    mMapWidget->setETool2moveMooring();
}

void MainWindow::OnMooringZONEMoveCtrlPoint()
{
    mMapWidget->setETool2ctrlMooring();
}

void MainWindow::OnMooringZONEDelCtrlPoint()
{
    mMapWidget->setETool2delCtrlMooring();
}

void MainWindow::OnMooringZONEAddCtrlPoint()
{
    mMapWidget->setETool2addCtrlMooring();
}

void MainWindow::OnCardMouthAreaAdd()
{
    mMapWidget->setETool2Draw4CardMouthArea();
}

void MainWindow::OnCardMouthZONESelected()
{
    mMapWidget->setETool2SelectCardMouth();
}

void MainWindow::OnCardMouthZONEMove()
{
    mMapWidget->setETool2moveCardMouth();
}

void MainWindow::OnCardMouthZONEMoveCtrlPoint()
{
    mMapWidget->setETool2ctrlCardMouth();
}

void MainWindow::OnCardMouthZONEDelCtrlPoint()
{
    mMapWidget->setETool2delCtrlCardMouth();
}

void MainWindow::OnCardMouthZONEAddCtrlPoint()
{
    mMapWidget->setETool2addCtrlCardMouth();
}

void MainWindow::OnStatistcLineAreaAdd()
{
    mMapWidget->setETool2Draw4StatistcLineArea();
}

void MainWindow::OnStatistcLineZONESelected()
{
    mMapWidget->setETool2SelectStatistcLine();
}

void MainWindow::OnStatistcLineZONEMove()
{
    mMapWidget->setETool2moveStatistcLine();
}

void MainWindow::OnStatistcLineZONEMoveCtrlPoint()
{
    mMapWidget->setETool2ctrlStatistcLine();
}

void MainWindow::OnStatistcLineZONEDelCtrlPoint()
{
    mMapWidget->setETool2delCtrlStatistcLine();
}

void MainWindow::OnStatistcLineZONEAddCtrlPoint()
{
    mMapWidget->setETool2addCtrlStatistcLine();
}

void MainWindow::OnSelectPlayZone()
{
    mMapWidget->setETool2SelectPlayZone();
}

void MainWindow::OnIslandLineAdd()
{
    mMapWidget->setETool2Draw4IslandLine();
}

void MainWindow::OnIslandLineSelected()
{
    mMapWidget->setETool2Select4IslandLine();
}

void MainWindow::OnIslandLineMove()
{
    mMapWidget->setETool2move4IslandLine();
}

void MainWindow::OnIslandLineMoveCtrlPoint()
{
    mMapWidget->setETool2moveCtrlPoint4IslandLine();
}

void MainWindow::OnIslandLineDelCtrlPoint()
{
    mMapWidget->setETool2delCtrlPoint4IslandLine();
}

void MainWindow::OnIslandLineAddCtrlPoint()
{
    mMapWidget->setETool2addCtrlPoint4IslandLine();
}

void MainWindow::itfToolBarShipPlanLineAdd()
{
    mMapWidget->setETool2DrawShipPlanLine();
}

void MainWindow::itfToolBarShipPlanLineSelected()
{
     mMapWidget->setETool2SelectShipPlanLine();
}

void MainWindow::itfToolBarShipPlanLineInsertCtrlPoint()
{
    mMapWidget->setETool2insertCtrlPointShipPlanLine();
}

void MainWindow::itfToolBarShipPlanLineMoveCtrlPoint()
{
    mMapWidget->setETool2moveCtrlPointShipPlanLine();
}

void MainWindow::itfToolBarShipPlanLineDelCtrlPoint()
{
    mMapWidget->setETool2delCtrlPointShipPlanLine();
}

void MainWindow::itfToolBarShipPlanLineAddCtrlPoint()
{
    mMapWidget->setETool2addCtrlPointShipPlanLine();
}

void MainWindow::itfSetShipPlanWaterDepth(QList<ZCHX::Data::ITF_WaterDepth> &WaterData)
{
    //mMapWidget->setShipPlanWaterDepth(WaterData);
}

void MainWindow::itfSetHistoryTrackStyle(const QString &color, const int lineWidth)
{
    m_dataMgrFactory->getAisDataMgr()->setHistoryTrackStyle(color, lineWidth);
}

void MainWindow::itfSetPrepushTrackStyle(const QString &color, const int lineWidth)
{
    m_dataMgrFactory->getAisDataMgr()->setPrepushTrackStyle(color, lineWidth);
}

void MainWindow::itfSetMapUnit(const MapUnit& uint)
{
    if(mMapWidget) mMapWidget->setMapUnit(uint);
}

void MainWindow::OnPickupAction()
{
    if(mMapWidget) mMapWidget->setETool2DrawPickup();
}

void MainWindow::OnShowAllAction()
{
    mMapWidget->ShowAll();
}

void MainWindow::OnLocalMarkAction()
{
    mMapWidget->setETool2DrawLocalMark();
}

void MainWindow::OnGpsAction()
{
    mMapWidget->setETool2DrawGps();
}

void MainWindow::OnSelBaseDisplayStyle()
{
    mMapWidget->setMapStyle(MapStyleBase);
}

void MainWindow::OnSelStandardDisplayStyle()
{
    mMapWidget->setMapStyle(MapStyleStandard);
}

void MainWindow::OnSelAllDisplayStyle()
{
    mMapWidget->setMapStyle(MapStyleAll);
}

void MainWindow::OnSelDayBlackBackDisplyStyle()
{
    mMapWidget->setMapStyle(MapStyleEcdisDayBlackBack);
}

void MainWindow::OnSelNightDisplyStyle()
{
     mMapWidget->setMapStyle(MapStyleEcdisNight);
}

void MainWindow::OnSelDayDUSKDisplyStyle()
{
     mMapWidget->setMapStyle(MapStyleEcdisDayDUSK);
}

void MainWindow::OnSelDayWhiteBackDisplyStyle()
{
     mMapWidget->setMapStyle(MapStyleEcdisDayWhiteBack);
}

void MainWindow::OnSelDayBrightDisplyStyle()
{
     mMapWidget->setMapStyle(MapStyleEcdisDayBright);
}

void MainWindow::OnZONESelected()
{
    mMapWidget->setETool2SelectCommonZONE();
}

void MainWindow::OnSetMapAngle(double angle)
{
    itfzchxUtilToolSetAngle4north(angle);
}

void MainWindow::setMapCenterAndZoom(double lat, double lon, int zoom)
{
    if(mMapWidget) mMapWidget->setCenterAndZoom(ZCHX::Data::LatLon(lat, lon), zoom);
}

void MainWindow::setMapCenterAndRotate(double lat, double lon, double ang)
{
    if(mMapWidget) mMapWidget->setMapRotateAtLL(lat, lon, ang * 180 / GLOB_PI);
}

void MainWindow::setMapCenterAndRotate_new(double lat, double lon, double ang)
{
    if(mMapWidget) mMapWidget->setMapRotateAtLL(lat, lon, ang);
}

void MainWindow::resetMapRotate(double lat, double lon)
{
    if(mMapWidget) mMapWidget->resetMapRotate(lat,lon);
}

void MainWindow::setMapCenter(double lat, double lon)
{
    if(mMapWidget) mMapWidget->setCenterAndZoom(ZCHX::Data::LatLon(lat, lon), -1);
}

void MainWindow::resetMap()
{
    if(mMapWidget) mMapWidget->reset();
}

void MainWindow::zchxSetStyleAutoChange(bool val)
{
    if(mMapWidget) mMapWidget->setStyleAutoChange(val);
}

void MainWindow::zchxSetGISDayBrightTime(const QTime &t)
{
    if(mMapWidget) mMapWidget->setGISDayBrightTime(t);
}

void MainWindow::zchxSetGISDuskTime(const QTime &t)
{
    if(mMapWidget) mMapWidget->setGISDuskTime(t);
}

void MainWindow::zchxSetGISNightTime(const QTime &t)
{
    if(mMapWidget) mMapWidget->setGISNightTime(t);
}

void MainWindow::zchxClearAisData(qint64 cTime, uint sec)
{
    //if(mMapWidget) mMapWidget->clearAisData(cTime, sec);
}

void MainWindow::itfzchxUtilToolCameraRodFocus(uint cameid, int tag)
{
    //if(mMapWidget) mMapWidget->zchxCameraRodFocus(cameid,tag);
}

void MainWindow::itfToolBarZoomIn()
{
    mMapWidget->ScalePlus();
}

void MainWindow::itfToolBarZoomOut()
{
     mMapWidget->ScaleMinus();
}

void MainWindow::itfToolBarRoam()
{
    OnExitEToolAction();
}

void MainWindow::itfToolBarMeasureDistance()
{
    OnDistantcAction();
}

void MainWindow::itfToolBarMeasureArea()
{
    OnMeasureAreaAction();
}

void MainWindow::itfToolBarElecBearLine()
{
    OnDirAngleAction();
}

void MainWindow::itfToolBarDrawArea()
{
    OnWarringZONEAction();
}

void MainWindow::itfToolBarWarringZONESelected()
{
    OnWarringZONESelected();
}

void MainWindow::itfToolBarWarringZONEmove()
{
    OnWarringZONEmove();
}

void MainWindow::itfToolBarWarringZONEmoveCtrlPoint()
{
    OnWarringZONEmoveCtrlPoint();
}

void MainWindow::itfToolBarWarringZONEDelCtrlPoint()
{
    OnWarringZONEDelCtrlPoint();
}

void MainWindow::itfToolBarWarringZONEAddCtrlPoint()
{
    OnWarringZONEAddCtrlPoint();
}

void MainWindow::itfToolBarCoastDataLineAdd()
{
    OnCoastDataLineAdd();
}

void MainWindow::itfToolBarSeabedPipeLineAdd()
{
    OnSeabedPipeLineLineAdd();
}

void MainWindow::itfToolBarStructurePointAdd()
{
    OnStructurePointAdd();
}

void MainWindow::itfToolBarAreaNetAreaAdd()
{
    OnAreaNetZoneAdd();
}

void MainWindow::itfToolBarChannelAreaAdd()
{
    OnChannelAreaAdd();
}

void MainWindow::itfToolBarChannelZONESelected()
{
    OnChannelZONESelected();
}

void MainWindow::itfToolBarChannelZONEmove()
{
    OnChannelZONEMove();
}

void MainWindow::itfToolBarChannelZONEmoveCtrlPoint()
{
    OnChannelZONEMoveCtrlPoint();
}

void MainWindow::itfToolBarChannelZONEDelCtrlPoint()
{
    OnChannelZONEDelCtrlPoint();
}

void MainWindow::itfToolBarChannelZONEAddCtrlPoint()
{
    OnChannelZONEAddCtrlPoint();
}

void MainWindow::itfToolBarMooringAreaAdd()
{
    OnMooringAreaAdd();
}

void MainWindow::itfToolBarMooringZONESelected()
{
    OnMooringZONESelected();
}

void MainWindow::itfToolBarMooringZONEmove()
{
    OnMooringZONEMove();
}

void MainWindow::itfToolBarMooringZONEmoveCtrlPoint()
{
    OnMooringZONEMoveCtrlPoint();
}

void MainWindow::itfToolBarMooringZONEDelCtrlPoint()
{
    OnMooringZONEDelCtrlPoint();
}

void MainWindow::itfToolBarMooringZONEAddCtrlPoint()
{
    OnMooringZONEAddCtrlPoint();
}

void MainWindow::itfToolBarCardMouthAreaAdd()
{
    OnCardMouthAreaAdd();
}

void MainWindow::itfToolBarCardMouthZONESelected()
{
    OnCardMouthZONESelected();
}

void MainWindow::itfToolBarCardMouthZONEmove()
{
    OnCardMouthZONEMove();
}

void MainWindow::itfToolBarCardMouthZONEmoveCtrlPoint()
{
    OnCardMouthZONEMoveCtrlPoint();
}

void MainWindow::itfToolBarCardMouthZONEDelCtrlPoint()
{
    OnCardMouthZONEDelCtrlPoint();
}

void MainWindow::itfToolBarCardMouthZONEAddCtrlPoint()
{
    OnCardMouthZONEAddCtrlPoint();
}

void MainWindow::itfToolBarStatistcLineAreaAdd()
{
    OnStatistcLineAreaAdd();
}

void MainWindow::itfToolBarStatistcLineZONESelected()
{
    OnStatistcLineZONESelected();
}

void MainWindow::itfToolBarStatistcLineZONEmove()
{
    OnStatistcLineZONEMove();
}

void MainWindow::itfToolBarStatistcLineZONEmoveCtrlPoint()
{
    OnStatistcLineZONEMoveCtrlPoint();
}

void MainWindow::itfToolBarStatistcLineZONEDelCtrlPoint()
{
    OnStatistcLineZONEDelCtrlPoint();
}

void MainWindow::itfToolBarStatistcLineZONEAddCtrlPoint()
{
    OnStatistcLineZONEAddCtrlPoint();
}

void MainWindow::itfToolBarSelectPlayZone()
{
    OnSelectPlayZone();
}

void MainWindow::itfToolBarIslandLineAdd()
{
    OnIslandLineAdd();
}

void MainWindow::itfToolBarIslandLineSelected()
{
    OnIslandLineSelected();
}

void MainWindow::itfToolBarIslandLineMove()
{
    OnIslandLineMove();
}

void MainWindow::itfToolBarIslandLineMoveCtrlPoint()
{
    OnIslandLineMoveCtrlPoint();
}

void MainWindow::itfToolBarIslandLineDelCtrlPoint()
{
    OnIslandLineDelCtrlPoint();
}

void MainWindow::itfToolBarIslandLineAddCtrlPoint()
{
    OnIslandLineAddCtrlPoint();
}

void MainWindow::itfToolBarPickup()
{
    OnPickupAction();
}

void MainWindow::itfToolBarShowAll()
{
    OnShowAllAction();
}

void MainWindow::itfToolBarLocalMark()
{
    OnLocalMarkAction();
}

void MainWindow::itfToolBarSelectAnRegion()
{
    mMapWidget->selectAnRegion();
}

void MainWindow::itfToolBarLocationMark()
{
    mMapWidget->setLocationMark();
}

void MainWindow::itfToolBarFixedReferencePoint()
{
    mMapWidget->setFixedReferencePoint();
}

void MainWindow::itfToolBarZONESelected()
{
    OnZONESelected();
}

void MainWindow::itfToolBarDrawRadarArea()
{
    mMapWidget->setETool2DrawRadarZONE();
}
void MainWindow::itfToolBarGps()
{
    OnGpsAction();
}

void MainWindow::itfToolBarMapModelChanged(ZCHX::Data::ECDIS_DISPLAY_MODEL mod)
{
    switch (mod) {
    case ZCHX::Data::ECDIS_MODEL_BASE:
        OnSelBaseDisplayStyle();
        break;
    case ZCHX::Data::ECDIS_MODEL_STANDARD:
        OnSelStandardDisplayStyle();
        break;
    case ZCHX::Data::ECDIS_MODEL_ALL:
        OnSelAllDisplayStyle();
        break;
    default:
        OnSelBaseDisplayStyle();
        break;
    }
}

void MainWindow::itfToolBarColorModelChanged(ZCHX::Data::ECDISCOLOR mod)
{
    switch (mod) {
    case ZCHX::Data::ECDIS_COLOR_DAYBLACKBACK:
        OnSelDayBlackBackDisplyStyle();
        break;
    case ZCHX::Data::ECDIS_COLOR_NIGHT:
        OnSelNightDisplyStyle();
        break;
    case ZCHX::Data::ECDIS_COLOR_DAYDUSK:
        OnSelDayDUSKDisplyStyle();
        break;
    case ZCHX::Data::ECDIS_COLOR_DAYWHITEBACK:
        OnSelDayWhiteBackDisplyStyle();
        break;
    case ZCHX::Data::ECDIS_COLOR_DAYBRIGHT:
        OnSelDayBrightDisplyStyle();
        break;
    default:
        OnSelDayBlackBackDisplyStyle();
        break;
    }
}

void MainWindow::itfWarringZoneData4id(int uuid, ZCHX::Data::ITF_WarringZone &info, bool &ok)
{
    ok = m_dataMgrFactory->getWarningZoneMgr()->data4id(uuid, info);
}

bool MainWindow::itfWarringZoneDataByName(const QString &name, ZCHX::Data::ITF_WarringZone &info)
{
    return m_dataMgrFactory->getWarningZoneMgr()->dataByName(name, info);
}

bool MainWindow::itfWarringZoneDataByName(const std::string &name, ZCHX::Data::ITF_WarringZone &info)
{
    return m_dataMgrFactory->getWarningZoneMgr()->dataByName(name, info);
}

void MainWindow::itfIslandData4id(int uuid, ZCHX::Data::ITF_IslandLine &info, bool &ok)
{
    ok = m_dataMgrFactory->getIslandlineMgr()->data4id(uuid, info);
}

void MainWindow::iftSetElementFlashStep(int step)
{
    if(mMapWidget) mMapWidget->setWarnColorAlphaStep(step);
}

void MainWindow::iftSetIsWarningType(bool bWarningType)
{
    mMapWidget->setIsWarningType(bWarningType);
}

void MainWindow::itfAddLayer(std::shared_ptr<MapLayer> layer, std::shared_ptr<MapLayer> parent)
{
    m_mapLayerMgr->addLayer(layer, parent);
}

void MainWindow::itfAddLayer(const QString &curLayer, const QString &curDisplayName, bool curVisible, const QString &parentLayer)
{
    m_mapLayerMgr->addLayer(curLayer, curDisplayName, curVisible, parentLayer);
}

bool MainWindow::itfContainsLayer(const QString &type) const
{
    return m_mapLayerMgr->containsLayer(type);
}

QStringList MainWindow::itfGetLayerList() const
{
    return m_mapLayerMgr->getLayerList();
}

std::shared_ptr<MapLayer> MainWindow::itfGetLayer(const QString &type)
{
    return m_mapLayerMgr->getLayer(type);
}

const std::list<std::shared_ptr<MapLayer> > &MainWindow::itfGetLayerTree()
{
    return m_mapLayerMgr->getLayerTree();
}

void MainWindow::itfUpdateIPCastDeviceList(const QList<ZCHX::Data::IPCastDevice>& list)
{
    m_dataMgrFactory->getRodDataMgr()->updateIPCastDeviceList(list);
}

void MainWindow::itfSetGPSDataList(std::list<std::shared_ptr<ZCHX::Data::GPSPoint> > list)
{
    return mMapWidget->setGPSDataList(list);
}

void MainWindow::itfSetGPSData(std::shared_ptr<ZCHX::Data::GPSPoint> data)
{
    return mMapWidget->setGPSData(data);
}

void MainWindow::itfClearGPSData()
{
    return mMapWidget->clearGPSData();
}


void MainWindow::setCurrentProjectID(int id)
{
    mMapWidget->setCurrentProjectID(id);
}

void MainWindow::itfSetCableBaseDataList(const QList<ZCHX::Data::CableBaseData> &list)
{
    if(mMapWidget) mMapWidget->setCableBaseDataList(list);
}

void MainWindow::itfSetCableInterfaceDataList(const QList<ZCHX::Data::CableInterfaceData> &list)
{
    if(mMapWidget) mMapWidget->setCableInterfaceDataList(list);
}

void MainWindow::itfSetDisplayRouteIndex(bool display)
{
    if(mMapWidget) mMapWidget->setDisplayRouteIndex(display);
}

void MainWindow::itfSetDisplayRouteAc(bool display)
{
    if(mMapWidget) mMapWidget->setDisplayRouteAc(display);
}

void MainWindow::itfAppendFocusAis(const QStringList &list)
{
    m_dataMgrFactory->getAisDataMgr()->appendConcernList(list, true);
}

void MainWindow::itfRemoveFocusAis(const QStringList &list)
{
    m_dataMgrFactory->getAisDataMgr()->removeConcernList(list);
}

void MainWindow::itfAppendFocusRadar(const QStringList& list)
{
    m_dataMgrFactory->getRadarDataMgr()->appendConcernList(list, true);
}

void MainWindow::itfRemoveFocusRadar(const QStringList& list)
{
    m_dataMgrFactory->getRadarDataMgr()->removeConcernList(list);
}

void MainWindow::itfAppendRadarTailTrackList(const QStringList &list)
{
    m_dataMgrFactory->getRadarDataMgr()->appendRealtimeTailTrackList(list, false);
}

void MainWindow::itfRemoveRadarTailTrackList(const QStringList &list)
{
    m_dataMgrFactory->getRadarDataMgr()->removeRealtimeTailTrackList(list);
}
void MainWindow::itfAppendAisTailTrackList(const QStringList &list)
{
    m_dataMgrFactory->getAisDataMgr()->appendRealtimeTailTrackList(list, false);
}

void MainWindow::itfRemoveAisTailTrackList(const QStringList &list)
{
    m_dataMgrFactory->getAisDataMgr()->removeRealtimeTailTrackList(list);
}

void MainWindow::itfToolBarCameraNetGridAdd(const QSizeF& size, const QString& camera)
{
    mMapWidget->setETool2DrawCameraNetGrid(size, camera);
}

void MainWindow::itfSetCameraNetGridList(const QList<ZCHX::Data::ITF_NetGrid> & list)
{
    m_dataMgrFactory->getNetGridMgr()->setData(list);
}

void MainWindow::itfAppendItemDataMgr(std::shared_ptr<zchxEcdisDataMgr> mgr)
{
    m_dataMgrFactory->appendDataMgr(mgr);
}

void MainWindow::itfRemoveItemDataMgr(std::shared_ptr<zchxEcdisDataMgr> mgr)
{
    m_dataMgrFactory->removeDataMgr(mgr);
}

void MainWindow::itfPickUpPTZ()
{
    if(mMapWidget) mMapWidget->setETool2PickUpPTZ();
}

void MainWindow::setEcdisMapSource(ZCHX::TILE_SOURCE source)
{
    if(mMapWidget) mMapWidget->setSource(source);
}

void MainWindow::setCtrlFrameVisible(bool sts)
{
    ui->ctrl_frame->setVisible(sts);
    ui->pos_frame->setVisible(sts);
    ui->load_frame->setVisible(sts);
}

void MainWindow::setZoomLabelVisible(bool sts)
{
    if(mMapWidget) mMapWidget->setZoomLabelVisible(sts);
}

void MainWindow::setImgNumberVisible(bool sts)
{
    if(mMapWidget) mMapWidget->setImgNumberVisible(sts);
}

void MainWindow::itfAppendAisChart(const ZCHX::Data::ITF_AIS_Chart& data)
{
    m_dataMgrFactory->getAisChartMgr()->setData(data);
}

void MainWindow::itfAppendRadarRoutePathNodeList(const QList<ZCHX::Data::ITF_RadarRouteNode> &list)
{
    m_dataMgrFactory->getRadarRoutePathMgr()->setData(list);
}

void MainWindow::itfAppendPoint(const QList<ZCHX::Data::ITF_ElePos>& data)
{
    m_dataMgrFactory->getPointMgr()->setData(data);
}

void MainWindow::itfAppendLine(const QList<ZCHX::Data::ITF_EleLine>& data)
{
    m_dataMgrFactory->getLineMgr()->setData(data);
}

void MainWindow::itfAppendPolygon(const QList<ZCHX::Data::ITF_ElePolygon>& data)
{
    m_dataMgrFactory->getPolygonMgr()->setData(data);
}


}

