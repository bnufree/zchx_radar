#include "zchxradardatamgr.h"
#include "zchxmapframe.h"

namespace qt {
zchxRadarDataMgr::zchxRadarDataMgr(zchxMapWidget* w, QObject *parent)
    : zchxEcdisDataMgr(w, ZCHX::DATA_MGR_RADAR, parent)
    , m_showRadarLabel(false)
    , m_targetSizeIndex(0)
    , m_traceLenIndex(0)
    , m_continueTimeIndex(0)
{
    mMaxConcernNum = Profiles::instance()->value(RADAR_DISPLAY_SETTING, RADAR_CONCERN_NUM, 10).toInt();
    mReplaceConcernWhenOver = Profiles::instance()->value(RADAR_DISPLAY_SETTING, RADAR_REPLACE_CONCERN, true).toBool();
    mMaxRealtimeTailTrackNum = Profiles::instance()->value(RADAR_DISPLAY_SETTING, RADAR_TAIL_TRACK_NUM, 10).toInt();
    mReplaceRealtimeTailTrackWhenOver = Profiles::instance()->value(RADAR_DISPLAY_SETTING, RADAR_REPLACE_TAIL_TRACK, true).toBool();
    mMaxHistoryTrackNum = Profiles::instance()->value(RADAR_DISPLAY_SETTING, RADAR_HISTORY_TRACK_NUM, 10).toInt();
    mReplaceHistoryTrackWhenOver = Profiles::instance()->value(RADAR_DISPLAY_SETTING, RADAR_REPLACE_HISTORY_TRACK, true).toBool();
}

void zchxRadarDataMgr::show(QPainter* painter)
{
    if(mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR))
    {
        QMutexLocker locker(&mDataMutex);

        QMap<int, QMap<QString, std::shared_ptr<RadarPointElement> > >::iterator topIt = m_RadarPointMap.begin();
        for(; topIt != m_RadarPointMap.end(); ++topIt)
        {
            const QMap<QString, std::shared_ptr<RadarPointElement> > &radarPoint
                    = m_RadarPointMap.value(topIt.key());
            QMap<QString, std::shared_ptr<RadarPointElement>>::const_iterator it = radarPoint.begin();

            for(; it != radarPoint.end(); ++it)
            {
                std::shared_ptr<RadarPointElement> item = it.value();
                if(mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR_CURRENT))
                {
                    item->setIsOpenMeet(mDisplayWidget->getIsOpenMeet());
                    //检查当前点是否在矩形区域内
                    if (!mDisplayWidget->rect().contains(item->getCurrentPos().toPoint()))
                    {
                        continue;
                    }

                    //显示设置
                    bool showStatus = isRadarDisplayByTargetSize(item->getData(), m_targetSizeIndex);
                    if (!showStatus)
                    {
                        continue;
                    }

                    item->setShowRadarLabel(m_showRadarLabel);

                    QString id = QString::number(item->getData().trackNumber);
                    if(isConcern(id)){
                        item->setIsConcern(true);
                    } else {
                        item->setIsConcern(false);
                    }
                    if(isRealtimeTailTrack(id)){
                        item->setIsRealtimeTailTrack(true);
                    } else {
                        item->setIsRealtimeTailTrack(false);
                    }

                    if(isHistoryTrack(id)){
                        item->setIsHistoryTrack(true);
                    } else {
                        item->setIsHistoryTrack(false);
                    }

                    if(item.get() == mDisplayWidget->getCurrentSelectedElement()) {
                        item->setIsActive(true);
                    } else {
                        item->setIsActive(false);
                    }
                    //预警状态, 闪烁; 0为无预警
                    if(item->getStatus() > 0)
                    {
                        item->drawFlashRegion(painter, item->getCurrentPos(), item->getStatus(), item->getData().warnStatusColor);
                    }
                    item->drawElement(painter);
                }
                if(mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR_TRACK) && item->getIsRealtimeTailTrack())
                {
                    item->drawTrack(painter);
                }
            }
        }
    }

    if(mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_HISTORY_RADAR))
    {
        for(int i = 0; i < m_HistoryRadarPoint.size(); i++)
        {
            std::shared_ptr<RadarPointElement> item = m_HistoryRadarPoint.at(i);
            //检查当前点是否在矩形区域内

            if (!mDisplayWidget->rect().contains(item->getCurrentPos().toPoint()))
            {
                continue;
            }

            //显示设置
            bool showStatus = isRadarDisplayByTargetSize(item->getData(), m_targetSizeIndex);
            if (!showStatus)
            {
                continue;
            }

            QString id = QString::number(item->getData().trackNumber);
            if(isConcern(id)){
                item->setIsConcern(true);
            } else {
                item->setIsConcern(false);
            }
            if(isRealtimeTailTrack(id)){
                item->setIsRealtimeTailTrack(true);
            } else {
                item->setIsRealtimeTailTrack(false);
            }

            if(isHistoryTrack(id)){
                item->setIsHistoryTrack(true);
            } else {
                item->setIsHistoryTrack(false);
            }

            //预警状态, 闪烁; 0为无预警
            if(item->getStatus() > 0)
            {
                item->drawFlashRegion(painter, item->getCurrentPos(), item->getStatus(), item->getData().warnStatusColor);
            }
            item->drawElement(painter);
        }
    }
}

