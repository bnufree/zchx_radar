#ifndef SIDECAR_ALGORITHMS_TRACK_INITIATOR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_TRACK_INITIATOR_H

#include <cmath>
#include <list>
#include <time.h> // for time_t and friends
#include <vector>

#include "Messages/Extraction.h"
#include "Messages/Track.h"

namespace ZCHX {
namespace Algorithms {

/**
   \ingroup Algorithms
*/
class TrackInitiator {
public:
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    TrackInitiator();

    /** Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    bool reset();

private:
    void init();

    /**

       \param mgr object containing the encoded or native data to process

       \return true if successful, false otherwise
    */
    bool process(const Messages::Extractions::Ref& msg);

    using Extraction = ZCHX::Messages::Extraction;

    struct Data {
        Data(const Messages::Extraction& extraction) :
            when_(extraction.getWhen()), extraction_(extraction), velocity_()
        {
        }
        double when_;
        Messages::Extraction extraction_;
        Messages::Track::Coord velocity_;
    };

    using Entry = std::list<Data>;
    std::vector<std::vector<Entry>> buffer_; // [x][y]
    size_t indexOffset_;

    void corrCell(Data&, Entry& candidates);
    void corr(Data&);

    double rMax_;
    size_t numBins_;

    double param_searchRadius;

    void on_searchRadius_changed(const double&);

    int param_scanTime;

    void on_scanTime_changed(const int&);

    int param_numScans;

    float searchRadius_;  // the maximum gating size for correlation to occur
    float searchRadius2_; // = searchRadius^2

    double param_assumedAltitude;
    double param_minRange;

    double t_new_, t_old_, t_veryold_;

    int currentTrackNum_; // ELY,G
};

} // namespace Algorithms
} // namespace SideCar

/** \file
 */

#endif
