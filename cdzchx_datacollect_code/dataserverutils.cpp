#include "dataserverutils.h"
#include "Log.h"
#include "QProcess"
#include <QHostInfo>
#include <QDebug>
#include <QDateTime>
#include <QNetworkInterface>

#include <winsock2.h>
//#pragma comment(lib, "ws2_32.lib")

DataServerUtils::DataServerUtils(QObject *parent) : QObject(parent)
{

}
bool DataServerUtils::isVirtualIpExist(const QString &pVirtualIp)
{
    QStringList iplist;
    foreach(QHostAddress address,QHostInfo::fromName(QHostInfo::localHostName()).addresses())
    {
         //qDebug()<<"address:"<<address;
         if(address.protocol() == QAbstractSocket::IPv4Protocol)
         {
             iplist.append(address.toString());
         }
    }
    //LOG(LOG_RTM, "current iplist number = %d, target virutal ip = %s", iplist.length(), pVirtualIp.toUtf8().data());
    int i=0;
    foreach (QString ip, iplist) {
        i++;
        //LOG(LOG_RTM, "current ip[%d] = %s", i, ip.toUtf8().data());
    }

    bool existflag = iplist.contains(pVirtualIp);
//    //LOG(LOG_RTM, "target ip status:%d", existflag);

//    {
//        WORD wVersionRequested = MAKEWORD(2, 2);

//           WSADATA wsaData;
//           if (WSAStartup(wVersionRequested, &wsaData) != 0)
//               return "";

//           char local[255] = {0};
//           gethostname(local, sizeof(local));
//           hostent* ph = gethostbyname(local);
//           if (ph == NULL)
//               return "";
//           const char* pszAddr;
//           for( i = 0; ph!= NULL && ph->h_addr_list[i]!= NULL; i++ )
//           {
//               /*对每一个IP地址进行处理*/
//               pszAddr=inet_ntoa (*(struct in_addr *)ph->h_addr_list[i]);
//               qDebug()<<"windows local ip:"<<pszAddr;
//           }

//           WSACleanup();
//    }

    return existflag;
}

QString DataServerUtils::setVirtualIp(const QString& netName, const QString &pVirtualIp,const QString& pSubNet)
{
    QString res = "";
    if(isVirtualIpExist(pVirtualIp)) return res;
    qDebug()<<__FUNCTION__<<__LINE__;
    QString strValue = QString("netsh interface IP add address %1 %2 %3").arg(netName).arg(pVirtualIp).arg(pSubNet);
    QProcess *process = new QProcess;
//    QProcess process(0);
//    process.execute(strValue);
    process->start(strValue);
    if(process->waitForReadyRead(3000))
    {
        res = QString::fromLocal8Bit(process->readAllStandardOutput());
        LOG(LOG_RTM, "%s", res.toStdString().data());
    }

    return res;
}


void DataServerUtils::delVirtualIp(const QString& netName, const QString &pVirtualIp, const QString& pSubNet)
{
    QString strValue = QString("netsh interface IP delete address %1 %2 %3").arg(netName).arg(pVirtualIp).arg(pSubNet);
    QProcess process(0);
    process.execute(strValue);
}

QStringList DataServerUtils::getMyIps()
{
    QStringList iplist;
    foreach(QHostAddress address,QHostInfo::fromName(QHostInfo::localHostName()).addresses())
    {
         qDebug()<<"address:"<<address;
         if(address.protocol() == QAbstractSocket::IPv4Protocol)
         {
             iplist.append(address.toString());
         }
    }

    return iplist;
}

QString DataServerUtils::time2String(qint64 val, bool msecs)
{
    if(!msecs)
    {
        return QDateTime::fromMSecsSinceEpoch(val).toString("yyyy-MM-dd hh:mm:ss");

    } else
    {
        return QDateTime::fromMSecsSinceEpoch(val).toString("yyyy-MM-dd hh:mm:ss zzz");
    }
}

QString DataServerUtils::currentTimeString(bool msec)
{
    return time2String(QDateTime::currentMSecsSinceEpoch(), msec);
}
