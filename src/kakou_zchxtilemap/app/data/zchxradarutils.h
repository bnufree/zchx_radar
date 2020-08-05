#ifndef ZCHXRADARUTILS_H
#define ZCHXRADARUTILS_H

#include "qt/zchxutils.hpp"
#include "zchxfuntiontimer.h"
#include <QThread>

namespace ZCHX_RADAR_RECEIVER
{
    enum ZCHX_RECV_TYPE{
        ZCHX_RECV_NONE = 0,
        ZCHX_RECV_AIS,
        ZCHX_RECV_AIS_CHART,
        ZCHX_RECV_RADAR_POINT,
        ZCHX_RECV_RADAR_VIDEO,
        ZCHX_RECV_RADAR_RECT,
        ZCHX_RECV_LIMIT_AREA,
    };

    struct ZCHX_Radar_Setting_Param
    {
        QString     m_sIP;
        QString     m_sPort;
        int          m_sSiteID;
        QStringList     m_sTopicList;

        bool operator ==(const ZCHX_Radar_Setting_Param& other)
        {
            return m_sIP == other.m_sIP
                    && m_sPort == other.m_sPort
                    && m_sTopicList == other.m_sTopicList;
        }
    };

struct ZCHX_RadarRect_Param
{
    ZCHX_Radar_Setting_Param    mSetting;
    QString     m_sCurColor;
    QString     m_sHistoryColor;
    QString     m_sEdgeColor;
    QString     m_sHistoryBackgroundColor;

    bool operator ==(const ZCHX_RadarRect_Param& other)
    {
        return mSetting == other.mSetting;
    }
};

class ZCHXReceiverThread : public QThread
{
    Q_OBJECT
public:
    explicit ZCHXReceiverThread(int type, const ZCHX_Radar_Setting_Param& param, QObject *parent = 0);
    virtual ~ZCHXReceiverThread();
    virtual void run();

    bool getIsOver() const {return isOver;}
    void setIsOver(bool value) {isOver = value;}
    virtual void parseRecvData(const QByteArrayList& data){}
    bool connectToHost();
    void disconnectToHost();
    int  getType() const {return mType;}
signals:
    void signalConnectedStatus(bool sts, const QString& msg);
    void signalRecvDataNow(int type,  int length);

protected:
    ZCHX_Radar_Setting_Param mRadarCommonSettings;
    bool  isOver;
    void  *mCtx;
    void  *mSocket;
    bool  mIsConnect;
    int   mType;
    QString mUrl;
};
}





#endif // ZCHXRADARUTILS_H
