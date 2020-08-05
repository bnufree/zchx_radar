#include "zchxaissitedatamgr.h"
#include "zchxmapframe.h"
#include <QDebug>

namespace qt {
zchxAisSiteDataMgr::zchxAisSiteDataMgr(zchxMapWidget* w, QObject *parent)
    : zchxEcdisDataMgr(w, ZCHX::DATA_MGR_AIS_SITE, parent)
{

}

void zchxAisSiteDataMgr::setAisSiteDevData(const QList<ZCHX::Data::ITF_AisSite> &data)
{
    foreach (const ZCHX::Data::ITF_AisSite & cam, data)
    {
        QString keyValue = QString::number(cam.id);
        std::shared_ptr<AisSiteElement> ele = m_AisSiteDev[keyValue];
        if (ele) {
            ele->setData(cam);
        } else {
            m_AisSiteDev[keyValue] = std::shared_ptr<AisSiteElement>(new AisSiteElement(cam, mDisplayWidget));
        }
    }
}

void zchxAisSiteDataMgr::show(QPainter* painter)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_AIS_SITE)) return;
    QMap<QString, std::shared_ptr<AisSiteElement>>::iterator it = m_AisSiteDev.begin();
    for(; it != m_AisSiteDev.end(); ++it)
    {
        std::shared_ptr<AisSiteElement> item = (*it);
        item->drawElement(painter);
    }
}

bool zchxAisSiteDataMgr::updateActiveItem(const QPoint &pt)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_AIS_SITE) || !isPickupAvailable()) return false;
    return false;
}

void zchxAisSiteDataMgr::updateAisSiteStatus(const QString &id, int sts)
{
    std::shared_ptr<AisSiteElement> ele = m_AisSiteDev[id];
    if(ele)
    {
//        ele->setStatus(sts);
    }
}

Element *zchxAisSiteDataMgr::selectItem(const QPoint &pt)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_AIS_SITE))
    {
        return NULL;
    }

    foreach(std::shared_ptr<AisSiteElement> item, m_AisSiteDev)
    {
        //检查AIS图元本身是否选中
        if(item->contains(10, pt.x() * 1.0, pt.y() * 1.0))
        {
            return item.get();
        }
    }
    return NULL;
}

}


