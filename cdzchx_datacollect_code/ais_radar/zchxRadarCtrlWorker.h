#ifndef ZCHXRADARCTRLWORKER_H
#define ZCHXRADARCTRLWORKER_H

#include <QObject>
#include "zchxMulticastDataSocket.h"
#include "radarccontroldefines.h"

class zchxRadarCtrlWorker : public QObject
{
    Q_OBJECT
public:
    explicit zchxRadarCtrlWorker(zchxMulticastDataScoket* soc,
                                 QObject *parent = 0);
    void open();
    void close();
    void setCtrValue(INFOTYPE infotype, int value);
private:
    QByteArray UINT82ByteArray(UINT8* arr, int count);

signals:
    void sendCtrlData(const QByteArray& data);
public slots:
private:
    zchxMulticastDataScoket*     mCtrlSocket;
};

#endif // ZCHXRADARCTRLWORKER_H
