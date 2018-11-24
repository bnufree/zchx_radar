///**************************************************************************
//* @File: TSPI.cpp
//* @Description:  Time, Space, Position Information(TSPI) 类
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
#include <QtCore/QSysInfo>
#include <QLoggingCategory>
#include <QtCore/QString>

#include <cmath>
#include <map>

#include <boost/lexical_cast.hpp>

#include "zchxRadarUtils.h"

#include "RadarConfig.h"
#include "TSPI.h"

using namespace ZCHX::Messages;

MetaTypeInfo TSPI::metaTypeInfo_(MetaTypeInfo::Value::kTSPI, "TSPI", &TSPI::CDRLoader);

const MetaTypeInfo&
TSPI::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

QMap<QString, GEO_LOCATION*> TSPI::originMap;

const GEO_LOCATION *TSPI::GetOrigin() const
{
    GEO_LOCATION* origin_ = originMap.value(radarConfig_->getName(), NULL);
    if(origin_)
        return origin_;

    // !!! NOTE1: this is not thread-safe. Potential memory leak here. NOTE2: be careful with initialization
    // !!! since this depends on RadarConfig class attributes which we wish to be initialized and valid before
    // !!! creating the origin.
    //
    if (! origin_) {
        //qCDebug(radarmsg) << "GetOrigin" ;

        origin_ = new GEO_LOCATION;

        // geoInitLocation() expects lat/lon in degrees, height in meters.
        //
        GeoIns->geoInitLocation(origin_,
                        radarConfig_->getSiteLat(),
                        radarConfig_->getSiteLong(),
                        radarConfig_->getSiteHeight(),
                        GEO_DATUM_DEFAULT,
                        "Radar");

//        qCInfo(radarmsg) << "LLH: " << origin_->lat << ' ' << origin_->lon
//                     << ' ' << origin_->hgt ;
//        qCInfo(radarmsg) << "EFG: " << origin_->e << ' ' << origin_->f << ' '
//                     << origin_->g ;

        originMap.insert(radarConfig_->getName(), origin_);
    }

    return origin_;
}

std::string
TSPI::GetSystemIdTag(uint16_t systemId)
{
    using Map = std::map<uint16_t,std::string>;

    // !!! NOTE: this is not thread-safe. Potential race situation here, with major badness if invoked from
    // !!! multiple threads.
    //
    static Map map_;
    if (map_.empty()) {
        map_.insert(Map::value_type(0x1201, "RFA"));
        map_.insert(Map::value_type(0x1202, "RFB"));
        map_.insert(Map::value_type(0x1001, "GTA"));
        map_.insert(Map::value_type(0x1006, "GTB"));
        map_.insert(Map::value_type(0x1002, "GTC"));
        map_.insert(Map::value_type(0x104F, "GTD"));
        map_.insert(Map::value_type(0x1050, "GTE"));
        map_.insert(Map::value_type(0x1084, "GTF"));
        map_.insert(Map::value_type(0x1085, "GTG"));
        map_.insert(Map::value_type(0x109A, "GTH"));
        map_.insert(Map::value_type(0x109B, "GTI"));
        map_.insert(Map::value_type(0x132D, "GTJ"));
        map_.insert(Map::value_type(0x103D, "MRA"));
        map_.insert(Map::value_type(0x1044, "MRB"));
        map_.insert(Map::value_type(0x1005, "MRC"));
        map_.insert(Map::value_type(0x1038, "MRD"));
        map_.insert(Map::value_type(0x1007, "MRE"));
        map_.insert(Map::value_type(0x1039, "MRF"));
        map_.insert(Map::value_type(0x1009, "MRG"));
    }

    Map::const_iterator pos = map_.find(systemId);
    if (pos != map_.end()) return pos->second;

    std::ostringstream os;
    switch (systemId & 0xF000) {
    case 0x0000: os << 'I'; break;
    case 0x1000: os << 'R'; break;
    case 0x2000: os << 'G'; break;
    default:     os << 'U'; break;
    }

    os << (systemId & 0x0FFF);
    return os.str();
}


TSPI::Ref
TSPI::MakeRAE(const std::string& producer, const std::string& id, double when,
              double r, double a, double e, RadarConfig* cfg)
{
    Ref ref(new TSPI(producer, id, when, InitFromRAE(r, a, e), cfg));
    return ref;
}

TSPI::Ref
TSPI::MakeLLH(const std::string& producer, const std::string& id, double when,
              double lat, double lon, double hgt)
{
    Ref ref(new TSPI(producer, id, when, InitFromLLH(lat, lon, hgt)));
    return ref;
}

TSPI::Ref
TSPI::MakeXYZ(const std::string& producer, const std::string& id, double when,
              double x, double y, double z)
{
    Ref ref(new TSPI(producer, id, when, InitFromXYZ(x, y, z)));
    return ref;
}

