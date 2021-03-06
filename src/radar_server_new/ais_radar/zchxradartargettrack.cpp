﻿#include "zchxradartargettrack.h"
#include <QDebug>
#include <QGeoCoordinate>

const bool track_debug = true;

const double point_near_line = 200;
const double ship_max_speed = 5.0;            //5m/s ~~ 10节  10m/s 就是20节


#define     DEBUG_TRACK_INFO                0


//double getDeltaTime(float now, float old)
//{
//    double delta = now - old;
//    if(delta < 0)
//    {
//        delta += 3600 * 24;
//    }
//    return delta;
//}
extern bool output_route_path;
extern bool restart_simulate;
extern int  target_silent_confirm_counter;
extern bool output_silent_node;

QPolygonF  predictionAreaLL2PolygonF(const com::zhichenhaixin::proto::PredictionArea& area)
{
    QPolygonF poly;
    for(int i=0; i<area.area_size(); i++)
    {
        com::zhichenhaixin::proto::Latlon pnt = area.area(i);
        poly.append(latlonToMercator(pnt.latitude(), pnt.longitude()).toPointF());
    }

    return poly;
}

zchxRadarTargetTrack::zchxRadarTargetTrack(int id, const Latlon& ll,
                                           int clear_time, double predictionWidth,
                                           bool route, bool output_silent_node,
                                           double min_target_speed,
                                           int target_confirm_counter, QObject *parent)
    : QThread(parent)
    , mRadarID(id)
    , mCenter(ll)
    , mClearTrackTime(clear_time)
    , mProcessWithRoute(route)
    , mMaxEstCount(5)
    , mRangeFactor(10)
    , mPredictionWidth(predictionWidth)
    , mTargetPredictionInterval(2)
    , mIsTargetPrediction(false)
    , mIsCheckTargetGap(false)
    , mMaxSpeed(5)
    , mScanTime(3.0)
    , mTargetConfirmCounter(target_confirm_counter)
    , mOutputSilentPoint(output_silent_node)
    , mOutputTargetMinSpeed(min_target_speed)
{
    mDirectionInvertThresholdVal = 60.0;
    mTargetMergeDis = 100.0;
    mAdjustCogEnabled = true;
    mMinNum = 1+id*10000;
    mMaxNum = mMinNum + 9998;
    mRectNum = mMinNum;
    target_silent_confirm_counter = mTargetConfirmCounter;
    qRegisterMetaType<zchxRadarSurfaceTrack>("const zchxRadarSurfaceTrack&");
}

void zchxRadarTargetTrack::appendTask(const zchxRadarRectDefList &task)
{
    QMutexLocker locker(&mMutex);
    mTaskList.append(task);
}

bool zchxRadarTargetTrack::getTask(zchxRadarTrackTask &task)
{
    QMutexLocker locker(&mMutex);
    if(mTaskList.size() == 0) return false;
    task = mTaskList.takeLast();
    if(mTaskList.size() > 0)
    {
        qDebug()<<"delete unprocessed task size:"<<mTaskList.size();
         mTaskList.clear();
    }

}

void zchxRadarTargetTrack::run()
{
    while (true) {
        //获取当前的任务
        zchxRadarTrackTask task;
        if(!getTask(task))
        {
            msleep(1000);
            continue;
        }
        //开始进行处理
        process(task);
    }
}

void zchxRadarTargetTrack::mergeRectTargetInDistance(zchxRadarTrackTask &temp_list, int target_merge_distance)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    //目标预处理.将距离太近的目标进行合并,合并的中心点取二者的重点
    for(int i=0; i<temp_list.size();)
    {
        zchxRadarRectDef &cur = temp_list[i];
        //获取和当前目标距离较近的目标,目标可能多个,这里暂且设定为list,
        zchxRadarRectDefList merge_list;
        for(int k = i+1; k<temp_list.size();)
        {
            zchxRadarRectDef next = temp_list[k];
            //计算两者的距离
            double distance = getDisDeg(cur.center().latitude(), cur.center().longitude(),
                                        next.center().latitude(), next.center().longitude());
            if(distance < target_merge_distance)
            {
                //目标存在,从原来的队列里边删除
                merge_list.append(next);
                temp_list.removeAt(k);
                continue;
            }
            k++;
        }
        if(merge_list.size() == 0)
        {
            i++;
            continue;
        }
        //合并目标
        merge_list.append(cur);
        Latlon ll = getMergeTargetLL(merge_list);
        changeTargetLL(ll, cur);
    }
}

Latlon zchxRadarTargetTrack::getMergeTargetLL(const zchxRadarRectDefList &list)
{
    double sum_x = 0, sum_y = 0;
    foreach (zchxRadarRectDef temp, list) {
        sum_x += temp.center().longitude();
        sum_y += temp.center().latitude();
    }
    if(list.size() > 0)
    {
        sum_x = sum_x / list.size();
        sum_y = sum_y / list.size();
    }

    return Latlon(sum_y, sum_x);
}

void zchxRadarTargetTrack::changeTargetLL(const Latlon &ll, zchxRadarRectDef &cur)
{
    double dlon = ll.lon - cur.center().longitude();
    double dlat = ll.lat - cur.center().latitude();
    cur.mutable_center()->set_longitude(ll.lon);
    cur.mutable_center()->set_latitude(ll.lat);
    //将目标的所有经纬度坐标进行同步的变换
    cur.mutable_boundrect()->mutable_topleft()->set_latitude(cur.boundrect().topleft().latitude() + dlat);
    cur.mutable_boundrect()->mutable_topleft()->set_longitude(cur.boundrect().topleft().longitude() + dlon);
    cur.mutable_boundrect()->mutable_bottomright()->set_latitude(cur.boundrect().bottomright().latitude() + dlat);
    cur.mutable_boundrect()->mutable_bottomright()->set_longitude(cur.boundrect().bottomright().longitude() + dlon);
    for(int i=0; i<cur.outline_size(); i++)
    {
        zchxLatlon *ll = cur.mutable_outline(i);
        ll->set_latitude(ll->latitude() + dlat);
        ll->set_longitude(ll->longitude() + dlon);
    }
    if(cur.has_seg())
    {
        cur.mutable_seg()->mutable_start()->set_latitude(cur.seg().start().latitude() + dlat);
        cur.mutable_seg()->mutable_start()->set_longitude(cur.seg().start().longitude() + dlon);
        cur.mutable_seg()->mutable_end()->set_latitude(cur.seg().end().latitude() + dlat);
        cur.mutable_seg()->mutable_end()->set_longitude(cur.seg().end().longitude() + dlon);
    }
}

bool zchxRadarTargetTrack::isDirectionChange(double src, double target)
{
    //计算原角度对应的相反角度的范围,反映到0-360的范围
    int min = int(src - mDirectionInvertThresholdVal);
    int max = int(src + mDirectionInvertThresholdVal);
    if(min < 0)
    {
        //原方向指向右上方向,则有效值的范围是min_+360 ~ 360, 0~max
        if((target >= 0  && target < max) || (target > min+360)) return false;
        return true;
    } else if(max > 360)
    {
        //原方向指向左上方向,则有效值的范围是min ~ 360, 0~max-360
        if((target >= 0  && target < max-360) || (target > min)) return false;
        return true;
    } else
    {
        //最大或者最小都在0-360的范围内
        if(target > min && target < max) return false;
        return true;
    }
    return false;
}

//这里的矩形块数据的时间越靠前就排在最后, 也就是说起点在最后
double zchxRadarTargetTrack::calAvgCog(const zchxRadarRectDefList &list)
{
    double angle  = 0.0;
    QList<double>    cogList;
#if 1
    if(list.size() > 0)
    {
        angle = list[0].cog();
        cogList.append(angle);
        for(int i=1; i<=list.size()-1; i++)
        {
            if(i == 3) break;
            cogList.append(list[i].cog());
            double cur_angle = list[i].cog();
            double min_angle = cur_angle < angle ? cur_angle : angle;
            double max_angle = cur_angle > angle ? cur_angle : angle;
            double sub_angle = fabs(cur_angle - angle);
            //计算这两个角的角平分线对应的角度作为下一次合成的角度
            if(sub_angle > 180.0)
            {
                sub_angle = 360 - sub_angle;  //取夹角
                angle = max_angle + 0.5 * sub_angle;
            } else
            {
                angle = min_angle + 0.5 * sub_angle;
            }

        }
        if(track_debug) qDebug()<<"src cog list:"<<cogList<<" res cog:"<<angle;
    }

#else
    //从起点为原点,计算各个轨迹点对应的直角坐标系的坐标
    QList<Mercator>  pntsList;
    QList<double>    calCogList;

    QGeoCoordinate p1(list.last().center().latitude(), list.last().center().longitude());
    for(int i=list.size()-1; i>=0; i--)
    {
        double cur_lat = list[i].center().latitude();
        double cur_lon = list[i].center().longitude();
        //转换为直角坐标
        Mercator cat = latlonToMercator(Latlon(cur_lat, cur_lon));
        pntsList.append(cat);
        cogList.append(list[i].cog());
        if(i == list.size() - 1) continue;
        QGeoCoordinate p2(list[i].center().latitude(), list[i].center().longitude());
        calCogList.append(p1.azimuthTo(p2));
    }
    //按起点进行归化处理, 起点就是坐标原点(0, 0), 所有向量合成以后的方向就是目标的平均方向
    Mercator start_pos = pntsList.first();
    double sum_x = 0, sum_y = 0;
    for(int i=0; i<pntsList.size(); i++)
    {
        Mercator& cur = pntsList[i];
        cur.mX = cur.mX + start_pos.mX * (-1);
        cur.mY = cur.mY + start_pos.mY * (-1);
        sum_x += cur.mX;
        sum_y += cur.mY;
    }
    //转换成角度顺时针
    angle = atan2(sum_y * (-1), sum_x) * 180 / GLOB_PI ;
    //转换为正北方向和顺时针
    angle -= 270.0;
    while(angle >= 360 ) angle -= 360;
    while(angle < 0) angle += 360;

    if(track_debug) qDebug()<<"src cog list:"<<cogList<<"cal cog list:"<<calCogList<<" res cog:"<<angle;
#endif
    return angle;

}

//通过使用5个点进行判断
int zchxRadarTargetTrack::getTargetDirectStatus(const zchxRadarRect& rect, int check_point_num, double *avg_cog)
{
    zchxRadarRectDefList list;
    list.append(rect.current_rect());
    for(int i=0; i<rect.history_rect_list_size()-1; i++) //不取第一个点,因为第一个点的方向未知
    {
        list.append(rect.history_rect_list(i));
        if(list.size() == check_point_num) break;
    }
    if(list.size() < check_point_num) return TARGET_DIRECTION_UNDEF;
    double pre_cog = list.last().cog();
    int same_num = 0, diff_num = 0;
    QList<double> coglist;
    coglist.append(pre_cog);
    for(int i=list.size()-2; i>=0; i--)
    {
        double cur_cog = list[i].cog();
        if(isDirectionChange(pre_cog, cur_cog))
        {
            diff_num++;
            break;

        } else
        {
            //在同一个方向
            same_num++;
        }
        pre_cog = cur_cog;
    }
    if(diff_num)
    {
        return TARGET_DIRECTION_UNSTABLE;
    }
    if(avg_cog)
    {
        *avg_cog = calAvgCog(list);
    }
    return TARGET_DIRECTION_STABLE;

}

//判断目标当前是否是在一定范围内跳动,方法
//1)目标的点数超过3个点
//2)连续点的方向不统一,每一个目标距离判定的起始点的距离都在合并的目标距离范围之内.
bool zchxRadarTargetTrack::isTargetJumping(const zchxRadarRect& rect, double merge_dis, int jump_target_num)
{
//    //目标已经连续,且方向统一,肯定不再跳跃
//    if(isTargetDirectStable(rect, jump_target_num, NULL)) return false;
    //目标的方向不在同一方向,计算目标到起始点的距离
    zchxRadarRectDefList list;
    list.append(rect.current_rect());
    for(int i=0; i<rect.history_rect_list_size(); i++)
    {
        list.append(rect.history_rect_list(i));
        if(list.size() == jump_target_num) break;
    }
    if(list.size() < jump_target_num) return false;
    //存在不一致的情况
    //检查距离是不是距离起点都很近,很近就认为目标在乱跳
    double start_lat = list.last().center().latitude();
    double start_lon = list.last().center().longitude();
    bool same_target = true;
    for(int i=list.size()-2; i>= 0; i--)
    {
        double cur_lat = list[i].center().latitude();
        double cur_lon = list[i].center().longitude();
        double distance = getDisDeg(start_lat, start_lon, cur_lat, cur_lon);
        if(distance > merge_dis)
        {
            same_target = false;
            break;
        }
    }
    if(same_target) return true;
    return false;
}

