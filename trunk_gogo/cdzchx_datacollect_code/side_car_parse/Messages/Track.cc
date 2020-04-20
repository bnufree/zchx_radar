#include <cmath>
#include <iomanip>
#include <map>

#include "zchxRadarUtils.h"

#include "RadarConfig.h"
#include "Track.h"
#include "common.h"

using namespace ZCHX::Messages;


MetaTypeInfo Track::metaTypeInfo_(MetaTypeInfo::Value::kTrack, "Track", &Track::CDRLoader);

//Logger::Log&
//Track::Log()
//{
//    static Logger::Log& log_ = Logger::Log::Find("SideCar.Messages.Track");
//    return log_;
//}

const MetaTypeInfo&
Track::GetMetaTypeInfo()
{
    return metaTypeInfo_;
}

//Track::Ref
//Track::Make(ACE_InputCDR& cdr)
//{
//    Ref ref(new Track);
//    ref->load(cdr);
//    return ref;
//}

Track::Ref
Track::Make(const QSharedPointer<QByteArray>& raw)
{
    Ref ref(new Track);
    ref->load(raw);
    return ref;
}

Track::Ref
Track::Make(const std::string& producer)
{
    Ref ref(new Track(producer));
    return ref;
}

/** Copy constructor */
Track::Ref
Track::Make(const std::string& producer, const Messages::Track::Ref& report)
{
    Ref ref(new Track(producer));
    ref->setWhen(report->getWhen());
    ref->setEstimate(report->getEstimate());
    ref->setVelocity(report->getVelocity());
    ref->setFlags(report->getFlags());
    ref->setBatchNum(report->getBatchNum());
    ref->setTotalInBatch(report->getTotalInBatch());
    ref->setTrackNumber(report->getTrackNumber());
    ref->setExtraction(report->getExtraction());
    ref->setExtractionTime(report->getExtractionTime());
    ref->setPrediction(report->getPrediction());
    ref->setPredictionTime(report->getPredictionTime());
    ref->setType(report->getType());
    ref->setStartTime(report->getStartTime());
    ref->setConfirmedTime(report->getConfirmedTime());
    return ref;
}

Header::Ref
Track::CDRLoader(const QSharedPointer<QByteArray>& raw)
{
    return Make(raw);
}

//Header::Ref
//Track::XMLLoader(const std::string& producer, XmlStreamReader& xsr)
//{
//    Ref ref(new Track(producer));
//    ref->loadXML(xsr);
//    return ref;
//}

Track::Track(const std::string& producer) :
    Super(producer, GetMetaTypeInfo()), batchNum_(0), totalInBatch_(0), trackNum_(0), aer_extraction_(),
    extractionTime_(0.0), llh_prediction_(), predictionTime_(0.0), type_(0), startTime_(0), confirmedTime_(0)
{
    ;
}

Track::Track() :
    Super(GetMetaTypeInfo()), batchNum_(0), totalInBatch_(0), trackNum_(0), aer_extraction_(), extractionTime_(0.0),
    llh_prediction_(), predictionTime_(0.0), type_(0), startTime_(0), confirmedTime_(0)
{
    ;
}

//ACE_InputCDR&
//Track::load(ACE_InputCDR& cdr)
//{
//    return loaderRegistry_.load(this, cdr);
//}

//ACE_InputCDR&
//Track::LoadV1(Track* obj, ACE_InputCDR& cdr)
//{
//    return obj->loadV1(cdr);
//}

//ACE_InputCDR&
//Track::loadV1(ACE_InputCDR& cdr)
//{
//    static Logger::ProcLog log("loadV1", Log());
//    LOGINFO << "BEGIN" << endl;

//    cdr >> batchNum_;
//    cdr >> totalInBatch_;
//    cdr >> trackNum_;

//    // current state estimate for position
//    cdr >> llh_[GEO_LAT];
//    cdr >> llh_[GEO_LON];
//    cdr >> llh_[GEO_HGT];

//    // current state estimate for velocity
//    cdr >> llh_velocity_[GEO_LAT];
//    cdr >> llh_velocity_[GEO_LON];
//    cdr >> llh_velocity_[GEO_HGT];
//    cdr >> when_;

//    // most recent measurement
//    cdr >> aer_extraction_[GEO_AZ];
//    cdr >> aer_extraction_[GEO_EL];
//    cdr >> aer_extraction_[GEO_RNG];
//    cdr >> extractionTime_;
//    cdr >> extractionNum_;

