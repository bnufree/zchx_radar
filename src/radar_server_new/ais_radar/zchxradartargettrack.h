#ifndef ZCHXRADARTARGETTRACK_H
#define ZCHXRADARTARGETTRACK_H

#include <QThread>
#include <QSharedPointer>
#include "zchxradarcommon.h"
#include <QMutex>
#include "zchxfunction.h"
#include "radardatautils.h"
#include "targetnode.h"

typedef     zchxRadarRectDefList        zchxRadarTrackTask;
typedef     QList<zchxRadarTrackTask>   zchxRadarTrackTaskList;


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

#define     TARGET_CONFIRM_NODE_COUNT         5

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

    explicit    zchxRadarTargetTrack(int id, const Latlon& ll,
                                     int clear_time, double predictionWidth,
                                     bool route, bool output_silent_node, double min_target_speed,
                                     int target_confirm_counter, QObject *parent = 0);
    void        setDirectionInvertVal(double val) {mDirectionInvertThresholdVal = val;}
    void        setTargetMergeDis(double val){mTargetMergeDis = val;}
    void        setAdjustCogEnabled(bool sts) {mAdjustCogEnabled = sts;}
    void        setRangefactor(double factor) {mRangeFactor = factor;}
    void        setMaxSpeed(double speed) {mMaxSpeed = speed;}
    void        setScanTime(double secs) {mScanTime = secs;}
    void        setIsTargetPrediction(bool sts) {mIsTargetPrediction = sts;}
    void        setIsTargetGapCheck(bool sts) {mIsCheckTargetGap = sts;}
public slots:
    void        appendTask(const zchxRadarRectDefList& task);
    void        process(const zchxRadarTrackTask& task);
    void        processWithPossibleRoute(const zchxRadarTrackTask& task);
//    void        processWithPossibleRoute2(const zchxRadarTrackTask& task);
    void        processWithoutRoute(const zchxRadarTrackTask& task);
    void        appendUserDefObj(const UserSpecifiedObj& obj);
    void        removeUserDefObj(const UserSpecifiedObj& obj);
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
    void        updateTrackPointWithNode(zchxRadarSurfaceTrack& list, TargetNode* node, int* silent_num = 0);
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
    int                         mRadarID;
    int                         mMinNum;
    int                         mMaxNum;
    int                         mClearTrackTime;
    Latlon                      mCenter;
    TargetNodeMap               mTargetNodeMap;                     //保存目标还未定性的点
    bool                        mProcessWithRoute;
    int                         mMaxEstCount;                       //目标的最大预推次数
    double                      mRangeFactor;
    double                      mPredictionWidth;                   //预推区域的宽度
    QMap<int, UserSpecifiedObj>     mUserDefObj;                    //
    //目标是否预推更新
    bool                        mIsTargetPrediction;                //是否进行预推
    int                         mTargetPredictionInterval;           //预推周期 比如2个周期更新一次
    bool                        mIsCheckTargetGap;                  //是否检查目标之间的距离
    //目标允许的最大速度和雷达的扫描周期
    double                      mMaxSpeed;
    double                      mScanTime;                           //最大速度(m/s)  时间周期秒
    //目标确认需要的周期数
    int                         mTargetConfirmCounter;              //默认为5个周期
    bool                        mOutputSilentPoint;                 //是否输出点目标
    double                      mOutputTargetMinSpeed;              //运动目标的输出时的最低速度
};

#endif // ZCHXRADARTARGETTRACK_H