#define         ANGLE_IN_180
#if 0
bool zchxRadarTargetTrack::isPointInPredictionArea(zchxRadarRectDef *src, double lat, double lon)
{
    return isPointInPredictionArea(src, Latlon(lat, lon));
}

bool zchxRadarTargetTrack::isPointInPredictionArea(zchxRadarRectDef *src, Latlon ll)
{
    if(!src || src->predictionareas_size() == 0) return false;
    for(int i=0; i<src->predictionareas_size(); i++)
    {

        QPolygonF poly;
        com::zhichenhaixin::proto::predictionArea area = src->predictionareas(i);
        for(int k=0; k<area.area_size(); k++)
        {

            double lat = area.area(k).latitude();
            double lon = area.area(k).longitude();
            poly.append(latlonToMercator(lat, lon).toPointF());
        }
        bool sts = poly.containsPoint(latlonToMercator(ll).toPointF(), Qt::OddEvenFill);
        if(sts) return true;
    }

    return false;
}

bool zchxRadarTargetTrack::isPointInPredictionArea(zchxRadarRectDef *src, zchxRadarRectDef *dest)
{
    if(!src || !dest) return false;
    return isPointInPredictionArea(src, dest->centerlatitude(), dest->centerlongitude());
}

void zchxRadarTargetTrack::makePredictionArea(zchxRadarRectDef *rect,  double width, double delta_time)
{
    if(!rect) return;
    if(rect->sog() < 0.1) return;
    //计算当前目标可能的预估位置
    QGeoCoordinate cur(rect->centerlatitude(), rect->centerlongitude());
    float est_distance = rect->sog() * delta_time;
    QGeoCoordinate dest = cur.atDistanceAndAzimuth(est_distance, rect->cog());
    //构造预估区域
    zchxTargetPredictionLine line(rect->centerlatitude(), rect->centerlongitude(),  dest.latitude(), dest.longitude(), width, Prediction_Area_Rectangle);
    if(!line.isValid()) return;
    QList<Latlon> list = line.getPredictionArea();
    if(list.size() == 0) return;
    com::zhichenhaixin::proto::predictionArea* area = rect->add_predictionareas();
    for(int i=0; i<list.size(); i++)
    {
        zchxSingleVideoBlock *pos = area->add_area();
        pos->set_latitude(list[i].lat);
        pos->set_longitude(list[i].lon);
    }

}
#endif

zchxRadarRectDefList zchxRadarTargetTrack::getDirDeterminTargets(bool& isTargetInVideo, zchxRadarRectDefList &list, zchxRadarRectDef* src, bool cog_usefull)
{
    isTargetInVideo = false;
    zchxRadarRectDefList result;
    if(list.size() == 0 || !src) return result;
    quint32 list_time = list.first().updatetime();
    quint32 old_time = src->updatetime();
    double old_sog = src->sogms();
    double old_lat = src->center().latitude();
    double old_lon = src->center().longitude();
    double old_cog = src->cog();
    //计算当前的时间间隔, 根据时间间隔算预估可能移动距离
    quint32 delta_time = list_time - old_time;
    if(delta_time < 1)
    {
        qDebug()<<"abnormal delta time found now."<<src->rectnumber();
        return result;
    }
    if(cog_usefull)
    {
        //计算当前目标可能的预估位置
        double est_lat = 0.0, est_lon = 0.0;
        //计算目标的最合理位置
        if(old_sog < 0.001) old_sog = 0.5 * ship_max_speed;
        float est_distance = old_sog * delta_time;
        QGeoCoordinate est_geo = QGeoCoordinate(old_lat, old_lon).atDistanceAndAzimuth(est_distance, old_cog);
        est_lat = est_geo.latitude();
        est_lon = est_geo.longitude();
#if 0
        //看看这两个计算的经纬度的差别,使用这个计算的经纬度导致数据有偏差
        double temp_lat = 0.0, temp_lon = 0.0;
        distbearTolatlon1(old_lat, old_lon, est_distance, old_cog, &temp_lat, &temp_lon);
#endif
        Mercator est_pos = latlonToMercator(est_lat, est_lon);
        //将目标的预估范围扩大最大允许的船舶速度,防止目标速度突然变大了的情况
        double cur_max_speed = old_sog * 2;
        if(cur_max_speed > ship_max_speed) cur_max_speed = ship_max_speed;
        est_distance = cur_max_speed * delta_time;
        est_geo = QGeoCoordinate(old_lat, old_lon).atDistanceAndAzimuth(est_distance, old_cog);
                est_lat = est_geo.latitude();
                est_lon = est_geo.longitude();
//        distbearTolatlon1(old_lat, old_lon, est_distance, old_cog, &est_lat, &est_lon);
        //预估点和前一位置连线，若当前点在连线附近，则认为是下一个点。点存在多个，则取预估位置距离最近的点。
        zchxTargetPrediction line(old_lat, old_lon, est_lat, est_lon, mPredictionWidth, 0.2);
        if(!line.isValid()) return result;
        //将目标的预估范围更新到地图上显示,检查预估范围的计算是否有错误
        if(1)
        {
            src->clear_prediction();
            QList<Latlon> area = line.getPredictionAreaLL();
            foreach (Latlon ll, area) {
                com::zhichenhaixin::proto::Latlon* block = src->mutable_prediction()->add_area();
                block->set_latitude(ll.lat);
                block->set_longitude(ll.lon);
            }
        }
        //从最新的目标矩形框中寻找预估位置附件的点列,将与目标方位偏离最小的点作为最终的点
        double est_target_index = -1;
        double min_distance = INT64_MAX;
        for(int k = 0; k<list.size(); k++)
        {
            zchxRadarRectDef next = list[k];
            Mercator now = latlonToMercator(next.center().latitude(), next.center().longitude());
            //检查是否在连线的范围内
            if(!line.isPointIn(now)) continue;            
            //计算点到预估位置的距离
            double distance = now.distanceToPoint(est_pos);
            if(distance / delta_time > ship_max_speed) continue;
            if(min_distance > distance)
            {
                est_target_index = k;
                min_distance = distance;
            }
        }
        //检查目标是否已经找到
        if(est_target_index > 0)
        {
            //目标已经找到,将目标从原来的矩形队列删除
            result.append(list.takeAt(est_target_index));
        }
    } else
    {
        double max_distance = delta_time * ship_max_speed;
        qDebug()<<" now update root in max distance:"<<max_distance<<" grid distance:"<<mRangeFactor<<"delta_time:"<<delta_time<<" max_ship_speed:"<<ship_max_speed<<src->rectnumber();
        for(int k = 0; k<list.size();)
        {
            zchxRadarRectDef next = list[k];
            Mercator now = latlonToMercator(next.center().latitude(), next.center().longitude());
            //计算待确定点到目标旧的位置点的距离.
            //1)如果超出来最大运行距离,则跳过不处理
            double distance = now.distanceToPoint(latlonToMercator(old_lat, old_lon));
            if(distance > max_distance)
            {
                k++;
                continue;
            }
            //2)目标要么没有移动,要么超出一个距离单元
            if(distance < 1.0 || distance >= mRangeFactor)
            {
                //符合要求,当前回波矩形块从原始队列删除,继续寻找下一个可能的块
                qDebug()<<" child of root found now. distance to root is:"<<distance;
                result.append(list.takeAt(k));
                continue;
            }
            k++;
        }        
        qDebug()<<"update root"<<src->rectnumber()<<"end with result size:"<<result.size();
    }
    if(result.size() == 0)
    {
        //如果当前目标的块没有找到,看看目标是否处在一个回波块的区域内.
        //出现这种情况的原因是因为回波块的形状等发生变化,导致回波块的
        //质心位置变化不规则,导致出现了以下的情况:
        //1)目标距离新回波质心的距离超出了最大的可能运动距离
        //2)质心位置没有在目标的预推区域内
        //这样就暂且认为目标的位置没有发生变化,把这个新的回波块从原始队列中删除
        //以此来避免回波显示中一个回波出现两个目标的情况
        for(int i=0; i<list.size();i++)
        {
            zchxRadarRectDef next = list[i];
            if(isRectAreaContainsPoint(next, old_lat, old_lon))
            {
                isTargetInVideo = true;
                list.takeAt(i);
#if 1
                double dir = Mercator::angle(old_lat, old_lon, next.center().latitude(), next.center().longitude());
                //目标的方向已经有了,也就是目标也不是初次出现,需要检查方向.
                if(cog_usefull && isDirectionChange(old_cog, dir)) break;
                //再次检查速度,速度不能超过目标的最大速度,如果超过了,就将位置更新到最大速度的位置
                Mercator now = latlonToMercator(next.center().latitude(), next.center().longitude());
                //计算点到预估位置的距离
                double distance = now.distanceToPoint(old_lat, old_lon);
                if(distance / delta_time > ship_max_speed)
                {
                    //更新next的经纬度
                    distance = delta_time * ship_max_speed;
                    QGeoCoordinate est_pos = QGeoCoordinate(old_lat, old_lon).atDistanceAndAzimuth(distance, dir);
                    next.mutable_center()->set_latitude(est_pos.latitude());
                    next.mutable_center()->set_longitude(est_pos.longitude());
                }

                //添加到队列中
                result.append(next);
#endif
                break;
            }
        }
    }

    qDebug()<<__FUNCTION__<<"target size:"<<result.size()<<" in video:"<<isTargetInVideo;

    return result;
}

bool zchxRadarTargetTrack::isRectAreaContainsPoint(const zchxRadarRectDef &rect, double lat, double lon)
{
    bool sts = false;
    QPolygonF poly;
    for(int i=0; i<rect.outline_size();i++)
    {
        zchxLatlon block = rect.outline(i);
        //将经纬度转换成墨卡托点列
        Mercator m = latlonToMercator(block.latitude(), block.longitude());
        poly.append(m.toPointF());
    }
    if(poly.size() > 0)
    {
        sts = poly.containsPoint(latlonToMercator(lat, lon).toPointF(), Qt::OddEvenFill);
    }
    return sts;
}

void zchxRadarTargetTrack::updateConfirmedRoute(TargetNode* topNode, zchxRadarRectDefList& left_list)
{
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(!topNode || topNode->mStatus == Node_UnDef) return;
    //当前的时间确认
    if(left_list.size() == 0) return;
    quint32 list_time = left_list.first().updatetime();
    //获取最后一次更新的节点
    TargetNode *last_update_node = topNode->getLastChild();
    //没有路径节点的情况
    if(!last_update_node || last_update_node == topNode) return;


    //根据已知的方向和速度预测目标的当前位置
    zchxRadarRectDef* pre_rect = last_update_node->mDefRect;
    if(!pre_rect) return;

    int    rect_num = pre_rect->rectnumber();
    quint32 old_time = pre_rect->updatetime();
    double old_sog = pre_rect->sogms();
    double old_lat = pre_rect->center().latitude();
    double old_lon = pre_rect->center().longitude();
    double old_cog = pre_rect->cog();
    //计算当前的时间间隔, 根据时间间隔算预估位置点
    quint32 delta_time = list_time - old_time;
    //这里开始进行位置的预估判断,
    if(track_debug) qDebug()<<"current target dir is confirmed."<<rect_num<<" cog:"<<old_cog;
    bool isTargetInVideo = false;
    zchxRadarRectDefList list = getDirDeterminTargets(isTargetInVideo, left_list, pre_rect, true);
    zchxRadarRectDef dest;
    //检查目标是否已经找到
    if(list.size() > 0)
    {
        //目标已经找到,将目标从原来的矩形队列删除
        dest = list.at(0);
        if(track_debug) qDebug()<<"next target found, distance to estmation:"<<rect_num;

    } else
    {        
        if(isTargetInVideo)
        {
            qDebug()<<"now target is in a video area with center at a opposite direction. make is as not move. only update its update_time";
            pre_rect->set_updatetime(list_time);
            topNode->mUpdateTime = list_time;
            pre_rect->set_sogms(0);  //认为目标在当前位置静止
            return;
        }
        if(track_debug) qDebug()<<"next target not found, fake one"<<rect_num;
        //估计目标的可能位置
        double est_distance = delta_time * old_sog;
#if 0
        if(est_distance < mRangeFactor)
        {
            //距离太短了,目标不用预估,认为目标没有移动
            pre_rect->set_updatetime(list_time);
            topNode->update_time = list_time;
            return;
        }
#endif
        QGeoCoordinate est_pos = QGeoCoordinate(old_lat, old_lon).atDistanceAndAzimuth(est_distance, old_cog);
        dest.CopyFrom(*pre_rect);
        dest.set_realdata(false);
        dest.set_updatetime(list_time);
        //将目标移动到现在的预推位置
        changeTargetLL(Latlon(est_pos.latitude(), est_pos.longitude()), dest);
    }

    //更新目标开始
    topNode->mUpdateTime = list_time;
    pre_rect->set_updatetime(list_time);

    double dir = Mercator::angle(old_lat, old_lon, dest.center().latitude(), dest.center().longitude());
    //目标的方向已经有了,也就是目标也不是初次出现,需要检查方向.
    if(isDirectionChange(old_cog, dir))
    {
        qDebug()<<"taregt direction should be same as old one, but now found a oppsite one. abnormal...";
        return;
    }


    //计算新目标和就目标之间的距离
    double distance = getDisDeg(old_lat, old_lon, dest.center().latitude(), dest.center().longitude());
    if(distance < 1.0)
    {
        //目标的距离太近,认为目标没有移动, 不进行处理
        if(track_debug) qDebug()<<"new destination too closed. not update. continue..."<<distance;
        return;
    }

    //确定新目标
    double cog = Mercator::angle(old_lat, old_lon, dest.center().latitude(), dest.center().longitude());
    double cal_dis = Mercator::distance(old_lat, old_lon, dest.center().latitude(), dest.center().longitude());
    dest.set_cog(cog);
    dest.set_sogms( cal_dis / delta_time);
    last_update_node->mChildren.append(QSharedPointer<TargetNode>(new TargetNode(dest)));
}

