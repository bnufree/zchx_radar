#ifndef ZCHXRADARREPORTWORKER_H
#define ZCHXRADARREPORTWORKER_H

#include <QObject>
#include <QUdpSocket>
#include "side_car_parse/Messages/RadarConfig.h"


using namespace ZCHX::Messages;

class zchxRadarReportWorker : public QObject
{
    Q_OBJECT
public:
    explicit zchxRadarReportWorker(RadarConfig* cfg, QThread* thread, QObject *parent = 0);
    bool    isFine() const {return mInit;}
    QUdpSocket* socket() {return mSocket;}
private:
    void init();
    void processReport(const QByteArray& bytes, size_t len);
signals:

public slots:
    void slotRecvReportData();
    void displayUdpReportError(QAbstractSocket::SocketError);

private:
    QUdpSocket*     mSocket;
    RadarConfig*    mRadarCfg;
    QThread*        mWorkThread;
    bool            mInit;
};

#endif // ZCHXRADARREPORTWORKER_H
