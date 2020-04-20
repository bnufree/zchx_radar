#ifndef COMMON_H
#define COMMON_H

#include <QDebug>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
#include <QRegularExpression>

#define             M_DEBUG             0
#define     SOCDS_DEBUG         if(M_DEBUG)         qDebug()<<QDateTime::currentDateTime()<<__FILE__<<__FUNCTION__<<__LINE__
#define     SOCDS_DEBUG_FUNC_START          SOCDS_DEBUG<<" START"
#define     SOCDS_DEBUG_FUNC_END            SOCDS_DEBUG<<" END"

//配置文件定义
/************************服务器设定********************************/
#define             SERVER_SETTING_SEC                      "Setting"
#define             SERVER_SETTING_SERVER_IP                "Server_IP"
#define             SERVER_SETTING_SERVER_PORT              "Server_Port"
#define             SERVER_SETTING_SITE_ID                  "Site_ID"
#define             SERVER_SETTING_OPLOAD_FREQUENCY         "Upload_Intervals"
#define             DISPLAY_PARSE_RESULT                    "display"
/************************界面参数设定********************************/
#define             COM_DEVICES_SEC                             "COM_DEVICE"
#define             COM_GPS_DEV                                 "GPS"
#define             COM_RDO_DEV                                 "RDO"                //溶解氧
#define             COM_ORP_DEV                                 "ORP"               //氧化还原电位
#define             COM_NHN_DEV                                 "NHN"               //氨氮
#define             COM_ZS_DEV                                  "ZS"                //浊度
#define             COM_WATER_DEV                               "WATER"             //水位测试
#define             COM_DDM_DEV                                 "DDM"               //透明度

//雷达解析参数定义
#define             RADAR_ROTATE_RATE_RPM               "Rotate_Rate"
#define             RADAR_PARSE_MODE                    "Parse_Mode"
#define             RADAR_CONST_THRESHOLD               "const_threshold_val"
#define             RADAR_THRESHOLD_MODE                "threshold_mode"
#define             RADAR_OSCFAR_ALPHA                  "oscfar_alpha"
#define             RADAR_OSCFAR_REFER_INDEX            "oscfar_refer_index"
#define             RADAR_OSCFAR_WINDOW_SIZE            "oscfar_window_size"
#define             RADAR_MAX_EXTRACTION_SIZE           "max_extraction_size"
#define             RADAR_MAX_EXTRACTION_AMZ            "max_extraction_amz"
#define             RADAR_DISCARD_EXTRACTION_AMZ        "discard_extraction_amz"
#define             RADAR_DISCARD_EXTRACTION_SIZE       "discard_extraction_size"
#define             RADAR_CORRECTION_SEARCH_RADIUS      "correct_search_radius"
#define             RADAR_CORRECTION_SCAN_COUNT         "correct_scans_num"
#define             RADAR_TRACK_SEARCH_RADIUS           "target_search_radius"
#define             RADAR_MERGE_RADIUS                  "target_merge_radius"
#define             RADAR_MERGER_ZAIMUTH                "target_merge_azimuth"

#define     DBG_TRACK_PROCESS                           0
#define     LOG_FUNC_DBG_DEP                            if(1) std::cout
#define     LOG_FUNC_DBG                                if(DBG_TRACK_PROCESS)       qDebug()<<__FUNCTION__<<__LINE__
#define     LOG_FUNC_DBG_START                          if(DBG_TRACK_PROCESS)       qDebug()<<__FUNCTION__<<"Start"
#define     LOG_FUNC_DBG_END                            if(DBG_TRACK_PROCESS)       qDebug()<<__FUNCTION__<<"End"

class TimeStamp {
public:
    TimeStamp() {mDateTime.setMSecsSinceEpoch(0);}
    TimeStamp(const QDateTime& dt) {mDateTime = dt;}
    TimeStamp(const TimeStamp& other) {mDateTime = other.mDateTime;}
    TimeStamp& operator =(const TimeStamp& other)
    {
        if(this != &other)
        {
            this->mDateTime = other.mDateTime;
        }
        return *this;
    }
    static TimeStamp timeStampFromMsecs(qint64 msec)
    {
        return TimeStamp(QDateTime::fromMSecsSinceEpoch(msec));
    }

    long secs() const {return mDateTime.toTime_t();}
    qint64 msec() const {return mDateTime.toMSecsSinceEpoch();}
    static int  timeOfDay()
    {
        QDateTime cur = QDateTime::currentDateTime();
        QDateTime day = QDateTime(cur.date());
        return day.secsTo(cur);
    }

private:
    QDateTime       mDateTime;
};

#endif // COMMON_H