void zchxRadarTargetTrack::updateDetermineRoute(TargetNode *topNode, zchxRadarRectDefList &left_list)
{
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(!topNode || topNode->mStatus != Node_UnDef) return;
    //当前的时间确认
    if(left_list.size() == 0) return;
    quint32 list_time = left_list.first().updatetime();

    zchxRadarRectDef *pre_rect = 0;
    int node_number = topNode->mSerialNum;
    //如果目标已经存在分支了,则只更新目标的分支,否则更新根节点
    if(topNode->hasChildren())
    {
        for(int i=0; i<topNode->mChildren.size(); i++)
        {
            TargetNode *child = topNode->mChildren[i].data();
            if(!child) continue;
            //获取分支的最后节点
            TargetNode *last_child = child->getLastChild(child);
            if(!last_child) continue;
            pre_rect = last_child->mDefRect;
            qDebug()<<"child number:"<<node_number<<" time:"<<pre_rect->updatetime()<<" sog:"<<pre_rect->sogms()<<" cog:"<<pre_rect->cog()<<" route id:"<<i;

            //根据分支节点的速度和角度取得待更新的矩形单元,这里的矩形单元数只有一个
            bool isTargetInVideo = false;
            zchxRadarRectDefList result = getDirDeterminTargets(isTargetInVideo, left_list, pre_rect, true);
            if(result.size() == 0)
            {
                if(isTargetInVideo)
                {
                    pre_rect->set_updatetime(list_time);
                    topNode->mUpdateTime = list_time;
                    qDebug()<<"found old target in video area, keep silent"<<node_number<<" route id:"<<i;
                } else
                {
                    qDebug()<<"found no target in specified area"<<node_number<<" route id:"<<i;
                }
                continue;
            }
            //更新对应的速度和方向
            zchxRadarRectDef now_rect = result.first();
            now_rect.set_realdata(true);
            now_rect.set_rectnumber(node_number);

            double cog = Mercator::angle(pre_rect->center().latitude(), pre_rect->center().longitude(), now_rect.center().latitude(), now_rect.center().longitude());
            quint32 delta_time = list_time - pre_rect->updatetime();
            double cal_dis = Mercator::distance(pre_rect->center().latitude(), pre_rect->center().longitude(), now_rect.center().latitude(), now_rect.center().longitude());
            //检查目标的距离和方向
            if(cal_dis < mRangeFactor)
            {
                //目标没有移动处理
                pre_rect->set_updatetime(list_time);
                topNode->mUpdateTime = list_time;
                continue;
            }
            if(delta_time < 0.000001)
            {
                qDebug()<<"abnormal delta time found now"<<node_number<<" route id:"<<i<<" deleta time:"<<delta_time;
                continue;
            }
            double sog = cal_dis / delta_time;
            if(sog > ship_max_speed)
            {
                qDebug()<<"abnormal target speed found now"<<node_number<<" route id:"<<i<<" speed:"<<sog;
                continue;
            }
            //检查目标是不是发生了以前有速度现在没速度的情况,如果是,还是使用以前的速度
            if(pre_rect->sogms() > 0 && sog < 0.001)
            {
//                sog = pre_rect->sog();
//                cog = pre_rect->cog();
            }
            now_rect.set_sogms(sog);
            now_rect.set_cog(cog);
            last_child->mChildren.append(QSharedPointer<TargetNode>(new TargetNode(now_rect)));
            topNode->mUpdateTime = list_time;
            if(track_debug) qDebug()<<"update root node:"<<pre_rect->rectnumber()<<" with possible route index"<<i<<"  cog "<<cog<<" sog:"<<sog;
        }
    } else
    {
        //分支目标不存在,再次更新根节点
         bool isTargetInvideo = false;
        zchxRadarRectDefList result = getDirDeterminTargets(isTargetInvideo, left_list, topNode->mDefRect, false);
        pre_rect = topNode->mDefRect;
        bool root_update = false;
        for(int i=0; i<result.size(); i++)
        {
            zchxRadarRectDef total = result[i];
            total.set_realdata(true);
            total.set_rectnumber(node_number);
            double cog = Mercator::angle(pre_rect->center().latitude(), pre_rect->center().longitude(), total.center().latitude(), total.center().longitude());
            quint32 delta_time = list_time - pre_rect->updatetime();
            double cal_dis = Mercator::distance(pre_rect->center().latitude(), pre_rect->center().longitude(), total.center().latitude(), total.center().longitude());
            qDebug()<<"cal distance:"<<cal_dis<<mRangeFactor;
            if(cal_dis >= mRangeFactor)
            {
                //存在可能运动的情况
                double sog = cal_dis / delta_time;
                total.set_sogms(sog);
                total.set_cog(cog);
                topNode->mChildren.append(QSharedPointer<TargetNode>(new TargetNode(total)));
                if(track_debug) qDebug()<<"update root node:"<<pre_rect->rectnumber()<<" with possible child:(distance, cog, sog, time) "<<cal_dis<<cog<<sog<<total.updatetime();
                root_update = true;
            }
        }
        //将目标对象的更新时间更新到列表时间
        if(result.size() > 0 || isTargetInvideo)   topNode->mUpdateTime = list_time;
        if((result.size() > 0 && !root_update) || isTargetInvideo)
        {
            //所有目标都距离太近, 证明目标此时没有移动,作为静止目标处理
            topNode->mDefRect->set_updatetime(list_time);
            qDebug()<<"root is not move update its time only."<<topNode->mDefRect->rectnumber();
        }
    }
}

//检查所有目标路径点的个数。若路径存在3个点，则目标航迹确定为对应路径。返回第一层节点对象
TargetNode* zchxRadarTargetTrack::checkNodeConfirmed(TargetNode *node)
{
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(!node || node->mChildren.size() == 0) return 0;
    for(int i=0; i<node->mChildren.size(); i++)
    {
        //取得路径的第一个节点
        TargetNode *child_lvl1 = node->mChildren[i].data();
        if(!child_lvl1) continue;
        //取得路径的第二个节点
        if(child_lvl1->mChildren.size() == 0) continue;
        //第二个节点存在
        TargetNode *child_lvl2 = child_lvl1->mChildren[0].data();
        if(!child_lvl2) continue;
        //第3个节点存在
        if(child_lvl2->mChildren.size() == 0) continue;
        TargetNode *child_lvl3 = child_lvl2->mChildren[0].data();
        if(!child_lvl3) continue;
        //将目标确认信息添加
        node->mStatus = Node_Moving;
        if(track_debug) qDebug()<<"now target "<<child_lvl1->mDefRect->rectnumber()<<" has been confirmed:"<<child_lvl1->mDefRect->cog()<<child_lvl1->mDefRect->sogms()<<child_lvl2->mDefRect->cog()<<child_lvl2->mDefRect->sogms();
        return child_lvl1;
    }

    return 0;

}

