#include "zchxRadarHeartWorker.h"
#include "common.h"
#include <QTimer>

zchxRadarHeartWorker::zchxRadarHeartWorker(RadarConfig* cfg, QThread* thread, QObject *parent) :
    QObject(parent),
    mSocket(Q_NULLPTR),
    mRadarCfg(cfg),
    mWorkThread(thread),
    mHeartTimer(Q_NULLPTR),
    mInit(false)
{
   init();
   if(mInit && mWorkThread)
   {
       this->moveToThread(mWorkThread);
   }
}

void zchxRadarHeartWorker::init()
{
    if(!mRadarCfg) return;
    if(mRadarCfg->getCmdIP().length() == 0 || mRadarCfg->getCmdPort() == 0 || mRadarCfg->getHeartTimeInterval() == 0) return;

    mSocket = new QUdpSocket();

    if(!mSocket->bind(QHostAddress::AnyIPv4, mRadarCfg->getCmdPort(),QAbstractSocket::ShareAddress))
    {
        LOG_FUNC_DBG<<"bind heart failed--";
        return;
    }

    mSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);//禁止本机接收
    if(!mSocket->joinMulticastGroup(QHostAddress(mRadarCfg->getCmdIP())))
    {
         LOG_FUNC_DBG<<"joinMuticastGroup heart failed--";
         return;
    }
    mHeartTimer = new QTimer(this);
    mHeartTimer->setInterval(mRadarCfg->getHeartTimeInterval() * 1000);
    connect(mHeartTimer, SIGNAL(timeout()), this, SLOT(slotHeartJob()));
    mInit = true;

    LOG_FUNC_DBG<<"init radar "<<mRadarCfg->getName() << " heart work succeed.";
    return;
}


void zchxRadarHeartWorker::slotHeartJob()
{
    if(!mInit) return;
    QHostAddress host(mRadarCfg->mCmdIP);
    uint16_t port = mRadarCfg->getCmdPort();

    //修改发送数据方式
    QByteArray line;
    line.resize(2);
    line[0] = 0Xa0;
    line[1] = 0xc1;
    mSocket->writeDatagram(line, host, port);
    line[0] = 0x03;
    line[1] = 0xc2;
    mSocket->writeDatagram(line, host, port);
    line[0] = 0x04;
    line[1] = 0xc2;
    mSocket->writeDatagram(line, host, port);
    line[0] = 0x05;
    line[1] = 0xc2;
    mSocket->writeDatagram(line, host, port);

}
