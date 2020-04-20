#include "zchxaidtonavigationdatamgr.h"
#include "zchxmapframe.h"
#include <QDebug>

namespace qt {
zchxAidtoNavigationDataMgr::zchxAidtoNavigationDataMgr(zchxMapWidget* w, QObject *parent)
    : zchxEcdisDataMgr(w, ZCHX::DATA_MGR_AIDTO_NAVIGATION, parent)
{

}

void zchxAidtoNavigationDataMgr::setAidtoNavigationDevData(const QList<ZCHX::Data::ITF_AidtoNavigation> &data)
{
    m_AidtoNavigationTraceDev.clear();

    foreach (const ZCHX::Data::ITF_AidtoNavigation & cam, data)
    {
        QString keyValue = QString::number(cam.id);
        std::shared_ptr<AidtoNavigationElement> ele = m_AidtoNavigationDev[keyValue];
        if (ele) {
            ele->setData(cam);
        } else {
            m_AidtoNavigationDev[keyValue] = std::shared_ptr<AidtoNavigationElement>(new AidtoNavigationElement(cam, mDisplayWidget));
        }

        std::pair<double,double> objPair(cam.lat, cam.lon);
        std::shared_ptr<AidtoNavigationTraceElement> traceEle = m_AidtoNavigationTraceDev[cam.name];
        if (traceEle) {
            ZCHX::Data::ITF_AidtoNavigationTrace traceData = traceEle->data();
            traceData.name = cam.name;
            traceData.path.push_back(objPair);
            traceEle->setData(traceData);
        } else {
            ZCHX::Data::ITF_AidtoNavigationTrace traceData;
            traceData.name = cam.name;
            traceData.path.push_back(objPair);
            m_AidtoNavigationTraceDev[cam.name] = std::shared_ptr<AidtoNavigationTraceElement>(new AidtoNavigationTraceElement(traceData, mDisplayWidget));
        }
    }
}

void zchxAidtoNavigationDataMgr::show(QPainter* painter)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_AIS_SITE)) return;

    QMap<QString, std::shared_ptr<AidtoNavigationTraceElement>>::iterator traceIt = m_AidtoNavigationTraceDev.begin();
    for(; traceIt != m_AidtoNavigationTraceDev.end(); ++traceIt)
    {
        std::shared_ptr<AidtoNavigationTraceElement> item = (*traceIt);

        item->drawElement(painter);
    }

    QMap<QString, std::shared_ptr<AidtoNavigationElement>>::iterator it = m_AidtoNavigationDev.begin();
    for(; it != m_AidtoNavigationDev.end(); ++it)
    {
        std::shared_ptr<AidtoNavigationElement> item = (*it);
        item->drawElement(painter);
    }
}

bool zchxAidtoNavigationDataMgr::updateActiveItem(const QPoint &pt)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_AIS_SITE) || !isPickupAvailable()) return false;
    return false;
}

void zchxAidtoNavigationDataMgr::updateAidtoNavigationStatus(const QString &id, int sts)
{
    std::shared_ptr<AidtoNavigationElement> ele = m_AidtoNavigationDev[id];
    if(ele)
    {
//        ele->setStatus(sts);
    }
}

Element *zchxAidtoNavigationDataMgr::selectItem(const QPoint &pt)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_AIS_SITE))
    {
        return NULL;
    }

    foreach(std::shared_ptr<AidtoNavigationElement> item, m_AidtoNavigationDev)
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


