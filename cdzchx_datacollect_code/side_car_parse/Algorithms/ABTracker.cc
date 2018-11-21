#include "QtCore/QString"

#include "boost/bind.hpp"
#include "Messages/TSPI.h"

#include "ABTracker.h"
#include "ABTracker_defaults.h"

#include "ABTrack.h"
#include "common.h"

using namespace ZCHX;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Algorithms::ABTrackerUtils;

ABTracker::ABTracker(QObject* parent) :QObject(parent),
    enabled_(kDefaultEnabled), rotationDuration_(kDefaultRotationDuration),
    timeScaling_(kDefaultTimeScaling),
    alpha_(kDefaultAlpha),
    beta_(kDefaultBeta),
    associationRadius_( kDefaultAssociationRadius),
    initiationCount_(kDefaultInitiationCount),
    initiationRotationCount_(kDefaultInitiationRotationCount),
    coastRotationCount_( kDefaultCoastRotationCount),
    minRange_( /*kDefaultMinRange*/0),
   /* reset_(Parameter::NotificationValue::Make("reset", "Reset", 0)),*/ trackIdGenerator_(), tracks_(), trackCount_(0),
    initiatingCount_(0), coastingCount_(0)
{
    associationRadius2_ = associationRadius_;
    associationRadius2_ *= associationRadius2_;
    //reset_->connectChangedSignalTo(boost::bind(&ABTracker::resetNotification, this, _1));
    //associationRadius_->connectChangedSignalTo(boost::bind(&ABTracker::associationRadiusChanged, this, _1));
}

//bool
//ABTracker::startup()
//{
//    endParameterChanges();
//    registerProcessor<ABTracker, Messages::Extractions>(&ABTracker::processInput);
//    return Super::startup() && registerParameter(enabled_) && registerParameter(rotationDuration_) &&
//           registerParameter(timeScaling_) && registerParameter(alpha_) && registerParameter(beta_) &&
//           registerParameter(associationRadius_) && registerParameter(initiationCount_) &&
//           registerParameter(initiationRotationCount_) && registerParameter(coastRotationCount_) &&
//           registerParameter(minRange_) && registerParameter(reset_);
//}

//bool
//ABTracker::shutdown()
//{
//    resetTracker();
//    return Super::shutdown();
//}

void
ABTracker::resetTracker()
{
    while (trackCount_) {
        ABTrack* track = tracks_.back();
        track->drop();
        delete track;
        tracks_.pop_back();
        --trackCount_;
    }

    initiatingCount_ = 0;
    coastingCount_ = 0;
}

bool
ABTracker::processInput(const Messages::Extractions::Ref& msg, QMap<int, TrackPoint>& pnts)
{
    LOGDEBUG << std::endl;

    // For each Extraction report in the message call updateTracks.
    //
    bool ok = true;
    for (size_t index = 0; index < msg->size(); ++index) {
        if (!updateTracks(msg[index], pnts)) ok = false;
    }

    return ok;
}

bool
ABTracker::updateTracks(const Messages::Extraction& plot, QMap<int, TrackPoint>& pnts)
{
    double when = plot.getWhen();

    LOGDEBUG << "when: " << when << " range: " << plot.getRange() << " az: " << plot.getAzimuth() << std::endl;

    ABTrack* found = 0;
    if (plot.getRange() >= minRange_) {
        // We only associate with something if it has a proximity value smaller
        // than associationRadius. We use the squared variants to save us from
        // the costly square root.
        //
        double best = associationRadius2_;
        Geometry::Vector v(plot.getX(), plot.getY(), plot.getElevation());

        // Visit all of the existing Track objects to find the one that best
        // associates with the given Extraction plot.
        //
        TrackList::iterator pos = tracks_.begin();
        TrackList::iterator end = tracks_.end();
        while (pos != end) {
            ABTrack* track = *pos++;
            double distance = track->getProximityTo(when, v);
            if (distance < best) {
                found = track;
                best = distance;
            }
        }

        // None found, so create a new Track object that starts in the
        // kInitiating state.
        //
        if (!found) {
            found = new ABTrack(*this, ++trackIdGenerator_, when, v);
            LOGDEBUG << "created new track - " << found->getId() << std::endl;
            tracks_.push_back(found);
            ++trackCount_;
        } else {
            LOGDEBUG << "found existing track - " << found->getId() << std::endl;

            // Apply the Extraction plot to the found track
            //
            found->updatePosition(when, v);
        }
    }

    // Revisit all of the tracks, emitting TSPI messages for those that are
    // dropping and the one that was updated above.
    //
    bool ok = true;
    TrackList::iterator pos = tracks_.begin();
    TrackList::iterator end = tracks_.end();
    while (pos != end) {
        ABTrack* track = *pos;
        if (track == found || track->isDropping()) {
            TrackPoint point = track->emitPosition();
            if(point.tracknumber() > 0)
            {
                pnts[point.tracknumber()] = point;
            }
            if (track->isDropping()) {
                LOGDEBUG << "dropping track " << track->getId() << std::endl;
                delete track;
                pos = tracks_.erase(pos);
                --trackCount_;
                continue;
            }
        }
        ++pos;
    }

    return ok;
}

//void
//ABTracker::setInfoSlots(IO::StatusBase& status)
//{
//    status.setSlot(kEnabled, enabled_->getValue());
//    status.setSlot(kAlpha, alpha_->getValue());
//    status.setSlot(kBeta, beta_->getValue());
//    status.setSlot(kTrackCount, int(trackCount_));
//}

void
ABTracker::resetNotification()
{
    resetTracker();
}

void
ABTracker::associationRadiusChanged(const double& value)
{
    associationRadius2_ = value;
    associationRadius2_ *= associationRadius2_;
}

void
ABTracker::endParameterChanges()
{
    calculateDurations();
}

void
ABTracker::calculateDurations()
{
    double scaledRotationDuration = rotationDuration_ * timeScaling_;
    scaledMaxInitiationDuration_ = initiationRotationCount_ * scaledRotationDuration;
    scaledMaxCoastDuration_ = coastRotationCount_ * scaledRotationDuration;
}

