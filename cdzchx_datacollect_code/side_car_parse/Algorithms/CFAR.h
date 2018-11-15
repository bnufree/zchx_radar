#ifndef SIDECAR_ALGORITHMS_CFAR_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_CFAR_H

#include "Algorithms/PastBuffer.h"
#include "Messages/Video.h"

namespace ZCHX {
namespace Algorithms {

class CFAR {
public:
    CFAR();

    bool reset();

private:
    bool processEstimate(const Messages::Video::Ref& msg);

    bool processVideo(const Messages::Video::Ref& msg);

    double alpha_;
    PastBuffer<Messages::Video> videoBuffer_;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
