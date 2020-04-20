#include "QtCore/QString"

#include "boost/bind.hpp"
#include "Messages/TSPI.h"

#include "ABTracker.h"
#include "ABTracker_defaults.h"

#include "ABTrack.h"
#include "common.h"

#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"
using namespace ZCHX;
using namespace ZCHX::Algorithms;
using namespace ZCHX::Algorithms::ABTrackerUtils;

ABTracker::ABTracker(RadarConfig* cfg, bool new_thread, QObject* parent) :
    QObject(parent),
    cfg_(cfg),
    mNewThreadFlag(new_thread),
    enabled_(kDefaultEnabled), rotationDuration_(/*kDefaultRotationDuration*/cfg->getRotationDuration()),
    timeScaling_(kDefaultTimeScaling),
    alpha_(kDefaultAlpha),
    beta_(kDefaultBeta),
    associationRadius_(/*kDefaultAssociationRadius*/cfg->getTargetSearchRadius()),
    initiationCount_(kDefaultInitiationCount),
    initiationRotationCount_(kDefaultInitiationRotationCount),
    coastRotationCount_( kDefaultCoastRotationCount),
    minRange_( /*kDefaultMinRange*/0),
   /* reset_(Parameter::NotificationValue::Make("reset", "Reset", 0)),*/
    trackIdGenerator_(0), tracks_(), trackCount_(0),
    initiatingCount_(0), coastingCount_(0),mSendTimer(0)
{
    associationRadius2_ = associationRadius_;
    associationRadius2_ *= associationRadius2_;
    endParameterChanges();
    if(mNewThreadFlag)
    {
        mSendTimer = new QTimer;
        mSendTimer->setInterval(3000);
        connect(mSendTimer, SIGNAL(timeout()), this, SLOT(slotTimeroutFunc()));
        this->moveToThread(&mWorkThread);
        mWorkThread.start();
        mSendTimer->start();
    }
    //trackIdGenerator_ = QDateTime::currentDateTime().toTime_t();
    //reset_->connectChangedSignalTo(boost::bind(&ABTracker::resetNotification, this, _1));
    //associationRadius_->connectChangedSignalTo(boost::bind(&ABTracker::associationRadiusChanged, this, _1));
}

//bool
//ABTracker::startup()
//{
//    endParameterChanges();
//    registerProcessor<ABTracker, Messages::Extractions>(&ABTracker::processInput);
//    return Super::startup() && registerParameter(enabled_) && registerParameter(rotationDuration_) &&
//           registerParameter(timeScaling_) && registerParameter(alpha_) && registerParameter(beta_) &&
//           registerParameter(associationRadius_) && registerParameter(initiationCount_) &&
//           registerParameter(initiationRotationCount_) && registerParameter(coastRotationCount_) &&
//           registerParameter(minRange_) && registerParameter(reset_);
//}

//bool
//ABTracker::shutdown()
//{
//    resetTracker();
//    return Super::shutdown();
//}

zchxTrackPointList ABTracker::getTrackPnts()
{
    //cout<<"最后一步获取目标了"<<tracks_.size();
    zchxTrackPointList list;
    TrackList::iterator pos = tracks_.begin();
    TrackList::iterator end = tracks_.end();
    while (pos != end) {
        ABTrack* track = *pos;
        if(track->isAlive())
        {
            list.append(track->emitPosition());
            //qDebug()<<"id:"<<track->getId().data()<<" pnts:"<<track->historyPntsStr()<<endl;
        }
        ++pos;
    }

    return list;
}

void ABTracker::slotTimeroutFunc()
{
//    TrackList::iterator pos = tracks_.begin();
//    TrackList::iterator end = tracks_.end();
//    double when = QDateTime::currentMSecsSinceEpoch() / 1000.0;
//    while (pos != end) {
//        ABTrack* track = *pos;
//        if (track->isShouldDropAtTime(when)) {
//            mTrackPointList.remove(QString::fromStdString(track->getId()).toInt());
//            qDebug() << "dropping track " << track->getId().data()<<" size:"<<mTrackPointList.size();
//            delete track;
//            pos = tracks_.erase(pos);
//            --trackCount_;
//            continue;
//        }
//        ++pos;
//    }
//    if(mNewThreadFlag && mTrackPointList.size() > 0)
//    {
//        emit sendTrackPoints(mTrackPointList.values());
//    }
}

void
ABTracker::resetTracker()
{
    while (trackCount_) {
        ABTrack* track = tracks_.back();
        track->drop();
        delete track;
        tracks_.pop_back();
        --trackCount_;
    }

    initiatingCount_ = 0;
    coastingCount_ = 0;
}

void ABTracker::slotRecvExtractions(const Extractions::Ref &msg)
{
    Extraction2Track(msg);
//    if(mNewThreadFlag && mTrackPointList.size() > 0)
//    {
//        emit sendTrackPoints(mTrackPointList.values());
//    }
}

