#include "haworker.h"
#include "profiles.h"
#include "dataserverutils.h"
#include <QDebug>
#include "Log.h"

HAWorker::HAWorker(QObject *parent) : QObject(parent)
{
    init();
    this->moveToThread(&mWorkThead);
    mWorkThead.start();
}

HAWorker::~HAWorker()
{
    if(mWorkThead.isRunning())
    {
        mWorkThead.quit();
    }
    delVirtualIp();
}

void HAWorker::init()
{
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec *local = QTextCodec::codecForLocale();
    QString src = Utils::Profiles::instance()->value("Host", "NetConnection", QString::fromUtf8("local connection")).toString();
    QString temp = utf8->toUnicode(src.toStdString().data());
    mNetConnectName = QString::fromLocal8Bit(local->fromUnicode(temp));
    mVirtualIp = Utils::Profiles::instance()->value("Host", "PublishIP", "192.168.8.110").toString();
    mSubNet = Utils::Profiles::instance()->value("Host", "SubNet", "255.255.255.0").toString();
    connect(this, SIGNAL(startSetVirtualIp()), this, SLOT(setVirtualIp()));
    connect(this, SIGNAL(startDelVirtualIp()), this, SLOT(delVirtualIp()));
    LOG(LOG_RTM,"srcnet = %s, net: %s, virtual ip :%s, subnet:%s", src.toStdString().data(), mNetConnectName.toStdString().data(), mVirtualIp.toStdString().data(), mSubNet.toStdString().data());
    qDebug("srcnet = %s, temp = %s, net(gbk): %s, virtual ip :%s, subnet:%s", src.toStdString().data(), temp.toStdString().data(), mNetConnectName.toStdString().data(), mVirtualIp.toStdString().data(), mSubNet.toStdString().data());
}

QString HAWorker::getVirtualIp()
{
    return mVirtualIp;
}

void HAWorker::setVirtualIp()
{
    emit signalWorkingString(QStringLiteral("正在设定虚拟IP[%1]......").arg(mVirtualIp));
    qDebug()<<__FILE__<<__FUNCTION__<<__LINE__;
    QThread::sleep(1);
    QString res;

    if(!DataServerUtils::isVirtualIpExist(mVirtualIp))
    {
        qDebug()<<__FUNCTION__<<__LINE__;
        DataServerUtils::delVirtualIp(mNetConnectName, mVirtualIp, mSubNet);
        QThread::msleep(200);
        qDebug()<<__FUNCTION__<<__LINE__;
        res = DataServerUtils::setVirtualIp(mNetConnectName, mVirtualIp, mSubNet);
        QThread::msleep(2000);
    }

    //检测当前的服务器是否配置了虚拟ip,等待1s中再检查
    bool sts = true;
    int cnt = 0;
    while(!DataServerUtils::isVirtualIpExist(mVirtualIp))
    {
        cnt++;
        if(cnt == 4)
        {
            sts = false;
            break;
        }
        QThread::msleep(1000);
    }
    qDebug()<<__FUNCTION__<<__LINE__;

    //emit addVirtualIPSuccess();
    emit signalSetVipStatus(mVirtualIp, sts, res);

}

void HAWorker::delVirtualIp()
{
    emit signalWorkingString(QStringLiteral("正在删除虚拟IP[%1]......").arg(mVirtualIp));
    DataServerUtils::delVirtualIp(mNetConnectName, mVirtualIp, mSubNet);
    while (DataServerUtils::isVirtualIpExist(mVirtualIp)) {
        QThread::msleep(200);
    }
    emit removeVirtualIPSuccess();
    emit signalWorkingString(QStringLiteral("虚拟IP[%1]未启动").arg(mVirtualIp));
}

bool HAWorker::IsFinish()
{
    return mWorkThead.isFinished();
}

