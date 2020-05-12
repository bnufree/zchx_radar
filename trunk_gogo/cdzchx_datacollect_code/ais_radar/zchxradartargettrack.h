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
    bool                                        cog_confirmed;
    int                                         not_move_cnt;
    int                                         est_count;
    double                                      time_of_day;
    zchxRadarRectDef                            *rect;
    QList<QSharedPointer<TargetNode>>           children;

    TargetNode()
    {
        cog_confirmed = false;
        rect = 0;
        children.clear();
        est_count = 0;
        time_of_day = 0;
        not_move_cnt = 0;
    }
    TargetNode(const zchxRadarRectDef& other)
    {
        cog_confirmed = false;
        rect = new zchxRadarRectDef;
        rect->CopyFrom(other);
        children.clear();
        est_count = 0;
        time_of_day = rect->timeofday();
        not_move_cnt = 0;
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
        return (not_move_cnt >= 3) && (!hasChildren());
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
};

typedef QMap<int, QSharedPointer<TargetNode> >      TargetNodeMap;


class zchxRadarTargetTrack : public QThread
{
    Q_OBJECT
public:
    explicit    zchxRadarTargetTrack(int id, const Latlon& ll, int clear_time, double predictionWidth, bool route, QObject *parent = 0);
    void        setDirectionInvertVal(double val) {mDirectionInvertThresholdVal = val;}
    void        setTargetMergeDis(double val){mTargetMergeDis = val;}
    void        setAdjustCogEnabled(bool sts) {mAdjustCogEnabled = sts;}
    void        setRangefactor(double factor) {mRangeFactor = factor;}
public slots:
    void        appendTask(const zchxRadarRectDefList& task);
    void        process(const zchxRadarTrackTask& task);
    void        processWithPossibleRoute(const zchxRadarTrackTask& task);
    void        processWithoutRoute(const zchxRadarTrackTask& task);
protected:
    void     run();
private:
    void        splitAllRoutesIntoTargets(TargetNode* node, TargetNode* routeNode);
    void        updateConfirmedRoute(TargetNode* node, zchxRadarRectDefList& left_list);
    void        updateDetermineRoute(TargetNode* node, zchxRadarRectDefList& left_list);
    TargetNode*        checkNodeConfirmed(TargetNode* node);
    void        deleteExpiredNode();
    void        outputTargets();
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
    zchxRadarRectDefList   getDirDeterminTargets(zchxRadarRectDefList &list, zchxRadarRectDef* src, bool cog_usefull);
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
