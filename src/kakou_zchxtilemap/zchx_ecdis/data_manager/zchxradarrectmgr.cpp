#include "zchxRadarRectMgr.h"
#include "zchxmapframe.h"
#include "zchxutils.hpp"

namespace qt {
zchxRadarRectMgr::zchxRadarRectMgr(zchxMapWidget *w, QObject *parent):
    zchxTemplateDataMgr<RadarRectGlowElement, ZCHX::Data::ITF_RadarRect>(w, ZCHX::DATA_MGR_RADAR_RECT, ZCHX::LAYER_RADARRECT, parent)
  , m_showRadarLabel(false)
  , m_targetSizeIndex(0)
  , m_traceLenIndex(0)
  , m_continueTimeIndex(0)
{
}

void zchxRadarRectMgr::setIsDisplay(bool sts)
{
    if (sts)
    {
        QMutexLocker locker(&m_mutex);

        QMap<int, QList<ZCHX::Data::ITF_RadarRect> >::iterator it;
        for (it = mRectMap.begin(); it != mRectMap.end(); ++it)
        {
            setData(mRectMap[it.key()]);
        }
    }
}

void zchxRadarRectMgr::SetRadarLabelVisible(bool showRadarLabel)
{
    m_showRadarLabel = showRadarLabel;
}

void zchxRadarRectMgr::SetRadarDisplayInfo(bool showRadarLabel, int targetSizeIndex, int traceLenIndex, int continueTimeIndex)
{
    m_showRadarLabel = showRadarLabel;
    m_targetSizeIndex = targetSizeIndex;
    m_traceLenIndex = traceLenIndex;
    m_continueTimeIndex = continueTimeIndex;
}

void zchxRadarRectMgr::setRadarRect(int radarSiteId, QList<ZCHX::Data::ITF_RadarRect> rectList)
{
    m_mutex.lock();

    if (!mRectMap.contains(radarSiteId))
    {
        mRectMap.insert(radarSiteId, QList<ZCHX::Data::ITF_RadarRect>());
    }
    QList<ZCHX::Data::ITF_RadarRect> &mRectList = mRectMap[radarSiteId];
    mRectList.clear();
    for (int i = 0; i < rectList.size(); i++)
    {        
        //显示设置
//        bool showStatus = isRadarDisplayByTargetSize(rectList.at(i));
//        if (!showStatus)
//        {
//            continue;
//        }

//        showStatus = isRadarDisplayByTraceLen(rectList.at(i));
//        if (!showStatus)
//        {
//            continue;
//        }

//        showStatus = isRadarDisplayByContinueTime(rectList.at(i));
//        if (!showStatus)
//        {
//            continue;
//        }

        mRectList.append(rectList.at(i));
    }

    QList<ZCHX::Data::ITF_RadarRect> rectLists;
    QMap<int, QList<ZCHX::Data::ITF_RadarRect> >::iterator it;
    for (it = mRectMap.begin(); it != mRectMap.end(); ++it)
    {
        rectLists.append(it.value());
    }
    m_mutex.unlock();

    setData(rectLists);
}

//bool zchxRadarRectMgr::isRadarDisplayByTargetSize(const ZCHX::Data::ITF_RadarRect &data)
//{
//    if (m_targetSizeIndex == 0)
//    {
//        return true;
//    }
	
//    int diameter = data.diameter;
	
//    switch (m_targetSizeIndex)
//    {
//    case 1:
//        if (diameter >= 200 && diameter < 300)
//        {
//            return true;
//        }
//        else
//        {
//            return false;
//        }
//        break;
//    case 2:
//        if (diameter >= 100 && diameter < 200)
//        {
//            return true;
//        }
//        else
//        {
//            return false;
//        }
//        break;
//    case 3:
//        if (diameter >= 50 && diameter < 100)
//        {
//            return true;
//        }
//        else
//        {
//            return false;
//        }
//        break;
//    default:
//        break;
//    }
	
//    return false;
//}

//bool zchxRadarRectMgr::isRadarDisplayByTraceLen(const ZCHX::Data::ITF_RadarRect &data)
//{
//    if (m_traceLenIndex == 0)
//    {
//        return true;
//    }

//    if (data.rects.size() < 2)
//    {
//        return true;
//    }

//    int traceLen = ZCHX::Utils::instance()->getDistanceDeg(data.rects.at(0).centerlatitude,
//                                                           data.rects.at(0).centerlongitude,
//                                                           data.rects.at(data.rects.size() - 1).centerlatitude,
//                                                           data.rects.at(data.rects.size() - 1).centerlongitude);

//    switch (m_traceLenIndex)
//    {
//    case 1:
//        if (traceLen >= 200 && traceLen < 300)
//        {
//           return true;
//        }
//        else
//        {
//            return false;
//        }
//    case 2:
//        if (traceLen >= 100 && traceLen < 200)
//        {
//           return true;
//        }
//        else
//        {
//            return false;
//        }
//    case 3:
//        if (traceLen >= 50 && traceLen < 100)
//        {
//           return true;
//        }
//        else
//        {
//            return false;
//        }
//    case 4:
//        if (traceLen < 50)
//        {
//           return true;
//        }
//        else
//        {
//            return false;
//        }
//    default:
//        return false;
//    }
//}

//bool zchxRadarRectMgr::isRadarDisplayByContinueTime(const ZCHX::Data::ITF_RadarRect &data)
//{
//    if (m_continueTimeIndex == 0)
//    {
//        return true;
//    }

//    if (data.rects.size() < 2)
//    {
//        return true;
//    }

//    int continueTime = data.rects.at(0).timeOfDay -
//            data.rects.at(data.rects.size() - 1).timeOfDay;

//    if (continueTime >= 5 && continueTime <= 30)
//    {
//        return true;
//    }
//    else
//    {
//        return false;
//    }
//}

}