void zchxRadarTargetTrack::processWithPossibleRoute(const zchxRadarTrackTask &task)
{
#if 0
    if(task.size() == 0) return;
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    zchxRadarRectDefList temp_list(task);             //保存的未经处理的所有矩形单元
    qDebug()<<"now process list time:"<<QDateTime::fromTime_t(task.first().updatetime()).toString("yyyy-MM-dd hh:mm:ss");

    if(mTargetNodeMap.size() != 0)
    {
        //1)先更新目标方向已经确认的数据
        for(TargetNodeMap::iterator it = mTargetNodeMap.begin(); it != mTargetNodeMap.end(); it++)
        {
            TargetNode *node = it->data();
            if(!node || !node->cog_confirmed) continue;
            updateConfirmedRoute(node, temp_list);
        }
        //2)再次更新目标方向还未确认的
        for(TargetNodeMap::iterator it = mTargetNodeMap.begin(); it != mTargetNodeMap.end(); it++)
        {
            TargetNode *node = it->data();
            if(!node || node->cog_confirmed) continue;
            //更新所有路径
            updateDetermineRoute(node, temp_list);
            //检查目标的方向是否已经确认
            TargetNode *route_node = checkNodeConfirmed(node);
            if(!route_node) continue;
            //目标确认,将路径进行分离,确认的路径保留,未确认的路径移除并作为单独的目标再次加入,等待下一个周期到来时进行继续更新
            splitAllRoutesIntoTargets(node, route_node);
        }

    }

    //剩余的目标都作为初始化的点迹保存
    for(int i=0; i<temp_list.size(); i++)
    {
        zchxRadarRectDef rect = temp_list[i];
        TargetNode *node = new TargetNode(rect);
        appendNode(node, 0);
    }

    //删除很久没有更新的目标点
    deleteExpiredNode();
    //现在将目标进行输出
    outputTargets();

#else
    if(task.size() == 0) return;
    int cur_cycle_index = task.first().videocycleindex();
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    zchxRadarRectDefList temp_list(task);             //保存的未经处理的所有矩形单元
    quint32 now_time = task.first().updatetime();
    qDebug()<<"now process list time:"<<QDateTime::fromTime_t(task.first().updatetime()).toString("yyyy-MM-dd hh:mm:ss")<<task.size();


    QTime t;
    int elapsed = 0;
    t.start();
    //默认扫描周期是3s，最大速度是10m/s
    QList<AreaNodeTable> areaTableList = calculateTargetTrackMode(mMaxSpeed, task.first().updatetime(), mScanTime);
    QList<int>  used_index_list;

    elapsed = t.elapsed();
    qDebug()<<metaObject()->className()<<__FUNCTION__<<" calculate prediction area elaped:"<<elapsed;
    t.start();

    if(areaTableList.size() > 0)
    {
        //先检查每一个区域内落入了哪些目标
        for(int i=0; i<areaTableList.size(); i++)
        {
            AreaNodeTable &table = areaTableList[i];
            for(int k=0; k<temp_list.size(); k++)
            {
                zchxRadarRectDef def = temp_list[k];
                Mercator pos = latlonToMercator(def.center().latitude(), def.center().longitude());
                if(table.mArea.containsPoint(pos.toPointF(), Qt::OddEvenFill))
                {
                    table.mRectList.append(def);
                }
            }
            //打印每一个预推区域对应的回波中心矩形
            if(DEBUG_TRACK_INFO)
            {
                QStringList node_number_list;
                foreach (TargetNode* node, table.mNodeList) {
                    node_number_list.append(QString::number(node->mSerialNum));
                }
                QStringList rectNumList;
                foreach (zchxRadarRectDef def, table.mRectList) {
                    rectNumList.append(QString::number(def.rectnumber()));
                }
                qDebug()<<"prediction["<<i+1<<"] has node list:"<<node_number_list.join(" ")<<" rect num:"<<rectNumList.join(" ");
            }
        }
        //开始统计整理，每一个节点现在有多少个待选节点
        QMap<TargetNode*, zchxRadarRectDefList> counter_map;
        QMap<int, QList<int>>                   used_node_rectMap;
        foreach (AreaNodeTable table, areaTableList) {
            foreach (TargetNode* node, table.mNodeList) {
#if 1
                counter_map[node].append(table.mRectList);
#else
                //这里需要添加矩形重复的过滤，避免多次重复
                foreach (zchxRadarRectDef def, table.mRectList) {
                    if(used_node_rectMap.contains(node->mSerialNum) &&
                            used_node_rectMap[node->mSerialNum].contains(def.rectnumber()))
                    {
                        continue;
                    }
                    counter_map[node].append(def);
                    used_node_rectMap[node->mSerialNum].append(def.rectnumber());
                }
#endif
            }
        }
        if(DEBUG_TRACK_INFO)
        {
            QMap<TargetNode*, zchxRadarRectDefList>::iterator it = counter_map.begin();
            for(; it != counter_map.end(); it++)
            {
                TargetNode* node = it.key();
                if(!node) continue;
                QStringList rectNumList;
                foreach (zchxRadarRectDef def, it.value()) {

                        rectNumList.append(QString::number(def.rectnumber()));
                }

                qDebug()<<"node: "<<node->mSerialNum<<" has releated rect:"<<rectNumList.join(",");
            }
        }
        //开始将新的目标更新到旧目标对应的路径
        struct DetermineNode{
            bool selectedRealNode;
            zchxRadarRectDefList targetList;
        };

        QMap<TargetNode*, zchxRadarRectDefList>::iterator it = counter_map.begin();
        for(; it != counter_map.end(); it++)
        {
            TargetNode* node = it.key();
            if(!node) continue;
            zchxRadarRectDefList targetList = it.value();
            TargetNode *topNode = node->topNode();
            int path_size = 0;
            if(topNode) path_size = topNode->mChildren.size();

            DetermineNode determination;

            if(targetList.size() > 0)
            {
                determination.selectedRealNode = true;
                if(DEBUG_TRACK_INFO)
                {
                    qDebug()<<"node: "<<node->mSerialNum<<" is about to find the corresponding rect. rect size:"<<targetList.size();
                }

                if(node->mParent)
                {
                    qDebug()<<"node: "<<node->mSerialNum<<" is a child. find the closest rect"<<" path size:"<<path_size;
                    //如果节点是目标路径上的子节点，那么候选目标就只有一个，选择距离预推运动点最近的目标
                    int    index = -1;
                    double delta_time = now_time - node->mDefRect->updatetime();
                    QGeoCoordinate source(node->mDefRect->center().latitude(), node->mDefRect->center().longitude());
                    //计算目标的预推位置
                    double delta_dis = node->mDefRect->sogms() * delta_time;
                    QGeoCoordinate dest = source.atDistanceAndAzimuth(delta_dis, node->mDefRect->cog());
                    int    min_dis = INT32_MAX;
                    QList<int>  index_list;
                    for(int i=0; i<targetList.size();i++)
                    {
                        zchxRadarRectDef def = targetList[i];
                        if(index_list.contains(def.rectnumber())) continue;
                        index_list.append(def.rectnumber());
                        double dis = dest.distanceTo(QGeoCoordinate(def.center().latitude(), def.center().longitude()));
                        if(dis < min_dis)
                        {
                            min_dis = dis;
                            index = i;
                        }
                    }
                    zchxRadarRectDef def =  targetList.takeAt(index);
                    targetList.clear();
                    targetList.append(def);
                } else
                {
                    //如果节点是第一个节点，那么目标的候选节点可以多个，暂且将所有点都添加到目标的候选中。
                    //这里就不进行任何其他操作
                    qDebug()<<"node: "<<node->mSerialNum<<" is a top node. all coresspond rect appended"<<" path size:"<<path_size<<" to be added targe size:"<<targetList.size();
                }
                determination.targetList.append(targetList);

                if(DEBUG_TRACK_INFO)
                {
                    QStringList rectNumList;
                    foreach (zchxRadarRectDef def, targetList) {

                            rectNumList.append(QString::number(def.rectnumber()));
                    }

                    qDebug()<<"node: "<<node->mSerialNum<<" has select rect:"<<rectNumList;
                }
            } else
            {
                determination.selectedRealNode = false;
                //预推区域内没有找到目标图形，现在看看是否在一个回波图形上
                if(DEBUG_TRACK_INFO) qDebug()<<"node "<<node->mSerialNum<<" has no selected rect. now check whether it is in an exist rect.";
                //检查当前目标是否落在了某一个回波图形上
                for(int i=0; i<temp_list.size(); i++)
                {
                    zchxRadarRectDef rect = temp_list[i];
                    if(isRectAreaContainsPoint(rect, node->mDefRect->center().latitude(), node->mDefRect->center().longitude()))
                    {
                        if(DEBUG_TRACK_INFO)    qDebug()<<"find node in rect. node_number:"<<node->mSerialNum<<" origin rect number:"<<rect.rectnumber();
                        targetList.append(rect);
                        break;
                    }
                }
                if(targetList.size() > 0)
                {
                    determination.targetList.append(targetList);
                }
            }
            if(determination.targetList.size() == 0) continue;


            //根据候选目标来源的不同，进行分别处理
            if(determination.selectedRealNode)
            {
                if(DEBUG_TRACK_INFO) qDebug()<<"update current node with rect in its prediction area";
                //目标来源于真实的路径预推，直接更新到对应的路径
                foreach (zchxRadarRectDef target, determination.targetList) {
                    //获取选择的目标，更新速度等
                    if(!used_index_list.contains(target.rectnumber()))
                    {
                        if(DEBUG_TRACK_INFO) qDebug()<<"add origin rect into used list , not make new node from it."<<target.rectnumber();
                        used_index_list.append(target.rectnumber());
                    }
                    if(node->containsRect(target)) continue;

                    target.set_realdata(true);
                    QGeoCoordinate source(node->mDefRect->center().latitude(), node->mDefRect->center().longitude());
                    QGeoCoordinate dest(target.center().latitude(), target.center().longitude());
                    double delta_time = now_time - node->mDefRect->updatetime();
                    double cog = source.azimuthTo(dest);
                    double distance = source.distanceTo(dest);
                    double sog = 0.0;
                    if(delta_time > 0) sog = distance / delta_time;
                    //开始平均速度和角度
                    double refer_sog = node->getReferenceSog();
                    if(refer_sog > 0.1)
                    {
                        sog = (sog + refer_sog) / 2.0;
                    }
                    target.set_cog(cog);
                    target.set_sogms(sog);
                    bool ok = true;
                    if(node->mDefRect->sogms() > 1.0 && (sog > node->mDefRect->sogms() * 2.0)) ok = false;
                    if(1)
                    {
                        node->mChildren.append(QSharedPointer<TargetNode>(new TargetNode(target, node)));
                        if(node->isTopNode()) qDebug()<<"top node. node number: "<<node->mSerialNum<<" chiildren size:"<<node->mChildren.size()<<" directly"<<target.rectnumber();
                        if(DEBUG_TRACK_INFO) qDebug()<<"add child node into path. node number: "<<node->mSerialNum<<" rect number:"<<target.rectnumber();
                    }
                 }

            } else
            {
                if(DEBUG_TRACK_INFO) qDebug()<<"update current node with rect for node is rect area...this wanna not see. ";
                //目标节点本身没有更新，只是他现在处在一个回波图形上，如果他不更新，则有可能同一个回波图形在不同的位置出现了多个目标，
                //为了避免这种情况,我们还是假定目标移动到了回波图形的中心。

                zchxRadarRectDef target = targetList.first();
                if(!used_index_list.contains(target.rectnumber()))
                {
                    if(DEBUG_TRACK_INFO) qDebug()<<"add origin rect into used list , not make new node from it."<<target.rectnumber();
                    used_index_list.append(target.rectnumber());
                }
                target.set_realdata(true);
                //根据不同的节点位置来进行区分，如果目标所在的路径只有一个节点，则直接将目标移动过去。
                //如果目标的路径上存在多个节点，如果在目标的运动方向上，则添加到目标的子节点，
                //如果在运动的反向上，则删除前面的节点，使得更新的节点和前面的节点看起来方向一致
                if(!node->mParent)
                {
                    //单独的目标，直接目标位置移动
                    //节点本身就是根节点，不用计算速度角度
                    target.set_sogms(0.0);
                    target.set_cog(0.0);
                    if(DEBUG_TRACK_INFO) qDebug()<<"move single node "<<node->mSerialNum<<" to including rect center. rect number:"<<target.rectnumber();
                    node->mDefRect->CopyFrom(target);
                    node->mUpdateTime = target.updatetime();
                    node->mVideoIndexList.append(target.videocycleindex());
                } else
                {
                    if(DEBUG_TRACK_INFO) qDebug()<<"fina a target in the path whose cog is same as the updated one. we will move up up to the parent";
                    //计算新位置对应的目标方向
                    QGeoCoordinate source(node->mDefRect->center().latitude(), node->mDefRect->center().longitude());
                    QGeoCoordinate dest(target.center().latitude(),target.center().longitude());
                    double cog = source.azimuthTo(dest);
                    //目标方向是否与原来的方向相同
                    TargetNode *startNode = 0;
                    if(!isDirectionChange(node->mDefRect->cog(), cog))
                    {
                        //目标相同
                        startNode = node;
                        if(DEBUG_TRACK_INFO)qDebug()<<"current node cog is ok, go on";
                    } else
                    {
                        if(DEBUG_TRACK_INFO)qDebug()<<"current node cog is wrong, we need to move up up";
                        //方向反了，从上寻找可能的开始位置
                        //这里需要排除根节点，根节点没有方向。如果遍历到了根节点，则直接将目标更新到根节点，删除原来的根节点下的子节点数据
                        TargetNode* child = node;
                        TargetNode* parent = node->mParent;
                        while (parent) {
                            if(parent->isTopNode())
                            {
                                if(DEBUG_TRACK_INFO)qDebug()<<"now we choose the top child for others not found in the path.";
                                startNode = parent;
                                //删除当前节点所在的分子
                                break;
                            }
                            source.setLatitude(parent->mDefRect->center().latitude());
                            source.setLongitude(parent->mDefRect->center().longitude());
                            cog = source.azimuthTo(dest);
                            if(!isDirectionChange(parent->mDefRect->cog(), cog))
                            {
                                startNode = parent;
                                break;
                            }
                            child = parent;
                            parent = parent->mParent;
                        }
                        if(startNode)
                        {
                            if(DEBUG_TRACK_INFO) qDebug()<<"remove child for path direction adjustment";
                            //将原来的子节点删除
                            startNode->removeChild(child);
                        }
                    }
                    if(startNode)
                    {
                        //检查目标的子类节点是否已经包含了当前的矩形目标，如果包含了就不再添加了。
                        if(!startNode->containsRect(target))
                        {

                            source.setLatitude(startNode->mDefRect->center().latitude());
                            source.setLongitude(startNode->mDefRect->center().longitude());
                            double distance = source.distanceTo(dest);
                            int delta_time = target.updatetime() - startNode->mDefRect->updatetime();
                            double sog = 0.0;
                            if(delta_time > 0) sog = distance / delta_time;
                            //开始平均速度和角度
                            double refer_sog = startNode->getReferenceSog();
                            if(refer_sog > 0.1)
                            {
                                sog = (sog + refer_sog) / 2.0;
                            }
                            target.set_cog(cog);
                            target.set_sogms(sog);
                            bool ok = true;
                            if(startNode->mDefRect->sogms() > 1.0 && (sog > startNode->mDefRect->sogms() * 2.0)) ok = false;
                            if(1)
                            {
                                startNode->mChildren.append(QSharedPointer<TargetNode>(new TargetNode(target, startNode)));
                                if(startNode->isTopNode()) qDebug()<<"top node. node number: "<<startNode->mSerialNum<<" chiildren size:"<<startNode->mChildren.size()<<" indirectly"<<target.rectnumber();
                                if(DEBUG_TRACK_INFO) qDebug()<<"add child node into path. node number: "<<startNode->mSerialNum<<" including rect number:"<<target.rectnumber();
                            }
                        }

                    }
                }
            }
        }
    }
    elapsed = t.elapsed();
    qDebug()<<metaObject()->className()<<__FUNCTION__<<" update exist obj elaped:"<<elapsed;
    t.start();

    //将未更新的矩形作为新目标添加
    for(int i=0; i<temp_list.size(); i++)
    {
        zchxRadarRectDef rect = temp_list[i];
        if(used_index_list.contains(rect.rectnumber())) continue;        
        TargetNode *node = new TargetNode(rect);        
        appendNode(node, 0);
        if(DEBUG_TRACK_INFO)qDebug()<<"make new node from origin rect ."<<"node number:"<<node->mSerialNum<<"origin rect number:"<<rect.rectnumber();
    }

    //开始检查已经存在的目标的更新状态（静止，运动，还是不能确定），如果路径节点数大于N，则目标确认输出。目标位置变化不大，就是静止，否则就是运动
    QList<int> exist_numbers = mTargetNodeMap.keys();
    foreach (int key, exist_numbers)
    {
        QSharedPointer<TargetNode> node = mTargetNodeMap[key];
        if(!node || node->mChildren.size() == 0) continue;
        //检查目标信息,查看目标的可能路径是否已经满足确定条件，
        if(node->mStatus  != Node_UnDef) continue;  //目标的路径信息已经确定，不处理
        TargetNode* move_child_node = 0;
        for(int i=0; i<node->mChildren.size(); i++)
        {
            //从这个子节点开始遍历，开始构造路径
            QList<TargetNode*> list;
            list.append(node.data());
            TargetNode* current = node->mChildren[i].data();
            if(!current) continue;
            while (current) {
                list.append(current);
                if(current->mChildren.size() == 0) break;
                current = current->mChildren.first().data();
            }
            QList<TargetNode*> new_path_list;
            NodeStatus sts = checkRoutePathSts(new_path_list, list);
            if(sts == Node_Moving)
            {
                if(DEBUG_TRACK_INFO) qDebug()<<"find obj moveing:"<<node->mSerialNum;
                move_child_node = node->mChildren[i].data();
                node->mStatus = sts;
            } else if(sts == Node_GAP_ABNORMAL)
            {
                //目标本身已经符合运动条件，但是突然出现目标之间的距离变化较大，超出了原来的2倍或者2分之一，这里从开始变化的点开始独立出来作成新的待确定目标
                QSharedPointer<TargetNode> topNode = node->mChildren.at(i);
                TargetNode* refer_node = new_path_list.first();
                bool found = false;
                while (topNode)
                {
                    if(topNode.data() == refer_node)
                    {
                        found = true;
                        if(topNode.data()->mParent)
                        {
                            topNode.data()->mParent->removeChild(refer_node);
                        }
                        break;
                    }
                    if(topNode->mChildren.size() == 0) break;
                    topNode = topNode->mChildren.first();
                }
                if(found)
                {
                    topNode.data()->mStatus = Node_UnDef;
                    topNode.data()->mParent = 0;
                    topNode.data()->mDefRect->set_sogms(0.0);
                    topNode.data()->mDefRect->set_cog(0.0);
                    int node_num = getCurrentNodeNum();
                    topNode->mSerialNum = node_num;
                    topNode->mPredictionNode = 0;
                    topNode->clearPrediction();
                    mTargetNodeMap.insert(node_num, topNode);
                }

            } else if(new_path_list.size() > 0)
            {
                //路径的方向混乱，重新按照最后的方向构造
                QSharedPointer<TargetNode> topNode = node->mChildren.at(i);
                TargetNode* refer_node = new_path_list.first();
                bool found = false;
                while (topNode)
                {
                    if(topNode.data() == refer_node)
                    {
                        found = true;
                        break;
                    }
                    if(topNode->mChildren.size() == 0) break;
                    topNode = topNode->mChildren.first();
                }
                if(found)
                {
                    topNode.data()->mStatus = Node_UnDef;
                    topNode.data()->mParent = 0;
                    int node_num = getCurrentNodeNum();
                    topNode->mSerialNum = node_num;
                    topNode->mPredictionNode = 0;
                    topNode->clearPrediction();
                    mTargetNodeMap.insert(node_num, topNode);
                    //删除这一条路径
                    node->mChildren.removeAt(i);
                    i--;
                }
            }
            if(move_child_node) break;
        }
        if(move_child_node)
        {
            //开始将确认的没有确认的路径进行重新分离编号
            splitAllRoutesIntoTargets(node.data(), move_child_node);
            node->updateRouteNodePathStatus(node->mStatus);
        }
    }

    elapsed = t.elapsed();
    qDebug()<<metaObject()->className()<<__FUNCTION__<<" update obj status elaped:"<<elapsed;
    t.start();

    //这里对目标进行总的遍历处理
    //1)将目标的时间更新到目标的最后节点的时间
    //2)获取目标对应的回波周期序列，确定静止目标是不是虚警。
    //3)统计每一个回波图形对应了多少个目标，严格说来，每一个回波图形只能有一个目标。
    //  如果存在多个目标，则有可能是多个目标同时因为相交相遇追越等原因更新到了一起.
    //  检查目标的运动方向和速度，如果两个都相同，则认为是同一个目标

    QMap<int, QList<TargetNode*>> counterNode;  //key为当前矩形的编号
    QList<int>      deleteNodeList;
    foreach (QSharedPointer<TargetNode> node, mTargetNodeMap)
    {
        if(!node) continue;
        node->setAllNodeSeriaNum(node->mSerialNum);
        uint child_time = node->getLatestChildUpdateTime();
        //更新时间
        if(node->mUpdateTime < child_time) node->mUpdateTime = child_time;
        //虚警判断
        node->mFalseAlarm = node->isFalseAlarm(cur_cycle_index);
        //寻找重复目标（速度和方向都相同）
        QList<TargetNode*> children = node->getAllBranchLastChild();
        if(children.size() > 1)
        {
            //目标现在还没有输出，不处理
            continue;
        }
        TargetNode* checkNode = node.data();
        if(children.size() > 0 ) checkNode = node->getLastChild();
        if(!checkNode) continue;
        int key = checkNode->mDefRect->rectnumber();
        QList<TargetNode*> &list = counterNode[key];
        //检查有没有方向相同的目标
        bool found = false;
        foreach (TargetNode* tmpNode, list)
        {
            double cog_difff = checkNode->mDefRect->cog() - tmpNode->mDefRect->cog();
            if(fabs(cog_difff) >= 10) continue;
            //相同，删除冗余的目标（路径最短的目标）
            found = true;
            TargetNode* chk_parent = checkNode->topNode();
            TargetNode* tmp_parent = tmpNode->topNode();
            if(chk_parent && tmp_parent)
            {
                if(chk_parent->getDepth() < tmp_parent->getDepth())
                {
                    deleteNodeList.append(chk_parent->mSerialNum);
                } else
                {
                    deleteNodeList.append(tmp_parent->mSerialNum);
                }
            }
            break;
        }
        if(!found)
        {
            counterNode[key].append(checkNode);
        }
    }
    //删除相同运动状态的目标
    foreach (int key, deleteNodeList) {
        mTargetNodeMap.remove(key);
    }

    elapsed = t.elapsed();
    qDebug()<<metaObject()->className()<<__FUNCTION__<<" check same status and merge elaped:"<<elapsed;
    t.start();

    //检查是否要对当次没有更新的目标进行预推更新
    if(mIsTargetPrediction)
    {
        int i= 1;
        foreach (QSharedPointer<TargetNode> node, mTargetNodeMap)
        {
//            qDebug()<<"start check make prediction"<<i++;
            //只对运动目标进行预推
            if(!node || !node->isNodeMoving()) continue;
            QList<uint> list = node->getVideoIndexList();
            if(list.size() == 0) continue;
            if(cur_cycle_index == list.last())
            {
                //目标本次进行了更新，则清楚目标原来的预推参数信息
                node->clearPrediction();
                continue;
            }
            //目标本次没有更新，需要进行检查本次是否需要预推
            uint baseIndex = list.last();
            if(node->mPredictionTimes > 0)
            {
                baseIndex = node->mPredictionIndex;
            }

            if(cur_cycle_index - baseIndex  < mTargetPredictionInterval) continue;
            //开始执行目标预推
            QTime test;
            test.start();
//            qDebug()<<"start make prediction";
            node->makePrediction(cur_cycle_index, task.first().updatetime());
//            qDebug()<<"make prediction end:"<<test.elapsed();
        }
    }

    elapsed = t.elapsed();
    qDebug()<<metaObject()->className()<<__FUNCTION__<<" exe obj prediction elaped:"<<elapsed <<mIsTargetPrediction<<mIsCheckTargetGap;
    t.start();

    //删除很久没有更新的目标点
    deleteExpiredNode();
    //现在将目标进行输出
    outputTargets();

    elapsed = t.elapsed();
    qDebug()<<metaObject()->className()<<__FUNCTION__<<" out put elaped:"<<elapsed;
    t.start();
#endif
}

