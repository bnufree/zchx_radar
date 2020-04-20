#include "zchxradarsitedatamgr.h"
#include "zchxmapframe.h"
#include <QDebug>

namespace qt {
zchxRadarSiteDataMgr::zchxRadarSiteDataMgr(zchxMapWidget* w, QObject *parent)
    : zchxEcdisDataMgr(w, ZCHX::DATA_MGR_RADAR_SITE, parent)
{

}

void zchxRadarSiteDataMgr::setRadarSiteDevData(const QList<ZCHX::Data::ITF_RadarSite> &data)
{
    foreach (const ZCHX::Data::ITF_RadarSite & cam, data)
    {
        QString keyValue = QString::number(cam.id);
        std::shared_ptr<RadarSiteElement> ele = m_RadarSiteDev[keyValue];
        if (ele) {
            ele->setData(cam);
        } else {
            m_RadarSiteDev[keyValue] = std::shared_ptr<RadarSiteElement>(new RadarSiteElement(cam, mDisplayWidget));
        }
    }
}

void zchxRadarSiteDataMgr::show(QPainter* painter)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR_SITE)) return;
    QMap<QString, std::shared_ptr<RadarSiteElement>>::iterator it = m_RadarSiteDev.begin();
    for(; it != m_RadarSiteDev.end(); ++it)
    {
        std::shared_ptr<RadarSiteElement> item = (*it);
        item->drawElement(painter);
    }
}

bool zchxRadarSiteDataMgr::updateActiveItem(const QPoint &pt)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR_SITE) || !isPickupAvailable()) return false;
    return false;
}

void zchxRadarSiteDataMgr::updateRadarSiteStatus(const QString &id, int sts)
{
    std::shared_ptr<RadarSiteElement> ele = m_RadarSiteDev[id];
    if(ele)
    {
//        ele->setStatus(sts);
    }
}

}


