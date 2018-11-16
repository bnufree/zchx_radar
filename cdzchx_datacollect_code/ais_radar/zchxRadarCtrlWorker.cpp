#include "zchxRadarCtrlWorker.h"

zchxRadarCtrlWorker::zchxRadarCtrlWorker(QUdpSocket* socket, RadarConfig* cfg, QObject *parent) :
    QObject(parent),
    mCtrlSocket(socket),
    mRadarCfg(cfg)
{

}