NodeStatus zchxRadarTargetTrack::checkRoutePathSts(QList<TargetNode*>& newRoueNodeList, const QList<TargetNode*>& path)
{
    //只有一个节点不能确定状态
    if(path.size() < mTargetConfirmCounter) return Node_UnDef;
    //检查对应的目标是否是方向统一
    QList<int> dir_change_index_list;
    for(int i=2; i<path.size(); i++)
    {
        double pre_cog = path[i-1]->mDefRect->cog();
        double cur_cog = path[i]->mDefRect->cog();
        if(isDirectionChange(pre_cog, cur_cog))
        {
            dir_change_index_list.append(i-1);
        }
    }
    if(dir_change_index_list.size() == 0)
    {
        //这里再检查每一段路径的长度是否符合要求。每一段路径的长度是否都差不多。如果差异较大，就从差异较大的节点开始抽出节点重新组成一个新的目标
        double pre_distance = -1.0;
        bool abnormal = false;
        if(mIsCheckTargetGap)
        {
            for(int i=2; i<path.size(); i++)
            {
                com::zhichenhaixin::proto::Latlon pre_center = path[i-1]->mDefRect->center();
                com::zhichenhaixin::proto::Latlon cur_center = path[i]->mDefRect->center();
                double distance = QGeoCoordinate(pre_center.latitude(), pre_center.longitude()).distanceTo(QGeoCoordinate(cur_center.latitude(), cur_center.longitude()));
                if(pre_distance < 0)
                {
                    pre_distance = distance;
                } else if(distance > 2.0 * pre_distance || distance > 0.5 * pre_distance)
                {
                    abnormal = true;
                }
                if(abnormal)
                {
                    newRoueNodeList.append(path[i]);
                }

            }
        }
        if(!abnormal)   return Node_Moving;
        return Node_GAP_ABNORMAL;
    }
    //从最后方向开始变化的点截断，让那个点作为目标最新的起点
    int start_index = dir_change_index_list.last();
    for(int i=start_index; i<path.size(); i++)
    {
        newRoueNodeList.append(path[i]);
    }
    return Node_UnDef;
}

