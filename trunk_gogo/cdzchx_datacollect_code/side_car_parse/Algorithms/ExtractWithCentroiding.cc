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

//m_centroidRangeMin(Parameter::DoubleValue::Make("centroidRangeMin", "Centroid Range Min (km)", 7.0)),
//m_centroidAzMin(Parameter::DoubleValue::Make("centroidAzMin", "Centroid Az Min (rad)", 0.30)),
//m_discardRangeMin(Parameter::DoubleValue::Make("discardRangeMin", "Discard Range Min (km)", 120.0)),
//m_discardAzMin(Parameter::DoubleValue::Make("discardAzMin", "Discard Az Min (rad)", M_PI))

ExtractWithCentroiding::ExtractWithCentroiding(Messages::RadarConfig* cfg) : mCfg(cfg)
//    m_centroidRangeMin(700),
//    m_centroidAzMin(0.30),
//    m_discardRangeMin(12000),
//    m_discardAzMin(M_PI)
{
    ;
}


void
ExtractWithCentroiding::Binary2Extraction(const Messages::BinaryVideo::Ref& msg, Messages::Extractions::Ref& extractions)
{
    //qDebug()<<"Binary2Extraction start !!!!!!!!!!! ";
    TargetSize minDiscardTargetSize;
    TargetSize minCentroidSize;

    Messages::Video::Ref video(msg->getVideoBasis());
    minCentroidSize.maxRange = (RANGEBIN)((mCfg->getExtractionMaxBin() - mCfg->getRangeMin()) / mCfg->getRangeFactor()) -1;
    minCentroidSize.maxRangeValid = true;
    minCentroidSize.maxAz = (AZIMUTH)(mCfg->getExtractionMaxAmz());
    minCentroidSize.maxAzValid = true;

    minDiscardTargetSize.maxRange = (RANGEBIN)((mCfg->getDiscardExtractionMaxBin() - mCfg->getRangeMin()) / mCfg->getRangeFactor()) - 1;
    minDiscardTargetSize.maxRangeValid = true;
    minDiscardTargetSize.maxAz = (AZIMUTH)(mCfg->getDiscardExtractionMaxAmz());
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
        //size.debugNow();
        if (!(size > minCentroidSize)) { // notice, this is not the same as size < minCentroidSize
            //qDebug()<<"cmp max_range:"<<minCentroidSize.maxRange<<" max_azm:"<<minCentroidSize.maxAz;
            //qDebug()<<"Target max_range:"<<size.maxRange<<" min_range:"<<size.minRange<<"range_extent:"<<size.RangeExtent()<<" min_azm:"<<size.minAz<<" max_azm:"<<size.maxAz<<" azm_extent:"<<size.AzmExtent();
//            if(size.RangeExtent() >= 2)
//            {
                TargetPosition pos = size.Center();
                float range = msg->getRangeAt(pos.range) * 1000;
                extractions->push_back(Messages::Extraction(QDateTime::currentMSecsSinceEpoch() / 1000.0, range, pos.az, 0.0));
//                qDebug() << "target found at az=" << pos.az << " range=" << range << "m range bin=" << pos.range<< "with size:"<<size.RangeExtent();
//            }
        } else {
#if 1
            // this target is too large, send to the centroider for
            //  further resolution

            // extract video and mask. the upper left corner of both the video
            // and mask lies at [minRange, minAz].  the lower right lies at
            // [maxRange, maxAz].  the images are (priCount) x (maxRange-minRange+1)
            // pixels.

            //qDebug() << "centroiding a target:" << size << " dRange=" << msg->getRangeAt(size.RangeExtent());

            BinaryTargetImagePtr mask = target->MakeBinaryTargetImage();
            VideoTargetImagePtr video = m_videoHistory.GetWindow(size);
            video->SetAzimuthData(mask->GetAzimuthData());

            // create a centroider
            Centroid centroider(mask, video);

            centroider.Process();

            while (!centroider.IsClosedTargetsEmpty()) {
                //TargetPosition cenPos = centroider.PopClosedTargetPosition();
                TargetPosition cenPos;
                if(!centroider.PopClosedTargetPositionWithCheck(cenPos)) continue;

                float cenRange = msg->getRangeAt(cenPos.range) * 1000;

                extractions->push_back(Messages::Extraction(QDateTime::currentMSecsSinceEpoch() / 1000.0, cenRange, cenPos.az, 0.0));

//                qDebug() << "  centroided target found at az=" << cenPos.az << " range=" << cenRange << "m range bin=" << cenPos.range<< "with size:"<<size.RangeExtent();
            }
#endif
        }
    }

    // discard any un-need video data
    //qDebug()<<"history_data_size:"<<m_videoHistory.GetDepth()<<m_is.GetMaxRowDepth();
    m_videoHistory.SetDepth(m_is.GetMaxRowDepth());
    //qDebug()<<"history_data_size:"<<m_videoHistory.GetDepth()<<m_is.getOpenTargetSize()<<m_is.getClosedTargetSize()<<m_is.getMergePendingSize();
    //qDebug()<<"Binary2Extraction end !!!!!!!!!!! ";
}
