#ifndef SIDECAR_ALGORITHMS_ABTRACKER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_ABTRACKER_H

#include <list>
#include "Messages/Extraction.h"
#include <QObject>
#include "ABTrack.h"

namespace ZCHX {
namespace Algorithms {
namespace ABTrackerUtils {
class ABTrack;
}

/** Documentation for the algorithm ABTracker. Please describe what the algorithm does, in layman's terms and,
    if possible, mathematical terms.
*/
class ABTracker:public QObject
{
    Q_OBJECT
public:
    enum InfoSlots { kEnabled = /*ControllerStatus::kNumSlots*/1, kAlpha, kBeta, kTrackCount, kTimeScaling, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    explicit ABTracker(RadarConfig* cfg, QObject* parent = NULL);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    //bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    //bool shutdown();

    void setRotationDuration(double value)
    {
        rotationDuration_ = value;
        calculateDurations();
    }

    double getRotationDuration() const { return rotationDuration_; }

    void setTimeScaling(double value)
    {
        timeScaling_ = value;
        calculateDurations();
    }

    double getTimeScaling() const { return timeScaling_; }

    void setAlpha(double value) { alpha_ = value; }

    double getAlpha() const { return alpha_; }

    void setBeta(double value) { beta_ = value; }

    double getBeta() const { return beta_; }

    void setAssociationRadius(double value) { associationRadius_ = value; }

    double getAssociationRadius() const { return associationRadius_; }

    void setInitiationCount(int value) { initiationCount_ = value; }

    uint32_t getInitiationCount() const { return initiationCount_; }

    void setInitiationRotationCount(double value)
    {
        initiationRotationCount_ = value;
        calculateDurations();
    }

    double getInitiationRotationCount() const { return initiationRotationCount_; }

    void setCoastRotationCount(double value)
    {
        coastRotationCount_ = value;
        calculateDurations();
    }

    double getCoastRotationCount() const { return coastRotationCount_; }

    void setMinRange(double value) { minRange_ = value; }

    double getMinRange() const { return minRange_; }

    double getScaledMaxInitiationDuration() const { return scaledMaxInitiationDuration_; }

    double getScaledMaxCoastDuration() const { return scaledMaxCoastDuration_; }
    void Extraction2Track(const Messages::Extractions::Ref& msg, QMap<int, TrackPoint>& pnts);

private:
    bool updateTracks(const Messages::Extraction& plot, QMap<int, TrackPoint>& pnts);

    size_t getNumInfoSlots() const { return kNumSlots; }

    //void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */


    void resetTracker();

    void resetNotification();

    void associationRadiusChanged(const double& value);

    void endParameterChanges();

    void calculateDurations();

    // Add attributes here
    //
    bool enabled_;
    double rotationDuration_;
    double timeScaling_;
    double alpha_;
    double beta_;
    double associationRadius_;
    uint initiationCount_;
    double initiationRotationCount_;
    double coastRotationCount_;
    double  minRange_;

    double associationRadius2_;
    uint32_t trackIdGenerator_;
    using TrackList = std::list<ABTrackerUtils::ABTrack*>;
    TrackList tracks_;
    size_t trackCount_;
    size_t initiatingCount_;
    size_t coastingCount_;

    struct Region {
        Region() : name(), rangeMin(0.0), rangeMax(0.0), azMin(0.0), azMax(0.0) {}
        QString name;
        double rangeMin;
        double rangeMax;
        double azMin;
        double azMax;
    };

    using RegionVector = std::vector<Region>;
    RegionVector initiationInhibitRegions_;

    double scaledMaxInitiationDuration_;
    double scaledMaxCoastDuration_;
    RadarConfig* cfg_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
