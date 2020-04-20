#ifndef CENTROID_H
#define CENTROID_H

#include "ImageDataTypes.h"
#include "ImageSegmentation.h"

class Centroid {
private:
    // m_mask is the binary mask of the image.
    BinaryTargetImagePtr m_mask;

    // m_video is the video data used to find the centroid of the target
    VideoTargetImagePtr m_video;

    // m_is is the image segmentation member used to find the center of the
    // peak regions
    ImageSegmentation m_is;

    // m_useOriginalTarget is true if no regions were found as a result of
    //  centroiding (this is a strange target if that is the case).  if
    //  true, the original target is simply used.
    bool m_useOriginalTarget;

public:
    // Constructor initializes members
    Centroid(BinaryTargetImagePtr mask, VideoTargetImagePtr video);

    // Process centroids the image
    void Process();

    // IsClosedTargetsEmpty returns true if no targets (or, in this case,
    // "sub-targets") are left
    bool IsClosedTargetsEmpty();

    // PopClosedTargetPosition pops a closed target off the stack and
    // returns its position.  the centroider does not determine a size
    // of the new target, just its center.  NB: the matlab centroid2.m
    // does calculate the size of the target.  however, this requires
    // a x and y second derivatives, another image segmentation, and
    // then a center-to-second derivative mapping.  all this is may be
    // time consuming and, regardless, is not used in the current implmentation.
    TargetPosition PopClosedTargetPosition();
    bool           PopClosedTargetPositionWithCheck(TargetPosition& pos);
};

/** \file
 */

#endif
