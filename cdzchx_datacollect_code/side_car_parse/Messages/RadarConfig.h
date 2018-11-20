///**************************************************************************
//* @File: RadarConfig.h
//* @Description:  雷达配置接口
//* @Copyright: Copyright (c) 2017
//* @Company: 深圳置辰海信科技有限公司
//* @WebSite: http://www.szcenterstar.com/
//* @author 李鹭
//* @Revision History
//*
//* <pre>
//* ----------------------------------------------------------------------
//*   Ver     Date       Who             Comments
//*  ----- ----------  --------  ---------------------------------------
//*   1.0  2017/03/13    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/
#pragma once

#include <inttypes.h>
#include <cmath>
#include <string>

#include <QtCore>
#include <QHostAddress>
#include "MessagesGlobal.h"


#define     RADAR_CAT240_STR        "cat240"
#define     RADAR_ZCHX240_STR       "zchx240"

class QDomElement;
class QString;

namespace ZCHX
{
namespace Messages
{

class VideoConfig;
class TSPIConfig;

/** Run-time configuration parameters that describe the radar. Since there is only one radar configuration in
    use at a time, this class supports no instances; all methods and attributes belong to the class.

    The RadarConfig 'instance' relies on data found in an XML configuration file. The internal Initializer class
    looks in the following locations (where ZCHX_RADAR_CONFIG and ZCHX_RADAR are process environment variables):

    - ${ZCHX_RADAR_CONFIG}
    - ${ZCHX_RADAR}/etc/configuration.xml
    - /opt/zchx_radar/etc/configuration.xml

    The above locations are ordered by priority -- a configuration file found via ${ZCHX_RADAR_CONFIG} will
    override one the could be found via ${ZCHX_RADAR}/etc/configuration.xml.

    NOTE: do not inline the accessor methods below. We need to execute code in RadarConfig.cc in order to make
    sure that the attributes initialize properly; C++ does not guarantee initialization of class (static)
    attributes until code containing the attribubtes first executes.

    NOTE: the rationale for having RadarConfig done this way versus a singleton is lost to history. Perhaps
    backwards capability with some other system?
*/
enum RADAR_TYPE{
    RADAR_UNDEF = 0,
    RADAT_LOWRANCE,
    RADAR_AGILTRACK,
    RADAR_CAT240 = RADAR_AGILTRACK,
    RADAR_ZCHX240 = RADAT_LOWRANCE,
};

class RadarConfig : public QObject
{
    Q_OBJECT
public:
    /** Prohibit instances of RadarConfig class.
     */
    RadarConfig(QObject *parent = 0);
    ~RadarConfig();

    /** Use a new file path for radar configuration values.

        \param path file path to use

        \return true if successfully installed
    */
    bool SetConfigurationFilePath( const std::string& path );

    /** Load in a new radar configuration from an XML DOM node

        \param config the DOM node containing the configuration

        \return true if successful
    */
    bool Load( const QDomElement &config );

    /** Load in new radar configuration from raw values.

        \param name name of the configuration

        \param gateCountMax max number of gate samples in a message

        \param shaftEncodingMax max shaft encoding value

        \param rotationRate expected rotation rate of the radar in RPMs

        \param rangeMin range value of first gate sample in kilometers

        \param rangeMax range value of last gate sample in kilometers

        \param beamWidth width of radar beam for one PRI
    */
    void Load( const std::string &name, uint32_t gateCountMax, uint32_t shaftEncodingMax,
                      double rotationRate, double rangeMin, double rangeMax, double beamWidth );

    /*!
     * \brief 获取当前雷达的ID
     */
    uint64_t getID() const {return mID;}
    void     setID(uint64_t id) {mID = id;}

    /** Obtain the current radar configuration name

        \return config name
    */

    const QString &getName() const {return mName;}
    void    setName(const QString& name) {mName = name;}

    /** Obtain the current max gate count value

        \return max gate count
    */
    uint32_t getGateCountMax() const {return mGateCountMax;}
    void     setGateCountMax(uint32_t val) {mGateCountMax = val; CalculateRangeFactor();}

