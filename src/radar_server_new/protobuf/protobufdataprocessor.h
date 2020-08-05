#ifndef PROTOBUFDATAPROCESSOR_H
#define PROTOBUFDATAPROCESSOR_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include "TWQMSComData.pb.h"
#include "protobufdatadisplaywidget.h"



using namespace com::zhichenhaixin::gps::proto;

#define     PROTOBUF_DATA           ProtoBufDataProcessor::instance()

class ProtoBufDataProcessor : public QObject
{
    Q_OBJECT
protected:
    explicit ProtoBufDataProcessor(QObject *parent = 0);
    ~ProtoBufDataProcessor();
public:
    QMutex* getMutex();
    friend class CGarbo;
    static ProtoBufDataProcessor* instance();
    DevInfo&        devInfo() {return mDevInfo;}
    com::zhichenhaixin::gps::proto::GPS&            gps(){return mGPSInfo;}
    ZS&             zs(){return mZSInfo;}
    RDO&            rdo(){return mRDOInfo;}
    ORP&            orp(){return mORPInfo;}
    DDM&            ddm(){return mDDMInfo;}
    NHN&            nhn(){return mNHNInfo;}
    WL&             wl(){return mWLInfo;}
    void            setDisplayWidget(ProtobufDataDisplayWidget* w) {mDisplayWidget = w;}

    void   startPublish();
    void   endPub();
signals:
    void    signalSendComData(const QByteArray& pData);
    void   signalSendGpsData(double,double);//发送GPS经纬度给雷达

private slots:
    void init();
    void slotPublish();

private:
    static ProtoBufDataProcessor *m_pInstance;
    class CGarbo        // 它的唯一工作就是在析构函数中删除CSingleton的实例
    {
    public:
        ~CGarbo()
        {
            qDebug()<<"delete ProtoBufDataProcessor now";
            if (ProtoBufDataProcessor::m_pInstance)
            {
                delete ProtoBufDataProcessor::m_pInstance;
                ProtoBufDataProcessor::m_pInstance = NULL;
            }
            qDebug()<<"delete ProtoBufDataProcessor end";
        }
    };
    static CGarbo s_Garbo; // 定义一个静态成员，在程序结束时，系统会调用它的析构函数

    QThread                     mWorkThread;

    QMutex mMutex;
    DevInfo     mDevInfo;
    com::zhichenhaixin::gps::proto::GPS         mGPSInfo;
    ZS          mZSInfo;
    RDO         mRDOInfo;
    ORP         mORPInfo;
    DDM         mDDMInfo;
    NHN         mNHNInfo;
    WL          mWLInfo;
    QString     mTopic;
    QTimer              *mWorkTimer;
    ProtobufDataDisplayWidget * mDisplayWidget;

};

#endif // PROTOBUFDATAPROCESSOR_H
