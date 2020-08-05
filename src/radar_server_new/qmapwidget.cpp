#include "qmapwidget.h"
#include "qt/mainwindow.h"
#include <QVBoxLayout>

QMapWidget::QMapWidget(QWidget *parent) : QWidget(parent),
    mEcdis(new qt::MainWindow)
{
    setLayout(new QVBoxLayout());
    this->layout()->addWidget(mEcdis);
}

void QMapWidget::slotRecvRadarPointList(const TrackPointList &list)
{
    QList<ZCHX::Data::tagITF_RadarPoint> radarList;
    for(int i=0; i< list.size(); ++i)
    {
        ITF_Track_point data = list[i];
        ZCHX::Data::tagITF_RadarPoint item;
        for(int k=0; k<data.tracks().tracks_size(); k++)
        {
            item.path.push_back(std::pair<double,double>(
                                    data.tracks().tracks(k).wgs84poslat(),
                                    data.tracks().tracks(k).wgs84poslong()));
        }
        item.path.push_back(std::pair<double,double>(data.wgs84poslat(),data.wgs84poslong()));
        item.wgs84PosLat = data.wgs84poslat();
        item.wgs84PosLon = data.wgs84poslong();
        item.trackNumber = data.tracknumber();
        item.cog = data.cog();
        item.sog = data.sog();
        item.status = data.warn_status();
        item.warnStatusColor = Qt::red;
        radarList.append(item);
    }
    mEcdis->itfSetRadarPointData(radarList);
}
