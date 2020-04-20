///**************************************************************************
//* @File: TSPI.h
//* @Description:  Time, Space, Position Information(TSPI) 接口
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
#ifndef ZCHX_RADAR_MESSAGES_TSPI_H // -*- C++ -*-
#define ZCHX_RADAR_MESSAGES_TSPI_H

#include <cmath>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "geoStars.h"

#include "Header.h"

#include "ais_radar/ZCHXRadar.pb.h"

using namespace com::zhichenhaixin::proto;

namespace ZCHX
{
namespace Messages
{


class TSPI : public Header
{
public:
    using Super = Header;
    using Ref = boost::shared_ptr<TSPI>;

    using Coord = std::vector<double>;

    enum Flags
    {
        kDropping = ( 1 << 0 )
    };

    /** Obtain the message type information for RawVideo objects.

        \return MetaTypeInfo reference
    */
    static const MetaTypeInfo &GetMetaTypeInfo();

    const GEO_LOCATION *GetOrigin() const;

    static std::string GetSystemIdTag( uint16_t systemId );

    /** Utility that converts 4 raw bytes into a double value, taking into account endianess of the host, where
        big-endian is taken as-is and little-endian is byte-swapped.

    \param ptr address of 4 bytes to use

    \return floating-point value
    */
    static double MakeEFGCoordinateFromRawBytes( const uint8_t *ptr );


    /** Class factory that creates new reference-counted TSPI message objects from range, azimuth, and elevation
        values.

        \param producer name of the entity that created the message

        \param tag name for the TSPI plot

    \param when time of the plot

        \param range distance (slant-range) to target in meters

        \param azimuth angle from 0 north to target in radians

        \param elevation angle from horizontal to target in radians

        \param latitude degrees latitude of the target

        \param longitude degrees longitude of the target

        \param height vertical distance to target in meters

        \return reference to new TSPI object
    */
    static Ref MakeRAE( const std::string &producer, const std::string &tag,
                        double when, double range, double azimuth,
                        double elevation,
                        RadarConfig* cfg);

    /** Class factory that creates new reference-counted TSPI message objects from latitude, longitude, and
        height values.

        \param producer name of the entity that created the message

        \param tag name for the TSPI plot

    \param when time of the plot

        \param latitude degrees latitude of the target

        \param longitude degrees longitude of the target

        \param height vertical distance to target in meters

        \return reference to new TSPI object
    */
    static Ref MakeLLH( const std::string &producer, const std::string &tag,
                        double when, double latitude, double longitude,
                        double height );

    /** Class factory that creates new reference-counted TSPI message objects using offsets from the radar
        position (given by RadarConfig)

        \param producer name of the entity that created the message

        \param tag name for the TSPI plot

    \param when time of the plot

        \param x north offset from radar in meters

        \param y east offset from radar in meters

        \param z up offset from radar in meters

        \return reference to new TSPI object
    */
    static Ref MakeXYZ( const std::string &producer, const std::string &tag,
                        double when, double x, double y, double z );

    /** Class factory that creates new reference-counted TSPI message objects using data from an input CDR
    stream.
    \param cdr input CDR stream to read from

        \return reference to new TSPI object
    */
    static Ref Make( const QSharedPointer<QByteArray> &raw );

    /** Destructor.
     */
    ~TSPI() {}

    /** Read in a TSPI message from an CDR input stream. Override of Header::load().

        \param cdr stream to read from

        \return stream read from
    */
    void load( const QSharedPointer<QByteArray> &raw );
    TrackPoint toTrackPoint() const;

    /** Print out a textual representation of the TSPI message. Override of Header::print().

        \param os C++ text stream to write to

        \return stream written to
    */
    std::ostream &printData( std::ostream &os ) const;

    /** Write out a TSPI message to a C++ output stream in XML format.

        \param os stream to write to

        \return stream written to
    */
    std::ostream &printDataXML( std::ostream &os ) const;

    /** Obtain the position in earth-centered coordinates

        \return earth-centered cartesian coordinates in meters
    */
    const double *getEarthCentered() const
    {
        return efg_;
    }

    /** Obtain the E component of the earth-centered position

        \return E value
    */
    double getE() const
    {
        return getEarthCentered()[GEO_E];
    }

    /** Obtain the F component of the earth-centered position

        \return F value
    */
    double getF() const
    {
        return getEarthCentered()[GEO_F];
    }

