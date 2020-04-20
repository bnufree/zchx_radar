#ifndef ZCHXRADARLIMITAREATHREAD_H
#define ZCHXRADARLIMITAREATHREAD_H

#include "zchxradarutils.h"

namespace ZCHX_RADAR_RECEIVER
{
class zchxRadarLimitAreaThread : public ZCHXReceiverThread
{
    Q_OBJECT
public:
    explicit zchxRadarLimitAreaThread(const ZCHX_Radar_Setting_Param& param, QObject *parent = 0);
    virtual void parseRecvData(const QByteArrayList& list);

private:
    void parseJsonFromByteArray(const QByteArray& data, QList<ZCHX::Data::ITF_IslandLine>& list );

signals:
    void sendMsg(const QList<ZCHX::Data::ITF_IslandLine>&);

public slots:
};
}

#endif // ZCHXRADARLIMITAREATHREAD_H

