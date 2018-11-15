#ifndef ROW_H
#define ROW_H

#include "ImageDataTypes.h"

// Segment class defines a portion of a scan line (PRI) where
// the pixels are "true" (i.e., a target is present).  A row
// may consist of multiple segments.  A Row of Segments is similar
// to a run-length-encoded PRI.
class Segment {
public:
    RANGEBIN start;
    RANGEBIN stop;
    Segment(RANGEBIN b, RANGEBIN e) : start(b), stop(e) {}
};

// less than operator is used for sorting segments (not actually used
//  except during debugging)
bool operator<(const Segment& lhs, const Segment& rhs);

// UpdateRangeToTargetMap is used to set the range-to-target mapping
// for a specified segment
void UpdateRangeToTargetMap(RANGEBIN start, RANGEBIN stop, std::vector<SegmentedTargetImagePtr>& rangeToTargetMap,
                            SegmentedTargetImagePtr newptr);

// Row class is a vector of segments that define the portions of a PRI where
// a target is present (i.e., the value is non-zero)
class Row {
public:
    // az is the azimuth of this row
    AZIMUTH az;

private:
    // mask is a vector of segments.  generally, segments are NOT sorted by range.
    std::vector<Segment> mask;

    // minRange and maxRange store the minimum and maximum range bins were
    //  a segment was found.  these could be calculated from mask, but
    //  that would be a costly operation.
    RANGEBIN minRange, maxRange;
    bool minRangeValid, maxRangeValid;

public:
    // Constructor initializes all valid fields to false
    Row() : minRangeValid(false), maxRangeValid(false) {}

    // AddMask pushes a segment on the mask vector. also, updates min and maxRange
    void AddMask(Segment seg);

    // returns minRange
    RANGEBIN GetFirstRangeBin();

    // returns maxRange
    RANGEBIN GetLastRangeBin();

    // ResetMask removes all segments from the row
    void ResetMask();

    // Empty returns true if there are no segments in this row
    bool Empty();

    // Sort sorts the segments.  this is a costly operation and not needed
    //  except during debugging.
    void Sort();

    // Merge combines the segments from two rows (since the mask is not sorted, the vectors
    //  are just combined.  the rangeToTargetMap is also updated.
    void Merge(Row sRow, std::vector<SegmentedTargetImagePtr>* rangeToTargetMap = NULL,
               SegmentedTargetImagePtr* newptr = NULL);

    // DumpSorted prints the image to the log.  the mask must be sorted before calling
    //  this function using Sort()
    void DumpSorted();

    // FillArray fills the provided array of size (stopRange-startRange+1) with data from
    //  all the segments.  the startRange and stopRange MUST be large enough to include all
    //  the segments from this row.  This function is used to convert the associate segmented image to
    //  an array
    void FillArray(BINARYDATA* dest, RANGEBIN startRange, RANGEBIN stopRange);
};

/** \file
 */

#endif
