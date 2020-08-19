#ifndef TARGETNODE_H
#define TARGETNODE_H

#include "zchxradarcommon.h"

enum  TARGET_DIRECTION{
    TARGET_DIRECTION_UNDEF = 0,     //点列不足的情况
    TARGET_DIRECTION_STABLE,        //目标点列方向稳定
    TARGET_DIRECTION_UNSTABLE,      //目标点列方向散乱
};


//根节点可以产生很多条分支,但是每一个分支点只有一个子节点
enum NodeStatus{
    Node_UnDef = 0,
    Node_Moving,
    Node_Lost,
};

//目标状态确定为静止，则目标路径上就始终只有一个点
struct TargetNode
{
public:
    int                                         mSerialNum;
    NodeStatus                                  mStatus;
    uint                                         mUpdateTime;        //目标最新的更新时间
    zchxRadarRectDef                            *mDefRect;
    QList<QSharedPointer<TargetNode>>           mChildren;           //孩子
    TargetNode*                                 mParent;             //父亲
    QList<uint>                                  mVideoIndexList;       //回波周期
    bool                                        mFalseAlarm;         //是否为虚警

    TargetNode();
    TargetNode(const zchxRadarRectDef& other, TargetNode* parentNode = 0);
    ~TargetNode();


    void setStatus(NodeStatus sts);
    QList<TargetNode*> getAllBranchLastChild();
    TargetNode* getLastChild(TargetNode* src);
    TargetNode* getLastChild();
    bool hasChildren() const;
    bool isNodePoint() const; //静止目标
    bool isNodeMoving() const;
    double getReferenceSog(bool average = true);
    double getReferenceCog(bool average = true);
    void updateSerialNum(int num);
    void setAllNodeSeriaNum(int num);
    void updateRouteNodePathStatus(NodeStatus sts);
    bool isOutput() const;
    TargetNode* topNode();
    int     getDepth();
    bool  isTopNode() const;
    void  removeChild(TargetNode* child);
    QList<uint>  getVideoIndexList();
    //获取子节点最后的更新时间
    uint getLatestChildUpdateTime();
    bool isFalseAlarm(int video_index_now);

};

typedef QMap<int, QSharedPointer<TargetNode> >      TargetNodeMap;


#endif // TARGETNODE_H
