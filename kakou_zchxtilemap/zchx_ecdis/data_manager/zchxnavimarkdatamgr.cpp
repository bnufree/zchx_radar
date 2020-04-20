#include "zchxnavimarkdatamgr.h"
#include "zchxmapframe.h"
#include <QDebug>

namespace qt {
zchxNaviMarkDataMgr::zchxNaviMarkDataMgr(zchxMapWidget* w, QObject *parent)
    : zchxEcdisDataMgr(w, ZCHX::DATA_MGR_NAVIMARK, parent)
{

}

void zchxNaviMarkDataMgr::setNaviMarkDevData(const QList<ZCHX::Data::ITF_NaviMark> &data)
{
    foreach (const ZCHX::Data::ITF_NaviMark & item, data)
    {
        QString keyValue = QString::number(item.id);
        std::shared_ptr<NaviMarkElement> ele = m_NaviMarkDev[keyValue];
        if (ele) {
            ele->setData(item);
        } else {
            m_NaviMarkDev[keyValue] = std::shared_ptr<NaviMarkElement>(new NaviMarkElement(item, mDisplayWidget));
        }
    }
}

void zchxNaviMarkDataMgr::show(QPainter* painter)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_NAVIMARK)) return;
    QMap<QString, std::shared_ptr<NaviMarkElement>>::iterator it = m_NaviMarkDev.begin();
    for(; it != m_NaviMarkDev.end(); ++it)
    {
        std::shared_ptr<NaviMarkElement> item = (*it);
        item->drawElement(painter);
    }
}

bool zchxNaviMarkDataMgr::updateActiveItem(const QPoint &pt)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_NAVIMARK) || !isPickupAvailable()) return false;
    return false;
}

void zchxNaviMarkDataMgr::updateNaviMarkStatus(const QString &id, int sts)
{
    std::shared_ptr<NaviMarkElement> ele = m_NaviMarkDev[id];
    if(ele)
    {
//        ele->setStatus(sts);
    }
}

Element *zchxNaviMarkDataMgr::selectItem(const QPoint &pt)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_NAVIMARK))
    {
        return NULL;
    }

    foreach(std::shared_ptr<NaviMarkElement> item, m_NaviMarkDev)
    {
        //检查图元本身是否选中
        if(item->contains(10, pt.x() * 1.0, pt.y() * 1.0))
        {
            return item.get();
        }
    }
    return NULL;
}

}