TSPI::Ref
TSPI::Make(const QSharedPointer<QByteArray>& raw)
{
    //qCDebug(radarmsg) << "Make" ;

    Ref ref(new TSPI);
    ref->load(raw);
    return ref;
}

Header::Ref
TSPI::CDRLoader(const QSharedPointer<QByteArray>& raw)
{
    return Make(raw);
}


double
TSPI::MakeEFGCoordinateFromRawBytes(const uint8_t* ptr)
{
    // Raw-data arrives in big-endian format. If on a little-endian machine we need to do some byte-reordering
    //
    int32_t tmp;
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
        // swap byte-order on little endian machine
        //
        tmp = (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) |
                ptr[0];
    }
    else { // on a big-endian machine, no nothing to bytes
        tmp = *((uint32_t*) ptr);
    }

    // TSPI values are scaled by 256 to preserve some fractional component.
    //
    return tmp / 256.0;
}


TSPI::TSPI(const std::string& producer, const std::string& tag, double when,
           const InitFromRAE& rae, RadarConfig* cfg)
    : Header(producer, GetMetaTypeInfo()), when_(when), flags_(0),
      tag_(tag), llh_(), rae_(), xyz_()
{
    setRadarConfig(cfg);
    //qCDebug(radarmsg) << "TSPI" ;
//    qCInfo(radarmsg) << "tag: " << QString::fromStdString(tag) << " when: " << when << " rng: " << rae.r_
//                 << " az: " << rae.a_ << " el: " << rae.e_ ;



    rae_.resize(3);
    rae_[GEO_RNG] = rae.r_;
    if (::fabs(rae.r_) < 1.0E-8) {
        rae_[GEO_AZ] = 0.0;
        rae_[GEO_EL] = 0.0;
    }
    else {
        rae_[GEO_AZ] = rae.a_;
        rae_[GEO_EL] = rae.e_;
    }

    GeoIns->geoRae2Efg(const_cast<GEO_LOCATION*>(GetOrigin()), &rae_[0], efg_);

    dump();
}

TSPI::TSPI(const std::string& producer, const std::string& tag, double when,
           const InitFromLLH& llh)
    : Header(producer, GetMetaTypeInfo()), when_(when), flags_(0),
      tag_(tag), llh_(), rae_(), xyz_()
{
    //qCDebug(radarmsg) << "TSPI" ;
    qCInfo(radarmsg) << "when: " << when << " lat: " << llh.lat_ << " lon: "
                 << llh.lon_ << " hgt: " << llh.hgt_ ;

    llh_.resize(3);
    llh_[GEO_LAT] = ZchxRadarUtils::degreesToRadians(llh.lat_);
    llh_[GEO_LON] = ZchxRadarUtils::degreesToRadians(llh.lon_);
    llh_[GEO_HGT] = llh.hgt_;
    GeoIns->geoLlh2Efg(llh_[GEO_LAT], llh_[GEO_LON], llh_[GEO_HGT],
               GEO_DATUM_DEFAULT, &efg_[GEO_E], &efg_[GEO_F],
               &efg_[GEO_G]);
    dump();
}

TSPI::TSPI(const std::string& producer, const std::string& tag, double when,
           const InitFromXYZ& xyz)
    : Header(producer, GetMetaTypeInfo()), when_(when), flags_(0),
      tag_(tag), llh_(), rae_(), xyz_()
{
    //qCDebug(radarmsg) << "TSPI" ;
    qCInfo(radarmsg) << "when: " << when << " x: " << xyz.x_ << " y: " << xyz.y_
                 << " z: " << xyz.z_ ;
    xyz_.resize(3);
    xyz_[GEO_X] = xyz.x_;
    xyz_[GEO_Y] = xyz.y_;
    xyz_[GEO_Z] = xyz.z_;
    rae_.resize(3);
    GeoIns->geoXyz2Rae(&xyz_[0], &rae_[0]);
    if (::fabs(rae_[GEO_RNG]) < 1.0E-8) {
        rae_[GEO_AZ] = 0.0;
        rae_[GEO_EL] = 0.0;
    }

    GeoIns->geoRae2Efg(const_cast<GEO_LOCATION*>(GetOrigin()), &rae_[0], efg_);

    dump();
}

TSPI::TSPI()
    : Header(GetMetaTypeInfo()), llh_(), rae_(), xyz_()
{
    ;
}

TSPI::TSPI(const std::string& producer)
    : Header(producer, GetMetaTypeInfo()), llh_(), rae_(), xyz_()
{
    ;
}

