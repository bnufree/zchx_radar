#ifndef DATASERVERUTILS_H
#define DATASERVERUTILS_H

#include <QObject>


class DataServerUtils : public QObject
{
    Q_OBJECT
public:
    explicit DataServerUtils(QObject *parent = 0);
#ifdef Q_OS_WIN
    static bool isVirtualIpExist(const QString& pVirtualIp);
    static QString setVirtualIp(const QString& netName, const QString& pVirtualIp, const QString& pSubNet = "255.255.255.0");
    static void delVirtualIp(const QString& netName, const QString& pVirtualIp, const QString& pSubNet = "255.255.255.0");
#endif
    static QStringList getMyIps();
    static QString time2String(qint64 val, bool msecs=false);
    static QString currentTimeString(bool msec=false);
signals:

public slots:
};

#endif // DATASERVERUTILS_H