    /** Obtain the G component of the earth-centered position

        \return G value
    */
    double getG() const
    {
        return getEarthCentered()[GEO_G];
    }

    /** Obtain the position in latitude, longitude, height

        \return latitude (radians), longitude (radians), and height (meters).
    */
    const Coord &getLatitudeLongitudeHeight() const;

    /** Obtain the latitude component

        \return value in radians
    */
    double getLatitude() const
    {
        return getLatitudeLongitudeHeight()[GEO_LAT];
    }

    /** Obtain the longitude component

        \return value in radians
    */
    double getLongitude() const
    {
        return getLatitudeLongitudeHeight()[GEO_LON];
    }

    /** Obtain the height component

        \return value in meters
    */
    double getHeight() const
    {
        return getLatitudeLongitudeHeight()[GEO_HGT];
    }

    /** Obtain the position in range, azimuth, elevation from the radar origin.

        \return range (meters), azimuth (radians), elevation (radians)
    */
    const Coord &getRangeAzimuthElevation() const;

    /** Obtain the range component

        \return value in meters
    */
    double getRange() const
    {
        return getRangeAzimuthElevation()[GEO_RNG];
    }

    /** Obtain the azimuth component

        \return value in radians
    */
    double getAzimuth() const
    {
        return getRangeAzimuthElevation()[GEO_AZ];
    }

    /** Obtain the elevation component

        \return value in radians
    */
    double getElevation() const
    {
        return getRangeAzimuthElevation()[GEO_EL];
    }

    /** Obtain the 3-D offsets from the radar origin.

        \return x (meters), y (meters), z (meters) offsets from radar
    */
    const Coord &getXYZ() const;

    /** Obtain the X component

        \return value in meters
    */
    double getX() const
    {
        return getXYZ()[GEO_X];
    }

    /** Obtain the Y component

        \return value in meters
    */
    double getY() const
    {
        return getXYZ()[GEO_Y];
    }

    /** Obtain the Z component

        \return value in meters
    */
    double getZ() const
    {
        return getXYZ()[GEO_Z];
    }

    /** Obtain the timestamp for the position report

        \return time value
    */
    double getWhen() const
    {
        return when_;
    }

    /** Obtain the name assigned to the report

        \return name
    */
    const std::string &getTag() const
    {
        return tag_;
    }

    /** Determine if the report is dropping

        \return true if so
    */
    bool isDropping() const
    {
        return flags_ & kDropping;
    }

    /** Set the dropping indicator
     */
    void setDropping()
    {
        flags_ |= kDropping;
    }

    /** Write the current position values to the debug logger
     */
    void dump() const;


private:

    struct InitFromRAE
    {
        InitFromRAE( double r, double a, double e )
            : r_( r ), a_( a ), e_( e ) {}
        double r_;
        double a_;
        double e_;
    };

    struct InitFromLLH
    {
        InitFromLLH( double lat, double lon, double hgt )
            : lat_( lat ), lon_( lon ), hgt_( hgt ) {}
        double lat_;
        double lon_;
        double hgt_;
    };

    struct InitFromXYZ
    {
        InitFromXYZ( double x, double y, double z )
            : x_( x ), y_( y ), z_( z ) {}
        double x_;
        double y_;
        double z_;
    };

    /** Constructor for new TSPI message.

        \param producer name of the entity that created the message
    */
    TSPI( const std::string &producer );

    TSPI( const std::string &producer, const std::string &tag, double when,
          const InitFromRAE &rae, RadarConfig* cfg );

    TSPI( const std::string &producer, const std::string &tag, double when,
          const InitFromLLH &llh );

    TSPI( const std::string &producer, const std::string &tag, double when,
          const InitFromXYZ &xyz );

    /** Constructor for RawVideo messages that will be filled in with data from a CDR stream.
     */
    TSPI();

    void calculateLLH() const;
    void calculateRAE() const;
    void calculateXYZ() const;


    double when_;
    double efg_[3]; // 以地球中心坐标
    uint16_t flags_;  //数据标记
    std::string tag_;  //目标标签
    mutable Coord llh_;  //大地坐标
    mutable Coord rae_;  //极坐标
    mutable Coord xyz_;  //笛卡尔坐标

    static Header::Ref CDRLoader( const QSharedPointer<QByteArray> &raw );

    static MetaTypeInfo metaTypeInfo_;
    static QMap<QString, GEO_LOCATION*> originMap;
};

} // end namespace Messages
} // end namespace ZCHX

/** \file
 */

#endif