void
TSPI::calculateLLH() const
{
    //qCDebug(radarmsg) << "calculateLLH" ;

    // Convert from earth-centered to longitude, latitude, height (geodetic). NOTE: geoEfg2Llh() expects lat/lon
    // in radians, height in meters.
    //
    llh_.resize(3);
    GeoIns->geoEfg2Llh(GEO_DATUM_DEFAULT, const_cast<double*>(efg_),
               &llh_[GEO_LAT], &llh_[GEO_LON], &llh_[GEO_HGT]);
//    qCInfo(radarmsg)  << "LLH: " << ZchxRadarUtils::radiansToDegrees(llh_[GEO_LAT]) << ' '
//                  << ZchxRadarUtils::radiansToDegrees(llh_[GEO_LON]) << ' '
//                  << llh_[GEO_HGT] ;
}

const TSPI::Coord&
TSPI::getLatitudeLongitudeHeight() const
{
    if (llh_.empty())
        calculateLLH();
    return llh_;
}

void
TSPI::calculateRAE() const
{
    //qCDebug(radarmsg) << "calculateRAE" ;

    if (xyz_.empty())
        calculateXYZ();

    // Convert from delta meters to slant range, azimuth, elevation, with azimuth and elevation in radians.
    //
    rae_.resize(3);
    GeoIns->geoXyz2Rae(&xyz_[0], &rae_[0]);
    if (::fabs(rae_[GEO_RNG]) < 1.0E-8) {
        rae_[GEO_AZ] = 0.0;
        rae_[GEO_EL] = 0.0;
    }

    qCDebug(radarmsg) << "RAE: " << rae_[GEO_RNG] << ' '
                  << ZchxRadarUtils::radiansToDegrees(rae_[GEO_AZ]) << ' '
                  << ZchxRadarUtils::radiansToDegrees(rae_[GEO_EL]) ;
}

const TSPI::Coord&
TSPI::getRangeAzimuthElevation() const
{
    if (rae_.empty())
        calculateRAE();
    return rae_;
}

void
TSPI::calculateXYZ() const
{
    if (llh_.empty())
        calculateLLH();

    // Create a new location for delta calculations to this point from the radar origin. NOTE: geoInitLocation()
    // expects lat/lon in degrees, height in meters.
    //
    GEO_LOCATION target;
    GeoIns->geoInitLocation(&target, ZchxRadarUtils::radiansToDegrees(llh_[GEO_LAT]),
                    ZchxRadarUtils::radiansToDegrees(llh_[GEO_LON]),
                    llh_[GEO_HGT], GEO_DATUM_DEFAULT, "Target");

    xyz_.resize(3);
    GeoIns->geoEfg2XyzDiff(const_cast<GEO_LOCATION*>(GetOrigin()), &target,
                   &xyz_[0]);
}

const TSPI::Coord&
TSPI::getXYZ() const
{
    if (xyz_.empty())
        calculateXYZ();
    return xyz_;
}

void
TSPI::load(const QSharedPointer<QByteArray>& raw)
{
    //qCDebug(radarmsg) << "load" ;
    Super::load(raw);

    com::zhichenhaixin::proto::TrackPoint trackPoint;

    if(trackPoint.ParseFromArray(raw->data(), raw->size()))
    {
        int systemAreaCode = trackPoint.systemareacode();
        int systemIdentificationCode = trackPoint.systemidentificationcode();
        uint32_t trackNumber = trackPoint.tracknumber();
        float cartesianPosX = trackPoint.cartesianposx();
        float cartesianPosY = trackPoint.cartesianposy();
        double wgs84PosLat = trackPoint.wgs84poslat();
        double wgs84PosLong = trackPoint.wgs84poslong();
        double cog = trackPoint.cog();
        double sog = trackPoint.sog();
        float  timeOfDay = trackPoint.timeofday();


//        qCDebug(radarmsg) << "trackPoint:"<< "  \n"
//                      << "systemAreaCode: "<< systemAreaCode << "  \n"
//                      << "systemIdentificationCode :" << systemIdentificationCode << "  \n"
//                      << "trackNumber :" << trackNumber << " \n"
//                      << "cartesianPosX  :" << cartesianPosX << "  \n"
//                      << "cartesianPosY :" << cartesianPosY << " \n"
//                      << "wgs84PosLat:" << wgs84PosLat  << " \n"
//                      << "wgs84PosLong:" << wgs84PosLong  << " \n"
//                      << "cog:" << cog  << " \n"
//                      << "sog:" << sog  << " \n"
//                      << "timeOfDay :" << timeOfDay << " \n";

        //@todo  载入数据到 data_容器
        QString tag;
        tag.sprintf("T%04d", trackNumber);

        std::string systemId = "RFA" ;
        when_ = timeOfDay;
        flags_= 0;
        tag_ = tag.toStdString();

        if ((wgs84PosLat !=0.0) && (wgs84PosLong !=0.0))
        {
            //qCDebug(radarmsg) << "calculateLLH" ;

            InitFromLLH llh = InitFromLLH(wgs84PosLat, wgs84PosLong, 0);
            llh_.resize(3);
            llh_[GEO_LAT] = ZchxRadarUtils::degreesToRadians(llh.lat_);
            llh_[GEO_LON] = ZchxRadarUtils::degreesToRadians(llh.lon_);
            llh_[GEO_HGT] = llh.hgt_;
            GeoIns->geoLlh2Efg(llh_[GEO_LAT], llh_[GEO_LON], llh_[GEO_HGT],
                       GEO_DATUM_DEFAULT, &efg_[GEO_E], &efg_[GEO_F],
                       &efg_[GEO_G]);
        }
        // qCDebug(radarmsg) << "calculateRAE" ;

        // llh_.resize(3);
        // llh_[GEO_LAT] = ZchxRadarUtils::degreesToRadians('112.00');
        // llh_[GEO_LON] = ZchxRadarUtils::degreesToRadians("22.00");
        // llh_[GEO_HGT] = 100.00;

        // xyz_.resize(3);
        // xyz_[GEO_X] = xyz.x_;
        // xyz_[GEO_Y] = xyz.y_;
        // xyz_[GEO_Z] = xyz.z_;

        // rae_.resize(3);
        // geoXyz2Rae(&xyz_[0], &rae_[0]);
        // if (::fabs(rae_[GEO_RNG]) < 1.0E-8) {
        // rae_[GEO_AZ] = 0.0;
        // rae_[GEO_EL] = 0.0;
        // }

//        qCDebug(radarmsg) << "====RAE===="<< "  \n"
//                      << "Tag:" << QString::fromStdString(tag_) << " \n"
//                      << "When:" << when_  << " \n"
//                      << "Rang:" <<  getRange()  << " \n"
//                      << "Azimuth:" << ZchxRadarUtils::radiansToDegrees(getAzimuth()) << " \n"
//                      << "getElevation() :" << getElevation() << " \n";

    }
}