void
ABTracker::Extraction2Track(const Messages::Extractions::Ref& msg)
{
    //先根据设定好的合并半径差和合并角度差将可能的目标合并为一个,并进行目标编号
    typedef QList<Messages::Extraction>     MergeTarget;
    typedef QList<MergeTarget>              MergeTargetList;
    double azimuth = cfg_->getMergeAzimuth();
    double radius = cfg_->getMergeRadius();
    MergeTargetList targetList;
    for (size_t index = 0; index < msg->size(); ++index)
    {
        Messages::Extraction ext = msg[index];
        //检查是否与已经存在的目标可以合并
        bool found = false;
        for(int i=0; i<targetList.size(); i++)
        {
            Messages::Extraction src = targetList[i].first();
            //比较目标的角度
            double azimuth_sub = fabs(src.getAzimuthDegree() - ext.getAzimuthDegree());
            //qDebug()<<"src:"<<src.getAzimuthDegree()<<ext.getAzimuthDegree()<<azimuth_sub;
            if(azimuth_sub > azimuth) continue;
            //比较距离是否符合要求
            double dis = src.dis2Extraction(ext);
            //qDebug()<<" distance:"<<dis;
            if(dis > radius) continue;
            found = true;
            targetList[i].append(ext);
            break;
        }
        if(!found)
        {
            //没有找到,作为新目标添加
            MergeTarget target;
            target.append(ext);
            targetList.append(target);
        }
    }
    //合并目标到中间的点,包括角度和半径
    QList<Messages::Extraction> mergeList;
    for(int i=0; i<targetList.size(); i++)
    {
        MergeTarget target = targetList[i];
        int size = target.size();
        if(size == 0) continue;
        if(size > 1)
        {
            double when = target.first().getWhen();
            double range = 0, angle = 0;
            foreach (Messages::Extraction obj, target) {
                range += obj.getRange();
                angle += obj.getAzimuth();
            }
            mergeList.append(Messages::Extraction(when, range / size, angle / size, 0));
        } else
        {
            mergeList.append(target.first());
        }
    }

    //将合并的目标转换为跟踪的目标
    associationRadiusChanged(cfg_->getTargetSearchRadius());
    // For each Extraction report in the message call updateTracks.
    //设定所有目标都为未更新状态,防止有的目标本轮多次更新,导致速度等计算异常
    TrackList::iterator pos = tracks_.begin();
    TrackList::iterator end = tracks_.end();
    while (pos != end) {
        ABTrack* track = *pos++;
        track->setUpdate(false);
    }

    for (size_t index = 0; index < mergeList.size(); ++index)
    {
        updateTracks(mergeList[index]);
    }
#if (1)
    pos = tracks_.begin();
    end = tracks_.end();
    while (pos != end) {
        ABTrack* track = *pos;
        ABTrack::State state = track->getState();
        if (state == ABTrack::kUninitiating || state == ABTrack::kDropping) {
            pos = tracks_.erase(pos);
            delete track;
            --trackCount_;
            continue;
        }
        ++pos;
    }
#else
//     Revisit all of the tracks, emitting TSPI messages for those that are
//     dropping and the one that was updated above.

    bool ok = true;
    TrackList::iterator pos = tracks_.begin();
    TrackList::iterator end = tracks_.end();
    while (pos != end) {
        ABTrack* track = *pos;
        if (track == found || track->isDropping()) {
            TrackPoint point = track->emitPosition();
            if(point.tracknumber() > 0)
            {
                pnts[point.tracknumber()] = point;
            }
            if(point.tracklastreport())
            {
                qDebug()<<"remove track num:"<<point.tracknumber();
                pnts.remove(point.tracknumber());
            }
            if (track->isDropping()) {
                LOG_FUNC_DBG << "dropping track " << track->getId().data() << endl;
                delete track;
                pos = tracks_.erase(pos);
                --trackCount_;
                continue;
            }
        }
        ++pos;
    }

    }else
    {
        qDebug()<<"!!!!!!!!!!!!!!!!!!!!!!!";
        TrackList::iterator pos = tracks_.begin();
        TrackList::iterator end = tracks_.end();
        double when = QDateTime::currentMSecsSinceEpoch() / 1000.0;
        while (pos != end) {
            ABTrack* track = *pos;
            if (track->isShouldDropAtTime(when)) {
                mTrackPointList.remove(QString::fromStdString(track->getId()).toInt());
                qDebug() << "dropping track " << track->getId().data()<<" size:"<<mTrackPointList.size();
                delete track;
                pos = tracks_.erase(pos);
                --trackCount_;
                continue;
            }
            ++pos;
        }
    }
#endif
}

