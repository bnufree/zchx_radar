#ifndef COMPARSER_H
#define COMPARSER_H

#include <QObject>
#include "protobuf/TWQMSComData.pb.h"
#include <QThread>


using namespace com::zhichenhaixin::gps::proto;

class ComParser : public QObject
{
    Q_OBJECT
public:
    explicit ComParser(QObject *parent = 0);
    ~ComParser();
private:
    bool    parseRecvByteData(const QByteArray& head, int cmd_len, const QByteArray& recv, const QString& type, QList<double>& list);
signals:
    void    signalRecvComStatusChange(const QString& type, bool sts);
    void    signalRecvComData(const QByteArray& head, int cmd_len,const QString& topic, qint64 time, const QByteArray& recv);

public slots:
    void    slotRecvComData(const QByteArray& head, int cmd_len,const QString& topic, qint64 time, const QByteArray& recv);
    void    slotRecvComStatusChange(const QString& type, bool sts);
private slots:    
    void    slotGPSReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data);
    void    slotZSReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data);
    void    slotRDOReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data);
    void    slotORPReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data);
    void    slotNHNReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data);
    void    slotDDMReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data);
    void    slotWLReciveComData(const QByteArray& head, int cmd_len, qint64 time, QByteArray data);

    double  convertLonlatFromddmmdotmmmm(const QString& ValueStr, const QString& NSEWFlgStr);
    qint64  convertTimeFromhhmmssdotsss(const QString& valstr);

private:
    QThread             mWorkThread;
    QString             mGpsCmdStr;

};

#endif // COMPARSER_H
