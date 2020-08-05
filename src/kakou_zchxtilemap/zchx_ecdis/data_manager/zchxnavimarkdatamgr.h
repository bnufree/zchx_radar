#ifndef ZCHXNAVIMARKDATAMGR_H
#define ZCHXNAVIMARKDATAMGR_H

#include "zchxecdisdatamgr.h"

namespace qt {
class zchxNaviMarkDataMgr : public zchxEcdisDataMgr
{
    Q_OBJECT
public:
    explicit zchxNaviMarkDataMgr(zchxMapWidget* w, QObject *parent = 0);
    void    show(QPainter* painter);
    bool    updateActiveItem(const QPoint& pt);
    void    updateNaviMarkStatus(const QString &id, int sts);
    Element*    selectItem(const QPoint &pt);

    void setNaviMarkDevData(const QList<ZCHX::Data::ITF_NaviMark> &data);

private:
    QMap<QString, std::shared_ptr<NaviMarkElement> > m_NaviMarkDev;
};
}

#endif // ZCHXNAVIMARKDATAMGR_H
