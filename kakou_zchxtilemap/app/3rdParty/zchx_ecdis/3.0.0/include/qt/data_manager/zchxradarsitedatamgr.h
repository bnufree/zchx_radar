#ifndef ZCHXRADARSITEDATAMGR_H
#define ZCHXRADARSITEDATAMGR_H

#include "zchxecdisdatamgr.h"

namespace qt {
class zchxRadarSiteDataMgr : public zchxEcdisDataMgr
{
    Q_OBJECT
public:
    explicit zchxRadarSiteDataMgr(zchxMapWidget* w, QObject *parent = 0);
    void    show(QPainter* painter);
    bool    updateActiveItem(const QPoint& pt);
    void    updateRadarSiteStatus(const QString &id, int sts);

    void setRadarSiteDevData(const QList<ZCHX::Data::ITF_RadarSite> &data);

private:
    QMap<QString, std::shared_ptr<RadarSiteElement> > m_RadarSiteDev;            //相机设备
};
}

#endif // ZCHXRADARSITEDATAMGR_H
