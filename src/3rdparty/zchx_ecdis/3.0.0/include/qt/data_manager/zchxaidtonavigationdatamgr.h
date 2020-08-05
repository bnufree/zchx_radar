#ifndef ZCHXAIDTONAVIGATIONDATAMGR_H
#define ZCHXAIDTONAVIGATIONDATAMGR_H

#include "zchxecdisdatamgr.h"

namespace qt {
class zchxAidtoNavigationDataMgr : public zchxEcdisDataMgr
{
    Q_OBJECT
public:
    explicit zchxAidtoNavigationDataMgr(zchxMapWidget* w, QObject *parent = 0);
    void    show(QPainter* painter);
    bool    updateActiveItem(const QPoint& pt);
    void    updateAidtoNavigationStatus(const QString &id, int sts);
    Element*    selectItem(const QPoint &pt);

    void setAidtoNavigationDevData(const QList<ZCHX::Data::ITF_AidtoNavigation> &data);

private:
    QMap<QString, std::shared_ptr<AidtoNavigationElement> > m_AidtoNavigationDev;
    QMap<QString, std::shared_ptr<AidtoNavigationTraceElement> > m_AidtoNavigationTraceDev;

};
}

#endif // ZCHXAIDTONAVIGATIONDATAMGR_H
