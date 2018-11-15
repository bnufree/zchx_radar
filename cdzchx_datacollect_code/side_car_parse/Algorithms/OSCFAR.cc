#include <cassert>
#include "OSCFAR.h"
#include "OSCFAR_defaults.h"
#include "common.h"

#include <QtCore/QString>

using namespace ZCHX;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Messages;

/** Utility class that maintains an ordered list of sample values from a Video message. When the window slides
    it drops the oldest sample and gains a new sample, while keeping the ordering intact.
*/
class OrderedSlidingWindow {
public:
    OrderedSlidingWindow(size_t windowSize, const Video::Container& initial);

    Video::DatumType getThreshold(size_t index) const { return sorted_[index]; }

    void insertAndRemove(Video::DatumType insert, Video::DatumType remove);

private:
    Video::Container sorted_;
};

OrderedSlidingWindow::OrderedSlidingWindow(size_t windowSize, const Video::Container& initial) :
    sorted_(initial.begin(), initial.begin() + windowSize)
{
    // Sort the initial window
    //
    assert(windowSize > 0);
    std::sort(sorted_.begin(), sorted_.end());
}

void
OrderedSlidingWindow::insertAndRemove(Video::DatumType insert, Video::DatumType remove)
{
    if (insert == remove) { return; }

    // Locate the position of the item to remove.
    //
    Video::Container::iterator insertPos;
    Video::Container::iterator removePos;
    if (insert < remove) {
        // Locate the lowest position in the ordered vector where we should find the old item.
        //
        removePos = std::lower_bound(sorted_.begin(), sorted_.end(), remove);

        // Locate the highest position in the ordered vector where we could insert the new item and keep the
        // vector ordered.
        //
        insertPos = std::upper_bound(sorted_.begin(), removePos, insert);

        // If the two positions are the same, just replace.
        //
        if (insertPos == removePos) {
            *insertPos = insert;
        } else {
            // Shift all elements from [insertPos, removePos) up one, thus effectively deleting the entry at
            // removePos, and allowing us to store into insertPos while keeping the elements ordered.
            //
            std::copy_backward(insertPos, removePos, removePos + 1);
            *insertPos = insert;
        }
    } else { // insert > remove

        // Locate the highest position in the ordered vector where we should find the old item.
        //
        removePos = std::upper_bound(sorted_.begin(), sorted_.end(), remove) - 1;

        // Locate the lowest position in the ordered vector where we could insert the new item and keep the
        // vector ordered.
        //
        insertPos = std::lower_bound(removePos, sorted_.end(), insert);

        // If the two positions refer to the same slot, just replace.
        //
        if (insertPos == (removePos + 1)) {
            *removePos = insert;
        } else {
            // Shift all elements from [removePos + 1, insertPos) down one, thus effectively deleting the entry
            // at removePos, and allowing us to store into insertPos while keeping the elements ordered.
            //
            std::copy(removePos + 1, insertPos, removePos);
            --insertPos;
            *insertPos = insert;
        }
    }
}

OSCFAR::OSCFAR() : windowSize_(kDefaultSize), thresholdIndex_(kDefaultRank), alpha_(kDefaultAlpha)
{
    //reset();
}

bool OSCFAR::process(const Messages::Video::Ref& in, Messages::BinaryVideo::Ref& out )
{
    double alpha = alpha_;
    LOGDEBUG<< "pri size: " << in->size() << " alpha " << alpha << std::endl;

    // Nothing to do if the input data size is smaller than our window size.
    //
    size_t windowSize = windowSize_;
    if (windowSize > in->size()) {
        LOGDEBUG << "windowSize > in->size()" << std::endl;
        return false;
    }

    // Verify that the threshold index we use is valid in the sliding window.
    //
    size_t percentile = thresholdIndex_;
    double indexIntoWindow = (percentile / 100.0) * windowSize;
    size_t thresholdIndex = static_cast<size_t>(std::floor(indexIntoWindow));

    LOGDEBUG << "thresholdIndex " << thresholdIndex << std::endl;
    if (thresholdIndex >= windowSize) {
        LOGDEBUG << "Threshold index >= window size";
        return false;
    }

    size_t halfWindowSize = windowSize / 2;

    // Create our sliding window. It should remain sorted as we slide over the input data.
    //
    OrderedSlidingWindow slidingWindow(windowSize, in->getData());

    // Calculate the number of times we will apply the window. This unsigned arithmetic is safe to do because
    // above we guarantee that windowSize_ <= in->size().
    //
    size_t limit = in->size() - windowSize;

    //BinaryVideo::Ref out(BinaryVideo::Make("OSCFAR", in));
    out->resize(in->size(), 0);

    // This was written as a for loop, but that is incorrect because in the last iteration the call to
    // OrderedSlidingWindow::insertAndRemove() got called with an invalid index into the input message. Instead,
    // do the termination check from inside the loop, before the call to insertAndRemove().
    //
    size_t index = 0;
    while (1) {
        size_t pos = index + halfWindowSize;
        bool passed = in[pos] > (alpha * slidingWindow.getThreshold(thresholdIndex));
        out[pos] = passed;

        // Check that we can safely continue and index into the input array.
        //
        if (index == limit) { break; }

        // Remove the element that will no longer be valid when the sliding window moves to the next sample, and
        // add the new element that is now visible in the window.
        //
        slidingWindow.insertAndRemove(in[index + windowSize], in[index]);
        ++index;
    }
    return true;
}

