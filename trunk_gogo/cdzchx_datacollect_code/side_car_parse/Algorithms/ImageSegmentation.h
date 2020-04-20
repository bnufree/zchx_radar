#ifndef IMAGESEGMENTATION_H
#define IMAGESEGMENTATION_H

#include "ScanLine.h"
#include "SegmentedTargetImage.h"
#include <list>

// ImageSegmentation class is used to peform image segmentation
//  (i.e., blob detection) on a binary image.
class ImageSegmentation {
public:
    using RangeToTargetMap = std::vector<SegmentedTargetImagePtr>;

    ImageSegmentation();
    virtual ~ImageSegmentation();

    void AppendScanLine(const BinaryScanLine& binLine, AZIMUTH az, TargetSize discardSize = TargetSize());

    void Dump();

    bool IsClosedTargetsEmpty();
    void CloseAllOpenTargets();
    SegmentedTargetImagePtr PopClosedTarget();
    PRI_COUNT GetMaxRowDepth() { return m_maxRowDepth; }
    int getOpenTargetSize() {return m_openTargets.size();}
    int getClosedTargetSize() {return m_closedTargets.size();}
    int getMergePendingSize() {return m_mergePendingTargets.size();}

protected:
    // m_openTargets contains a list of all Targets that have not
    //   been terminated (i.e., closed).  Targets in m_openTargets
    //   may still grow and should not be processed by any external code.
    //   this member is a list instead of a vector because random erases will be needed.
    TargetList m_openTargets;

    // m_closedTargets contains Targets that are complete and ready to
    // be processed by external code.
    TargetList m_closedTargets;

    // m_mergePendingTargets is a vector which holds temporary Targets that need
    // to be merged.
    TargetVector m_mergePendingTargets;

    // m_currentMap is a vector the size of the longest PRI that provides a mapping from
    //  range to target images.  these target images are the targets of the most recent
    //  PRI that are open.
    RangeToTargetMap m_currentMap;

    // m_nextMap is a vector the size of the longest PRI that provides the next-PRI mapping
    //  from range to target images.
    RangeToTargetMap m_nextMap;

    // m_maxRowDepth is the number of PRI's of the largest openTarget.  This is used to
    // determine how much video history must be maintained.
    PRI_COUNT m_maxRowDepth;

    // IdentifyNeighborsAndSetMergePending uses 8-connectivity to determine
    // what targets are adjacent to the specified range bin if it were set.
    // any such targets are set as "merge pending"
    void IdentifyNeighborsAndSetMergePending(RANGEBIN range);

    // MergePendingTargetsAndScanLine merges all targets that have "merge pending"
    //  set along with a scan line.
    void MergePendingTargetsAndSegment(RANGEBIN start, RANGEBIN stop);
};

/** \file
 */

#endif
