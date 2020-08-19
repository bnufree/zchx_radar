#include "targetnode.h"
#include "zchxfunction.h"

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
}
TargetNode::TargetNode(const zchxRadarRectDef& other, TargetNode* parentNode = 0)
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
    return mStatus == Node_UnDef && mChildren.size() == 0;
}

bool TargetNode::isNodeMoving() const
{
    return mStatus == Node_Moving;
}

double TargetNode::getReferenceSog(bool average = true)
{
    if(!mDefRect) return 0.0;
    double sum = mDefRect->sog();
    if(!average) return sum;
    int num = 1;
    TargetNode* pre = mParent;
    while (pre) {
        if(pre->mDefRect && pre->mParent)  //根节点没有速度  暂且不考虑
        {
            sum += pre->mDefRect->sog();
            num++;
        }
        if(num == 5) break;
        pre = pre->mParent;
    }

    return sum / num;
}

double TargetNode::getReferenceCog(bool average = true)
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
    TargetNode * parent = mParent;
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
    TargetNode * now = this;
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
    if(mChildren.size() > 0) return false;
    QList<uint> list = getVideoIndexList();
    if(list.size() == 0) return false;
    //获取目标统计的开始周期
    uint now = video_index_now;
    uint start = list.first();
    uint end = video_index_now;
    if(end < start) end += MAX_RADAR_VIDEO_INDEX_T;
    int seg = end - start + 1;
    if(seg < FALSE_ALARM_INDEX_SIZE) return false;

    //检查是否连续空缺的周期数
    for(int i=1 ; i<list.size(); i++)
    {
        int now = list[i];

    }
}
