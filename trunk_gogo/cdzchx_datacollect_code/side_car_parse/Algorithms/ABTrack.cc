#include <limits>
#include <sstream>

#include "ABTracker.h"
#include "ABTrack.h"
#include "common.h"
#include "Messages/TSPI.h"

using namespace ZCHX::Messages;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Algorithms::ABTrackerUtils;

const int default_speed_threshold = 30;

ABTrack::ABTrack(ABTracker& owner, uint32_t id, double when, const Geometry::Vector& pos, RadarConfig*  cfg) :
    owner_(owner), t0_(), initialPosition_(pos), id_(""), state_(kInitiating), cfg_(cfg), update_(true), not_update_num(0), update_times(1)
{
    std::ostringstream os;
    os << id;
    id_ = os.str();
    t0_.when_ = when;
    t0_.position_ = pos;
    mHistoryPntsList.append(pos);
    checkIfInitiated(when);
}

void ABTrack::updatePos(const Geometry::Vector &pos)
{
    t0_.position_ = pos;
    mHistoryPntsList.clear();
    mHistoryPntsList.append(pos);
}

void ABTrack::checkIfInitiated(double when)
{
    LOG_FUNC_DBG << "when: " << when << endl;

    initiationTimeStamps_.push_back(when);

    // Prune any stale timestamps.
    //
    double maxDuration = owner_.getScaledMaxInitiationDuration();
    for (size_t index = 0; index < initiationTimeStamps_.size(); ++index) {
        double delta = when - initiationTimeStamps_[index];
        if (delta <= maxDuration) {
            if (index) {
                initiationTimeStamps_.erase(initiationTimeStamps_.begin(), initiationTimeStamps_.begin() + index - 1);
                break;
            }
        }
    }

    // Only become active if after pruning we have the required amount of hits.
    //
    if (initiationTimeStamps_.size() >= owner_.getInitiationCount()) {
        LOG_FUNC_DBG << "track " << id_ .data()<< " is now alive" << endl;
        initiationTimeStamps_.clear();
        state_ = kAlive;
    }
}

bool ABTrack::isShouldDropAtTime(double when)
{
    double deltaTime = when - t0_.when_;
    if (deltaTime >= owner_.getScaledMaxInitiationDuration() ||
        deltaTime >= owner_.getScaledMaxCoastDuration()) {
        return true;
    }
    return false;
}

double
ABTrack::getProximityTo(double when, const Geometry::Vector& pos)
{
    LOG_FUNC_DBG << id_ .data()<< " when: " << when << endl;
    // How much time has passed since the last update?
    //
    double deltaTime = when - t0_.when_;

    // If too much time has elapsed to initiate the track or to coast it, drop it.
    //
    if (state_ == kInitiating) {
        if (deltaTime >= owner_.getScaledMaxInitiationDuration()) {
            LOG_FUNC_DBG << id_.data() << " uninitiating - " << deltaTime << endl;
            state_ = kUninitiating;
        }
    } else if (state_ == kAlive) {
        if (deltaTime >= owner_.getScaledMaxCoastDuration()) {
            LOG_FUNC_DBG << id_.data() << " dropping - " << deltaTime << endl;
            state_ = kDropping;
        }
    }

    // If the track is not initiating or active, don't let it participate in plot association.
    //
    if (state_ == kDropping || state_ == kUninitiating) return std::numeric_limits<double>::max();

    // Predict track position assuming constant velocity. X1 = X0 + V0 * T
    //
    //时间太短了，认为不是一个目标
    Geometry::Vector position = t0_.position_ + t0_.velocity_ * deltaTime;

    // Calculate proximity of given position to estimated position.
    position -= pos;
//    if(deltaTime < 0.001)
//    {
//        qDebug()<<"old:"<<t0_.position_<<" time:"<<deltaTime<<" too small skiped";
//        return std::numeric_limits<double>::max();
//    }
    double value = position.getMagnitudeSquared();
    //qDebug()<<"old:"<<t0_.position_<<" speed:"<<position/deltaTime<<" time:"<<deltaTime<<" value:"<<value;
    //计算目标的速度，速度超出阈值认为不是同一个目标(默认速度值30)
//    double speed = sqrt(value) * 1000.0 / deltaTime;
//    if(qRound(speed) >  default_speed_threshold) return std::numeric_limits<double>::max();
    LOG_FUNC_DBG << id_.data() << " return: " << value << endl;
    return value;
}

