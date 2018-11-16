#ifndef ZCHXRADARHEARTWORKER_H
#define ZCHXRADARHEARTWORKER_H

#include <QObject>
#include <QUdpSocket>
#include <QThread>
#include "side_car_parse/Messages/RadarConfig.h"


using namespace ZCHX::Messages;
class QTimer;

class zchxRadarHeartWorker : public QObject
{
    Q_OBJECT
public:
    explicit zchxRadarHeartWorker(RadarConfig* cfg, QThread* thread, QObject *parent = 0);
    bool    isFine() const {return mInit;}
    void    startHeart() {if(mHeartTimer) mHeartTimer->start();}
    QUdpSocket* socket() {return mSocket;}
private:
    void init();

signals:

public slots:
    void slotHeartJob();

private:
    QUdpSocket*     mSocket;
    RadarConfig*    mRadarCfg;
    QThread*        mWorkThread;
    QTimer*         mHeartTimer;
    bool            mInit;
};

#endif // ZCHXRADARHEARTWORKER_H
