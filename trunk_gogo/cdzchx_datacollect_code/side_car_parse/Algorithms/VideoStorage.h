#ifndef VIDEOSTORAGE_H
#define VIDEOSTORAGE_H

#include "ImageDataTypes.h"
#include "ScanLine.h"
#include "TargetImage.h"
#include <deque>
#include "common.h"

// VideoStorage is a simple deque encapsulation used to store all video
// data.  Video data must be stored until the longest (in pri counts)
// target has been released (i.e., closed). When the target is closed,
// the code will request the window of video data that the target overlaps
// and merge the two for centroiding. Note that we cannot know what video
// data a target will need before it is closed.  This is because we want
// all video data, even where the mask is not defined and because target
// regions can merge in unpredictable ways. when a target is released,
// the calling code will recalculate how many video pri's need to be stored
// and then call SetDepth.  SetDepth must always be less than the current
// depth.  any pri's greater than the new depth will be released (so that
// we don't end up storing the video history since the beginning of time).
class VideoStorage {
public:
    VideoStorage() {}

    void Append(VideoScanLineVector& vid)
    {
        // the front of the deque has the most recent scan
        m_video.push_front(vid);
    }

    void SetDepth(PRI_COUNT maxNeededDepth)
    {
        // remove the least recent scans
        m_video.resize(maxNeededDepth);
    }

    PRI_COUNT GetDepth() { return m_video.size(); }

    void Clear() { m_video.clear(); }

    VideoScanLineVector* GetPRI(PRI_COUNT pri)
    {
        if (pri >= GetDepth()) {
            LOG_FUNC_DBG << "VideoStorage::GetWindow : pri too large" ;
            //abort();
        }
        return &(m_video[pri]);
    }

    // GetWindow - return the requested window of video data
    VideoTargetImagePtr GetWindow(const TargetSize& size)
    {
        if (!size.priValid) {
            LOG_FUNC_DBG << "VideoStorage::GetWindow : unable to extract video data";
            //abort();
        }

        if ((PRI_COUNT)m_video.size() < size.priCount) {
            LOG_FUNC_DBG << "VideoStorage::GetWindow : too few lines in history to extract video";
            //abort();
        }

        RANGEBIN rows = size.priCount;
        PRI_COUNT cols = size.RangeExtent();
        VIDEODATA* rawVid = new VIDEODATA[rows * cols];

        VIDEODATA* rawLine = rawVid;
        for (int i = size.priCount - 1; i >= 0; i--) {
            // src = m_video[i] from range bins [size.minRange, size.maxRange]
            // dest = last row in video image
            for (RANGEBIN r = size.minRange; r <= size.maxRange; r++) { rawLine[r - size.minRange] = m_video[i][r]; }
            rawLine += cols;
        }

        VideoTargetImagePtr video(new VideoTargetImage(size.minRange, size.maxRange, rows, rawVid));

        return video;
    }

private:
    std::deque<VideoScanLineVector> m_video;
};

/** \file
 */

#endif
