#include "zchxRadarHeartWorker.h"
#include "common.h"
#include <QTimer>

zchxRadarHeartWorker::zchxRadarHeartWorker(const QString& host,
                                           int port,
                                           int interval,
                                           QThread* thread,
                                           QObject *parent) :
    zchxMulticastDataScoket(host, port, "HEART_SOCKET", 0, ModeAsyncRecv, parent),
    mWorkThread(thread),
    mHeartTimer(Q_NULLPTR)
{
    if(isFine())
    {
        mHeartTimer = new QTimer(this);
        mHeartTimer->setInterval(interval);
        connect(mHeartTimer, SIGNAL(timeout()), this, SLOT(slotHeartJob()));
    }
   if(mWorkThread)
   {
       this->moveToThread(mWorkThread);
   }
}

void zchxRadarHeartWorker::slotHeartJob()
{
    if(!isFine()) return;
    //修改发送数据方式
    QByteArray line;
    line.resize(2);
    line[0] = 0Xa0;
    line[1] = 0xc1;
    writeData(line);
    line[0] = 0x03;
    line[1] = 0xc2;
    writeData(line);
    line[0] = 0x04;
    line[1] = 0xc2;
    writeData(line);
    line[0] = 0x05;
    line[1] = 0xc2;
    writeData(line);
}
