#ifndef ZCHXRADARVIDEODMGR_H
#define ZCHXRADARVIDEODMGR_H

#include "zchxtemplatedatamgr.h"

namespace qt {
class zchxRadarVideoMgr : public zchxTemplateDataMgr<RadarVideoGlowElement, ZCHX::Data::ITF_RadarVideoGLow>
{
    Q_OBJECT
public:
    explicit zchxRadarVideoMgr(zchxMapWidget* w, QObject *parent = 0);
    void    setIsDisplay(bool sts);
    void    setRadarVideoData(int radarSiteId, double dCentreLon, double dCentreLat, double dDistance, int uDisplayType,int uLoopNum);
    void    setRadarVideoPixmap(int radarSiteId, int uIndex, const QByteArray &objPixmap, const QByteArray &prePixmap);
    void    setCurrentRadarVideoPixmap(int radarSiteId, const QByteArray &objPixmap);
signals:

public slots:

private:
    QMap<int, ZCHX::Data::ITF_RadarVideoGLow> mRadarVideoDataMap;
};
}

#endif // ZCHXRADARVIDEODMGR_H
