#ifndef SIDECAR_ALGORITHMS_ABTRACKER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_ABTRACKER_H

#include <list>
#include "Messages/Extraction.h"
#include <QObject>
#include "ABTrack.h"
#include <QThread>
#include <QTimer>

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
    struct CloseTrackData{
        ABTrackerUtils::ABTrack* track;
        double distance;
    };

    enum InfoSlots { kEnabled = /*ControllerStatus::kNumSlots*/1, kAlpha, kBeta, kTrackCount, kTimeScaling, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    explicit ABTracker(RadarConfig* cfg, bool new_thread = false, QObject* parent = NULL);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    //bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    //bool shutdown();
    //设置旋转持续时间
    void setRotationDuration(double value)
    {
        rotationDuration_ = value;
        calculateDurations();//计算时长
    }
    //获取旋转持续时间
    double getRotationDuration() const { return rotationDuration_; }

    void setTimeScaling(double value)
    {
        timeScaling_ = value;
        calculateDurations();//计算时长
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
    void Extraction2Track(const Messages::Extractions::Ref& msg);
    zchxTrackPointList getTrackPnts();
    void setNewLoop(bool sts);
signals:
    void sendTrackPoints(const zchxTrackPointList& list);
public slots:
    void slotRecvExtractions(const Extractions::Ref& msg);
    void slotTimeroutFunc();

private:
    bool updateTracks(const Messages::Extraction& plot);

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
    //using TrackList = std::list<ABTrackerUtils::ABTrack>;
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
    QThread                     mWorkThread;
    QTimer*                     mSendTimer;
    bool                        mNewThreadFlag;
    bool                        mNewLoop;
    ZCHX::Algorithms::ABTrackerUtils::ABTrack * found;
//    ZCHX::Algorithms::ABTrackerUtils::ABTrack track;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
