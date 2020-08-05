#ifndef ZCHXRADARTARGETTRACK_H
#define ZCHXRADARTARGETTRACK_H

#include <QThread>
#include <QSharedPointer>
#include "zchxradarcommon.h"
#include <QMutex>
#include "zchxfunction.h"

typedef     zchxRadarRectDefList        zchxRadarTrackTask;
typedef     QList<zchxRadarTrackTask>   zchxRadarTrackTaskList;

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
    NodeStatus                                         mStatus;
    int                                         mUpdateTime;        //目标最新的更新时间
    zchxRadarRectDef                            *mDefRect;
    QList<QSharedPointer<TargetNode>>           mChildren;           //孩子
    TargetNode*                                 mParent;             //父亲

    TargetNode()
    {
        mDefRect = 0;
        mChildren.clear();
        mUpdateTime = QDateTime::currentDateTime().toTime_t();
        mParent = 0;
        mStatus = Node_UnDef;
        mSerialNum = 0;
    }
    TargetNode(const zchxRadarRectDef& other, TargetNode* parentNode = 0)
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
    }

    void setStatus(NodeStatus sts)
    {
        mStatus = sts;
    }

    ~TargetNode()
    {
        mChildren.clear();
        if(mDefRect)
        {
//            qDebug()<<"node has been delete now. node num:"<<rect->rectnumber();
            delete mDefRect;
        }
    }
    QList<TargetNode*> getAllBranchLastChild()
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

    TargetNode* getLastChild(TargetNode* src)
    {
        if(src->mChildren.size() == 0) return src;
        TargetNode *child = src->mChildren.first().data();
        return child->getLastChild(child);
    }

    TargetNode* getLastChild()
    {
        if(mChildren.size() == 0) return this;
        TargetNode *child = mChildren.first().data();
        return child->getLastChild();
    }

    bool hasChildren() const
    {
        return mChildren.size() != 0;
    }

    bool isNodePoint() const
    {
        return mStatus == Node_UnDef && mChildren.size() <= 1;
    }

    bool isNodeMoving() const
    {
        return mStatus == Node_Moving;
    }

    double getReferenceSog(bool average = true)
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

    double getReferenceCog(bool average = true)
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

    void updateSerialNum(int num)
    {
        if(mSerialNum != num) mSerialNum = num;
    }

    void setAllNodeSeriaNum(int num)
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

    void updateRouteNodePathStatus(NodeStatus sts)
    {
        if(this->mChildren.size() >= 2) return;
        TargetNode *node = this;
        while (node) {
            if(node->mStatus != sts) node->mStatus = sts;
            if(node->mChildren.size() == 0) break;
            node = node->mChildren.first().data();
        }
    }

    bool isOutput() const
    {
        return isNodeMoving() || isNodePoint();
    }

    TargetNode* topNode()
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


    int     getDepth()
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
};

typedef QMap<int, QSharedPointer<TargetNode> >      TargetNodeMap;

//将预推区域和目标进行关联
struct PredictionNode{
    zchxTargetPrediction        *mPrediction;
    TargetNode*                 mNode;
};

struct AreaNodeTable{
    QString             mType;
    QPolygonF           mArea;
    QList<TargetNode*>  mNodeList;
    zchxRadarRectDefList        mRectList;          //落入这个区域的所有目标
};


class zchxRadarTargetTrack : public QThread
{
    Q_OBJECT
public:
    enum        TargetTrackModel{
        Model_None = 0,
        Model_Cross,
        Model_Metting,
        Model_Overtake,
    };

