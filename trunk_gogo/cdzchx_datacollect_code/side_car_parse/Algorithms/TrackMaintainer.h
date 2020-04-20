#ifndef SIDECAR_ALGORITHMS_TRACKMAINTAINER_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_TRACKMAINTAINER_H

#include <vector>

#include "Messages/Track.h"

namespace ZCHX {
namespace Algorithms {

/** Documentation for the algorithm TrackMaintainer This task simply receives track messages and passes them
    along.
*/
class TrackMaintainer {
    using TrackMsgVector = std::vector<Messages::Track::Ref>;

    /** A map, indexed by track number, that contains the most recent track report for this track number that
        has come in
    */
    using Mapping = std::map<int, TrackMsgVector>;

public:
    enum InfoSlots { kEnabled = /*ControllerStatus::kNumSlots*/1, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    TrackMaintainer();

    /** Implementation of the Algorithm::startup interface. Register runtime parameters and data processors.

        \return true if successful, false otherwise
    */
    //bool startup();

    /** Implementation of the Algorithm::shutdown interface. Dispose of any resources (such as memory) allocated
        from within the startup() method.

        \return true if successful, false otherwise
    */
    //bool shutdown();

    bool reset();

    void processAlarm();

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    //void setInfoSlots(IO::StatusBase& status);

    /** Process messages from channel

        \param msg the input message to process

        \returns true if no error; false otherwise
    */
    bool processInput(const Messages::Track::Ref& msg);

    /** Gets called whenever a track that has been corrected by a measurement comes in
     */
    void updateDatabase(const Messages::Track::Ref& msg);

    /** Gets called by alarm to check for tracks that should be dropped or promoted.
     */
    void checkDatabase();

    // Add attributes here
    //
    bool enabled_;
    uint hitsBeforePromote_;
    uint missesBeforeDrop_;

    /** This is the differential between the current computer time and the time reported in the extractions. For
        real-time data, this should be negligible. For pre-canned data that is being played back though the
        system, this needs to be kept track of.
    */
    double epoch_;
    Mapping trackDatabase_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
