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
struct TargetNode;

struct TargetNode
{
public:
    enum TargetStatus{
        TARGET_POINT = 0,
        TARGET_MOVING,
        TARGET_LOST,
    };

    bool                                        cog_confirmed;
    int                                         not_move_cnt;
    int                                         est_count;
    quint32                                      update_time;
    zchxRadarRectDef                            *rect;    
    TargetStatus                                 status;
    QList<QSharedPointer<TargetNode>>           children;           //孩子
    TargetNode*                                  parent;             //父亲

    TargetNode()
    {
        cog_confirmed = false;
        rect = 0;
        children.clear();
        est_count = 0;
        update_time = QDateTime::currentDateTime().toTime_t();
        not_move_cnt = 0;
        parent = 0;
        status = TARGET_POINT;
    }
    TargetNode(const zchxRadarRectDef& other, TargetNode* parentNode = 0)
    {
        cog_confirmed = false;
        rect = new zchxRadarRectDef;
        rect->CopyFrom(other);
        children.clear();
        est_count = 0;
        update_time = rect->updatetime();
        not_move_cnt = 0;
        parent = parentNode;
        status = TARGET_POINT;
        if(parentNode)
        {
            status = parent->status;
        }
    }

    void setStatus(TargetStatus sts)
    {
        status = sts;
    }

    ~TargetNode()
    {
        children.clear();
        if(rect)
        {
//            qDebug()<<"node has been delete now. node num:"<<rect->rectnumber();
            delete rect;
        }
    }
    QList<TargetNode*> getAllBranchLastChild()
    {
        QList<TargetNode*> result;
        for(int i=0; i<children.size(); i++)
        {
            TargetNode *child = children[i].data();
            if(!child) continue;
            result.append(child->getLastChild(child));
        }
        return result;
    }

    TargetNode* getLastChild(TargetNode* src)
    {
        if(src->children.size() == 0) return src;
        TargetNode *child = src->children.first().data();
        return child->getLastChild(child);
    }

    TargetNode* getLastChild()
    {
        if(children.size() == 0) return this;
        TargetNode *child = children.first().data();
        return child->getLastChild();
    }

    bool hasChildren() const
    {
        return children.size() != 0;
    }

    bool isMotionlessObj() const
    {
        return ((not_move_cnt >= 3) && (!hasChildren())) || (status == TARGET_POINT);
    }

    void motionlessMore()
    {
        not_move_cnt++;
        if(not_move_cnt >= 100) not_move_cnt = 100;
    }

    void clearMotionless()
    {
        not_move_cnt = 0;
    }

    double getReferenceSog(bool average = true)
    {
        if(!rect) return 0.0;
        double sum = rect->sog();
        if(!average) return sum;
        int num = 1;
        TargetNode* pre = parent;
        while (pre) {
            if(pre->rect)
            {
                sum += pre->rect->sog();
            }
            num++;
            if(num == 5) break;
            pre = pre->parent;
        }

        return sum / num;
    }

    double getReferenceCog(bool average = true)
    {
        if(!rect) return 0.0;
        double sum = rect->cog();
        if(!average) return sum;
        int num = 1;
        TargetNode* pre = parent;
        while (pre) {
            if(pre->rect)
            {
                sum += pre->rect->cog();
            }
            num++;
            if(num == 5) break;
            pre = pre->parent;
        }

        return sum / num;
    }

    void updateRectNum(int rect_num)
    {
        if(rect && rect->rectnumber() != rect_num) rect->set_rectnumber(rect_num);
    }

    void setAllNodeRectNum(int rect_num)
    {
        updateRectNum(rect_num);
        for(int i=0; i<children.size();i++)
        {
            TargetNode* child = children[i].data();
            while (child) {
                updateRectNum(rect_num);
                if(child->children.size() == 0) break;
                child = child->children.first().data();
            }
        }
    }

    void updateRouteNodePathStatus(TargetStatus sts)
    {
        if(this->children.size() >= 2) return;
        TargetNode *node = this;
        while (node) {
            if(node->status != sts) node->status = sts;
            if(node->children.size() == 0) break;
            node = node->children.first().data();
        }
    }
};

typedef QMap<int, QSharedPointer<TargetNode> >      TargetNodeMap;

//将预推区域和目标进行关联
struct PredictionNode{
    zchxTargetPrediction        *mPrediction;
    TargetNode*                 mNode;
};

struct AreaNodeTable{
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
    TargetNode::TargetStatus        checkRoutePathSts(QList<TargetNode*> path);
    void        splitAllRoutesIntoTargets(TargetNode* node, TargetNode* routeNode);
    void        updateConfirmedRoute(TargetNode* node, zchxRadarRectDefList& left_list);
    void        updateDetermineRoute(TargetNode* node, zchxRadarRectDefList& left_list);
    TargetNode*        checkNodeConfirmed(TargetNode* node);
    void        deleteExpiredNode();
    void        outputTargets();
    void        outputRoutePath();
    void        updateTrackPointWithNode(zchxRadarSurfaceTrack& list, TargetNode* node);
    void        updateRectMapWithNode(zchxRadarRectMap& map, TargetNode* node);

    int         getCurrentRectNum();
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
};

#endif // ZCHXRADARTARGETTRACK_H
