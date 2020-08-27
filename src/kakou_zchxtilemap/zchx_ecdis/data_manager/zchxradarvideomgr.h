#ifndef ZCHXRADARVIDEODMGR_H
#define ZCHXRADARVIDEODMGR_H

#include "zchxtemplatedatamgr.h"

namespace qt {
class zchxRadarVideoMgr : public zchxTemplateDataMgr<RadarVideoImageElement, ZCHX::Data::ITF_RadarVideoImage>
{
    Q_OBJECT
public:
    explicit zchxRadarVideoMgr(zchxMapWidget* w, QObject *parent = 0);
    void    setRadarVideoData(int radarSiteId, double dCentreLon, double dCentreLat, double dDistance, const QByteArray &objPixmap);
signals:

public slots:

private:
    QMap<int, ZCHX::Data::ITF_RadarVideoImage> mRadarVideoDataMap;
};
}

#endif // ZCHXRADARVIDEODMGR_H