    /** Obtain the current max shaft encoding value

        \return max shaft encoding value
    */
    uint32_t getShaftEncodingMax() const {return mShaftEncodingMax;}
    void    setShaftEncodingMax(int val)  {mShaftEncodingMax = val;}

    /** Obtain the current rotation rate value.

        \return rotation rate in RPMs
    */
    double getRotationRate() {return mRotationRate;}
    void   setRotationRate(double val) {mRotationRate = val;}

    /** Obtain the current min range value, the range value associated with the first sample.

        \return min range value
    */
    double getRangeMin() {return mRangeMin;}
    void   setRangeMin(double val) {mRangeMin = val; CalculateRangeFactor();}

    /** Obtain the current max range value, the range value associated with the gateCountMax_ - 1 sample.

        \return max range value
    */
    double getRangeMax() {return mRangeMax;}
    void   setRangeMax(double val) {mRangeMax = val; CalculateRangeFactor();}

    /** Obtain the multiplier used to convert from gate sample index values to a range value in kilometers.

        \return range factor
    */
    double getRangeFactor() {return mRangeFactor;}

    /** Obtain the current beam width value //波束宽度

        \return beam width
    */
    double getBeamWidth() {return mBeamWidth;}
    void   setBeamWidth(double val) {mBeamWidth = val;}

    /** Obtain the number radials in one revolution of the radar. This is an upper bound.

        \return
    */
    uint32_t getRadialCount()
    {
        return uint32_t( ::ceil( 2.0 * M_PI / getBeamWidth() ) );
    }

    /** Obtain the range value for a gate sample index.

        \param gateIndex value to convert

        \return range in kilometers
    */
    double getRangeAtIndex( uint32_t gateIndex )
    {
        return gateIndex * getRangeFactor() + getRangeMin();
    }

    /** Obtain an azimuth reading in radians from a shaft encoding value

        \param shaftEncoding value to convert

        \return azimuth in radians
    */
    double getAzimuth( uint32_t shaftEncoding )
    {
        return M_PI * 2.0 * shaftEncoding / ( getShaftEncodingMax() + 1 );
    }

    /** Obtain the duration in seconds of one rotation of the radar.

        \return duration in seconds
    */
    double getRotationDuration()
    {
        return 60.0 / getRotationRate();
    }

    /** Obtain the latitude (north/south) of the site location.

        \return degrees latitude
    */
    double getSiteLat() {return mLat;}
    void   setSiteLat(int val) {mLat = val;}

    /** Obtain the longitude (east/west) of the site location.

        \return degrees longitude
    */
    double getSiteLong() {return mLon;}
     void   setSiteLon(int val) {mLon = val;}

    /** Obtain the height of the site location.

        \return height in meters
    */
    double getSiteHeight() {return mHeight;}
    void   setSiteHeight(int val) {mHeight = val;}


    /** Obtain a read-only reference to  loaded VideoConfig objects.

        \return reference to VideoConfig
    */
    VideoConfig* getVideoConfig() {return mVideoConfig;}
    void    setVideoConfig(VideoConfig* cfg)
    {
        if(mVideoConfig) delete mVideoConfig;
        mVideoConfig = cfg;
    }

    /** Obtain a read-only reference to  loaded TSPIConfig objects.

        \return reference to TSPIConfig
    */
    TSPIConfig* getTSPIConfig() {return mTSPIConfig;}
    void        setTSPIConfig(TSPIConfig* cfg)
    {
        if(mTSPIConfig) delete mTSPIConfig;
        mTSPIConfig = cfg;
    }

    int getHead() {return mHead;}
    void   setHead(int val) {mHead = val;}

    QDir getCfgDir() {return mCfgDir;}
    void   setCfgDir(const QDir& val) {mCfgDir = val;}

    void    setCmdIP(const QString& ip) {mCmdIP = ip;}
    QString getCmdIP() const {return mCmdIP;}