void zchxRadarDataMgr::setRadarPointData(int radarSiteId, const QList<ZCHX::Data::ITF_RadarPoint> &data)
{
    QMutexLocker locker(&mDataMutex);
    //删除当前实时数据中不存在的旧数据
    QList<QString> newDataId;
    for(ZCHX::Data::ITF_RadarPoint radar : data)
    {
        newDataId.append(QString::number(radar.trackNumber));
    }
    if (m_RadarPointMap.contains(radarSiteId))
    {
        QMap<QString, std::shared_ptr<RadarPointElement> > & radarPointMap
                = m_RadarPointMap[radarSiteId];
        QList<QString> oldDataId = radarPointMap.uniqueKeys();
        for(int number = 0; number < oldDataId.size(); ++number)
        {
            QString trackID = oldDataId.at(number);
            if(!newDataId.contains(trackID))
            {
                //取消原来数据的关注和尾迹
                removeConcern(trackID);
                removeRealtimeTailTrack(trackID);
                removeHistoryTrack(trackID);
                std::shared_ptr<RadarPointElement> item = radarPointMap.value(trackID, 0);
                if(item) item.reset();
                //删除对应的实时数据
                radarPointMap.remove(trackID);
            }
        }
    }
    else
    {
        m_RadarPointMap.insert(radarSiteId, QMap<QString, std::shared_ptr<RadarPointElement> >());
    }

    QMap<QString, std::shared_ptr<RadarPointElement> > & radarPointMap
            = m_RadarPointMap[radarSiteId];
    //更新实时雷达数据
    for (const ZCHX::Data::ITF_RadarPoint &aisdata : data)
    {
        //更新item对应的数据
        QString id = QString::number(aisdata.trackNumber);
        std::shared_ptr<RadarPointElement> item = radarPointMap.value(id, 0);
        if(!item) {
            item = std::shared_ptr<RadarPointElement>(new RadarPointElement(aisdata, mDisplayWidget));
            radarPointMap[id] = item;
        } else {
            item->setData(aisdata);
        }
        item->setUpdateUTC(QDateTime::currentMSecsSinceEpoch());
    }

#if 0
    if (m_bCameraTargerTrack &&m_cameraTrackTarget.type == 1)
    {
        AisElement *item = m_aisMap.value(m_cameraTrackTarget.id, NULL);
        if(item)
        {
                m_cameraTrackTarget.id = item->getStrID(); //设置当前选中的船舶的
                m_cameraTrackTarget.lon = item->lon();
                m_cameraTrackTarget.lat = item->lat();
                m_cameraTrackTarget.type = 1;
                emit signalIsEcdisCameraTrackTarget(m_cameraTrackTarget);
                //releaseDrawStatus();
        }
        else // 目标丢失(千米目标丢失的给移除了)
        {
            m_cameraTrackTarget.type = 0;
            //m_bCameraTargerTrack = false;
        }
    }
#endif
    //TODO: 判断是否需要移除过长时间没有数据更新的对象
}

bool zchxRadarDataMgr::updateActiveItem(const QPoint &pt)
{
    if(!mDisplayWidget ||
       !mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR) ||
       !mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR_CURRENT) ||
       !isPickupAvailable()) return false;

    Element* ele = selectItem(pt);
    if(ele)
    {
        mDisplayWidget->setCurrentSelectedItem(ele);
        return true;
    }

    return false;
}

Element* zchxRadarDataMgr::selectItem(const QPoint &pt)
{
    if( !mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR) || !mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_RADAR_CURRENT)) return 0;

    QMap<int, QMap<QString, std::shared_ptr<RadarPointElement> > >::iterator topIt = m_RadarPointMap.begin();
    for(; topIt != m_RadarPointMap.end(); ++topIt)
    {
        const QMap<QString, std::shared_ptr<RadarPointElement> > &radarPoint
                = m_RadarPointMap.value(topIt.key());
        foreach (std::shared_ptr<RadarPointElement> item, radarPoint) {
            if(item->contains(10, pt.x(), pt.y())){
                return item.get();
            }
        }
    }

    return 0;
}