    explicit    zchxRadarTargetTrack(int id, const Latlon& ll, int clear_time, double predictionWidth, bool route, QObject *parent = 0);
    void        setDirectionInvertVal(double val) {mDirectionInvertThresholdVal = val;}
    void        setTargetMergeDis(double val){mTargetMergeDis = val;}
    void        setAdjustCogEnabled(bool sts) {mAdjustCogEnabled = sts;}
    void        setRangefactor(double factor) {mRangeFactor = factor;}
public slots:
    void        appendTask(const zchxRadarRectDefList& task);
    void        process(const zchxRadarTrackTask& task);
    void        processWithPossibleRoute(const zchxRadarTrackTask& task);
//    void        processWithPossibleRoute2(const zchxRadarTrackTask& task);
    void        processWithoutRoute(const zchxRadarTrackTask& task);
protected:
    void     run();
private:
    QList<AreaNodeTable>  calculateTargetTrackMode(double max_speed, quint32 now, double scan_time);
    NodeStatus        checkRoutePathSts(QList<TargetNode*>& newRoueNodeList, const QList<TargetNode*>& path);
    void        splitAllRoutesIntoTargets(TargetNode* node, TargetNode* routeNode);
    void        updateConfirmedRoute(TargetNode* node, zchxRadarRectDefList& left_list);
    void        updateDetermineRoute(TargetNode* node, zchxRadarRectDefList& left_list);
    TargetNode*        checkNodeConfirmed(TargetNode* node);
    void        deleteExpiredNode();
    void        outputTargets();
    void        outputRoutePath();
    void        updateTrackPointWithNode(zchxRadarSurfaceTrack& list, TargetNode* node);
    void        updateRectMapWithNode(zchxRadarRectMap& map, TargetNode* node);

    int         getCurrentNodeNum();
    void        appendNode(TargetNode* node, int source);
    bool        getTask(zchxRadarTrackTask& task);    
    void        mergeRectTargetInDistance(zchxRadarTrackTask &temp_list, int target_merge_distance);
    Latlon      getMergeTargetLL(const zchxRadarRectDefList &list);
    void        changeTargetLL(const Latlon &ll, zchxRadarRectDef &cur);
    bool        isDirectionChange(double src, double target);
    double      calAvgCog(const zchxRadarRectDefList &list);
    int         getTargetDirectStatus(const zchxRadarRect& rect, int check_point_num, double *avg_cog = 0);
    bool        isTargetJumping(const zchxRadarRect& rect, double merge_dis, int jump_target_num);
    void        dumpTargetDistance(const QString &tag, double merge_dis);
    void        checkTargetRectAfterUpdate(double merge_dis);
    zchxRadarRectDefList   getDirDeterminTargets(bool &isTargetInVideo, zchxRadarRectDefList &list, zchxRadarRectDef* src, bool cog_usefull);
    bool        isRectAreaContainsPoint(const zchxRadarRectDef& rect, double lat, double lon);
#if 0
    void        makePredictionArea(zchxRadarRectDef* rect, double width, double delta_time = 10.0);
    bool        isPointInPredictionArea(zchxRadarRectDef* src, double lat, double lon);
    bool        isPointInPredictionArea(zchxRadarRectDef* src, Latlon ll);
    bool        isPointInPredictionArea(zchxRadarRectDef* src, zchxRadarRectDef* dest);
#endif

signals:
    void        signalSendTracks(const zchxRadarSurfaceTrack& track);
    void        signalSendRectData(const zchxRadarRectMap& map);
    void        signalSendRoutePath(const zchxRadarRouteNodes& list);
public slots:

private:
    zchxRadarTrackTaskList      mTaskList;
    QMutex                      mMutex;
    zchxRadarRectMap            mRadarRectMap;//用于发送的回波矩形MAP
    double                      mDirectionInvertThresholdVal;       //目标运动反向的阈值
    double                      mTargetMergeDis;                    //目标合并的距离值
    bool                        mAdjustCogEnabled;
    int                         mRectNum;
    int                         mMinNum;
    int                         mMaxNum;
    int                         mClearTrackTime;
    Latlon                      mCenter;
    TargetNodeMap               mTargetNodeMap;                     //保存目标还未定性的点
    bool                        mProcessWithRoute;
    int                         mMaxEstCount;                       //目标的最大预推次数
    double                      mRangeFactor;
    double                      mPredictionWidth;                   //预推区域的宽度
    QMap<QString, int>          mSameRectNodeCounter;               //同一个矩形区域上两个回波点在一起的统计 如果超过5次  则认为是一个 删除其中一个 key 就是num——num
};

#endif // ZCHXRADARTARGETTRACK_H
