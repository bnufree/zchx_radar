#ifndef ZCHXRADARHEARTWORKER_H
#define ZCHXRADARHEARTWORKER_H

#include "zchxMulticastDataSocket.h"
#include <QThread>

class QTimer;

class zchxRadarHeartWorker : public zchxMulticastDataScoket
{
    Q_OBJECT
public:
    explicit zchxRadarHeartWorker(const QString& host,
                                  int port,
                                  int interval,
                                  QThread* thread,
                                  QObject *parent = 0);

    void    startHeart() {if(mHeartTimer) mHeartTimer->start();}

signals:

public slots:
    void slotHeartJob();

private:
    QThread*        mWorkThread;
    QTimer*         mHeartTimer;
};

#endif // ZCHXRADARHEARTWORKER_H
