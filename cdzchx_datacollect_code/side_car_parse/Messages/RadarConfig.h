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
class RadarConfig : public QObject
{
    Q_OBJECT
public:
    /** Prohibit instances of RadarConfig class.
     */
    RadarConfig(QObject *parent);
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
    uint64_t getID();

    /** Obtain the current radar configuration name

        \return config name
    */
    const QString &GetName();

    /** Obtain the current max gate count value

        \return max gate count
    */
    uint32_t GetGateCountMax();

    /** Obtain the current max shaft encoding value

        \return max shaft encoding value
    */
    uint32_t GetShaftEncodingMax();

    /** Obtain the current rotation rate value.

        \return rotation rate in RPMs
    */
    double GetRotationRate();

    /** Obtain the current min range value, the range value associated with the first sample.

        \return min range value
    */
    double GetRangeMin_deprecated();

    /** Obtain the current max range value, the range value associated with the gateCountMax_ - 1 sample.

        \return max range value
    */
    double GetRangeMax();

    /** Obtain the multiplier used to convert from gate sample index values to a range value in kilometers.

        \return range factor
    */
    double GetRangeFactor_deprecated();

    /** Obtain the current beam width value

        \return beam width
    */
    double GetBeamWidth();

    /** Obtain the number radials in one revolution of the radar. This is an upper bound.

        \return
    */
    uint32_t GetRadialCount()
    {
        return uint32_t( ::ceil( 2.0 * M_PI / GetBeamWidth() ) );
    }

    /** Obtain the range value for a gate sample index.

        \param gateIndex value to convert

        \return range in kilometers
    */
    double GetRangeAt_deprecated( uint32_t gateIndex )
    {
        return gateIndex * GetRangeFactor_deprecated() + GetRangeMin_deprecated();
    }

    /** Obtain an azimuth reading in radians from a shaft encoding value

        \param shaftEncoding value to convert

        \return azimuth in radians
    */
    double GetAzimuth( uint32_t shaftEncoding )
    {
        return M_PI * 2.0 * shaftEncoding / ( GetShaftEncodingMax() + 1 );
    }

    /** Obtain the duration in seconds of one rotation of the radar.

        \return duration in seconds
    */
    double GetRotationDuration()
    {
        return 60.0 / GetRotationRate();
    }

    /** Obtain the latitude (north/south) of the site location.

        \return degrees latitude
    */
    double GetSiteLatitude();

    /** Obtain the longitude (east/west) of the site location.

        \return degrees longitude
    */
    double GetSiteLongitude();

    /** Obtain the height of the site location.

        \return height in meters
    */
    double GetSiteHeight();


    /** Obtain a read-only reference to  loaded VideoConfig objects.

        \return reference to VideoConfig
    */
    const VideoConfig* getVideoConfig();

    /** Obtain a read-only reference to  loaded TSPIConfig objects.

        \return reference to TSPIConfig
    */
    const TSPIConfig* getTSPIConfig();

    QString getRadarName() const;
    void setRadarName(const QString &radarName);

    QStringList getAllCommand();
    QByteArray getRadarCommand(const QString &cmd);

    QHostAddress getCmdAddress() const;
    void  changeAddress(const QString str);

    quint16 getCmdPort() const;
    void setCmdPort(const quint16 &cmdPort);

    bool saveRadarConfiguration();

    void setLatitude(double latitude);

    void setLongitude(double longitude);

    static QStringList getGRadarNameList();
    static void setGRadarNameList(const QStringList &value);
    int  lineNum() const {return mLineNum;}
    int  cellNum() const {return mCellNum;}
    int  heading() const {return mHeading;}

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

    uint64_t id_;
    QString name_;
    uint32_t gateCountMax_;
    uint32_t shaftEncodingMax_;
    double rotationRate_;
    double rangeMin_;
    double rangeMax_;
    double rangeFactor_;
    double beamWidth_;
    double latitude_;    ///< Radar site longitude (degrees)
    double longitude_;   ///< Radar site latitude (degrees)
    double height_;  ///< Radar site height (meters)
    int     mLineNum;
    int     mCellNum;
    int     mHeading;

    VideoConfig *videoConfig_;
    TSPIConfig *tspiConfig_;
    QString radarName_;

    QDir dir_;
    QMap<QString, QString> mapCmd_;
    QHostAddress cmdAddress_;
    quint16 cmdPort_;

    static QStringList gRadarNameList;
};

} // end namespace Message
} // end namespace ZCHX
