#include "ImageSegmentation.h"
#include "common.h"

ImageSegmentation::ImageSegmentation() : m_maxRowDepth(0)
{
}

ImageSegmentation::~ImageSegmentation()
{
}

void
ImageSegmentation::CloseAllOpenTargets()
{
    m_closedTargets.insert(m_closedTargets.end(), m_openTargets.begin(), m_openTargets.end());
    m_openTargets.clear();
}

bool
ImageSegmentation::IsClosedTargetsEmpty()
{
    return m_closedTargets.empty();
}

SegmentedTargetImagePtr
ImageSegmentation::PopClosedTarget()
{
    if (m_closedTargets.size() <= 0) {
        LOG_FUNC_DBG << "ImageSegmentation::PopClosedTarget : No images to return" << endl;
        return 0;
    }

    SegmentedTargetImagePtr temp;
    temp = m_closedTargets.back();
    m_closedTargets.pop_back();

    return temp;
}

void
ImageSegmentation::Dump()
{
    TargetList::iterator region;
    int count;

    count = 0;
    for (region = m_openTargets.begin(); region != m_openTargets.end(); region++) {
        LOG_FUNC_DBG << "--- Open Target #" << count++ << " ---" << endl;
        (*region)->Dump();
    }

    count = 0;
    for (region = m_closedTargets.begin(); region != m_closedTargets.end(); region++) {
        LOG_FUNC_DBG << "--- Closed Target #" << count++ << " ---" << endl;
        (*region)->Dump();
    }

    LOG_FUNC_DBG << "---most recent PRI range->target mapping---" << endl;
    std::vector<SegmentedTargetImagePtr>::iterator range;
    for (range = m_currentMap.begin(); range != m_currentMap.end(); range++) {
        if ((*range)) {
            count = 0;
            bool found = false;
            for (region = m_openTargets.begin(); region != m_openTargets.end(); region++) {
                count++;
                if ((*range) == (*region)) {
                    LOG_FUNC_DBG << count << ",";
                    found = true;
                    break;
                }
            }
            if (!found) { LOG_FUNC_DBG << "?,"; }
        } else {
            LOG_FUNC_DBG << "N,";
        }
    }
    LOG_FUNC_DBG << endl;
}

void
ImageSegmentation::IdentifyNeighborsAndSetMergePending(RANGEBIN range)
{
    // test the upper-left neighbor/pixel
    if (range > 0) {
        if (m_currentMap[range - 1]) {
            if (!m_currentMap[range - 1]->MergePending()) {
                m_currentMap[range - 1]->SetMergePending();
                m_mergePendingTargets.push_back(m_currentMap[range - 1]);
            }
        }
    }

    // test the upper neighbor/pixel
    if (m_currentMap[range]) {
        if (!m_currentMap[range]->MergePending()) {
            m_currentMap[range]->SetMergePending();
            m_mergePendingTargets.push_back(m_currentMap[range]);
        }
    }

    // test the upper-right neighbor/pixel
    if ((range + 1) < (RANGEBIN)m_currentMap.size()) {
        if (m_currentMap[range + 1]) {
            if (!m_currentMap[range + 1]->MergePending()) {
                m_currentMap[range + 1]->SetMergePending();
                m_mergePendingTargets.push_back(m_currentMap[range + 1]);
            }
        }
    }
}

