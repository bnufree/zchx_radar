#include <algorithm> // for std::transform
#include <cmath>     // for floor
#include <deque>
#include <functional> // for std::bind* and std::mem_fun*

#include "Centroid.h"
#include "Messages/BinaryVideo.h"
#include "Messages/RadarConfig.h"
#include "ScanLine.h"
#include "zchxRadarUtils.h"
#include "ExtractWithCentroiding.h"

using namespace ZCHX;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Messages;

ExtractWithCentroiding::ExtractWithCentroiding() :
    m_centroidRangeMin(7.0),
    m_centroidAzMin(0.30),
    m_discardRangeMin(120.0),
    m_discardAzMin(M_PI)
{
    ;
}


void
ExtractWithCentroiding::Binary2Extraction(const Messages::BinaryVideo::Ref& msg, Messages::Extractions::Ref& extractions)
{
    TargetSize minDiscardTargetSize;
    TargetSize minCentroidSize;

    Messages::Video::Ref video(msg->getVideoBasis());

    // setup centroiding and discard sizes
    double minVideoRange = video->getRangeMin();
    double range_factor = video->getRangeFactor();
    //qDebug()<<minVideoRange<<range_factor;
    minCentroidSize.maxRange = (RANGEBIN)((m_centroidRangeMin- minVideoRange) / range_factor);
    minCentroidSize.maxRangeValid = true;
    minCentroidSize.maxAz = (AZIMUTH)(m_centroidAzMin);
    minCentroidSize.maxAzValid = true;

    minDiscardTargetSize.maxRange = (RANGEBIN)((m_discardRangeMin - minVideoRange) / range_factor);
    minDiscardTargetSize.maxRangeValid = true;
    minDiscardTargetSize.maxAz = (AZIMUTH)(m_discardAzMin);
    minDiscardTargetSize.maxAzValid = true;

    //qDebug()<<minCentroidSize<<minDiscardTargetSize;

    // create a vector for extractions
    //Messages::Extractions::Ref extractions;

    // process the binary data to update image segmentation (i.e., target
    // detection)
    m_is.AppendScanLine(BinaryScanLineVector(&(msg->getData())), msg->getAzimuthStart(), minDiscardTargetSize);

    // process the video data (i.e., just store it for later use)
    m_videoHistory.Append(video->getData());

    // process every target that is complete (i.e., ready for extraction and
    // further processing)
    while (!m_is.IsClosedTargetsEmpty()) {
        if (!extractions) extractions = Messages::Extractions::Make("Extract", msg);

        SegmentedTargetImagePtr target = m_is.PopClosedTarget();
        if(!target) continue;
        TargetSize size = target->GetSize();

        if (!(size > minCentroidSize)) { // notice, this is not the same as size < minCentroidSize

            TargetPosition pos = size.Center();
            float range = msg->getRangeAt(pos.range);
            extractions->push_back(Messages::Extraction(QDateTime::currentMSecsSinceEpoch() / 1000.0, range, pos.az, 0.0));
            LOG_FUNC_DBG << "target found at az=" << pos.az << " range=" << range << "km range bin=" << pos.range;
        } else {
            // this target is too large, send to the centroider for
            //  further resolution

            // extract video and mask. the upper left corner of both the video
            // and mask lies at [minRange, minAz].  the lower right lies at
            // [maxRange, maxAz].  the images are (priCount) x (maxRange-minRange+1)
            // pixels.

            LOG_FUNC_DBG << "centroiding a target:" << size << " dRange=" << msg->getRangeAt(size.RangeExtent());

            BinaryTargetImagePtr mask = target->MakeBinaryTargetImage();
            VideoTargetImagePtr video = m_videoHistory.GetWindow(size);
            video->SetAzimuthData(mask->GetAzimuthData());

            // create a centroider
            Centroid centroider(mask, video);

            centroider.Process();

            while (!centroider.IsClosedTargetsEmpty()) {
                TargetPosition cenPos = centroider.PopClosedTargetPosition();

                float cenRange = msg->getRangeAt(cenPos.range);

                extractions->push_back(Messages::Extraction(QDateTime::currentMSecsSinceEpoch() / 1000.0, cenRange, cenPos.az, 0.0));

                LOG_FUNC_DBG << "  centroided target found at az=" << cenPos.az << " range=" << cenRange
                         << "km range bin=" << cenPos.range << endl;
            }
        }
    }

    // discard any un-need video data
    m_videoHistory.SetDepth(m_is.GetMaxRowDepth());
}