QList<AreaNodeTable> zchxRadarTargetTrack::calculateTargetTrackMode(double max_speed, quint32 now, double scan_time)
{
    QList<AreaNodeTable> areaNodeTableList;
    if(mTargetNodeMap.size() == 0) return areaNodeTableList;

    QTime t;
    t.start();
    qDebug()<<"start calculate node's prediction area. and now current node size:"<<mTargetNodeMap.size();

    //通过计算目标的预推区域来计算区域是否存在相交，追越， 平行相遇等模型
    QList<PredictionNode> result_list;
    //1)先计算预推区域
    foreach (QSharedPointer<TargetNode> top_node, mTargetNodeMap)
    {
        if(!top_node) continue;

        //检查目标是否已经存在子类末端节点
        QList<TargetNode*> node_list = top_node->getAllBranchLastChild();
        //不存在子节点的情况，直接添加顶层节点
        if(node_list.size() == 0) node_list.append(top_node.data());
        int length = 0;
        foreach (TargetNode* node, node_list)
        {
            double old_speed = node->mDefRect->sogms();
            double old_lat = node->mDefRect->center().latitude();
            double old_lon = node->mDefRect->center().longitude();
            double old_cog = node->mDefRect->cog();
            double distance = old_speed * (now - top_node->mUpdateTime);
            bool point_node = false;
            if(node->mDefRect->sogms() < 0.1 && node->mStatus == Node_UnDef)
            {
                distance = max_speed * scan_time;
                point_node = true;
            }

            zchxTargetPrediction *prediction = 0;
            if(point_node)
            {
                //目标静止,目标的预推区域就是周围的圆形区域
                prediction = new zchxTargetPrediction(old_lat, old_lon, distance);
            } else
            {
                //目标运动，就在他的运动方向上构造区域
                QGeoCoordinate est_geo = QGeoCoordinate(old_lat, old_lon).atDistanceAndAzimuth(distance, old_cog);
                double est_lat = est_geo.latitude();
                double est_lon = est_geo.longitude();
                prediction = new zchxTargetPrediction(old_lat, old_lon, est_lat, est_lon, mPredictionWidth, 0.2);
            }
            if(prediction && prediction->isValid())
            {
                //更新节点对应的预推区域
                node->mDefRect->clear_prediction();
                QList<Latlon> area = prediction->getPredictionAreaLL();
                foreach (Latlon ll, area) {
                    com::zhichenhaixin::proto::Latlon* block = node->mDefRect->mutable_prediction()->add_area();
                    block->set_latitude(ll.lat);
                    block->set_longitude(ll.lon);
                }
                PredictionNode res;
                res.mNode = node;
                res.mPrediction = prediction;
                result_list.append(res);
                length++;
            }
        }
        //检查每一个节点是否都已经更新了预推区域
        if(length == 0)
        {
            //节点没有更新预推区域，这是异常情况
            qDebug()<<"node:"<<top_node->mSerialNum<<" has no prediction area. abnormal occoured...";
        }
    }
    //开始计算目标预推区域和节点的对应关系
    //对应相交追越相遇模型计算他们的共同区域，并且抽出来，区域被重新分割，然后进行重新赋值
    for(int i=0; i<result_list.size(); i++)
    {
        PredictionNode cur = result_list[i];
        //分别计算相交 追越  相遇的预推区域
        bool found = false;
        QGeoCoordinate cur_geo(cur.mNode->mDefRect->center().latitude(), cur.mNode->mDefRect->center().longitude());
        for(int k=i+1; k<result_list.size(); k++)
        {
            if(i==k) continue;
            PredictionNode next = result_list[k];
            //先计算两者之间的实际距离值，距离太远(100m)，就不考虑二者的位置关系
            QGeoCoordinate next_geo(next.mNode->mDefRect->center().latitude(), next.mNode->mDefRect->center().longitude());
            double distance = cur_geo.distanceTo(next_geo);
            if(distance >= 100) continue;
            //相交
            QPolygonF andPoly = cur.mPrediction->getPredictionAreaMC().intersected(next.mPrediction->getPredictionAreaMC());
            if(andPoly.size() > 0)
            {
                found = true;
                AreaNodeTable table;
                table.mType = "AND";
                table.mArea = andPoly;
                table.mNodeList.append(cur.mNode);
                table.mNodeList.append(next.mNode);
                areaNodeTableList.append(table);
                QPolygonF subPoly = cur.mPrediction->getPredictionAreaMC().subtracted(next.mPrediction->getPredictionAreaMC());
                if(subPoly.size() > 0)
                {
                    table.mNodeList.clear();
                    table.mType = "SUB";
                    table.mArea = subPoly;
                    table.mNodeList.append(cur.mNode);
                    areaNodeTableList.append(table);
                }
                continue;
            }
            //追越或者相遇。计算二者运动方向的夹角，如果是小于5度，将预推区域旋转到同一方向，如果很近或者相交，则满足条件
            double sub_angle = cur.mNode->mDefRect->cog() - next.mNode->mDefRect->cog();
            //计算两个区域的或作为共同的区域
            QPolygonF unitedPoly = cur.mPrediction->getPredictionAreaMC().united(next.mPrediction->getPredictionAreaMC());
            //同向的判断 二者的角度差不多大小，就是相差5度  如果一个在0度左边，一个在0度右边 就是相差355
            if(fabs(sub_angle) <= 5.0 || fabs(sub_angle) >= 355.0)
            {
                AreaNodeTable table;
                table.mType = "OR";
                table.mArea = unitedPoly;
                table.mNodeList.append(cur.mNode);
                table.mNodeList.append(next.mNode);
                areaNodeTableList.append(table);
                found = true;

            } else
            {
                //检查是否是反向
                if(sub_angle < 0) sub_angle += 360.0;
                sub_angle -= 180.0;
                if(fabs(sub_angle) <= 5.0)
                {
                    AreaNodeTable table;
                    table.mType = "OR";
                    table.mArea = unitedPoly;
                    table.mNodeList.append(cur.mNode);
                    table.mNodeList.append(next.mNode);
                    areaNodeTableList.append(table);
                    found = true;
                }
            }


        }

        if(!found)
        {
            AreaNodeTable table;
            table.mType = "SELF";
            table.mArea = cur.mPrediction->getPredictionAreaMC();
            table.mNodeList.append(cur.mNode);
            areaNodeTableList.append(table);
        }

        if(cur.mPrediction)
        {
            delete cur.mPrediction;
            cur.mPrediction = 0;
        }
    }

    qDebug()<<"end calculate node's prediction area with table size:"<<areaNodeTableList.size()<<" time:"<<t.elapsed();

    if(DEBUG_TRACK_INFO)
    {
        //打印预推区域和节点
        qDebug()<<"start printf prediction area related node;";
        for(int i=0; i<areaNodeTableList.size(); i++)
        {
            AreaNodeTable table = areaNodeTableList[i];
            QStringList node_number_list;
            foreach (TargetNode* node, table.mNodeList) {
                node_number_list.append(QString::number(node->mSerialNum));
            }
            qDebug()<<"prediction["<<i+1<<"] type"<<table.mType<<" has node list:"<<node_number_list.join(" ");
        }
    }
    return areaNodeTableList;
}

void zchxRadarTargetTrack::splitAllRoutesIntoTargets(TargetNode *node, TargetNode *routeNode)
{
    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(!node || !routeNode) return;
    //目标确认,将路径进行分离,确认的路径保留,未确认的路径移除并作为单独的目标再次加入,等待下一个周期到来时进行继续更新
    for(int i=0; i<node->mChildren.size();)
    {
        //取得路径的第一个节点
        qDebug()<<"i = "<<i<<node->mChildren.size();
        TargetNode *child_lvl1 = node->mChildren[i].data();
        if(!child_lvl1) continue;
        if(child_lvl1 == routeNode)
        {
            //路径点保留
            i++;
            continue;
        }
        //将节点移除
        QSharedPointer<TargetNode> topNode = node->mChildren.takeAt(i);
        if(topNode)
        {
            topNode->mStatus = Node_UnDef;
            topNode->mParent = 0;
            int node_num = getCurrentNodeNum();
            topNode->mSerialNum = node_num;
            topNode->mPredictionNode = 0;
            topNode->clearPrediction();
            mTargetNodeMap.insert(node_num, topNode);
        }
    }
}

void zchxRadarTargetTrack::deleteExpiredNode()
{
    //清理目标,删除超时未更新的目标或者预推次数太多的目标
    quint32 time_of_day = QDateTime::currentDateTime().toTime_t();
    QList<int> allKeys = mTargetNodeMap.keys();
    foreach (int key, allKeys) {
        QSharedPointer<TargetNode> node = mTargetNodeMap[key];
        if(!node) continue;
        quint32 node_time = node->mUpdateTime;
        quint32 delta_time = time_of_day - node_time;
        if(delta_time >= mClearTrackTime)
        {
            if(DEBUG_TRACK_INFO) qDebug()<<"remove node:"<<node.data()->mSerialNum<<"now:"<<time_of_day<<" node time:"<<node_time<<" delta_time:"<<delta_time<<" clear time:"<<mClearTrackTime;
            mTargetNodeMap.remove(key);
            continue;
        }
    }
}

void zchxRadarTargetTrack::updateTrackPointWithNode(zchxRadarSurfaceTrack& list, TargetNode *node, int* silent_num)
{
    if(!node) return;
    TargetNode *child = node->getLastChild();
    if(node->mStatus == Node_Moving && node->mPredictionNode)
    {
        child = node->mPredictionNode;
    }
    if(!child) return;
    //确认目标是否输出
    if(node->isNodeSilent())
    {
        if(node->mFalseAlarm) return;
        if(!mOutputSilentPoint) return;
        if(silent_num) *silent_num = (*silent_num) + 1;
    } else if(node->isNodeMoving())
    {
        if(child->mDefRect->sogms() < mOutputTargetMinSpeed) return;
    } else
    {
        //目标状态未确定
        return;
    }


    int node_number = node->mSerialNum;
    zchxTrackPoint *trackObj = list.add_trackpoints();
    if(!trackObj) return;
    zchxRadarRectDef *target = child->mDefRect;

    //编号
    trackObj->set_radarsiteid(QString::number(mRadarID).toStdString());
    trackObj->set_tracknumber(node_number);
    trackObj->set_trackconfirmed(node->mStatus == Node_Moving? true : false);
    trackObj->set_objtype(1);
    //当前目标
    trackObj->mutable_current()->CopyFrom(*target);
    trackObj->mutable_current()->set_sogknot(target->sogms() * 3.6 / 1.852);  //输出的速度为节
    //历史轨迹输出
    if(node->isNodeMoving())
    {
        //历史轨迹数据按照时间顺序进行输出，时间越早则靠后。top节点在最后面
        TargetNode* cur = node->getLastChild();
        while (cur) {
            zchxRadarRectDef *history = trackObj->add_tracks();
            history->CopyFrom(*(cur->mDefRect));
            cur = cur->mParent;
        }
    }
}

void zchxRadarTargetTrack::updateRectMapWithNode(zchxRadarRectMap &map, TargetNode *node)
{
    if(!node) return;

    //开始更新余晖数据
    zchxRadarRect rect;
    rect.set_node_num(node->mSerialNum);
    rect.set_cur_est_count(/*node->est_count*/0);
    rect.set_dir_confirmed(node->mStatus == Node_Moving);
    if(node->mStatus == Node_Moving)
    {
        //需要更新历史数据
        TargetNode * latest_node = node->getLastChild();
        if(!latest_node) return;
        //当前的矩形数据
        rect.mutable_current_rect()->CopyFrom(*(latest_node->mDefRect));
//        rect.mutable_current_rect()->set_rectnumber(rect_num);
        //历史的矩形数据
        TargetNode* work_node = node;
        QList<TargetNode*> route_list;      //遍历的路径
        while (work_node) {
            route_list.prepend(work_node); //从后面开始添加路径,时间越靠近的在最前面
            if(work_node == latest_node) break;
            if(work_node->mChildren.size())
            {
                work_node = work_node->mChildren.first().data();
            } else
            {
                break;
            }
        }
        for(int i=0; i<route_list.size(); i++)
        {
            zchxRadarRectDef *his_rect = rect.add_history_rect_list();
            his_rect->CopyFrom(*(route_list[i]->mDefRect));
            if(rect.history_rect_list_size() == 20) break;
        }
    } else
    {
        //不需要更新历史数据,只更新目标的最初数据
        rect.mutable_current_rect()->CopyFrom(*(node->getLastChild()->mDefRect));
#if 0
        // 这里便于调试,将所有的节点的预推区域也加上
        for(int i=0; i<node->children.size(); i++)
        {
            TargetNode *worknode = node->children[i].data();
            while (worknode)
            {
                if(worknode->rect->has_predictionareas())
                {
                    rect.mutable_current_rect()->mutable_predictionareas()->CopyFrom(worknode->rect->predictionareas());
                }
                if(worknode->children.size() == 0) break;
                worknode = worknode->children[0].data();
            }
        }
#endif
    }
    map[node->mSerialNum] = rect;
}

void zchxRadarTargetTrack::outputRoutePath()
{
    if(!output_route_path) return;
    static qint64 count = 0;
    zchxRadarRouteNodes nodeList;
    //遍历目标进行数据组装
    for(TargetNodeMap::iterator it = mTargetNodeMap.begin(); it != mTargetNodeMap.end(); it++)
    {
        TargetNode *node = it->data();
        if(!node) continue;
        com::zhichenhaixin::proto::RouteNode* route = nodeList.add_node_list();
        route->set_node_num(node->mSerialNum);
        route->mutable_top_node()->CopyFrom(*(node->mDefRect));
        for(int i=0; i<node->mChildren.size();i++)
        {
            TargetNode* child = node->mChildren.at(i).data();
            com::zhichenhaixin::proto::RoutePath *path = route->add_path_list();
            path->add_path()->CopyFrom(*(child->mDefRect));
            while (child->mChildren.size() > 0) {
                child = child->mChildren[0].data();
                path->add_path()->CopyFrom(*(child->mDefRect));
            }
        }
    }
    if(nodeList.node_list_size() > 0)
    {
        emit signalSendRoutePath(nodeList);
    }
    count++;
    if(count >= 10000) count = 10000;
}

