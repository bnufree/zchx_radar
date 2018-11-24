#ifndef SIDECAR_ALGORITHMS_EXTRACTWITHCENTROIDING_H // -*- C++ -*-
#define SIDECAR_ALGORITHMS_EXTRACTWITHCENTROIDING_H

#include "boost/shared_ptr.hpp"
#include "ImageSegmentation.h"
#include "Messages/BinaryVideo.h"
#include "VideoStorage.h"
#include "Messages/Extraction.h"

namespace ZCHX {
namespace Algorithms {

class ExtractWithCentroiding
{
public:
    ExtractWithCentroiding();
    void Binary2Extraction(const Messages::BinaryVideo::Ref& msg, Messages::Extractions::Ref& out);
private:

    ImageSegmentation m_is;
    VideoStorage m_videoHistory;

    double m_centroidRangeMin;
    double m_centroidAzMin;
    double m_discardRangeMin;
    double m_discardAzMin;
};

} // end namespace Algorithms
} // end namespace SideCar

/** \file
 */

#endif
