#ifndef ZCHXRADARREPORTWORKER_H
#define ZCHXRADARREPORTWORKER_H

#include <QObject>
#include "zchxMulticastDataSocket.h"
#include "radarccontroldefines.h"

class zchxRadarReportWorker : public zchxMulticastDataScoket
{
    Q_OBJECT
public:
    explicit zchxRadarReportWorker(const QString& host,
                                   int port,
                                   QThread* thread,
                                   QObject* parent = 0);
    void processRecvData(const QByteArray &data);
    void updateValue(INFOTYPE controlType, int value);

signals:
    void signalRadarStatusChanged(const RadarStatus& sts);

private:
    QThread*        mWorkThread;
    QMap<INFOTYPE, RadarStatus>   mRadarStatusMap; //雷达状态容器
};

#endif // ZCHXRADARREPORTWORKER_H
