#ifndef ZCHXRECTEXTRACTIONTHREAD_H
#define ZCHXRECTEXTRACTIONTHREAD_H

#include <QObject>
#include "zchxfunction.h"
#include <QDateTime>
#include <QMutex>
#include "zchxradarcommon.h"
#include <QRunnable>
#include "zchxmsgcommon.h"

typedef     QList<Latlon>       zchxLatlonList;
struct      parseTarget{
    quint64     mTime;                           //时间戳.矩形目标发现的时间
    QPointF     mCenter;                         //直角中心点
    double      mArea;
    double      mLength;
    double      mWidth;
    double      mAngle;
    QPolygonF   mPolygons;                       //直角外形点列
    QPolygonF   mScaledPolygons;                 //固定输出的目标外形点列
    quint64     mID;                             //目标的ID
    double      mSpeed;                          //目标速度值 对应的是直角坐标
    double      mEstCog;                         //目标预计的运动方向
    double      mRealCog;                        //目标实际的运动方向
    bool        mRealDefined;                    //目标实际的运动方向是否确定

    parseTarget()
    {
        mRealDefined = false;
    }
};

typedef QList<parseTarget>          parseTargetList;

class zchxRadarRectExtraction : public QObject
{
    Q_OBJECT
public:
    explicit zchxRadarRectExtraction(double lat, double lon, int id,  QObject *parent = 0);
    ~zchxRadarRectExtraction();
    void  setRadarLL(double lat, double lon);
    void  setTargetAreaRange(double min, double max);
    void  setTargetLenthRange(double min, double max);
    void  setRangeFactor(double factor) {mRangeFactor = factor;}
    void  parseVideoPieceFromImage(QImage& result, zchxRadarRectDefList& list, const QImage &img, double range_factor, int video_index, bool output = false);
    void  setFilterAreaData(const QList<zchxMsg::filterArea>& list);
    void  setFilterAreaEnabled(bool sts);
protected:
    void  transferLatlonArea2PixelArea( bool lock);
    bool  isVideoPolygonNotAvailable(const QPolygonF& poly);
    void  trackTarget(QList<parseTarget>& list, double target_merge_distance, bool merge_target = true);
    void  mergeTargetInDistance(QList<parseTarget> &list, double target_merge_distance);
    bool  getMergeTarget(parseTarget& target, const QList<parseTarget> &list, bool area_weight = true);
    double calCog(const QPointF& start, const QPointF& end);
    bool    isTargetDirectStable(double &avg_cog, const parseTargetList& list, int min_target_num);

private:
    //雷达中心点的经纬度
    double                  mCentreLon;
    double                  mCentreLat;

    //限制区域
    bool                            mIsFilterAreaEnabled;              //是否设置限制区域
    QList<zchxMsg::filterArea>      mFilterAreaLatlonList;            //原生的过滤数据
    QList<QPolygonF>                mInFilterAreaPixelList;            //目标必须在这些区域内
    QList<QPolygonF>                mOutFilterAreaPixelList;            //目标必须在这些区域外
    QMutex                          mFilterAreaMutex;

    //回波块区域大小识别区间
    double                  mMaxTargetArea;                //参考标准是1024*1024的图片大小
    double                  mMinTargetArea;
    double                  mMaxTargetLength;               //回波图目标的长度..单位米
    double                  mMinTargetLength;
    //
    double                  mRangeFactor;                   //当前回波的单元长度
    int                     mImageWidth;
    int                     mImageHeight;
};

#endif // ZCHXRECTEXTRACTIONTHREAD_H
