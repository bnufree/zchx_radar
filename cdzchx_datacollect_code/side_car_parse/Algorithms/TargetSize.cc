#include "TargetSize.h"
#include <iostream>

std::ostream&
operator<<(std::ostream& os, const TargetSize& size)
{
    if (size.maxRangeValid && size.minRangeValid) {
        os << " bins=[" << size.minRange << ", " << size.maxRange << "]"
           << " dbins=" << size.RangeExtent();
    }
    if (size.minAzValid && size.maxAzValid) {
        os << " az=[" << size.minAz << ", " << size.maxAz << "]"
           << " dAz=" << AZIMUTH_EXTENT(size.minAz, size.maxAz);
    }
    if (size.priValid) { os << " priCount=" << size.priCount; }
    return os;
}
