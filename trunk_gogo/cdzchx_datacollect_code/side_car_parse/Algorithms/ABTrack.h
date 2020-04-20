#ifndef SIDECAR_ALGORITHMS_ABTRACKERUTILS_TRACK_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_ABTRACKERUTILS_TRACK_H

#include "Vector.h"
#include "ais_radar/zchxradarcommon.h"
#include "Messages/RadarConfig.h"

using namespace ZCHX::Messages;
namespace ZCHX {
namespace Messages {
class Extraction;
class RadarConfig;
}
namespace Algorithms {

class ABTracker;

namespace ABTrackerUtils {

/** Representation of a track managed by the ABTracker.
 */
class ABTrack {
public:
    enum State {
        kInitiating,   ///< Track needs more reports to be active
        kAlive,        ///< Track is active and emitting reports
        kDropping,     ///< Track is dropping
        kUninitiating, ///< Track failed to initiate
        kNumStates
    };

    /** Constructor

        \param owner ABTracker reference

        \param id unique integer ID for this track

        \param when time of creation

        \param pos initial position of the track
    */
    ABTrack(ABTracker& owner, uint32_t id, double when, const Geometry::Vector& pos, RadarConfig* cfg);
    void    updatePos(const Geometry::Vector& pos);

    /** Obtain the track's unique ID tag

        \return track ID tag
    */
    const std::string& getId() const { return id_; }

    /** Determine how close this track is from a given position. Estimates the track's position at the given
        time, and then calculates magnitude^2 between the estimationed positon and the given one.

        \param when time to use for estimation

        \param pos position to compare against

        \return magnitude^2 value
    */
    double getProximityTo(double when, const Geometry::Vector& pos);
    bool isShouldDropAtTime(double when);

    /** Determine if the track is alive (not initiating and not dropping).

        \return true if so
    */
    bool isAlive() const { return state_ == kAlive; }

    /** Determine if the track is dropping

        \return true if so
    */
    bool isDropping() const { return state_ == kDropping; }

    /** Update the track with a new position report.

        \param when time associated with the new report

        \param pos location to update with
    */
    void updatePosition(double when, const Geometry::Vector& pos);

    /** Send out a TSPI message with the current position.

        \return true if successful
    */
    TrackPoint emitPosition();

    /** Change the track state to kDropping and send out a TSPI message with the current position.
     */
    void drop();
    std::string state();
    bool isUpdate() {return update_;}
    void setUpdate(bool sts) {update_ = sts;}
    void update_me() {update_times++;}
    void not_update_me(){not_update_num++; update_times = 0;}
    void not_update_clear(){not_update_num = 0;}
    int not_update_number() {return not_update_num;}
    int updateTimes() {return update_times;}
    void appendHistory(const Geometry::Vector& vec) {mHistoryPntsList.append(vec);}
    QList<Geometry::Vector> historyPnts() const {return mHistoryPntsList;}
    QString  historyPntsStr() const {
        QStringList content;
        QList<Geometry::Vector> list = historyPnts();
        for(int i=0; i<list.size(); i++)
        {
            content.append(QString("(%1, %2)").arg(list[i].getX(), 0, 'f', 3).arg(list[i].getY(), 0, 'f', 3));
        }

        return content.join(" ");
    }

    Geometry::Vector pos() const {return t0_.position_;}
    void setAzimuth(double val) {azimuth_ = val;}
    double azimuth() const {return azimuth_;}
    State  getState() const {return state_;}

    //目标更新的备份
    void restore() {t0_ = last_round;}
    void save() {last_round = t0_;}
    double last_round_time()const {return last_round.when_;}


private:
    /** Check the track length to see if we have an active track.
     */
    void checkIfInitiated(double when);

    struct Estimate {
        Estimate() : when_(0.0), position_(), velocity_() {}
        double when_;
        Geometry::Vector position_;
        Geometry::Vector velocity_;
    };

    ABTracker& owner_;

    Estimate t0_;
    Geometry::Vector initialPosition_;
    std::string id_;
    std::vector<double> initiationTimeStamps_;

    RadarConfig* cfg_;

    State state_;
    bool update_;
    int  not_update_num;//  when a loop is end, if not updated, ++num.
    int  update_times;
    QList<Geometry::Vector> mHistoryPntsList;
    double      azimuth_;   //当前点的方位

    Estimate last_round;
};

} // end namespace ABTrackerUtils
} // end namespace Algorithms
} // end namespace SideCar

#endif