//    // most recent prediction
//    cdr >> llh_prediction_[GEO_LAT];
//    cdr >> llh_prediction_[GEO_LON];
//    cdr >> llh_prediction_[GEO_HGT];
//    cdr >> predictionTime_;

//    cdr >> flags_;
//    cdr >> type_;
//    cdr >> startTime_;
//    cdr >> confirmedTime_;

//    return cdr;
//}

//ACE_OutputCDR&
//Track::write(ACE_OutputCDR& cdr) const
//{
//    Header::write(cdr);
//    cdr << loaderRegistry_.getCurrentVersion();

//    cdr << batchNum_;
//    cdr << totalInBatch_;
//    cdr << trackNum_;

//    // current state estimate
//    cdr << llh_[GEO_LAT];
//    cdr << llh_[GEO_LON];
//    cdr << llh_[GEO_HGT];
//    cdr << llh_velocity_[GEO_LAT];
//    cdr << llh_velocity_[GEO_LON];
//    cdr << llh_velocity_[GEO_HGT];

//    cdr << when_;

//    // most recent measurement
//    cdr << aer_extraction_[GEO_AZ];
//    cdr << aer_extraction_[GEO_EL];
//    cdr << aer_extraction_[GEO_RNG];
//    cdr << extractionTime_;
//    cdr << extractionNum_;

//    // most recent prediction
//    cdr << llh_prediction_[GEO_LAT];
//    cdr << llh_prediction_[GEO_LON];
//    cdr << llh_prediction_[GEO_HGT];
//    cdr << predictionTime_;

//    cdr << flags_;
//    cdr << type_;

//    cdr << startTime_;
//    cdr << confirmedTime_;

//    return cdr;
//}

std::ostream&
Track::printData(std::ostream& os) const
{
    os << "Track: " << trackNum_ << " When: " << std::setprecision(15) << when_
       << " LLH: " << ZchxRadarUtils::radiansToDegrees(llh_[GEO_LAT]) << "/" << ZchxRadarUtils::radiansToDegrees(llh_[GEO_LON]) << "/"
       << llh_[GEO_HGT] << " Flags: " << flags_ << endl;

    os << "Prediction at time " << std::setprecision(15) << predictionTime_ << ": "
       << ZchxRadarUtils::radiansToDegrees(llh_prediction_[GEO_LAT]) << "/" << ZchxRadarUtils::radiansToDegrees(llh_prediction_[GEO_LON])
       << "/" << llh_prediction_[GEO_HGT] << endl;

    os << "Extraction at time " << std::setprecision(15) << extractionTime_ << ": "
       << ZchxRadarUtils::radiansToDegrees(aer_extraction_[GEO_AZ]) << "/" << ZchxRadarUtils::radiansToDegrees(aer_extraction_[GEO_EL])
       << "/" << aer_extraction_[GEO_RNG] << endl;

    os << "Velocity: " << ZchxRadarUtils::radiansToDegrees(llh_velocity_[GEO_LAT]) << "/"
       << ZchxRadarUtils::radiansToDegrees(llh_velocity_[GEO_LON]) << "/" << llh_velocity_[GEO_HGT] << endl;

    return os;
}

std::ostream&
Track::printDataXML(std::ostream& os) const
{
    return os << "<plot track=\"" << trackNum_ << "\" when=\"" << when_ << "\" lat=\"" << getLatitude() << "\" lon=\""
              << getLongitude() << "\" height=\"" << getHeight() << "\" flags=\"" << flags_ << "\" />";
}

//void
//Track::loadXML(XmlStreamReader& xsr)
//{
//    std::cerr << "loadXML" << endl;

//    Header::loadXML(xsr);
//    if (!xsr.readNextEntityAndValidate("plot")) ::abort();

//    when_ = xsr.getAttribute("when").toDouble();
//    llh_[GEO_LAT] = xsr.getAttribute("latitude").toDouble();
//    llh_[GEO_LON] = xsr.getAttribute("longitude").toDouble();
//    llh_[GEO_HGT] = xsr.getAttribute("height").toDouble();
//    flags_ = xsr.getAttribute("flags").toShort();
//}

void
Track::dump() const
{
    LOG_FUNC_DBG << "Track: " << trackNum_ << " When: " << when_ << endl;

    LOG_FUNC_DBG << "LLH: " << ZchxRadarUtils::radiansToDegrees(getLatitude()) << ' ' << ZchxRadarUtils::radiansToDegrees(getLongitude())
             << ' ' << getHeight() << endl;
}