    int     getCmdPort() const {return mCmdPort;}
    void    setCmdPort(int val) {mCmdPort = val;}

    int    getRadarType() const {return mRadarType;}
    void    setRadarType(int val) {mRadarType = val;}
    void    setRadarType(const QString& type)
    {
        if(type == RADAR_CAT240_STR)
        {
            mRadarType = RADAR_CAT240;
        } else if(type == RADAR_ZCHX240_STR)
        {
            mRadarType = RADAR_ZCHX240;
        } else
        {
            mRadarType = RADAR_UNDEF;
        }
    }

    void    setLimit(bool val) {mIsLimitFilter = val;}
    bool    getLimit() const {return mIsLimitFilter;}

    void    setFilterFileName(const QString& val) {mFilterFileName = val;}
    QString    getFilterFileName() const {return mFilterFileName;}

    void    setLoopNum(int val) {mLoopNum = val;}
    int    getLoopNum() const {return mLoopNum;}

    void    setHeartTimeInterval(int val) {mHeartTimeInterval = val;}
    int    getHeartTimeInterval() const {return mHeartTimeInterval;}

    void    setDistance(int val) {mDistance = val;}
    int    getDistance() const {return mDistance;}

    void    setTrackClearTime(int val) {mTrackClearTime = val;}
    int    getTrackClearTime() const {return mTrackClearTime;}

    void    setReportIP(const QString& val) {mReportIP = val;}
    QString    getReportIP() const {return mReportIP;}

    void    setReportPort(int val) {mReportPort = val;}
    int    getReportPort() const {return mReportPort;}

    void    setReportOpen(bool val) {mReportOpen = val;}
    bool    getReportOpen() const {return mReportOpen;}

    QStringList getAllCommand();
    QByteArray getRadarCommand(const QString &cmd);

    bool saveRadarConfiguration();

    static QStringList getGRadarNameList();
    static void setGRadarNameList(const QStringList &value);

    QString getVideoIP();
    int     getVideoPort();

    QString getTrackIP();
    int     getTrackPort();


public slots:
    void sendRadarCommand(const QString &cmd);

signals:
    void radarInfoChanged();

private:

    bool GetEntry( const QDomElement &config, const QString &name, QString &value, QString &units );

    /** Calculate the range factor value, which is dependent on the rangeMin_, rangeMax_, and gateCountMax_
        values.

        \return range factor
    */
    double CalculateRangeFactor();

    /** Internal initializer class that starts/stops a configuration file monitor,
    using RAII principle.
    */
    void Initializer();

    uint64_t                    mID;
    QString                     mName;
    uint32_t                    mGateCountMax;      //cell number
    uint32_t                    mShaftEncodingMax;  //line number
    double                      mRotationRate;
    double                      mRangeMin;
    double                      mRangeMax;
    double                      mRangeFactor;
    double                      mBeamWidth;
    double                      mLat;    ///< Radar site longitude (degrees)
    double                      mLon;   ///< Radar site latitude (degrees)
    double                      mHeight;  ///< Radar site height (meters)
    int                         mHead;
    VideoConfig*                mVideoConfig;    //回波服务器设定
    TSPIConfig*                 mTSPIConfig;      //雷达目标服务器设定
    QDir                        mCfgDir;
    QMap<QString, QString>      mMapCmd;
    QString                     mCmdIP;
    quint16                     mCmdPort;
    int                         mRadarType;

    bool                        mIsLimitFilter;   //是否执行目标过滤
    QString                     mFilterFileName;    //过滤文件名称
    int                         mLoopNum;           //循环次数
    int                         mHeartTimeInterval;  //发送心跳消息的频率
    int                         mDistance;          //范围限制距离大小
    int                         mTrackClearTime;    //目标清除时间间隔
    QString                     mReportIP;          //雷达参数报告IP
    int                         mReportPort;        //雷达参数报告端口
    bool                        mReportOpen;        //是否接收雷达参数报告


    static QStringList          gRadarNameList;
};

} // end namespace Message
} // end namespace ZCHX
