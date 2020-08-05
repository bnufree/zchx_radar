#include "zchxradarvideomgr.h"
#include "zchxmapframe.h"

namespace qt {
zchxRadarVideoMgr::zchxRadarVideoMgr(zchxMapWidget *w, QObject *parent):
    zchxTemplateDataMgr<RadarVideoGlowElement, ZCHX::Data::ITF_RadarVideoGLow>(w, ZCHX::DATA_MGR_RADAR_VIDEO, ZCHX::LAYER_RADARVIDEO, parent)
{
}

void zchxRadarVideoMgr::setIsDisplay(bool sts)
{
    QMap<int, ZCHX::Data::ITF_RadarVideoGLow>::iterator it;
    for (it = mRadarVideoDataMap.begin(); it != mRadarVideoDataMap.end(); ++it)
    {
        ZCHX::Data::ITF_RadarVideoGLow & mRadarVideoData = mRadarVideoDataMap[it.key()];
        mRadarVideoData.showvideo = sts;
    }
    setData(mRadarVideoDataMap.values());
}

void zchxRadarVideoMgr::setRadarVideoData(int radarSiteId, double dCentreLon, double dCentreLat, double dDistance, int uDisplayType,int uLoopNum)
{
    if (!mRadarVideoDataMap.contains(radarSiteId))
    {
        ZCHX::Data::ITF_RadarVideoGLow mRadarVideoData;

        mRadarVideoData.name = QString::number(radarSiteId);
        mRadarVideoData.afterglowType = 3;
        mRadarVideoData.type = ZCHX::Data::ITF_RadarVideoGLow::RadarVideo;
        mRadarVideoData.afterglowIndex = 0;
        mRadarVideoData.showvideo = false;
//        QPixmap objPixmap(ZCHX::Data::RadarVideoPixmapWidth, ZCHX::Data::RadarVideoPixmapHeight);
//        objPixmap.fill(Qt::transparent);//用透明色填充
        mRadarVideoData.videoPixmap = NULL;
        for(int i = 0;i<12;i++)
        {
            mRadarVideoData.afterglowPixmap[i] = NULL;
        }

        mRadarVideoDataMap.insert(radarSiteId, mRadarVideoData);
    }

    ZCHX::Data::ITF_RadarVideoGLow & mRadarVideoData = mRadarVideoDataMap[radarSiteId];
    mRadarVideoData.lat = dCentreLat;
    mRadarVideoData.lon = dCentreLon;
    mRadarVideoData.distance = dDistance;
    mRadarVideoData.type = ZCHX::Data::ITF_RadarVideoGLow::RadarVideoGLowType(uDisplayType);
    mRadarVideoData.afterglowType = ((uLoopNum > 12) ? 12 : uLoopNum);

    setData(mRadarVideoDataMap.values());
}

void zchxRadarVideoMgr::setRadarVideoPixmap(int radarSiteId, int uIndex, const QByteArray &objPixmap, const QByteArray &prePixmap)
{
    if (!mRadarVideoDataMap.contains(radarSiteId))
    {
        return;
    }

    ZCHX::Data::ITF_RadarVideoGLow & mRadarVideoData = mRadarVideoDataMap[radarSiteId];
    if(mRadarVideoData.type == ZCHX::Data::ITF_RadarVideoGLow::RadarVideo)
    {
        if (mRadarVideoData.videoPixmap != NULL)
        {
            delete mRadarVideoData.videoPixmap;
        }

        mRadarVideoData.videoPixmap = new QByteArray(objPixmap);
    } else {
//        if(uIndex >= 12) return;
//        //最新的数据 固定放在5,4位置
//        mRadarVideoData.afterglowPixmap[5] = objPixmap;
//        mRadarVideoData.afterglowIndex = 5;
//        for(int i=1; i<5; ++i)
//        {
//            mRadarVideoData.afterglowPixmap[i-1] = mRadarVideoData.afterglowPixmap[i];
//        }
//        mRadarVideoData.afterglowPixmap[4] = prePixmap;
    }
    mRadarVideoData.showvideo = true;

    setData(mRadarVideoDataMap.values());
}

void zchxRadarVideoMgr::setCurrentRadarVideoPixmap(int radarSiteId, const QByteArray &objPixmap)
{
    if (!mRadarVideoDataMap.contains(radarSiteId))
    {
        return;
    }

    ZCHX::Data::ITF_RadarVideoGLow & mRadarVideoData = mRadarVideoDataMap[radarSiteId];
    if(mRadarVideoData.type == ZCHX::Data::ITF_RadarVideoGLow::RadarVideo)
    {
        if (mRadarVideoData.videoPixmap != NULL)
        {
            delete mRadarVideoData.videoPixmap;
        }

        mRadarVideoData.videoPixmap = new QByteArray(objPixmap);
    } else {
//        mRadarVideoData.afterglowPixmap[5] = objPixmap;
//        mRadarVideoData.afterglowIndex = 5;
    }
    mRadarVideoData.showvideo = true;

    setData(mRadarVideoDataMap.values());
}

}
