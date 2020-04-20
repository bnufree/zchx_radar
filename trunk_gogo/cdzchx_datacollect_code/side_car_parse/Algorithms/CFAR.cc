#include "Messages/BinaryVideo.h"

#include "CFAR.h"
#include "CFAR_defaults.h"
#include "common.h"

using namespace ZCHX;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Messages;

CFAR::CFAR() :alpha_(kDefaultAlpha), videoBuffer_(100)
{
    reset();
}

bool CFAR::reset()
{
    videoBuffer_.clear();
    return true;
}

bool CFAR::processEstimate(const Video::Ref& estMsg)
{
    uint32_t sequenceCounter = estMsg->getSequenceCounter();
    LOG_FUNC_DBG << sequenceCounter << endl;

    Video::Ref vidMsg;
    while (!videoBuffer_.empty()) {
        Video::Ref tmp(videoBuffer_.back());
        videoBuffer_.pop_back();
        if (tmp->getSequenceCounter() == sequenceCounter) {
            vidMsg = tmp;
            break;
        }
    }

    if (!vidMsg) return true;

    // Normalize the size of the messages.
    //
    size_t vidSize = vidMsg->size();
    size_t estSize = estMsg->size();
    if (vidSize < estSize) {
        vidMsg->resize(estSize);
        vidSize = estSize;
    } else if (estSize < vidSize) {
        estMsg->resize(vidSize);
        estSize = vidSize;
    }

    // Create the output message.
    //
    BinaryVideo::Ref outMsg(BinaryVideo::Make("CFAR", vidMsg));
    outMsg->resize(vidSize);

    // Calculate binary samples by comparing video samples against calculated threshold.
    //
    double alpha = alpha_;
    for (size_t index = 0; index < vidSize; ++index) {
        double threshold = alpha * estMsg[index];
        outMsg[index] = (vidMsg[index] > threshold) ? true : false;
    }

    bool rc = /*send(outMsg)*/true;
    LOG_FUNC_DBG << "send: " << rc << endl;

    return rc;
}

bool CFAR::processVideo(const Video::Ref& vidMsg)
{
    LOG_FUNC_DBG << vidMsg->getSequenceCounter() << endl;
    videoBuffer_.add(vidMsg);
    return true;
}