void
ABTrack::updatePosition(double when, const Geometry::Vector& pos)
{
    static const double kMinDeltaTime = /*1.0E-3*/1.0;
    LOG_FUNC_DBG<< id_.data() << " when: " << when << endl;

    // Calculate the delta time
    //
    double deltaTime = when - t0_.when_;
    if (deltaTime < kMinDeltaTime) {
        //qDebug() << "delta too small - " << deltaTime << endl;
        //deltaTime = 1;
        return;
    }

    t0_.when_ = when;

    // See if we can move into kAlive state
    //
    if (state_ == kInitiating) checkIfInitiated(when);

    // Calculate distance moved
    //
    Geometry::Vector distance = pos;
    Geometry::Vector error = pos;
    Geometry::Vector estimate = t0_.position_ + t0_.velocity_ * deltaTime;

    // Compute the difference between the measured position and the expected position
    //
    error -= estimate;
    distance -= t0_.position_;

    // If track is initiating, don't use the AB equation for the velocity since we don't have an initial
    // velocity estimate, and we want to quickly settle on one based on the first N associations.
    //
    if (state_ == kInitiating) {
        // Estimate velocity as distance traveled over time.
        //
        Geometry::Vector velocity = distance / deltaTime;
        qDebug()<<"init distance:"<<distance<<" deltaTime:"<<deltaTime<<" v:"<<velocity.getMagnitudeSquared();

        if (initiationTimeStamps_.size() == 2) {
            // Just use the estimate.
            //
            t0_.velocity_ = velocity;
        } else {
            // Update velocity as an average of estimated and last value
            //
            t0_.velocity_ = (t0_.velocity_ + velocity) / 2.0;
        }
    } else {
        // Othewise, update using AB tracking-filter equation: V1 = V_Predicted + Beta * (X_Measured -
        //   X_Predicted) / T
        //
        t0_.velocity_ += owner_.getBeta() * error / deltaTime;
//        qDebug()<<"error:"<<error.getMagnitude()<<" deltaTime:"<<deltaTime<<" v:"<<t0_.velocity_.getMagnitude();
    }

    // Calculate estimated position: X1 = X_Predicted + Alpha * (X_Measured - X_Predicted)
    //
    t0_.position_ = estimate + owner_.getAlpha() * error;
}

TrackPoint ABTrack::emitPosition()
{
    TrackPoint point;
    point.set_tracknumber(0);

    //if (state_ == kInitiating) return point;
    TSPI::Ref msg = TSPI::MakeRAE(owner_.metaObject()->className(), id_, t0_.when_, t0_.position_.getMagnitude() /** 1000.0*/,
                                t0_.position_.getDirection(), t0_.position_.getZ(), cfg_);
    if (state_ == kDropping)
    {
        msg->setDropping();
    }
    point = msg->toTrackPoint();
    point.set_sog(t0_.velocity_.getMagnitude());
//    qDebug()<<"sog:"<<point.sog();
    return point;

    //return owner_.send(msg);
}

void
ABTrack::drop()
{
    state_ = kDropping;
    emitPosition();
}

std::string ABTrack::state()
{
    std::string str;
    switch (state_) {
    case kInitiating:
        str = "initiating";
        break;
    case kAlive:
        str = "alive";
        break;
    case kDropping:
        str = "drop";
        break;
    case kUninitiating:
        str = "uninitiating";
        break;
    default:
        str = "undef";
        break;
    }
    return str;
}
