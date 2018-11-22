#include "SegmentedTargetImage.h"

void
SegmentedTargetImage::Dump()
{
    int rowCount = m_image.size() - 1;

    for (Rows::iterator i = m_image.begin(); i != m_image.end(); i++) {
        // loop through each row
        (*i).Sort();
        LOG_FUNC_DBG << "std::setw(3)" << rowCount-- << ":";
        for (RANGEBIN r = 0; r < (*i).GetFirstRangeBin(); r++) LOG_FUNC_DBG << " ";
        (*i).DumpSorted();
        LOG_FUNC_DBG << endl;
    }
    LOG_FUNC_DBG << "cur:";
    if (!m_currentRow.Empty()) {
        for (RANGEBIN r = 0; r < m_currentRow.GetFirstRangeBin(); r++) LOG_FUNC_DBG << " ";
        m_currentRow.DumpSorted();
    }
    LOG_FUNC_DBG << endl;
}

TargetSize
SegmentedTargetImage::GetSize()
{
    // gets the size of the image, does not consider the m_currentRow
    TargetSize size;
    size.minRange = m_minRange;
    size.maxRange = m_maxRange;
    size.minAz = m_image.front().az;
    size.maxAz = m_image.back().az;
    size.priCount = (PRI_COUNT)m_image.size();
    size.maxRangeValid = true;
    size.minRangeValid = true;
    size.minAzValid = true;
    size.maxAzValid = true;
    size.priValid = true;
    return size;
}

void
SegmentedTargetImage::UpdateMinMaxRanges(RANGEBIN start, RANGEBIN stop)
{
    if (!m_maxRangeValid || (stop > m_maxRange)) {
        m_maxRangeValid = true;
        m_maxRange = stop;
    }
    if (!m_minRangeValid || (start < m_minRange)) {
        m_minRangeValid = true;
        m_minRange = start;
    }
    LOG_FUNC_DBG<<this<<"range["<<m_minRange<<", "<<m_maxRange<<"]";
}

void
SegmentedTargetImage::AddDataToLastRow(RANGEBIN start, RANGEBIN stop)
{
    // LOG_FUNC_DBG << "SegmentedTargetImage::AddDataToLastRow : start=" << start << " stop=" << stop << std::end;
    m_currentRow.AddMask(Segment(start, stop));
#ifdef VERIFY_SEGMENTS
    if (stop < start) {
        LOG_FUNC_DBG << "TargetImage::AddDataToLastRow : attempted to add an invalid segment" << endl;
        //abort();
    }
#endif
    // LOG_FUNC_DBG << "SegmentedTargetImage::AddDataToLastRow : exit" << std::end;
}

void
SegmentedTargetImage::FinalizeRow(AZIMUTH az)
{
    // LOG_FUNC_DBG << "SegmentedTargetImage::FinalizeRow : az=" << az << std::end;
    m_currentRow.az = az;
    m_image.push_back(m_currentRow);
    UpdateMinMaxRanges(m_currentRow.GetFirstRangeBin(), m_currentRow.GetLastRangeBin());
    m_currentRow.ResetMask();
    // LOG_FUNC_DBG << "SegmentedTargetImage::FinalizeRow : exit" << az << std::end;
}

SegmentedTargetImagePtr
SegmentedTargetImage::Merge(TargetVector& mergers, RANGEBIN start, RANGEBIN stop,
                            std::vector<SegmentedTargetImagePtr>& currentMap,
                            std::vector<SegmentedTargetImagePtr>& nextMap)
{
    // find the image with the largest PRI count, this will be the new image
    SegmentedTargetImagePtr masterIm = mergers[0];
    for (TargetVector::size_type i = 1; i < mergers.size(); i++) {
        if (mergers[i]->m_image.size() > masterIm->m_image.size()) { masterIm = mergers[i]; }
    }

    int minRange = masterIm.get()->GetSize().minRange;
    int maxRange = masterIm.get()->GetSize().maxRange;

    // starting at the bottom of the images, merge the rows
    for (TargetVector::size_type i = 0; i < mergers.size(); i++) {
        if (mergers[i] != masterIm) {
            int destRowOffset = masterIm->m_image.size() - mergers[i]->m_image.size();
            for (TargetVector::size_type srcRow = 0; srcRow < mergers[i]->m_image.size(); srcRow++) {
                // merge dest row (srcRow+destRowOffset) with src row (srcRow)
#ifdef VERIFY_SEGMENTS
                if (masterIm->m_image[srcRow + destRowOffset].az != mergers[i]->m_image[srcRow].az) {
                    LOG_FUNC_DBG<< "SegmentedTargetImage::Merge : tried to merge incompatible rows (az)" << endl;
                    //abort();
                }
#endif
                if ((srcRow + 1) == mergers[i]->m_image.size()) {
                    // if this is the last row, we need to update the references in the current map
                    masterIm->m_image[srcRow + destRowOffset].Merge(mergers[i]->m_image[srcRow], &currentMap,
                                                                    &masterIm);
                } else {
                    masterIm->m_image[srcRow + destRowOffset].Merge(mergers[i]->m_image[srcRow]);
                }

            }
            if(minRange > mergers[i]->GetSize().minRange) minRange = mergers[i]->GetSize().minRange;
            if(maxRange < mergers[i]->GetSize().maxRange) maxRange = mergers[i]->GetSize().maxRange;
            // if this is the "next" row, then update the references in teh nextMap
            masterIm->m_currentRow.Merge(mergers[i]->m_currentRow, &nextMap, &masterIm);
        }
    }

//    masterIm->AddDataToLastRow(start, stop);
//    UpdateRangeToTargetMap(start, stop, nextMap, masterIm);
    masterIm->AddDataToLastRow(minRange, maxRange);
    UpdateRangeToTargetMap(minRange, maxRange, nextMap, masterIm);
    return masterIm;
}

BinaryTargetImagePtr
SegmentedTargetImage::MakeBinaryTargetImage()
{
    RANGEBIN cols = m_maxRange - m_minRange + 1;
    RANGEBIN rows = GetRowCount();

    BINARYDATA* data = new BINARYDATA[rows * cols];

    AzimuthDataPtr az(new AzimuthData());
    az->reserve(m_image.size());

    PRI_COUNT rowCount = 0;
    for (Rows::iterator i = m_image.begin(); i != m_image.end(); i++) {
        (*i).FillArray(data + (rowCount * cols), m_minRange, m_maxRange);
        az->push_back((*i).az);
        rowCount++;
    }

    BinaryTargetImagePtr bti(new BinaryTargetImage(m_minRange, m_maxRange, az, data));
    return bti;
}

void
SegmentedTargetImage::TruncateToOneRow()
{
    if (m_image.size() > 0) {
        Row lastRow = m_image.back();
        m_image.clear();
        m_image.push_back(lastRow);
        m_minRange = lastRow.GetFirstRangeBin();
        m_maxRange = lastRow.GetLastRangeBin();
    }
}
