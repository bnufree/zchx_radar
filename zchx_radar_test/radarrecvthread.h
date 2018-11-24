#ifndef RADARRECVTHREAD_H
#define RADARRECVTHREAD_H

#include <QThread>
#include "protobuf/ZCHXRadar.pb.h"

using namespace com::zhichenhaixin::proto;
class RadarRecvThread : public QThread
{
    Q_OBJECT
public:
    explicit RadarRecvThread(QObject *parent = 0);
    void run();

signals:
    void signalSendTrackPointList(const TrackPointList& list);

public slots:
};

#endif // RADARRECVTHREAD_H
