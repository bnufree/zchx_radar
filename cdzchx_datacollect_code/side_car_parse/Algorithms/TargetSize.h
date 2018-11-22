#ifndef TARGETSIZE_H
#define TARGETSIZE_H

#include "ImageDataTypes.h"
#include <QDebug>

// TargetPosition is used to store the (range, az) position of a target (usually the center
// of the target)
struct TargetPosition {
    RANGEBIN range;
    AZIMUTH az;
};

// TargetSize is used to calculate the extents (a rectange) in {range, az} of a target.
//  this class is also used for comparison purposes (to classify targets based on size).
//  data members are only valid if their corresponding "*Valid" members are true
class TargetSize {
public:
    // the target extends [minRange, maxRange] (inclusive).
    RANGEBIN minRange;
    RANGEBIN maxRange;
    bool maxRangeValid;
    bool minRangeValid;

    // the target extends [minAz, maxAz] (inclusive)
    AZIMUTH minAz;
    AZIMUTH maxAz;
    bool maxAzValid;
    bool minAzValid;

    // the target has priCount "rows" or periods. this number cannot
    //  necessarily be determined from the min/max Az because PRI spacing
    //  may not be regular.
    PRI_COUNT priCount;
    bool priValid;

    // constructor initializes all members to invalid
    TargetSize() : maxRangeValid(false), minRangeValid(false), maxAzValid(false), minAzValid(false), priValid(false) {}

    bool operator<(const TargetSize& rhs)
    {
        // return true if this-size < rhs-size in any {range, az, pri} dimension
        return (CompareSize(rhs) < 0);
    }

    bool operator>(const TargetSize& rhs)
    {
        // return true if this-size > rhs-size in any {range, az, pri} dimension
        return (CompareSize(rhs) > 0);
    }

    char CompareSize(TargetSize cmp)
    {
        // return +1 if this-size > rhs-size in any {range, az, pri} dimension
        // return -1 if this-size < rhs-size in any {range, az, pri} dimension
        // return 0 otherwise (equal condition)

        RANGEBIN myrange = RangeExtent();
        AZIMUTH myaz = AZIMUTH_EXTENT(minAz, maxAz);

        if (cmp.maxRangeValid && (cmp.maxRange < myrange)) return 1;
        if (cmp.minRangeValid && (cmp.minRange > myrange)) return -1;
        if (cmp.maxAzValid && (cmp.maxAz < myaz)) return 1;
        if (cmp.minAzValid && (cmp.minAz > myaz)) return -1;
        if (cmp.priValid && (cmp.priCount < priCount)) return 1;
        if (cmp.priValid && (cmp.priCount > priCount)) return -1;

        return 0;
    }

    TargetPosition Center() const
    {
        // determine the geometric center of the target's size
        TargetPosition pos;
        pos.az = (minAz + maxAz) / (AZIMUTH)2;
        pos.range = (minRange + maxRange) / (RANGEBIN)2;
        return pos;
    }

    RANGEBIN RangeExtent() const
    {
        // determine the range of the garget
        return (maxRange - minRange + 1);
    }
};

std::ostream& operator<<(std::ostream& os, const TargetSize& size);
QDebug operator<<(QDebug debug, const TargetSize& size);

/** \file
 */

#endif
