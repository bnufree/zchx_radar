#ifndef SEGMENTEDTARGETIMAGE_H
#define SEGMENTEDTARGETIMAGE_H

#include "ImageDataTypes.h"
#include "Row.h"
#include "TargetImage.h"
#include "TargetSize.h"
#include "boost/shared_ptr.hpp"
#include <algorithm>
#include <iomanip>
#include <list>
#include <vector>

class SegmentedTargetImage {
public:
    using Rows = std::vector<Row>;

private:
    // m_currentRow is the last (most recent) row in the image
    // to which data is being appended.  a row is a collection
    // of segments that define where the "region" is non-zero.
    Row m_currentRow;

    // m_image is a vector of rows that contain the image data.
    Rows m_image;

    // m_wasUpdated is true if this image was updated during a pass.
    bool m_wasUpdated;

    // m_mergePending is true if this image is to be merged
    bool m_mergePending;

    // m_minRange, m_maxRange define the min and max range that this
    // target covers.  these could be calculated from m_image on request,
    // but this would be a very expensive operation.
    RANGEBIN m_minRange, m_maxRange;
    bool m_minRangeValid, m_maxRangeValid;

public:
    // Constructor initializes variables to false
    SegmentedTargetImage() :
        m_wasUpdated(false), m_mergePending(false), m_minRangeValid(false), m_maxRangeValid(false)
    {
    }

    // read and modify the merge pending flag
    bool MergePending() { return m_mergePending; }
    void ClearMergePending() { m_mergePending = false; }
    void SetMergePending() { m_mergePending = true; }

    // read and modify the update flag
    bool WasUpdated() { return m_wasUpdated; }
    void ClearUpdated() { m_wasUpdated = false; }
    void SetUpdated() { m_wasUpdated = true; }

    // GetRowCount gets the number of rows in this image
    PRI_COUNT GetRowCount() { return m_image.size(); }

    // Dump output's the image to the logger
    void Dump();

    // GetSize returns this image's size (contsant time)
    TargetSize GetSize();

    // AddDataToLastRow adds a segment to the m_currentRow
    void AddDataToLastRow(RANGEBIN start, RANGEBIN stop);

    // FinailizeRow updates min and max range then pushes the m_currentRow on the m_image
    void FinalizeRow(AZIMUTH az);

    // Merge is a static function that merges all images in a TargetVector "mergers" as
    //  well as a new segment [start, stop].  when images are merged, maps must be updated
    //  so this function also updates currentMap (some of the targets may link to later parts
    //  in the currentMap) and the nextMap (for the next PRI).
    static SegmentedTargetImagePtr Merge(TargetVector& mergers, RANGEBIN start, RANGEBIN stop,
                                         std::vector<SegmentedTargetImagePtr>& currentMap,
                                         std::vector<SegmentedTargetImagePtr>& nextMap);

    // TruncateToOneRow discards all but the last row of this image.  this is handy for
    //  eliminating the clutter targets that grow unbounded.  it is better to trucnate
    //  a clutter image than delete it because deleting a non-closed image is expensive
    //  (it must be removed from the current map) and it will most likely just be recreated
    //  on the next PRI.
    void TruncateToOneRow();

    // converts this segmented image to a TargetImage.  this is a
    // time consuming operation and should be used sparingly.
    BinaryTargetImagePtr MakeBinaryTargetImage();

private:
    // UpdateMinMaxRanges is used internally to constantly update the
    // min and max ranges so that GetSize can be constant time.
    void UpdateMinMaxRanges(RANGEBIN start, RANGEBIN stop);
};

/** \file
 */

#endif
