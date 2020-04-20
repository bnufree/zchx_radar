#include "boost/bind.hpp"
#include <cmath>

#include "Messages/RadarConfig.h"
#include "Messages/TSPI.h"
#include "Messages/Track.h"
#include "zchxRadarUtils.h"

#include "TrackInitiator.h"
#include "TrackInitiator_defaults.h"
#include "common.h"

using namespace ZCHX;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Messages;

TrackInitiator::TrackInitiator() :
    param_searchRadius(kDefaultSearchRadius),
    param_scanTime(time_t(RadarConfig(0).getRotationDuration())),
    param_numScans(kDefaultNumScans),
    param_assumedAltitude(/*kDefaultAssumedAltitude*/1.0),
    param_minRange(kDefaultMinRange)
{
    //param_searchRadius->connectChangedSignalTo(boost::bind(&TrackInitiator::on_searchRadius_changed, this, _1));
    //param_scanTime->connectChangedSignalTo(boost::bind(&TrackInitiator::on_scanTime_changed, this, _1));
    on_scanTime_changed(param_scanTime);
}


bool
TrackInitiator::reset()
{
    searchRadius_ = param_searchRadius;
    searchRadius2_ = searchRadius_ * searchRadius_;
    return true;
}

bool
TrackInitiator::process(const Messages::Extractions::Ref& msg)
{

    int num_scans = param_numScans;

    Extractions::const_iterator pos = msg->begin();
    Extractions::const_iterator end = msg->end();

    while (pos != end) {
        Data d(*pos++);
        corr(d);

        Extraction& ext(d.extraction_);
        LOG_FUNC_DBG << "correlated? " << ext.getCorrelated() << " at " << ext.getX() << ", " << ext.getY() << endl;

        if (ext.getCorrelated() && ext.getNumCorrelations() >= num_scans - 1) {
            LOG_FUNC_DBG << "extraction is correlated for " << (ext.getNumCorrelations() + 1) << " scans " << endl;

            // Make new Track Message
            Messages::Track::Ref trk(Track::Make("TrackInitiator"));

            trk->setFlags(Messages::Track::kNew);
            trk->setType(Messages::Track::kTentative);
            trk->setTrackNumber(currentTrackNum_++);

            LOG_FUNC_DBG << "sending track message type " << trk->getType() << endl;
            LOG_FUNC_DBG << "sending track message Num: " << trk->getTrackNumber() << endl;

            // Set track estimate = last known measurement in this hypothesis convert the last measurement to
            // llh compute elevation angle given assumed altitude
            //这里取得雷达的经纬度配置

            // geoInitLocation() expects lat/lon in degrees, height in meters.
            //



            GEO_LOCATION* origin = new GEO_LOCATION;
            GeoIns->geoInitLocation(origin,
                            RadarConfig(0).getSiteLat(),
                            RadarConfig(0).getSiteLong(),
                            RadarConfig(0).getSiteHeight(),
                            GEO_DATUM_DEFAULT,
                            "Radar");

            Track::Coord rae;
            rae[GEO_AZ] = ext.getAzimuth();       // radians
            rae[GEO_RNG] = ext.getRange() * 1000; // meters
            rae[GEO_EL] = ZchxRadarUtils::el_from_range_and_alt(rae[GEO_RNG], param_assumedAltitude,
                                                       ZchxRadarUtils::degreesToRadians(origin->lat));

            // Convert measurement to llh
            //
            Track::Coord efg;
            GeoIns->geoRae2Efg(const_cast<GEO_LOCATION*>(origin), &rae[0], efg.tuple_);

            Track::Coord measurement;
            GeoIns->geoEfg2Llh(GEO_DATUM_DEFAULT, efg.tuple_, &measurement[GEO_LAT], &measurement[GEO_LON],
                       &measurement[GEO_HGT]);

            trk->setEstimate(measurement);
            double when = ext.getWhen();
            trk->setWhen(when);
            trk->setExtractionTime(when);

            // Velocity estimate should be a simple point-to-point velocity between last two measurements in the
            // hypothesis convert enu velocity to llh velocity
            //
            Track::Coord xyz_vel(d.velocity_[0] * 1000, d.velocity_[1] * 1000);
            Track::Coord rae_vel;
            GeoIns->geoXyz2Rae(xyz_vel.tuple_, rae_vel.tuple_);

            GEO_LOCATION* morigin = new GEO_LOCATION;

            //
            // geoInitLocation() expects lat/lon in degrees, height in meters.
            // geoInitLocation(measurement_origin,measurement[GEO_LAT],measurement[GEO_LON],measurement[GEO_HGT],
            // GEO_DATUM_DEFAULT,"Measurement");

            GeoIns->geoInitLocation(morigin, ZchxRadarUtils::radiansToDegrees(measurement[GEO_LAT]),
                            ZchxRadarUtils::radiansToDegrees(measurement[GEO_LON]), measurement[GEO_HGT], GEO_DATUM_DEFAULT,
                            "Measurement");

            Track::Coord efg_vel;
            GeoIns->geoRae2Efg(morigin, &rae_vel[0], efg_vel.tuple_);
            Track::Coord llh_vel;
            GeoIns->geoEfg2Llh(GEO_DATUM_DEFAULT, efg_vel.tuple_, &llh_vel[GEO_LAT], &llh_vel[GEO_LON], &llh_vel[GEO_HGT]);
            llh_vel[GEO_LAT] -= measurement[GEO_LAT];
            llh_vel[GEO_LON] -= measurement[GEO_LON];
            llh_vel[GEO_HGT] = 0.0;

            trk->setVelocity(llh_vel);

            LOG_FUNC_DBG << "sending new track report for track " << trk->getTrackNumber() << endl;

            //return send(trk);
        }
    }

    return true;
}