void zchxRadarDataMgr::setClearHistoryData(bool states)
{
    if (states)
    {
        m_HistoryRadarPoint.clear();
    }
}

void zchxRadarDataMgr::SetRadarLabelVisible(bool showRadarLabel)
{
    m_showRadarLabel = showRadarLabel;
}

void zchxRadarDataMgr::SetRadarDisplayInfo(bool showRadarLabel, int targetSizeIndex, int traceLenIndex, int continueTimeIndex)
{
    m_showRadarLabel = showRadarLabel;
    m_targetSizeIndex = targetSizeIndex;
    m_traceLenIndex = traceLenIndex;
    m_continueTimeIndex = continueTimeIndex;
}

void zchxRadarDataMgr::setPickUpRadarPoint(QString id)
{
    mDisplayWidget->setCurrentSelectedItem(0);
    mDisplayWidget->setCurPickupType(ZCHX::Data::ECDIS_PICKUP_RADARPOINT);
    //将当前所有的船舶选择状态清空    
    QMap<int, QMap<QString, std::shared_ptr<RadarPointElement> > >::iterator topIt = m_RadarPointMap.begin();
    for(; topIt != m_RadarPointMap.end(); ++topIt)
    {
        const QMap<QString, std::shared_ptr<RadarPointElement> > &radarPoint
                = m_RadarPointMap.value(topIt.key());
        QMap<QString, std::shared_ptr<RadarPointElement>>::const_iterator it = radarPoint.begin();
        for(; it != radarPoint.end(); ++it)
        {
            it.value()->setIsActive(false);
        }

        //将当前id的船舶设定为active状态
        if(id.isEmpty()) return;
        std::shared_ptr<RadarPointElement> item = radarPoint.value(id, NULL);
        if(!item) return;
        item->setIsActive(true);
        //将当前的船舶居中放大显示
        std::pair<double, double> ll = item->getLatLon();
        mDisplayWidget->setCenterAtTargetLL(ll.first, ll.second);
        //更新当前选择的图元目标
        mDisplayWidget->setCurrentSelectedItem(item.get());
    }
}


void zchxRadarDataMgr::setRadarAreaData(const std::vector<RadarAreaElement> &data)
{
    m_RadarArea = data;
}

void zchxRadarDataMgr::setHistoryRadarPointData(const QList<ZCHX::Data::ITF_RadarPoint> &data)
{
    //存储上一次雷达的转向(绘制雷达的当前转向)
//    m_pRadarPointHistory.clear();
//    if(m_HistoryRadarPoint.size() > 1)
//    {
//        for(int i=0; i < m_HistoryRadarPoint.size(); ++i)
//        {
//            RadarPointElement &item  = m_HistoryRadarPoint[i];
//            double point = item.getData().cog;
//            int radar_number = item.getData().trackNumber;
//            m_pRadarPointHistory.insert(radar_number, point);
//        }
//    }

    m_HistoryRadarPoint.clear();
    for (int i = 0; i < data.size(); i++)
    {
        std::shared_ptr<RadarPointElement> item = std::shared_ptr<RadarPointElement>(new RadarPointElement(data.at(i), mDisplayWidget));
        m_HistoryRadarPoint.append(item);
    }

#if 0

    if (m_bCameraTargerTrack)
    {
        if(m_cameraTrackTarget.type == 2) // 上次数据为雷达
        {
            bool isFind = false;
            RadarPoint *pRadarPointItem = NULL;
            for(int i=0; i< m_HistoryRadarPoint.size(); ++i)
            {
                RadarPoint &item = m_HistoryRadarPoint[i];
                pRadarPointItem = &item;
                if(pRadarPointItem)
                {
                    ZCHX::Data::ITF_RadarPoint radarPonintData = pRadarPointItem->getData();
                    QString numberid = QString::number(radarPonintData.trackNumber);
                    if(numberid == m_cameraTrackTarget.id)
                    {
                        isFind = true;
                        std::vector<std::pair<double, double>> path = radarPonintData.path;
                        int point = path.size();
                        std::pair<double, double> ll = path[point -1];

                        m_cameraTrackTarget.id = QString::number(radarPonintData.trackNumber); //设置当前选中的船舶的
                        m_cameraTrackTarget.lon = ll.second;
                        m_cameraTrackTarget.lat = ll.first;
                        m_cameraTrackTarget.type = 2;
                        emit signalIsEcdisCameraTrackTarget(m_cameraTrackTarget);
                        //releaseDrawStatus();
                        break;
                    }
                }
            }
            if (!isFind) // 目标丢失
            {
                m_cameraTrackTarget.type = 0;
                //m_bCameraTargerTrack = false;
            }
        }
    }
    //判断是否用鼠标选择了船
    bool ok = false;
    uint trackNumber =  m_curActivShip.toInt(&ok);
    if(!ok)
    {
        return;
    }
    for(int i=0; i < m_HistoryRadarPoint.size(); ++i)
    {
        RadarPoint &item  = m_HistoryRadarPoint[i];
        const ZCHX::Data::ITF_RadarPoint& point = item.getData();
        if(point.trackNumber == trackNumber)
        {
            item.setIsActive(true);
            //emit signalIsSelected4RadarPoint(point,true);
            return;
        }
        else
        {
            item.setIsActive(false);
        }
    }
#endif
}


