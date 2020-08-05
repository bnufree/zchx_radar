#ifndef QMAPWIDGET_H
#define QMAPWIDGET_H

#include <QWidget>
#include "ais_radar/ZCHXRadar.pb.h"

namespace qt {
    class MainWindow;
}

class QMapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QMapWidget(QWidget *parent = 0);

signals:

public slots:
    void slotRecvRadarPointList(const TrackPointList& list);
private:
    qt::MainWindow*         mEcdis;
};

#endif // QMAPWIDGET_H