void zchxRadarTargetTrack::outputTargets()
{
    //遍历目标进行数据组装
    QList<TargetNode*> output_list;
    for(TargetNodeMap::iterator it = mTargetNodeMap.begin(); it != mTargetNodeMap.end(); it++)
    {
        TargetNode *node = it->data();
        if(!node) continue;
        zchxRadarRectDef *top_rect = node->mDefRect;
        if(!top_rect) continue;
        if(node->isOutputEnabled()) output_list.append(node);
    }
    if(output_list.size() == 0) return;


    zchxRadarSurfaceTrack   track_list;         //雷达目标数据输出
    track_list.set_flag(1);
    track_list.set_sourceid("240");
    track_list.set_utc(QDateTime::currentMSecsSinceEpoch());
    //遍历目标进行数据组装
    int silent_obj_count = 0;
    int total_size = output_list.size();
    foreach (TargetNode *node, output_list)
    {
        if(!node) continue;
        zchxRadarRectDef *top_rect = node->mDefRect;
        if(!top_rect) continue;
        //静止点饥数据输出控制
        if(node->isNodeSilent() && silent_obj_count > 0 && silent_obj_count > total_size * 0.4) continue;
        //构造目标数据
        updateTrackPointWithNode(track_list, node, &silent_obj_count);
    }
    track_list.set_length(track_list.trackpoints_size());

    if(track_list.trackpoints_size())
    {
        emit signalSendTracks(track_list);
    }
}

void zchxRadarTargetTrack::appendNode(TargetNode *node, int source)
{
    if(!node) return;
    node->mStatus = Node_UnDef;
    node->mSerialNum = getCurrentNodeNum();
    node->mParent = 0;
    node->mPredictionNode = 0;
    node->clearPrediction();
    if(source == 0)
    {
        if(DEBUG_TRACK_INFO)qDebug()<<"make new now from orignal rect:"<<node<<node->mDefRect->rectnumber()<<node->mSerialNum;
    } else
    {
        if(DEBUG_TRACK_INFO)qDebug()<<"make new now from old ununsed route rect:"<<node<<node->mDefRect->rectnumber()<<node->mSerialNum;
    }
    mTargetNodeMap.insert(node->mSerialNum, QSharedPointer<TargetNode>(node));
    if(track_debug) qDebug()<<"new node maked now:"<<node->mSerialNum<<node->mUpdateTime;
}

int zchxRadarTargetTrack::getCurrentNodeNum()
{
    //更新编号,没有找到重复的点,直接添加
    if(mRectNum > mMaxNum)
    {
        mRectNum = mMinNum;
    }

    //检查当前是否还存在空闲的
    while (mRectNum <= mMaxNum) {
        if(mTargetNodeMap.contains(mRectNum)){
            mRectNum++;
            continue;
        }
        break;
    }

    return mRectNum;
}

void zchxRadarTargetTrack::process(const zchxRadarTrackTask &task)
{
    zchxTimeElapsedCounter counter(QString(metaObject()->className()) + " : " + QString(__FUNCTION__));
    if(mProcessWithRoute)
    {
        processWithPossibleRoute(task);
        return;
    }
    processWithoutRoute(task);
    return;
}

