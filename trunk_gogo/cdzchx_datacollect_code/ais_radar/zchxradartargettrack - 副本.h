#ifndef ZCHXRADARTARGETTRACK_H
#define ZCHXRADARTARGETTRACK_H

#include <QThread>
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
struct TargetNode
{
public:
    bool                        cog_confirmed;
    zchxRadarRectDef            *rect;
    QList<TargetNode*>          children;

    TargetNode() {cog_confirmed = false; rect = 0; children.clear();}
    TargetNode(const zchxRadarRectDef& other) {cog_confirmed = false; rect = new zchxRadarRectDef; rect->CopyFrom(other); children.clear();}
    QList<TargetNode*> getAllBranchLastChild()
    {
        QList<TargetNode*> result;
        for(int i=0; i<children.size(); i++)
        {
            TargetNode *child = children[i];
            if(!child) continue;
            result.append(child->getLastChild(child));
        }
        return result;
    }

    TargetNode* getLastChild(TargetNode* src)
    {
        if(src->children.size() == 0) return src;
        else return src->children[0];
    }
};

typedef QMap<int, TargetNode*>      TargetNodeMap;


class zchxRadarTargetTrack : public QThread
{
    Q_OBJECT
public:
    explicit    zchxRadarTargetTrack(int id, const Latlon& ll, int clear_time, QObject *parent = 0);
    void        setDirectionInvertVal(double val) {mDirectionInvertThresholdVal = val;}
    void        setTargetMergeDis(double val){mTargetMergeDis = val;}
    void        setAdjustCogEnabled(bool sts) {mAdjustCogEnabled = sts;}
public slots:
    void        appendTask(const zchxRadarRectDefList& task);
    void        process(const zchxRadarTrackTask& task);
    void        processWithTargetNode(const zchxRadarTrackTask& task);
    void        updateConfirmedTarget(const zchxRadarTrackTask& task, zchxRadarRectDefList& left_list);
protected:
    void     run();
private:
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
};

#endif // ZCHXRADARTARGETTRACK_H
