#ifndef ZCHXAISSITEDATAMGR_H
#define ZCHXAISSITEDATAMGR_H

#include "zchxecdisdatamgr.h"

namespace qt {
class zchxAisSiteDataMgr : public zchxEcdisDataMgr
{
    Q_OBJECT
public:
    explicit zchxAisSiteDataMgr(zchxMapWidget* w, QObject *parent = 0);
    void    show(QPainter* painter);
    bool    updateActiveItem(const QPoint& pt);
    void    updateAisSiteStatus(const QString &id, int sts);
    Element*    selectItem(const QPoint &pt);

    void setAisSiteDevData(const QList<ZCHX::Data::ITF_AisSite> &data);

private:
    QMap<QString, std::shared_ptr<AisSiteElement> > m_AisSiteDev;
};
}

#endif // ZCHXAISSITEDATAMGR_H