void zchxRadarTargetTrack::processWithoutRoute(const zchxRadarTrackTask &task)
{
    if(task.size() == 0) return;
    zchxTimeElapsedCounter counter(__FUNCTION__);
    zchxTrackPointMap   tracks;

    zchxRadarRectDefList temp_list(task);
    quint32 list_time = task.first().updatetime();

    double target_merge_distance = mTargetMergeDis;
    double target_min_speed = 3.0;
    double target_max_speed = 14.0;
    //首先合并距离太近的目标, 使之成为一个目标
    mergeRectTargetInDistance(temp_list,target_merge_distance);
//    counter.reset("merge rect");

    if(mRadarRectMap.size() > 0)
    {
        dumpTargetDistance("old target before update", target_merge_distance);
    }
//    counter.reset("check target distance");

    //开始确定雷达目标点
    int target_check_num = 4;
    bool cog_avg_adjust = mAdjustCogEnabled;
    if(mRadarRectMap.size() != 0)
    {
        //雷达目标已经存在了,现在开始找可能的下一个目标点
        zchxRadarRectMap::iterator it = mRadarRectMap.begin();
        for(; it != mRadarRectMap.end(); it++)
        {
            int    rect_num = it->node_num();
            quint32 old_time = it.value().current_rect().updatetime();
            double old_sog = it.value().current_rect().sogms();
            double old_lat = it.value().current_rect().center().latitude();
            double old_lon = it.value().current_rect().center().longitude();

            //检查目标的态势是否已经确定.目标态势确定的方法就是最近出现了连续三笔的数据的方向大体一致  目标没有回头的情况
            //目标态势确定的情况,已经存在的目标的方向角就是已经经过平均估计的方向角
            //否则就是目标的实际方向角
            double est_cog = it->current_rect().cog();
            //计算当前的时间间隔, 根据时间间隔算预估位置点
            quint32 delta_time = list_time - old_time;

            zchxRadarRectDefList merge_list;
            int cur_est_cnt = it->cur_est_count();
            //这里开始进行位置的预估判断,
            //目标的态势已经确定,则根据的预推位置确定目标,目标没有找到,则将预推位置估计为目标位置
            //目标的态势未确定,则根据距离来判断目标的可能位置
            if(it->dir_confirmed())
            {
                if(track_debug) qDebug()<<"current target dir is confirmed."<<rect_num<<" cog:"<<est_cog;
                //计算当前目标可能的预估位置
                double est_lat = 0.0, est_lon = 0.0;
                if(delta_time < 0) continue;
                float est_distance = old_sog * delta_time;
                distbearTolatlon1(old_lat, old_lon, est_distance, est_cog, &est_lat, &est_lon);
                QGeoCoordinate last_pos(old_lat, old_lon);
                //从最新的目标矩形框中寻找预估位置附件的点列
                for(int k = 0; k<temp_list.size();)
                {
                    zchxRadarRectDef next = temp_list[k];
                    //计算两者的距离
                    double distance = getDisDeg(est_lat, est_lon, next.center().latitude(),next.center().longitude());
                    if(distance < target_merge_distance)
                    {
                        //计算二者的方向角,待选取的点必须在这个点的前方
                        QGeoCoordinate cur_pos(next.center().latitude(), next.center().longitude());
                        double cal_cog = last_pos.azimuthTo(cur_pos);
                        if(isDirectionChange(est_cog, cal_cog))
                        {
                            //点跑到当前点的后方去了,与原来的运动方向相反, 不处理.
                            if(track_debug) qDebug()<<"found near target but direction not allowed. skip...";
                            k++;
                        } else
                        {
                            //目标存在,从原来的队列里边删除
                            merge_list.append(next);
                            temp_list.removeAt(k);
                        }
                        continue;
                    }
                    k++;
                }
                //目标没有找到的情况,直接构造一个目标
                if(merge_list.size() == 0)
                {
                    if(track_debug) qDebug()<<"next target not found, now fake one...."<<rect_num;
                    zchxRadarRectDef fakeData;
                    fakeData.set_realdata(false);
                    fakeData.CopyFrom(it->current_rect());
                    fakeData.set_updatetime(list_time);
                    //将目标移动到现在的预推位置
                    changeTargetLL(Latlon(est_lat, est_lon), fakeData);
                    merge_list.append(fakeData);
                    cur_est_cnt++;
                } else
                {
                    if(track_debug) qDebug()<<"next target found, with size:"<<merge_list.size();
                    cur_est_cnt = 0;  //目标重新回到了实际的环境中
                }
            } else
            {
                if(track_debug) qDebug()<<"current target dir isn't confirmed. find target near old pos."<<rect_num<<" cog:"<<est_cog;
                //这里不计算目标的预估位置,直接计算目标范围之类的点
                //从最新的目标矩形框中寻找预估位置附件的点列
                for(int k = 0; k<temp_list.size();)
                {
                    zchxRadarRectDef next = temp_list[k];
                    //计算两者的距离
                    double distance = getDisDeg(old_lat, old_lon, next.center().latitude(),next.center().longitude());
                    if(distance < target_merge_distance)
                    {
                        //目标存在,从原来的队列里边删除
                        merge_list.append(next);
                        temp_list.removeAt(k);
                        continue;
                    }
                    k++;
                }
                if(track_debug) qDebug()<<"find target near old pos with size:"<<merge_list.size();
            }
            //没有找到符合要求的目标,目标不更新
            if(merge_list.size() == 0)
            {
                if(track_debug) qDebug()<<"next target not found. now next loop"<<rect_num<<" dir confirmed:"<<it->dir_confirmed();
                continue;
            }
            if(track_debug) qDebug()<<"target need to be merge with size:"<<merge_list.size();
            //根据找到的目标进行位置更新
            //过滤筛选符合方位角要求的目标
            zchxRadarRectDefList target_list;
            if(it->history_rect_list_size() == 0)
            {
                //如果目标还是第一个点,则方向角还未确定,则所有目标的均值作为下一个目标
                target_list.append(merge_list);
            } else
            {
                //如果目标的方向角已经存在了,则优先使用和目标方向值大体方向相同的所有目标的均值作为下一个目标,
                //如果相同的不存在,则使用所有的均值作为下一个目标
                QGeoCoordinate last_pos(old_lat, old_lon);
                for(int i=0; i < merge_list.size(); i++)
                {
                    zchxRadarRectDef cur = merge_list[i];
                    QGeoCoordinate cur_pos(cur.center().latitude(), cur.center().longitude());
                    double cog = last_pos.azimuthTo(cur_pos);
                    if(!isDirectionChange(est_cog, cog))
                    {
                        target_list.append(cur);
                    }
                }
                if(target_list.size() == 0)
                {
                    target_list.append(merge_list);
                }

            }
            if(track_debug) qDebug()<<"finnaly target need to be merge with size:"<<target_list.size();
            //计算目标合并后的中心位置
            Latlon ll = getMergeTargetLL(target_list);
            //计算新目标和就目标之间的距离
            double distance = getDisDeg(old_lat, old_lon, ll.lat, ll.lon);
            if(distance < 1.0)
            {
                //目标的距离太近,认为目标没有移动, 不进行处理
                if(track_debug) qDebug()<<"merge target too closed. not update. continue..."<<distance;
                //仅仅更新目标的当前时间,防止目标被删除
                it->mutable_current_rect()->set_updatetime(list_time);
                continue;
            }
            //新目标对应的矩形单元确定
            zchxRadarRectDef target = target_list.first();
            target.set_rectnumber(it.key());
            changeTargetLL(ll, target);
            //           if(target.timeofday() == 0)
            //           {
            //               if(track_debug) qDebug()<<"is time_of_day set:"<<target.has_timeofday();
            //               for(int i=0; i<target_list.size(); i++)
            //               {
            //                   if(track_debug) qDebug()<<"target timeof day:"<<target_list[i].timeofday();
            //               }
            //           }

            //确定新目标
            zchxRadarRect total;
            total.set_dir_confirmed(it->dir_confirmed());  //初始化目标是否确认方向
            total.set_cur_est_count(cur_est_cnt);
            total.mutable_current_rect()->CopyFrom(target);
            //旧目标添加历史轨迹点的0号位置
            zchxRadarRectDef *single_Rect = total.add_history_rect_list();
            single_Rect->CopyFrom(it->current_rect());
            //移动就目标的历史轨迹点到新目表的历史轨迹点.历史轨迹点最大数目为20
            int update_his_size = 20 - total.history_rect_list_size();
            int k = 0;
            for(int i= 0; i<it->history_rect_list_size(); i++)
            {
                if(k >= update_his_size) break;
                zchxRadarRectDef *history = total.add_history_rect_list();
                history->CopyFrom(it->history_rect_list(i));
                k++;
            }
            //更新速度角度
            if(total.history_rect_list_size() > 0)
            {
                zchxRadarRectDef* last = total.mutable_history_rect_list(0);
                QGeoCoordinate last_pos(last->center().latitude(), last->center().longitude());
                QGeoCoordinate cur_pos(total.current_rect().center().latitude(), total.current_rect().center().longitude());
                total.mutable_current_rect()->set_cog(last_pos.azimuthTo(cur_pos));
                //检查新目标的方向是否与原来确定的目标方向相同
                if(it->dir_confirmed())
                {
                    if(isDirectionChange(it->current_rect().cog(), total.current_rect().cog()))
                    {
                        //目标反方向了, 不再继续更新
                        if(track_debug) qDebug()<<"target go to opposite..when update new sog cog  continue yet..";
                        continue;
                    }
                }
                quint32 delta_time = total.current_rect().updatetime() - last->updatetime();
                if(delta_time <= 0) continue;
                double cal_dis = last_pos.distanceTo(cur_pos);
                total.mutable_current_rect()->set_sogms( cal_dis / delta_time);
                if(total.history_rect_list_size() == 1)
                {
                    //添加第一个历史轨迹点
                    //第一个点的方向还未确定,默认赋值为第二个点的角度
                    last->set_cog(total.current_rect().cog());
                }
                QList<double> history_cog;
                for(int i=0; i<total.history_rect_list_size(); i++)
                {
                    history_cog.append(total.history_rect_list(i).cog());
                }
                if(track_debug) qDebug()<<"now history cogs:"<<history_cog;


                //           cout<<current.current_rect().rectnumber()<<"dis:"<<cal_dis<<"time_gap:"<<delta_time<<"speed:"<<current.current_rect().sog()<< current.current_rect().timeofday()<< last.timeofday();
            }
            //目标没有跳跃,更新目标的方位角为平均值
            if(!total.dir_confirmed())
            {
                if(track_debug) qDebug()<<"now check target is dir stable or not...."<<total.current_rect().cog();
                double avg_cog = 0;
                int target_dir = getTargetDirectStatus(total, target_check_num, &avg_cog);
                if( target_dir == TARGET_DIRECTION_STABLE)
                {
                    total.set_dir_confirmed(true);
                    if(cog_avg_adjust)total.mutable_current_rect()->set_cog(avg_cog);
                    if(track_debug) qDebug()<<"taregt is stable now. with new cog:"<<total.current_rect().cog();

                } else if(target_dir == TARGET_DIRECTION_UNSTABLE)
                {

                    //现在进行最后的确认.检查目标是否是来回地跳来跳去,如果是,删除跳来跳去的轨迹点,保留最初的点
                    if(track_debug) qDebug()<<"now start check new target is jumping or not"<<"  time:"<<total.current_rect().updatetime()<< " size:"<<total.history_rect_list_size();
                    int jump_num = target_check_num + 1;
                    if(isTargetJumping(total, target_merge_distance, jump_num))
                    {
                        //将目标退回到check_num对应的轨迹点
                        int restore_history_index = jump_num - 2;
                        if(restore_history_index > total.history_rect_list_size())
                        {
                            //异常情况
                            continue;
                        }
                        zchxRadarRectDef now = total.history_rect_list(restore_history_index);
                        total.mutable_current_rect()->CopyFrom(now);
                        total.mutable_history_rect_list()->DeleteSubrange(0, restore_history_index + 1);
                        if(track_debug) qDebug()<<"target is jumping... move to old one:"<<total.current_rect().updatetime()<<" size:"<<total.history_rect_list_size();

                    }
                } else
                {
                    //目标的点列数还不够,难以判断
                }
            } else if(cog_avg_adjust)
            {
                //根据最近的点列的坐标更新当前点的方位角
                zchxRadarRectDefList list;
                list.append(total.current_rect());
                for(int i=0; i<total.history_rect_list_size(); i++)
                {
                    list.append(total.history_rect_list(i));
                    if(list.size() == 5) break;
                }
                double avg_cog = calAvgCog(list);
                total.mutable_current_rect()->set_cog(avg_cog);
            }

            if(total.history_rect_list_size() < target_check_num)
            {
                if(track_debug) qDebug()<<"target is dir not stable  ....for not enough history size";
                total.set_dir_confirmed(false);
            }

            //将就目标的历史轨迹清除,只有新目标保存历史轨迹
            it->clear_history_rect_list();
            //将新目标更新到队列中
            it->CopyFrom(total);
            //           if(track_debug) qDebug()<<"update exist target:"<<it->current_rect().rectnumber()<<it->current_rect().timeofday()<<target.timeofday();
        }
    }

//    counter.reset("track object");

    checkTargetRectAfterUpdate(target_merge_distance);

//    counter.reset("check target after update");
    //检查目标之间的距离值
    dumpTargetDistance("old target after update", target_merge_distance);

//    counter.reset("old target after update");
    //将剩余的矩形目标作为单独的目标添加
    //目标点为空,所有的雷达目标点自动生成初始化的矩形点
    for(int i=0; i<temp_list.size(); i++)
    {
        zchxRadarRect rect;
        rect.set_dir_confirmed(false);
        rect.set_cur_est_count(0);
        rect.mutable_current_rect()->CopyFrom(temp_list[i]);
        //更新编号,没有找到重复的点,直接添加
        if(mRectNum > mMaxNum)
        {
            mRectNum = mMinNum;
        }

        //检查当前是否还存在空闲的
        while (mRectNum <= mMaxNum) {
            if(mRadarRectMap.contains(mRectNum)){
                mRectNum++;
                continue;
            }
            break;
        }
        rect.mutable_current_rect()->set_rectnumber(mRectNum++);
        mRadarRectMap[rect.current_rect().rectnumber()] = rect;
        if(track_debug) qDebug()<<"new rect maked now:"<<rect.current_rect().rectnumber()<<rect.current_rect().updatetime();
    }

//    counter.reset("make new target");

    checkTargetRectAfterUpdate(target_merge_distance);

//    counter.reset("check target after new");

    //   if(0){
    //       zchxRadarRectDefList list;
    //       foreach (zchxRadarRect rect, mRadarRectMap) {
    //           list.append(rect.current_rect());
    //       }
    //       exportRectDef2File(list, QString("%1_rect").arg(QDateTime::currentMSecsSinceEpoch()));
    //   }

    //清理目标,删除超时未更新的目标
    quint32 time_of_day = QDateTime::currentDateTime().toTime_t();
    QList<int> allKeys = mRadarRectMap.keys();
    foreach (int key, allKeys) {
        zchxRadarRect obj = mRadarRectMap[key];
        if((time_of_day - obj.current_rect().updatetime()) > mClearTrackTime)
        {
            mRadarRectMap.remove(key);
            continue;
        }
        if(obj.cur_est_count() >= 10)
        {
            mRadarRectMap.remove(key);
        }
    }

//    counter.reset("remove old target");

    //检查目标之间的距离值
    dumpTargetDistance("all target with new ", target_merge_distance);

//    counter.reset("check all target");
#if 0
    //连续存在3笔数据的认为是目标，然后输出
    //目标构造
    zchxRadarSurfaceTrack list;
    list.set_flag(1);
    list.set_sourceid("240");
    list.set_utc(QDateTime::currentMSecsSinceEpoch());
    foreach (zchxRadarRect rectObj, mRadarRectMap)
    {
        zchxRadarRectDef target;
        target.CopyFrom(rectObj.current_rect());
        zchxTrackPoint *trackObj = list.add_trackpoints();
        LatLong startLatLong(mCenter.lon, mCenter.lat);
        //编号
        trackObj->set_tracknumber(target.rectnumber());
        //速度
        trackObj->set_sog(target.sog());
        //方向
        trackObj->set_cog(target.cog());
        if(!rectObj.dir_confirmed())
        {
            trackObj->set_sog(0.0);
            trackObj->set_trackconfirmed(false);
            trackObj->set_cog(0);
        } else
        {
            trackObj->set_trackconfirmed(true);
        }

        //经纬度
        trackObj->set_wgs84poslat(target.center().latitude());
        trackObj->set_wgs84poslong(target.center().longitude());
        //笛卡尔坐标
        double x,y;
        double lat = trackObj->wgs84poslat();
        double lon = trackObj->wgs84poslong();
        getDxDy(startLatLong, lat, lon, x, y);
        trackObj->set_cartesianposx(x);
        trackObj->set_cartesianposy(y);
        //other
        trackObj->set_timeofday(timeOfDay(target.updatetime()));
        trackObj->set_systemareacode(0);
        trackObj->set_systemidentificationcode(1);
        trackObj->set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
        trackObj->set_tracklastreport(false);
        trackObj->set_cartesiantrkvel_vx(0);
        trackObj->set_cartesiantrkvel_vy(0);
        //直径
        trackObj->set_diameter(target.diameter());
        //目标类型和名称
        if(mUserDefObj.contains(trackObj->tracknumber()))
        {
            UserSpecifiedObj obj = mUserDefObj[trackObj->tracknumber()];
            trackObj->set_objtype(obj.mType);
            trackObj->set_objname(obj.mName.toStdString().data());
        }
        //赋值目标的外形点列
        for(int i=0; i<target.blocks_size(); i++)
        {
            com::zhichenhaixin::proto::Point *pnt = trackObj->mutable_shape()->add_pnts();
            pnt->set_lat(target.blocks(i).latitude());
            pnt->set_lon(target.blocks(i).longitude());
        }
    }
    list.set_length(list.trackpoints_size());
    emit signalSendTracks(list);
    emit signalSendRectData(mRadarRectMap);
#endif
//    counter.reset("make track object");
}


void zchxRadarTargetTrack::dumpTargetDistance(const QString &tag, double merge_dis)
{
    if(track_debug) qDebug()<<"dump target distance now:"<<tag;
    QList<int> keys = mRadarRectMap.keys();
    for(int i=0; i<keys.size(); i++)
    {
        zchxRadarRect cur = mRadarRectMap[keys[i]];
        double cur_lat = cur.current_rect().center().latitude();
        double cur_lon = cur.current_rect().center().longitude();
        for(int k = i+1; k<keys.size(); k++)
        {
            zchxRadarRect next = mRadarRectMap[keys[k]];
            double next_lat = next.current_rect().center().latitude();
            double next_lon = next.current_rect().center().longitude();
            double distance = getDisDeg(cur_lat, cur_lon, next_lat, next_lon);
            if(distance < merge_dis)
            {
                if(track_debug) qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.current_rect().updatetime()<<next.current_rect().updatetime();
            }
        }
    }
}

void zchxRadarTargetTrack::checkTargetRectAfterUpdate(double merge_dis)
{
    QList<int> keys = mRadarRectMap.keys();
    for(int i=0; i<keys.size(); i++)
    {
        if(!mRadarRectMap.contains(keys[i])) continue;
        zchxRadarRect cur = mRadarRectMap[keys[i]];
        double cur_lat = cur.current_rect().center().latitude();
        double cur_lon = cur.current_rect().center().longitude();
        for(int k = i+1; k<keys.size(); k++)
        {
            if(!mRadarRectMap.contains(keys[k])) continue;
            zchxRadarRect next = mRadarRectMap[keys[k]];
            double next_lat = next.current_rect().center().latitude();
            double next_lon = next.current_rect().center().longitude();
            double distance = getDisDeg(cur_lat, cur_lon, next_lat, next_lon);
            if(distance < merge_dis)
            {
                if(cur.dir_confirmed() ^ next.dir_confirmed())
                {
                    if(track_debug) qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" confirmed:"<<cur.dir_confirmed()<<next.dir_confirmed()<<" remove not confirmed one.";

                    if(cur.dir_confirmed() && !next.dir_confirmed())
                    {
                        mRadarRectMap.remove(keys[k]);
                        continue;
                    }
                    if((!cur.dir_confirmed()) && next.dir_confirmed())
                    {
                        mRadarRectMap.remove(keys[i]);
                        break;
                    }
                } else
                {
                    if(track_debug) qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.current_rect().updatetime()<<next.current_rect().updatetime()<<" remove old one.";

                    if(next.current_rect().updatetime() <= cur.current_rect().updatetime())
                    {
                        mRadarRectMap.remove(keys[k]);
                    } else
                    {
                        mRadarRectMap.remove(keys[i]);
                        break;
                    }
                }


            }
        }
    }
}


void zchxRadarTargetTrack::appendUserDefObj(const UserSpecifiedObj &obj)
{
    if(obj.mTrackNum <= 0) return;
    mUserDefObj[obj.mTrackNum] = obj;
}

void zchxRadarTargetTrack::removeUserDefObj(const UserSpecifiedObj &obj)
{
    mUserDefObj.remove(obj.mTrackNum);
}