void
TrackInitiator::init()
{
    double rMax = RadarConfig(0).getRangeMax();
    currentTrackNum_ = 0;

    // Initialize the buffer
    //
    indexOffset_ = int(ceil(rMax / searchRadius_)) + 1;
    numBins_ = 2 * indexOffset_ + 2; // +2 is to allow for less code in corr()
    buffer_.resize(numBins_);
    for (size_t i = 0; i < numBins_; ++i) {
        buffer_[i].clear();
        buffer_[i].resize(numBins_);
    }
}

void
TrackInitiator::corrCell(Data& d, Entry& candidates)
{

    Extraction& ext = d.extraction_;

    LOG_FUNC_DBG << "cell count: " << candidates.size() << endl;

    Entry::iterator ci = candidates.begin();
    Entry::iterator end = candidates.end();
    while (ci != end) {
        double delta = d.when_ - ci->when_;
        if (delta > t_old_) {
            LOG_FUNC_DBG << "too old. delta=" << delta << " threshold=" << t_old_ << endl;

            if (delta > t_veryold_) {
                LOG_FUNC_DBG << "very old, erase." << endl;
                ci = candidates.erase(ci);
            } else {
                ++ci;
            }
            continue;
        }

        // Ignore new entries
        //
        if (delta < t_new_) {
            LOG_FUNC_DBG << "too new." << endl;
            ++ci;
            continue;
        }

        const Extraction& other = ci->extraction_;

        // Verify the exact cartesian distance
        //
        float dx = ext.getX() - other.getX();
        float dy = ext.getY() - other.getY();
        float dist2 = dx * dx + dy * dy;
        LOG_FUNC_DBG << "too far? ";

        if (dist2 < searchRadius2_) {
            LOG_FUNC_DBG << "no.  correlating." << endl;

            ext.setCorrelated(true);
            int num_scans = other.getNumCorrelations() + 1;
            ext.setNumCorrelations(num_scans);

            //
            // Compute simple point-to-point velocity in enu coordinates
            //
            delta = ext.getWhen() - other.getWhen();
            d.velocity_[0] = dx / delta;
            d.velocity_[1] = dy / delta;
            return;
        }

        LOG_FUNC_DBG << "yes." << endl;
        ++ci;
    }
}

void
TrackInitiator::corr(Data& d)
{
    LOG_FUNC_DBG << endl;

    // Determine which bin this extraction belongs to (add +1) to shift the logical data buffer within the
    // larger buffer
    //
    int binX = int(floor(d.extraction_.getX() / searchRadius_)) + indexOffset_ + 1;
    int binY = int(floor(d.extraction_.getY() / searchRadius_)) + indexOffset_ + 1;

    d.extraction_.setCorrelated(false);

    LOG_FUNC_DBG << "Correlating extraction (" << d.extraction_.getX() << ", " << d.extraction_.getY() << ") into [" << binX
             << ", " << binY << "] numBins=" << numBins_ << endl;

    if (binX < 1 || binX >= numBins_ - 1 || binY < 1 || binY >= numBins_ - 1) {
        LOG_FUNC_DBG << "Bad bin #, bailing" << endl;
        return;
    }

    // Perform the correlations binX and binY are never = 0 or numBins - 1, so this code holds for all possible
    // cases
    //
    corrCell(d, buffer_[binX - 1][binY - 1]);
    corrCell(d, buffer_[binX - 1][binY]);
    corrCell(d, buffer_[binX - 1][binY + 1]);
    corrCell(d, buffer_[binX][binY - 1]);
    corrCell(d, buffer_[binX + 1][binY - 1]);
    corrCell(d, buffer_[binX][binY]);
    corrCell(d, buffer_[binX][binY + 1]);
    corrCell(d, buffer_[binX + 1][binY]);
    corrCell(d, buffer_[binX + 1][binY + 1]);

    if (!d.extraction_.getCorrelated()) d.extraction_.setNumCorrelations(0);

    buffer_[binX][binY].push_back(d);
}

void
TrackInitiator::on_searchRadius_changed(const double& x)
{
    param_searchRadius = x;
    searchRadius_ = param_searchRadius;
    searchRadius2_ = searchRadius_ * searchRadius_;
    init();
}

void
TrackInitiator::on_scanTime_changed(const int& x)
{
    param_scanTime = x;
    double scanRate = param_scanTime;
    t_new_ = scanRate / 2;
    t_old_ = scanRate + t_new_;
    t_veryold_ = 2 * scanRate;
}