void ABTracker::setNewLoop(bool sts)
{
    //cout<<"setNewLoop"<<tracks_.size();
    mNewLoop = sts;
    if(mNewLoop)
    {
        //init
        TrackList::iterator pos = tracks_.begin();
        TrackList::iterator end = tracks_.end();
        while (pos != end) {
            ABTrack* track = *pos++;
            track->setUpdate(false);
        }
    } else{
        TrackList::iterator pos = tracks_.begin();
        TrackList::iterator end = tracks_.end();
        while (pos != end) {
            ABTrack* track = *pos;
            if(!track->isUpdate()){
                track->not_update_me();
                //qDebug() << " track not update in loop " << track->getId().data() << " and count is:"<<track->not_update_number();
            } else {
                track->not_update_clear();
                track->update_me();
            }
            // if track is not update for specified times, track is drop
            if(track->not_update_number() == 20)
            {
                //qDebug() << "dropping track " << track->getId().data() <<track->historyPntsStr()<< endl;
                delete track;
                pos = tracks_.erase(pos);
                --trackCount_;
                continue;
            }
            pos++;
        }
    }
}

bool
ABTracker::updateTracks(const Messages::Extraction& plot)
{
    double when = plot.getWhen();

    //qDebug() << "when: " << when << " range: " << plot.getRange() << " az: " << plot.getAzimuth()<<" x:"<<plot.getX()<<" y:"<<plot.getY();

    found = 0;
    if (plot.getRange() >= minRange_) {
        // We only associate with something if it has a proximity value smaller
        // than associationRadius. We use the squared variants to save us from
        // the costly square root.
        //
        double best = associationRadius2_;
        Geometry::Vector v(plot.getX(), plot.getY(), plot.getElevation());

        // Visit all of the existing Track objects to find the one that best
        // associates with the given Extraction plot.
        //
        TrackList::iterator pos = tracks_.begin();
        TrackList::iterator end = tracks_.end();
        QList<CloseTrackData> templist;
        while (pos != end) {
            ABTrack* track = *pos++;
            //if(track->isUpdate()) continue;
            double distance = track->getProximityTo(when, v);
            //qDebug()<<track->getId().data()<<track->pos()<<distance<<v;
            if (distance < best) {
                //if(fabs(plot.getAzimuth() - track->azimuth()) > 0.3) continue;
                if(templist.isEmpty())
                {
                    //cout<<"距离^2"<<best;
                    templist.append(CloseTrackData{track, distance});
                    continue;
                }
                if(templist.first().distance >= distance)
                {
                    //cout<<"---"<<templist.first().distance <<distance;
                    templist.replace(0, CloseTrackData{track, distance});
                }
            }
        }
        if(!templist.isEmpty())
        {
            //qDebug()<<"nearest distance:"<<templist.first().distance;
            found = templist.first().track;
        }
        // None found, so create a new Track object that starts in the
        // kInitiating state.
        //
        if (!found) {
            found = new ABTrack(*this, ++trackIdGenerator_, when, v, cfg_);
            found->setAzimuth(plot.getAzimuth());
            //qDebug() << "created new track - " << found->getId().data()<<found->state().data();
            ABTrack absize = *found;
            //cout<<"内存大小"<<sizeof(absize)<<sizeof(ABTrack);
            tracks_.push_back(found);
            ++trackCount_;
#if 0
           // cout<<"新增内存地址"<<found;
            if(trackCount_ > 10)
            {
                while (trackCount_)
                {
                    ABTrack* track = tracks_.back();
                   // cout<<"删除内存地址"<<tracks_.size()<<track;
                    track->drop();
                    delete track;
                    track = NULL;
                    tracks_.pop_back();
                    --trackCount_;
                }
                //delete found;
            }
#endif
        } else {
            //qDebug() << "found existing track - " << found->getId().data()<<found->state().data();
            //检查目标是否在本轮更新过,如果更新过,将目标回退到上一次的状态

            if(found->isUpdate())
            {
                //检查是否是同一轮更新
                if(found->last_round_time() > 10.0)
                {
                    found->restore();
                }
            } else
            {
                found->save();
            }
            if(found->last_round_time() < 10)
            {
                //同一轮更新
                found->updatePos(v);
            } else
            {
                found->updatePosition(when, v);
            }
            found->setUpdate(true);
            found->appendHistory(v);
            found->setAzimuth(plot.getAzimuth());
        }

        found = NULL;
    }
//    //qDebug()<<"current track cache size:"<<tracks_.size();

}

//void
//ABTracker::setInfoSlots(IO::StatusBase& status)
//{
//    status.setSlot(kEnabled, enabled_->getValue());
//    status.setSlot(kAlpha, alpha_->getValue());
//    status.setSlot(kBeta, beta_->getValue());
//    status.setSlot(kTrackCount, int(trackCount_));
//}

void
ABTracker::resetNotification()
{
    resetTracker();
}

void
ABTracker::associationRadiusChanged(const double& value)
{
    associationRadius2_ = value;
    associationRadius2_ *= associationRadius2_;
}

void
ABTracker::endParameterChanges()
{
    calculateDurations();
}

void
ABTracker::calculateDurations()
{
    double scaledRotationDuration = rotationDuration_ * timeScaling_;
    scaledMaxInitiationDuration_ = initiationRotationCount_ * scaledRotationDuration;
    scaledMaxCoastDuration_ = coastRotationCount_ * scaledRotationDuration;
}

