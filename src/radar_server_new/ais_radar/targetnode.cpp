#include "targetnode.h"
#include "zchxfunction.h"
#include <QSharedPointer>

#define         FALSE_ALARM_INDEX_SIZE   10
#define         FALSE_ALARM_CONTINUE_EMPTY  5
#define         FALSE_ALARM_COUNTER_PERCENT  0.5


TargetNode::TargetNode()
{
    mDefRect = 0;
    mChildren.clear();
    mUpdateTime = QDateTime::currentDateTime().toTime_t();
    mParent = 0;
    mStatus = Node_UnDef;
    mSerialNum = 0;
    mFalseAlarm = false;
    mPredictionNode = 0;
    clearPrediction();
}
TargetNode::TargetNode(const zchxRadarRectDef& other, TargetNode* parentNode)
{
    mDefRect = new zchxRadarRectDef(other);
    mChildren.clear();
    mUpdateTime = mDefRect->updatetime();
    mParent = parentNode;
    mSerialNum = 0;
    mStatus = Node_UnDef;
    if(parentNode)
    {
        mStatus = mParent->mStatus;
        mSerialNum = mParent->mSerialNum;
    }
    mVideoIndexList.append(other.videocycleindex());
    mFalseAlarm = false;
    mPredictionNode = 0;
    clearPrediction();
}

void TargetNode::setStatus(NodeStatus sts)
{
    mStatus = sts;
}

TargetNode::~TargetNode()
{
    mChildren.clear();
    if(mDefRect)
    {
//            qDebug()<<"node has been delete now. node num:"<<rect->rectnumber();
        delete mDefRect;
    }
}
QList<TargetNode*> TargetNode::getAllBranchLastChild()
{
    QList<TargetNode*> result;
    for(int i=0; i<mChildren.size(); i++)
    {
        TargetNode *child = mChildren[i].data();
        if(!child) continue;
        result.append(child->getLastChild(child));
    }
    return result;
}

TargetNode* TargetNode::getLastChild(TargetNode* src)
{
    if(src->mChildren.size() == 0) return src;
    TargetNode *child = src->mChildren.first().data();
    return child->getLastChild(child);
}

TargetNode* TargetNode::getLastChild()
{
    if(mChildren.size() == 0) return this;
    TargetNode *child = mChildren.first().data();
    return child->getLastChild();
}

bool TargetNode::hasChildren() const
{
    return mChildren.size() != 0;
}

bool TargetNode::isNodePoint() const //静止目标
{
    return mStatus == Node_UnDef && mChildren.size() == 0 && mVideoIndexList.size() >= 3;
}

bool TargetNode::isNodeMoving() const
{
    return mStatus == Node_Moving;
}

double TargetNode::getReferenceSog(bool average)
{
    if(!mDefRect) return 0.0;
    double sum = mDefRect->sogms();
    if(!average) return sum;
    int num = 1;
    TargetNode* pre = mParent;
    while (pre) {
        if(pre->mDefRect && pre->mParent)  //根节点没有速度  暂且不考虑
        {
            sum += pre->mDefRect->sogms();
            num++;
        }
        if(num == 5) break;
        pre = pre->mParent;
    }

    return sum / num;
}

double TargetNode::getReferenceCog(bool average)
{
    if(!mDefRect) return 0.0;
    double sum = mDefRect->cog();
    if(!average) return sum;
    int num = 1;
    TargetNode* pre = mParent;
    while (pre) {
        if(pre->mDefRect)
        {
            sum += pre->mDefRect->cog();
        }
        num++;
        if(num == 5) break;
        pre = pre->mParent;
    }

    return sum / num;
}

void TargetNode::updateSerialNum(int num)
{
    if(mSerialNum != num) mSerialNum = num;
}

void TargetNode::setAllNodeSeriaNum(int num)
{
    updateSerialNum(num);
    for(int i=0; i<mChildren.size();i++)
    {
        TargetNode* child = mChildren[i].data();
        while (child) {
            updateSerialNum(num);
            if(child->mChildren.size() == 0) break;
            child = child->mChildren.first().data();
        }
    }
}

void TargetNode::updateRouteNodePathStatus(NodeStatus sts)
{
    if(this->mChildren.size() >= 2) return;
    TargetNode *node = this;
    while (node) {
        if(node->mStatus != sts) node->mStatus = sts;
        if(node->mChildren.size() == 0) break;
        node = node->mChildren.first().data();
    }
}

bool TargetNode::isOutput() const
{
    return isNodeMoving() || isNodePoint();
}

TargetNode* TargetNode::topNode()
{
    if(!mParent) return this;
    TargetNode *parent = mParent;
    while (parent) {
        if(parent->mParent)
        {
            parent = parent->mParent;
        } else
        {
            break;
        }
    }

    return parent;
}