void
ImageSegmentation::MergePendingTargetsAndSegment(RANGEBIN start, RANGEBIN stop)
{
//    if(stop - start < 2)
//    {
//        m_mergePendingTargets.clear();
//        return;
//    }

    SegmentedTargetImagePtr newTarget;
    switch (m_mergePendingTargets.size()) {
    case 0:
        // the range [start,stop] is associated with no existing Targets, create a new Target
        newTarget = SegmentedTargetImagePtr(new SegmentedTargetImage());
        newTarget->SetUpdated();
        newTarget->AddDataToLastRow(start, stop);
        UpdateRangeToTargetMap(start, stop, m_nextMap, newTarget);
        m_openTargets.push_back(newTarget);
        LOG_FUNC_DBG << "created a new target with range=[" << start << ", " << stop << "]"<<newTarget.get();
        break;
    case 1:
        // the range [start,stop] is associated with only a single Target, simply add the
        // range to the existing Target
        newTarget = m_mergePendingTargets.front();
        newTarget->SetUpdated();
        newTarget->ClearMergePending();
        LOG_FUNC_DBG << "update exist target"<<newTarget.get()<<"with range=[" << newTarget.get()->GetSize().minRange << ", " << newTarget.get()->GetSize().maxRange << "]";
        newTarget->AddDataToLastRow(start, stop);
        UpdateRangeToTargetMap(start, stop, m_nextMap, newTarget);
        LOG_FUNC_DBG << "updated exist target with range=[" << start << ", " << stop << "]"<<newTarget.get()<<" to be merged next";
        break;
    default:
        // the range was associated with 2 or more Targets, merge the Targets and the range
        // to create a new Target.

        // remove all Targets in m_mergePendingTargets from m_openTargets. each "*ri" can only
        //  occur once in m_openTargets, so this is faster than using std::remove)
        LOG_FUNC_DBG << "merging Targets (";
        TargetVector::iterator ri;
        for (ri = m_mergePendingTargets.begin(); ri != m_mergePendingTargets.end(); ri++) {
            TargetList::iterator rj;
            int count = 0;
            for (rj = m_openTargets.begin(); rj != m_openTargets.end(); rj++) {
                count++;
                if ((*ri) == (*rj)) {
                    // LOG_FUNC_DBG << count-1 << ",";
                    m_openTargets.erase(rj);
                    break;
                }
            }
        }
        LOG_FUNC_DBG << ") and range [" << start << ", " << stop << "]" ;

        // merge all Targets in m_mergePendingTargets with [start:stop] into newTarget
        //  this will also update the m_currentMap vector
        newTarget = SegmentedTargetImage::Merge(m_mergePendingTargets, start, stop, m_currentMap, m_nextMap);

        // newTarget->Dump();

        newTarget->ClearMergePending();
        newTarget->SetUpdated();
        m_openTargets.push_back(newTarget);
        // LOG_FUNC_DBG << "merge done" << endl;
        break;
    }

    m_mergePendingTargets.clear();
}

void
ImageSegmentation::AppendScanLine(const BinaryScanLine& binLine, AZIMUTH az, TargetSize discardSize /*= TargetSize()*/)
{
    // assumes that binLine[i] corresponds to the i-th range bin

    if ((RANGEBIN)m_currentMap.size() < binLine.size()) {
        // resize the map to match the number of range bins, newly created entries will
        //  be "NULL" smart pointers, indicating that the range is not associated with
        //  a target
        m_currentMap.resize(binLine.size());
        m_nextMap.resize(binLine.size());
    }

    // empty the mappings for the next pri
    std::fill(m_nextMap.begin(), m_nextMap.end(), SegmentedTargetImagePtr());

    bool lastBin = false;
    RANGEBIN startOfTarget = 0;
    for (RANGEBIN range = 0; range < (RANGEBIN)binLine.size(); range++) {
        if (binLine[range] && !lastBin) {
            // the start of a Target
            //qDebug() << "starting a Target at az=" << az << " range=" << range;
            IdentifyNeighborsAndSetMergePending(range);
            startOfTarget = range;
            lastBin = true;
        } else if (binLine[range] && lastBin) {
            // in the middle of a Target
            IdentifyNeighborsAndSetMergePending(range);
        } else if (!binLine[range] && lastBin) {
            // end of a Target
            //qDebug() << "ending a Target at az=" << az << " range=" << range-1;
            MergePendingTargetsAndSegment(startOfTarget, range - 1);
            lastBin = false;
        }
    }

    if (lastBin) {
        // the row ended with an open Target
        //qDebug() << "ending a Target at az=" << az << " range=" << ((RANGEBIN)binLine.size())-1;
        MergePendingTargetsAndSegment(startOfTarget, ((RANGEBIN)binLine.size()) - 1);
    }

    m_maxRowDepth = 0;

    TargetList::iterator region;
    for (region = m_openTargets.begin(); region != m_openTargets.end();) {
        SegmentedTargetImagePtr ptr = *region;
        if (ptr->WasUpdated()) {
            ptr->FinalizeRow(az);

            ptr->ClearUpdated();

            if (ptr->GetSize() > discardSize) {
                // this Target has violated some constraint. we could remove it completely,
                //  but that would require updating m_currentMap, which could be costly.
                //  further, since the Target isn't acutally closed, it will probably just
                //  be recreated on the next scan line, so deleting it would just be a
                //  waste. instead, truncate it to one row to conserve memory and keep
                //  going.
                TargetSize size = ptr->GetSize();
                LOG_FUNC_DBG << "truncating a Target rangebin=[" << size.minRange << ", " << size.maxRange << "] az=["
                         << size.minAz << ", " << size.maxAz << "]";
                ptr->TruncateToOneRow();
            }

            // determine the most number of rows any target has
            //qDebug()<<"target:"<<ptr.get()<<"depth:"<<ptr->GetRowCount();
            m_maxRowDepth = std::max(m_maxRowDepth, ptr->GetRowCount());

            region++;

        } else {
            //if(ptr->GetSize().RangeExtent() >= 2)
            {
                m_closedTargets.push_back(*region);
            }

            region = m_openTargets.erase(region);
        }
    }

    // make the next map the new map for the next PRI
    m_nextMap.swap(m_currentMap);
}
