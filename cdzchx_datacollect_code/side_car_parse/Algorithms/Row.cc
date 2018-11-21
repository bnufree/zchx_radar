#include "Row.h"
#include "common.h"

bool
operator<(const Segment& lhs, const Segment& rhs)
{
    return lhs.start < rhs.start;
}

void
UpdateRangeToTargetMap(RANGEBIN start, RANGEBIN stop, std::vector<SegmentedTargetImagePtr>& rangeToTargetMap,
                       SegmentedTargetImagePtr newptr)
{
    for (RANGEBIN r = start; r <= stop; r++) { rangeToTargetMap[r] = newptr; }
}

void
Row::AddMask(Segment seg)
{
    mask.push_back(seg);
    if (!minRangeValid || (minRange > seg.start)) {
        minRange = seg.start;
        minRangeValid = true;
    }
    if (!maxRangeValid || (maxRange < seg.stop)) {
        maxRange = seg.stop;
        maxRangeValid = true;
    }
}

RANGEBIN
Row::GetFirstRangeBin()
{
#ifdef VERIFY_ROW
    if (!minRangeValid) {
        std::cout << "invalid first range bin";
        //abort();
    }
#endif
    return minRange;
}

RANGEBIN
Row::GetLastRangeBin()
{
#ifdef VERIFY_ROW
    if (!maxRangeValid) {
        std::cout << "invalid last range bin";
        //abort();
    }
#endif
    return maxRange;
}

void
Row::ResetMask()
{
    mask.clear();
    minRangeValid = false;
    maxRangeValid = false;
}

bool
Row::Empty()
{
    return mask.empty();
}

void
Row::Sort()
{
    sort(mask.begin(), mask.end());
}

void
Row::Merge(Row sRow, std::vector<SegmentedTargetImagePtr>* rangeToTargetMap /*= NULL*/,
           SegmentedTargetImagePtr* newptr /*= NULL*/)
{
    std::vector<Segment>::iterator i;
    for (i = sRow.mask.begin(); i != sRow.mask.end(); i++) {
        AddMask(*i);
        if (rangeToTargetMap && newptr) UpdateRangeToTargetMap((*i).start, (*i).stop, *rangeToTargetMap, *newptr);
    }
}

void
Row::DumpSorted()
{
    // precondition: row is sorted
    for (std::vector<Segment>::size_type i = 0; i < mask.size(); i++) {
        for (RANGEBIN r = mask[i].start; r <= mask[i].stop; r++) LOGDEBUG << "x";
        if ((i + 1) < mask.size()) {
            for (RANGEBIN f = mask[i].stop + 1; f < mask[i + 1].start; f++) LOGDEBUG << " ";
        }
    }
}

void
Row::FillArray(BINARYDATA* dest, RANGEBIN startRange, RANGEBIN stopRange)
{
#ifdef VERIFY_ROW
    if (!maxRangeValid || !minRangeValid) {
        std::cout << "invalid fill array";
        //abort();
    }
    if (startRange > minRange) {
        std::cout << "invalid fill array";
        //abort();
    }
    if (stopRange < maxRange) {
        std::cout << "invalid fill array";
        //abort();
    }
#endif

    memset(dest, 0x00, sizeof(BINARYDATA) * (stopRange - startRange + 1));

    std::vector<Segment>::iterator i;
    for (i = mask.begin(); i != mask.end(); i++) {
        RANGEBIN offset = (*i).start - startRange;
        memset(dest + offset, 0x01, sizeof(BINARYDATA) * ((*i).stop - (*i).start + 1));
    }
}