int TargetNode::getDepth()
{
    if(mChildren.size() != 1) return 1;
    int depth = 1;
    TargetNode *now = this;
    while (now->mChildren.size() > 0) {
        depth++;
        now = now->mChildren.first().data();
    }

    return depth;
}

bool  TargetNode::isTopNode() const
{
    return mParent == 0;
}

void  TargetNode::removeChild(TargetNode* child)
{
    for(int i=0; i<mChildren.size(); i++)
    {
        if(mChildren[i].data() == child)
        {
            mChildren.removeAt(i);
            break;
        }
    }
}

QList<uint>  TargetNode::getVideoIndexList()
{
    QList<uint> result;
    result.append(this->mVideoIndexList);
    for(int i=0; i<mChildren.size(); i++)
    {
        //将孩子节点的周期也添加进去
        TargetNode* child = mChildren[i].data();
        while (child) {
            foreach (int id, child->mVideoIndexList) {
                if(result.contains(id)) continue;
                result.append(id);
            }
        }
    }
    //进行升序排列，最多保持1000个
    std::sort(result.begin(), result.end());
    if(result.size() >= 1000)
    {
        int index = result.size() - 1000;
        result = result.mid(index, 1000);
    }

    return result;
}

//获取子节点最后的更新时间
uint TargetNode::getLatestChildUpdateTime()
{
    QList<TargetNode*> list = getAllBranchLastChild();
    if(list.size() == 0) return mUpdateTime;
    uint time = list.first()->mUpdateTime;
    for(int i=1; i<list.size(); i++)
    {
        if(time < list[i]->mUpdateTime)
        {
            time = list[i]->mUpdateTime;
        }
    }
    return time;
}
bool TargetNode::isFalseAlarm(int video_index_now)
{
    //静止目标才判断是不是虚警。静止目标没有子节点 只有一个节点
    if(!isNodePoint()) return false;
    QList<uint> list = getVideoIndexList();
    if(list.size() == 0) return false;
    //获取目标统计的开始周期
    uint now = video_index_now;
    uint end = list.last();
    if(now < end) now += MAX_RADAR_VIDEO_INDEX_T;
    uint start = list.first();
    uint seg = now - start + 1;
    if(seg < FALSE_ALARM_INDEX_SIZE) return false;
    start = now - FALSE_ALARM_INDEX_SIZE + 1;
    //从开始位置看，各个值是否包含在回波周期队列中，进行连续计数或者缺失统计
    int continue_num = 0;
    int total_num = 0;
    for(uint i=start; i<=now; i++)
    {
        if(list.contains(i))
        {
            continue_num = 0;
            continue;
        }
        continue_num++;
        total_num++;
        if(continue_num == FALSE_ALARM_CONTINUE_EMPTY)
        {
            return true;
        }
        if(total_num >= int(ceil(FALSE_ALARM_INDEX_SIZE * FALSE_ALARM_COUNTER_PERCENT)))
        {
            return true;
        }
    }
    return false;
}

void TargetNode::clearPrediction()
{
    if(mPredictionNode) delete mPredictionNode;
    mPredictionIndex = 0;
    mPredictionTimes = 0;
    mPredictionNode = 0;
}

void TargetNode::makePrediction(int videoIndex, uint videoTime, bool fixed_space_time)
{
    TargetNode *baseNode = getLastChild();
    if(mPredictionTimes > 0)
    {
        baseNode = mPredictionNode;
    }
    //构造回波矩形
    zchxRadarRectDef def(*mDefRect);
    if(mPredictionNode) def.CopyFrom(*(mPredictionNode->mDefRect));
    uint last_time = def.updatetime();
    uint delta_time = videoTime - last_time;        //预推的时间间隔（S）
    if(fixed_space_time) delta_time = 3;
    double distance = def.sogms() * delta_time;
    QGeoCoordinate src(def.center().latitude(), def.center().longitude());
    QGeoCoordinate dest = src.atDistanceAndAzimuth(distance, def.cog());
    def.mutable_center()->set_latitude(dest.latitude());
    def.mutable_center()->set_longitude(dest.longitude());
    def.set_updatetime(videoTime);
    def.set_videocycleindex(videoIndex);
    mPredictionIndex = videoIndex;
    mPredictionTimes++;
    if(mPredictionNode)
    {
        mPredictionNode->mDefRect->CopyFrom(def);
    } else
    {
        mPredictionNode = new TargetNode(def, 0);
    }
    mPredictionNode->mSerialNum = mSerialNum;
    mPredictionNode->mUpdateTime = videoTime;
    mPredictionNode->mStatus = mStatus;
}
