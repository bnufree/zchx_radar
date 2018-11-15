#ifndef IMAGEDATATYPES_H
#define IMAGEDATATYPES_H

#include "boost/shared_ptr.hpp"
#include <cmath>
#include <list>
#include <vector>

#include "Messages/BinaryVideo.h"
#include "Messages/Video.h"

#define VERIFY_SEGMENTS
#define VERIFY_ROW

using RANGEBIN = int;
using PRI_COUNT = int;
using AZIMUTH = float;
using VIDEODATA = ZCHX::Messages::Video::DatumType;
using BINARYDATA = ZCHX::Messages::BinaryVideo::DatumType;

class SegmentedTargetImage;

using SegmentedTargetImagePtr = boost::shared_ptr<SegmentedTargetImage>;
using TargetList = std::list<SegmentedTargetImagePtr>;     // use for faster random deletes
using TargetVector = std::vector<SegmentedTargetImagePtr>; // use for faster size() operations

#define AZIMUTH_EXTENT(min, max) (((min) <= (max)) ? (max - min) : (max + ((AZIMUTH)2) * M_PI - min))

/** \file
 */

#endif
