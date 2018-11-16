#ifndef ZCHXRADARCTRLWORKER_H
#define ZCHXRADARCTRLWORKER_H

#include <QObject>
#include <QUdpSocket>
#include "side_car_parse/Messages/RadarConfig.h"

using namespace ZCHX::Messages::RadarConfig;

class zchxRadarCtrlWorker : public QObject
{
    Q_OBJECT
public:
    explicit zchxRadarCtrlWorker(QUdpSocket* socket, RadarConfig* cfg, QObject *parent = 0);

signals:

public slots:
private:
    QUdpSocket*     mCtrlSocket;
    RadarConfig*    mRadarCfg;
};

#endif // ZCHXRADARCTRLWORKER_H
