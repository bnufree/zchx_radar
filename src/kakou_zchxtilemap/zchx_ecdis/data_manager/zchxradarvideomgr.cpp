#include "zchxradarvideomgr.h"
#include "zchxmapframe.h"

using namespace qt;
zchxRadarVideoMgr::zchxRadarVideoMgr(zchxMapWidget *w, QObject *parent):
    zchxTemplateDataMgr<RadarVideoImageElement, ZCHX::Data::ITF_RadarVideoImage>(w, ZCHX::DATA_MGR_RADAR_VIDEO, ZCHX::LAYER_RADARVIDEO, parent)
{
}

void zchxRadarVideoMgr::setRadarVideoData(int radarSiteId, double dCentreLon, double dCentreLat, double dDistance, const QByteArray &objPixmap)
{
    ZCHX::Data::ITF_RadarVideoImage &mRadarVideoData = mRadarVideoDataMap[radarSiteId];
    mRadarVideoData.lat = dCentreLat;
    mRadarVideoData.lon = dCentreLon;
    mRadarVideoData.distance = dDistance;
    mRadarVideoData.image = objPixmap;
    mRadarVideoData.timestamp = QDateTime::currentMSecsSinceEpoch();
    setData(mRadarVideoDataMap.values());
}


