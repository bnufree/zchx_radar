#ifndef SIDECAR_ALGORITHMS_THRESHOLD_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_THRESHOLD_H

#include "Messages/Video.h"

namespace ZCHX {
namespace Algorithms {

/** This algorithm is a dummy thresholding algorithm for temporary use until the proper one is finished. It
    outputs a binary PRI message with gates set to true or false depending on whether corresponding video values
    are above ore below a threshold value.
*/
class Threshold {
public:
    using DatumType = Messages::Video::DatumType;

    enum InfoSlot { kThreshold = /*ControllerStatus::kNumSlots*/1, kNumSlots };

    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    Threshold(int hold_val);

    /** Implementation of the Algorithm::startup interface. Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    //bool startup();
    bool process(const Messages::Video::Ref& msg, Messages::BinaryVideo::Ref& out);

private:
    size_t getNumInfoSlots() const { return kNumSlots; }

    /** Override of Algorithm::setInfoSlots(). Stores XML representation of cancellation statistics ino the
        given XML-RPC container.

        \param status XML-RPC container to hold the stats
    */
    //void setInfoSlots(IO::StatusBase& status);

    /** Message processor for Video data.

        \param in Video message to process

        \return true if successful, false otherwise
    */


    /** Notifiation that the threshold value has been changed by an external entitiy.

        \param value new value
    */
    void thresholdChanged(const DatumType& value);

    int threshold_;
    DatumType thresholdValue_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
