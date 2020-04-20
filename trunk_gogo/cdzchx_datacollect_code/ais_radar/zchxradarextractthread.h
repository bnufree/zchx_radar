#ifndef ZCHXRADAREXTRACTTHREAD_H
#define ZCHXRADAREXTRACTTHREAD_H

#include "Messages/RadarConfig.h"
#include <QThread>
#include <QMutex>
#include "zchxradarcommon.h"

using namespace ZCHX;
using namespace ZCHX::Messages;

class zchxRadarExtractionThread : public QThread
{
    Q_OBJECT
public:
    zchxRadarExtractionThread(RadarConfig* cfg, QObject* parent = 0);
    void appendTask(const zchxVideoFrameList& list);
    bool getTask(zchxVideoFrameList& list);
    void run();
signals:
    void sendTrackPoint(const zchxTrackPointList& list);

private:
    QList<zchxVideoFrameList>  mFrameList;       //task list;
    QMutex      mTaskMutex;
    RadarConfig *mCfg;

};

#endif // ZCHXRADAREXTRACTTHREAD_H
