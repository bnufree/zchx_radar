#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "boost/bind.hpp"
#include "Messages/BinaryVideo.h"
#include "Threshold.h"
#include "Threshold_defaults.h"
#include <QtCore/QString>
#include "common.h"

using namespace ZCHX;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Messages;

Threshold::Threshold(int val) : threshold_(/*kDefaultThreshold*/val)
{
    thresholdValue_ = threshold_;
    //threshold_->connectChangedSignalTo(boost::bind(&Threshold::thresholdChanged, this, _1));
}

//bool
//Threshold::startup()
//{
//    registerProcessor<Threshold, Video>(&Threshold::process);
//    thresholdValue_ = threshold_->getValue();
//    return registerParameter(threshold_) && Algorithm::startup();
//}

/** Functor that performs a threshold check on given values to see if they are >= a threshold value.
 */
struct ThresholdFilter {
    Threshold::DatumType threshold_;
    ThresholdFilter(Threshold::DatumType v) : threshold_(v) {}
    bool operator()(Threshold::DatumType v) const {
        return v >= threshold_;
    }
};

bool
Threshold::process(const Messages::Video::Ref& in, Messages::BinaryVideo::Ref& out)
{
    if(!out) out = BinaryVideo::Ref(BinaryVideo::Make("Threshold", in));
    // Fill output message with boolean values that represent whether or not sample values were >= thresholdValue_.
    //
    std::transform(in->begin(), in->end(), std::back_inserter<>(out->getData()), ThresholdFilter(thresholdValue_));
    return true;
}

void
Threshold::thresholdChanged(const DatumType& value)
{
    thresholdValue_ = value;
}

//void
//Threshold::setInfoSlots(IO::StatusBase& status)
//{
//    status.setSlot(kThreshold, thresholdValue_);
//}