TrackPoint TSPI::toTrackPoint() const
{
    TrackPoint trackPoint;
    trackPoint.set_tracknumber(QString::fromStdString(tag_).toInt());
    trackPoint.set_timeofday(when_);
    trackPoint.set_systemareacode(0);
    trackPoint.set_systemidentificationcode(1);
    trackPoint.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
    trackPoint.set_cartesianposx(getX());
    trackPoint.set_cartesianposy(getY());
    trackPoint.set_wgs84poslong(ZchxRadarUtils::radiansToDegrees(getLongitude()));
    trackPoint.set_wgs84poslat(ZchxRadarUtils::radiansToDegrees(getLatitude()));
    trackPoint.set_tracklastreport(false);
    trackPoint.set_cartesiantrkvel_vx(0);
    trackPoint.set_cartesiantrkvel_vy(0);
    trackPoint.set_cog(0.0);
    trackPoint.set_sog(0.0);
    if(isDropping())
    {
        trackPoint.set_tracklastreport(true);
    }
    return trackPoint;
}



std::ostream&
TSPI::printData(std::ostream& os) const
{
    os << "Tag: " << tag_ << " When: " << when_ << " RAE: "
       << getRange() << "/" << ZchxRadarUtils::radiansToDegrees(getAzimuth()) << "/"
       << getElevation() << " Flags: " << flags_;
    return os;
}

std::ostream&
TSPI::printDataXML(std::ostream& os) const
{
    return os << "<plot tag=\"" << tag_
              << "\" when=\"" << when_
              << "\" range=\"" << getRange()
              << "\" azimuth=\"" << ZchxRadarUtils::radiansToDegrees(getAzimuth())
              << "\" elevation=\"" << getElevation()
              << "\" flags=\"" << flags_
              << "\" />";
}

void
TSPI::dump() const
{
//    qCDebug(radarmsg) << "dump" ;
//    qCDebug(radarmsg) << "Tag: " << QString::fromStdString(tag_) << " When: " <<  when_ ;
//    qCDebug(radarmsg) << "EFG: " << efg_[GEO_E] <<  ' '
//                  << efg_[GEO_F] << ' '
//                  << efg_[GEO_G] ;
//    qCDebug(radarmsg) << "LLH: " << ZchxRadarUtils::radiansToDegrees(getLatitude()) << ' '
//                  << ZchxRadarUtils::radiansToDegrees(getLongitude()) << ' '
//                  << getHeight() ;
//    qCDebug(radarmsg) << "RAE: " << getRange() << ' '
//                  << ZchxRadarUtils::radiansToDegrees(getAzimuth()) << ' '
//                  << ZchxRadarUtils::radiansToDegrees(getElevation()) ;
//    qCDebug(radarmsg) << "XYZ: " << getX() << ' '
//                  << getY() << ' '
//                  << getZ() ;
}
