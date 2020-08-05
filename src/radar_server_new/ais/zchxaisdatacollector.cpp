#include "zchxaisdatacollector.h"
#include "ais.h"
#include <QTcpSocket>
#include <QDateTime>

zchxAisDataCollector::zchxAisDataCollector(Ais_Collector_Mode mode, QObject *parent)
    : QObject(parent)
    , mType(mode)
{
    BuildNmeaLookup();
    mLastRecvDataTime = QDateTime::currentMSecsSinceEpoch();
    this->moveToThread(&mWorkThread);
    mWorkThread.start();
}

zchxAisDataCollector::~zchxAisDataCollector()
{
    if(mWorkThread.isRunning())
    {
        mWorkThread.quit();
    }
}

bool zchxAisDataCollector::init()
{
    return true;
}

void zchxAisDataCollector::slotSocketReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if(!socket) return;
    mLastRecvDataTime = QDateTime::currentMSecsSinceEpoch();
    QByteArray aisArray = socket->readAll();
    emit signalSendAisData(aisArray);
}

void zchxAisDataCollector::slotDisplayError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
}

void zchxAisDataCollector::slotStateChanged(QAbstractSocket::SocketState state)
{
    Q_UNUSED(state)
}
