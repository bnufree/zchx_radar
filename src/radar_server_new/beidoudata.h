#ifndef BEIDOUDATA_H
#define BEIDOUDATA_H
#include <bitset>
#include <string>
#include <QWidget>
#include <QDir>
#include <QDialog>
#include <QThread>
#include <QTcpSocket>
#include <QTimer>
#include <QDateTime>
#include "ZmqMonitorThread.h"
#include "ZCHXBd.pb.h"

typedef com::zhichenhaixin::proto::BD ITF_BD;
typedef com::zhichenhaixin::proto::BDList ITF_BDList;

using std::bitset;
using std::ostream;
using std::string;
using std::vector;
namespace Ui {
class beidouData;
}

class beidouData : public QWidget
{
    Q_OBJECT

public:
    explicit beidouData(int id = 1,QWidget *parent = 0);
    ~beidouData();
    int er2shi(QString );
    void praseLonLat(double f);
    uchar checkxor(QByteArray data);
    bool CheckXor(QByteArray data);
public slots:
    void aisToBeidou(double lon, double lat);
    void updateServerProgress();
    void slotCheckAisRecv();
    void dealBdData(QByteArray data);
    void init();
    void initZmq();
signals:
    void starProcess();
private slots:
    void on_pushButton_clicked();

private:
    Ui::beidouData *ui;
    QByteArray beidouArray;
    QString crc;
    QThread mWorkThread;
    QString str_bd;

    QTcpSocket *m_pTcpSocket;//1_通信套接字
    QString m_sIP;
    int  m_uPort;
    qint64 mLastRecvBdDataTime;
    QTimer*     mAisHeartTimer;
    int         mDataTimeOut;

    void *m_pAISContext;
    void *m_pAISLisher;
    int  m_uBDSendPort;
    QString m_sBDTopic;
    ZmqMonitorThread *m_pMonitorThread;
};

#endif // BEIDOUDATA_H