QList<QAction*> zchxRadarDataMgr::getRightMenuActions(const QPoint &pt)
{
    QList<QAction*> list;
    //获取当前选择的目标对象
    if(mDisplayWidget)
    {
        Element* item = mDisplayWidget->getCurrentSelectedElement();
        if(item && item->getElementType() == ZCHX::Data::ELE_RADAR_POINT)
        {
            //目标确定为AIS,弹出对应的右键菜单
            RadarPointElement* ele = static_cast<RadarPointElement*>(item);
            if(ele){
                list.append(addAction(tr("画中画"),this, SLOT(setPictureInPicture()), (void*) ele));
                list.append(addAction(tr("实时轨迹"),this, SLOT(setRealTimeTraces()), (void*) ele));
                list.append(addAction(tr("关注"),this, SLOT(setConcern()), (void*) ele));
                list.append(addAction(tr("联动"),this, SLOT(invokeLinkageSpot()), (void*) ele));
            }
        }
    }
    return list;
}


void zchxRadarDataMgr::setPictureInPicture()
{
    RadarPointElement* ele = static_cast<RadarPointElement*>(getElementOfSender());
    if(!ele) return;
    if(mDisplayWidget)
    {
        mDisplayWidget->signalSendPictureInPictureTarget(ele->getElementType(), QString::number(ele->getData().trackNumber));
    }
}

bool zchxRadarDataMgr::isRadarDisplayByTargetSize(const ZCHX::Data::ITF_RadarPoint &data, int targetSizeIndex)
{
    if (targetSizeIndex == 0)
    {
        return true;
    }

    switch (targetSizeIndex)
    {
    case 1:
        if (data.diameter >= 200 && data.diameter < 300)
        {
           return true;
        }
        else
        {
            return false;
        }
    case 2:
        if (data.diameter >= 100 && data.diameter < 200)
        {
           return true;
        }
        else
        {
            return false;
        }
    case 3:
        if (data.diameter >= 50 && data.diameter < 100)
        {
           return true;
        }
        else
        {
            return false;
        }
    default:
        return false;
    }
}

void zchxRadarDataMgr::setRealTimeTraces()
{
    RadarPointElement* ele = static_cast<RadarPointElement*>(getElementOfSender());
    if(!ele) return;
    QString id = QString::number(ele->getData().trackNumber);
    if(isRealtimeTailTrack(id))
    {
        removeRealtimeTailTrack(id);
    } else
    {
        appendRealtimeTailTrackList(QStringList()<<id, true);
    }
    if(mDisplayWidget) mDisplayWidget->signalSendRealTimeTrail(id, isRealtimeTailTrack(id));
}

void zchxRadarDataMgr::invokeLinkageSpot()
{
    RadarPointElement* ele = static_cast<RadarPointElement*>(getElementOfSender());
    if(!ele) return;
    ZCHX::Data::ITF_CloudHotSpot data;
    data.fllow = ZCHX::Data::ITF_CloudHotSpot::FLLOW_TYPE_LINKAGE_TRACKING;
    data.mode = ZCHX::Data::ITF_CloudHotSpot::MODE_HANDLE;
    data.targetNumber = QString::number(ele->getData().trackNumber);
    data.targetType = 2;
    data.targetLon = ele->getData().getLon();
    data.targetLat = ele->getData().getLat();
    if(mDisplayWidget) mDisplayWidget->signalInvokeHotSpot(data);
}

void zchxRadarDataMgr::setConcern()
{
    RadarPointElement* ele = static_cast<RadarPointElement*>(getElementOfSender());
    if(!ele) return;
    QString id = QString::number(ele->getData().trackNumber);
    appendConcernList(QStringList()<<id, true);
}

}
