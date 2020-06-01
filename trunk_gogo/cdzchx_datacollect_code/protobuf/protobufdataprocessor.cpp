#include "protobufdataprocessor.h"
#include "profiles.h"
#include "common.h"
//#include <QDebug>
//#include <synchapi.h>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

ProtoBufDataProcessor* ProtoBufDataProcessor::m_pInstance = 0;
ProtoBufDataProcessor::CGarbo ProtoBufDataProcessor::s_Garbo;

ProtoBufDataProcessor::ProtoBufDataProcessor(QObject *parent) : QObject(parent), mWorkTimer(0),mDisplayWidget(0)
{
    init();
    moveToThread(&mWorkThread);
    mWorkThread.start();
}

ProtoBufDataProcessor::~ProtoBufDataProcessor()
{
    if(mWorkTimer)
    {
        mWorkTimer->stop();
        mWorkTimer->deleteLater();
    }
    mWorkThread.quit();
    mWorkThread.terminate();
    mWorkThread.wait(500);
    mDevInfo.release_ddm_info();
    mDevInfo.release_gps_info();
    mDevInfo.release_nhn_info();
    mDevInfo.release_orp_info();
    mDevInfo.release_rdo_info();
    mDevInfo.release_wl_info();
    mDevInfo.release_zs_info();

}

QMutex* ProtoBufDataProcessor::getMutex()
{
    return &mMutex;
}

ProtoBufDataProcessor *ProtoBufDataProcessor::instance()
{
    if(m_pInstance == 0)
    {
        m_pInstance = new ProtoBufDataProcessor();
    }
    return m_pInstance;
}

void ProtoBufDataProcessor::init()
{
    //初始化数据的发送频率
    if(!mWorkTimer) mWorkTimer = new QTimer();
    mWorkTimer->setInterval(3000);
    connect(mWorkTimer, SIGNAL(timeout()), this, SLOT(slotPublish()));
    //初始化数据结构
    //船体数据
    mGPSInfo.set_ship_update_time(0);
    mGPSInfo.set_ship_lat(0.0);
    mGPSInfo.set_ship_lon(0.0);
    mGPSInfo.set_ship_speed(0.0);
    mGPSInfo.set_ship_head(0.0);
    mGPSInfo.set_ship_course(0.0);
    mGPSInfo.set_ship_id(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_ID").toString().toStdString().data());
    //
    mZSInfo.set_sts(0);
    mZSInfo.set_temp(0);
    mZSInfo.set_time(0);
    mZSInfo.set_zs(0);
    //
    mWLInfo.set_lvl(0);
    mWLInfo.set_press(0);
    mWLInfo.set_sts(0);
    mWLInfo.set_temp(0);
    mWLInfo.set_time(0);
    //
    mRDOInfo.set_rdo(0);
    mRDOInfo.set_sts(0);
    mRDOInfo.set_temp(0);
    mRDOInfo.set_time(0);
    //
    mORPInfo.set_orp(0);
    mORPInfo.set_sts(0);
    mORPInfo.set_time(0);
    //
    mDDMInfo.set_ddm(0);
    mDDMInfo.set_sts(0);
    mDDMInfo.set_temp(0);
    mDDMInfo.set_time(0);
    //
    mNHNInfo.set_nhn(0);
    mNHNInfo.set_sts(0);
    mNHNInfo.set_temp(0);
    mNHNInfo.set_time(0);
    //
    mDevInfo.set_allocated_ddm_info(&mDDMInfo);
    mDevInfo.set_allocated_gps_info(&mGPSInfo);
    mDevInfo.set_allocated_nhn_info(&mNHNInfo);
    mDevInfo.set_allocated_orp_info(&mORPInfo);
    mDevInfo.set_allocated_rdo_info(&mRDOInfo);
    mDevInfo.set_allocated_wl_info(&mWLInfo);
    mDevInfo.set_allocated_zs_info(&mZSInfo);
    mDevInfo.set_cur_utc_time(QDateTime::currentMSecsSinceEpoch());
    mDevInfo.set_site_id(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, SERVER_SETTING_SITE_ID).toString().toStdString().data());

}

void ProtoBufDataProcessor::slotPublish()
{
    SOCDS_DEBUG_FUNC_START;
    QMutexLocker locker(getMutex());
    mDevInfo.set_cur_utc_time(QDateTime::currentMSecsSinceEpoch());
    mGPSInfo.set_ship_id(PROFILES_INSTANCE->value(SERVER_SETTING_SEC, "GPS_ID").toString().toStdString().data());
    QByteArray dst;
    dst.resize(mDevInfo.ByteSize());
    mDevInfo.SerializeToArray(dst.data(), dst.size());
    SOCDS_DEBUG_FUNC_END;
    //cout<<"数据解析完成,发送";
    //Sleep(1000);
    emit signalSendComData(dst);
    //cout<<"经纬度:"<<mDevInfo.gps_info().ship_lat()<<mDevInfo.gps_info().ship_lon()<<mDevInfo.gps_info().ship_speed();
    if(PROFILES_INSTANCE->value("Radar_1", "Limit_gps").toBool())
        emit signalSendGpsData(mDevInfo.gps_info().ship_lat(),mDevInfo.gps_info().ship_lon());
    if(mDisplayWidget) mDisplayWidget->display(mDevInfo);

}

void ProtoBufDataProcessor::startPublish()
{
    if(mWorkTimer)
    {
        mWorkTimer->start();
    }
}

void ProtoBufDataProcessor::endPub()
{
    if(mWorkTimer)
    {
        mWorkTimer->stop();
    }
}


