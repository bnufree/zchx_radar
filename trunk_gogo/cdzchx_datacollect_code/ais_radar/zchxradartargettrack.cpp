#include "zchxradartargettrack.h"
#include <QDebug>
#include <QGeoCoordinate>

const bool track_debug = true;

const double point_near_line = 200;
const double ship_max_speed = 5;            //5m/s ~~ 10节  10m/s 就是20节


double getDeltaTime(float now, float old)
{
    double delta = now - old;
    if(delta < 0)
    {
        delta += 3600 * 24;
    }
    return delta;
}

zchxRadarTargetTrack::zchxRadarTargetTrack(int id, const Latlon& ll, int clear_time,  bool route, QObject *parent)
    : QThread(parent)
    , mCenter(ll)
    , mClearTrackTime(clear_time)
    , mProcessWithRoute(route)
    , mMaxEstCount(5)
    , mRangeFactor(10)
{
    mDirectionInvertThresholdVal = 90.0;
    mTargetMergeDis = 100.0;
    mAdjustCogEnabled = true;
    mMinNum = 1+id*10000;
    mMaxNum = mMinNum + 9998;
    mRectNum = mMinNum;
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
            double distance = getDisDeg(cur.centerlatitude(), cur.centerlongitude(),
                                        next.centerlatitude(),next.centerlongitude());
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
        sum_x += temp.centerlongitude();
        sum_y += temp.centerlatitude();
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
    double dlon = ll.lon - cur.centerlongitude();
    double dlat = ll.lat - cur.centerlatitude();
    cur.set_centerlongitude(ll.lon);
    cur.set_centerlatitude(ll.lat);
    //将目标的所有经纬度坐标进行同步的变换
    cur.set_topleftlatitude(cur.topleftlatitude() + dlat);
    cur.set_topleftlongitude(cur.topleftlongitude() + dlon);
    cur.set_bottomrightlatitude(cur.bottomrightlatitude() + dlat);
    cur.set_bottomrightlongitude(cur.bottomrightlongitude() + dlon);
    for(int i=0; i<cur.blocks_size(); i++)
    {
        zchxSingleVideoBlock *block = cur.mutable_blocks(i);
        block->set_latitude(block->latitude() + dlat);
        block->set_longitude(block->longitude() + dlon);
    }
    if(cur.has_startlatitude())
    {
        cur.set_startlatitude(cur.startlatitude() + dlat);
    }
    if(cur.has_startlongitude())
    {
        cur.set_startlongitude(cur.startlongitude() + dlon);
    }

    if(cur.has_endlatitude())
    {
        cur.set_endlatitude(cur.endlatitude() + dlat);
    }
    if(cur.has_endlongitude())
    {
        cur.set_endlongitude(cur.endlongitude() + dlon);
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

    QGeoCoordinate p1(list.last().centerlatitude(), list.last().centerlongitude());
    for(int i=list.size()-1; i>=0; i--)
    {
        double cur_lat = list[i].centerlatitude();
        double cur_lon = list[i].centerlongitude();
        //转换为直角坐标
        Mercator cat = latlonToMercator(Latlon(cur_lat, cur_lon));
        pntsList.append(cat);
        cogList.append(list[i].cog());
        if(i == list.size() - 1) continue;
        QGeoCoordinate p2(list[i].centerlatitude(), list[i].centerlongitude());
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
    list.append(rect.currentrect());
    for(int i=0; i<rect.historyrects_size()-1; i++) //不取第一个点,因为第一个点的方向未知
    {
        list.append(rect.historyrects(i));
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
    list.append(rect.currentrect());
    for(int i=0; i<rect.historyrects_size(); i++)
    {
        list.append(rect.historyrects(i));
        if(list.size() == jump_target_num) break;
    }
    if(list.size() < jump_target_num) return false;
    //存在不一致的情况
    //检查距离是不是距离起点都很近,很近就认为目标在乱跳
    double start_lat = list.last().centerlatitude();
    double start_lon = list.last().centerlongitude();
    bool same_target = true;
    for(int i=list.size()-2; i>= 0; i--)
    {
        double cur_lat = list[i].centerlatitude();
        double cur_lon = list[i].centerlongitude();
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

zchxRadarRectDefList zchxRadarTargetTrack::getDirDeterminTargets(zchxRadarRectDefList &list, zchxRadarRectDef* src, bool cog_usefull)
{
    zchxRadarRectDefList result;
    if(list.size() == 0 || !src) return result;
    double list_time = list.first().timeofday();
    double old_time = src->timeofday();
    double old_sog = src->sog();
    double old_lat = src->centerlatitude();
    double old_lon = src->centerlongitude();
    double old_cog = src->cog();
    //计算当前的时间间隔, 根据时间间隔算预估可能移动距离
    float delta_time = getDeltaTime(list_time, old_time);
    if(delta_time < 0.000001)
    {
        qDebug()<<"abnormal delta time found now."<<src->rectnumber();
        return result;
    }
    if(cog_usefull)
    {        
        //计算当前目标可能的预估位置
        double est_lat = 0.0, est_lon = 0.0;
        //计算目标的最合理位置
        float est_distance = old_sog * delta_time;
        distbearTolatlon1(old_lat, old_lon, est_distance, old_cog, &est_lat, &est_lon);
        Mercator est_pos = latlonToMercator(est_lat, est_lon);
        //将目标的预估范围扩大最大允许的船舶速度,防止目标速度突然变大了的情况
        double cur_max_speed = old_sog * 2;
        if(cur_max_speed > ship_max_speed) cur_max_speed = ship_max_speed;
        est_distance = cur_max_speed * delta_time;
        distbearTolatlon1(old_lat, old_lon, est_distance, old_cog, &est_lat, &est_lon);
        //预估点和前一位置连线，若当前点在连线附近，则认为是下一个点。点存在多个，则取预估位置距离最近的点。
        zchxTargetPredictionLine line(old_lat, old_lon, est_lat, est_lon, 200, Prediction_Area_Rectangle);
        if(!line.isValid()) return result;
        //从最新的目标矩形框中寻找预估位置附件的点列,将与目标方位偏离最小的点作为最终的点
        double est_target_index = -1;
        double min_distance = INT64_MAX;
        for(int k = 0; k<list.size(); k++)
        {
            zchxRadarRectDef next = list[k];
            Mercator now = latlonToMercator(next.centerlatitude(), next.centerlongitude());
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
            Mercator now = latlonToMercator(next.centerlatitude(), next.centerlongitude());
            //计算待确定点到目标旧的位置点的距离
            double distance = now.distanceToPoint(latlonToMercator(old_lat, old_lon));
            if(distance / delta_time > ship_max_speed)
            {
                k++;
                continue;
            }
            //判断目标的距离的条件就是要么目标没有移动,要么目标的移动距离超出了一个距离单元且不超出客户端设定的最大速度
            if(distance < 1.0 || (distance >= mRangeFactor && (distance <= max_distance)))
            {
                qDebug()<<" child of root found now. distance to root is:"<<distance;
                result.append(list.takeAt(k));
                continue;
            }
            k++;
        }
        qDebug()<<"update root"<<src->rectnumber()<<"end with result size:"<<result.size();
    }

    return result;
}

void zchxRadarTargetTrack::updateConfirmedRoute(TargetNode* topNode, zchxRadarRectDefList& left_list)
{
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(!topNode || !topNode->cog_confirmed) return;
    //当前的时间确认
    if(left_list.size() == 0) return;
    double list_time = left_list.first().timeofday();
    //获取最后一次更新的节点
    TargetNode *last_update_node = topNode->getLastChild();
    //没有路径节点的情况
    if(!last_update_node || last_update_node == topNode) return;


    //根据已知的方向和速度预测目标的当前位置
    zchxRadarRectDef* pre_rect = last_update_node->rect;
    if(!pre_rect) return;

    int    rect_num = pre_rect->rectnumber();
    double old_time = pre_rect->timeofday();
    double old_sog = pre_rect->sog();
    double old_lat = pre_rect->centerlatitude();
    double old_lon = pre_rect->centerlongitude();
    double old_cog = pre_rect->cog();
    //计算当前的时间间隔, 根据时间间隔算预估位置点
    float delta_time = getDeltaTime(list_time, old_time);

    int cur_est_cnt = topNode->est_count;
    //这里开始进行位置的预估判断,
    if(track_debug) qDebug()<<"current target dir is confirmed."<<rect_num<<" cog:"<<old_cog;    
    zchxRadarRectDefList list = getDirDeterminTargets(left_list, pre_rect, true);
    zchxRadarRectDef dest;
    //检查目标是否已经找到
    if(list.size() > 0)
    {
        //目标已经找到,将目标从原来的矩形队列删除
        dest = list.at(0);
        cur_est_cnt = 0;
        if(track_debug) qDebug()<<"next target found, distance to estmation:"<<rect_num;

    } else
    {
        if(track_debug) qDebug()<<"next target not found, now fake one...."<<rect_num;
        //估计目标的可能位置
        double est_distance = delta_time * old_sog;
        if(est_distance >= mRangeFactor)
        {
            //时间太短了,目标不用预估,不用更新
            return;
        }
        QGeoCoordinate est_pos = QGeoCoordinate(old_lat, old_lon).atDistanceAndAzimuth(est_distance, old_cog);
        dest.CopyFrom(*pre_rect);
        dest.set_realdata(false);
        dest.set_timeofday(list_time);
        //将目标移动到现在的预推位置
        changeTargetLL(Latlon(est_pos.latitude(), est_pos.longitude()), dest);
        cur_est_cnt++;
    }

    //更新目标开始
    topNode->est_count = cur_est_cnt;
    topNode->time_of_day = list_time;
    //计算新目标和就目标之间的距离
    double distance = getDisDeg(old_lat, old_lon, dest.centerlatitude(), dest.centerlongitude());
    if(distance < 1.0)
    {
        //目标的距离太近,认为目标没有移动, 不进行处理
        if(track_debug) qDebug()<<"new destination too closed. not update. continue..."<<distance;
        //仅仅更新目标的当前时间,防止目标被删除
        pre_rect->set_timeofday(list_time);
        return;
    }

    //确定新目标
    double cog = Mercator::angle(old_lat, old_lon, dest.centerlatitude(), dest.centerlongitude());
    double cal_dis = Mercator::distance(old_lat, old_lon, dest.centerlatitude(), dest.centerlongitude());
    dest.set_cog(cog);
    dest.set_sog( cal_dis / delta_time);
    last_update_node->children.append(QSharedPointer<TargetNode>(new TargetNode(dest)));
}

void zchxRadarTargetTrack::updateDetermineRoute(TargetNode *topNode, zchxRadarRectDefList &left_list)
{
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(!topNode || topNode->cog_confirmed) return;
    //当前的时间确认
    if(left_list.size() == 0) return;
    double list_time = left_list.first().timeofday();

    zchxRadarRectDef *pre_rect = 0;
    int node_number = topNode->rect->rectnumber();
    //如果目标已经存在分支了,则只更新目标的分支,否则更新根节点
    if(topNode->hasChildren())
    {
        for(int i=0; i<topNode->children.size(); i++)
        {
            TargetNode *child = topNode->children[i].data();
            if(!child) continue;
            //获取分支的最后节点
            TargetNode *last_child = child->getLastChild(child);
            if(!last_child) continue;
            pre_rect = last_child->rect;
            qDebug()<<"child number:"<<node_number<<" time:"<<pre_rect->timeofday()<<" sog:"<<pre_rect->sog()<<" cog:"<<pre_rect->cog()<<" route id:"<<i;

            //根据分支节点的速度和角度取得待更新的矩形单元,这里的矩形单元数只有一个
            zchxRadarRectDefList result = getDirDeterminTargets(left_list, pre_rect, true);
            if(result.size() == 0)
            {
                qDebug()<<"found no target in specified area"<<node_number<<" route id:"<<i;
                continue;
            }
            //更新对应的速度和方向
            zchxRadarRectDef now_rect = result.first();
            now_rect.set_realdata(true);
            now_rect.set_rectnumber(node_number);
            double cog = Mercator::angle(pre_rect->centerlatitude(), pre_rect->centerlongitude(), now_rect.centerlatitude(), now_rect.centerlongitude());
            double delta_time = getDeltaTime(list_time, pre_rect->timeofday());
            double cal_dis = Mercator::distance(pre_rect->centerlatitude(), pre_rect->centerlongitude(), now_rect.centerlatitude(), now_rect.centerlongitude());

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
            if(pre_rect->sog() > 0 && sog < 0.001)
            {
                sog = pre_rect->sog();
                cog = pre_rect->cog();
            }
            now_rect.set_sog(sog);
            now_rect.set_cog(cog);
            last_child->children.append(QSharedPointer<TargetNode>(new TargetNode(now_rect)));
            topNode->time_of_day = list_time;
            if(track_debug) qDebug()<<"update root node:"<<pre_rect->rectnumber()<<" with possible route index"<<i<<"  cog "<<cog<<" sog:"<<sog;
        }
    } else
    {
        //分支目标不存在,再次更新根节点
        zchxRadarRectDefList result = getDirDeterminTargets(left_list, topNode->rect, false);
        pre_rect = topNode->rect;
        bool root_update = false;
        for(int i=0; i<result.size(); i++)
        {
            zchxRadarRectDef total = result[i];
            total.set_realdata(true);
            total.set_rectnumber(node_number);
            double cog = Mercator::angle(pre_rect->centerlatitude(), pre_rect->centerlongitude(), total.centerlatitude(), total.centerlongitude());
            double delta_time = getDeltaTime(list_time, pre_rect->timeofday());
            double cal_dis = Mercator::distance(pre_rect->centerlatitude(), pre_rect->centerlongitude(), total.centerlatitude(), total.centerlongitude());
            qDebug()<<"cal distance:"<<cal_dis<<mRangeFactor;
            if(cal_dis >= mRangeFactor)
            {
                //存在可能运动的情况
                double sog = cal_dis / delta_time;
                total.set_sog(sog);
                total.set_cog(cog);
                topNode->children.append(QSharedPointer<TargetNode>(new TargetNode(total)));
                topNode->clearMotionless();
                if(track_debug) qDebug()<<"update root node:"<<pre_rect->rectnumber()<<" with possible child:(distance, cog, sog, time) "<<cal_dis<<cog<<sog<<total.timeofday();
                root_update = true;
            }
        }
        //将目标对象的更新时间更新到列表时间
        if(result.size() > 0)   topNode->time_of_day = list_time;
        if(result.size() > 0 && !root_update)
        {
            //所有目标都距离太近, 证明目标此时没有移动,作为静止目标处理
            topNode->rect->set_timeofday(list_time);
            topNode->motionlessMore();
            qDebug()<<"root is not move update its time only."<<topNode->rect->rectnumber();
        }
    }
}

//检查所有目标路径点的个数。若路径存在3个点，则目标航迹确定为对应路径。返回第一层节点对象
TargetNode* zchxRadarTargetTrack::checkNodeConfirmed(TargetNode *node)
{
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(!node || node->children.size() == 0) return 0;
    for(int i=0; i<node->children.size(); i++)
    {
        //取得路径的第一个节点
        TargetNode *child_lvl1 = node->children[i].data();
        if(!child_lvl1) continue;
        //取得路径的第二个节点
        if(child_lvl1->children.size() == 0) continue;
        //第二个节点存在
        TargetNode *child_lvl2 = child_lvl1->children[0].data();
        if(!child_lvl2) continue;
        //将目标确认信息添加
        node->cog_confirmed = true;
        node->est_count = 0;
        if(track_debug) qDebug()<<"now target "<<child_lvl1->rect->rectnumber()<<" has been confirmed:"<<child_lvl1->rect->cog()<<child_lvl1->rect->sog()<<child_lvl2->rect->cog()<<child_lvl2->rect->sog();
        return child_lvl1;
    }

    return 0;

}

void zchxRadarTargetTrack::processWithPossibleRoute(const zchxRadarTrackTask &task)
{
    if(task.size() == 0) return;
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    zchxRadarRectDefList temp_list(task);             //保存的未经处理的所有矩形单元
    qDebug()<<"now process list time:"<<timeStamps(task.first().timeofday()).toString("yyyy-MM-dd hh:mm:ss");

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
}

void zchxRadarTargetTrack::splitAllRoutesIntoTargets(TargetNode *node, TargetNode *routeNode)
{
    zchxTimeElapsedCounter counter(__FUNCTION__);
    if(!node || !routeNode) return;
    //目标确认,将路径进行分离,确认的路径保留,未确认的路径移除并作为单独的目标再次加入,等待下一个周期到来时进行继续更新
    for(int i=0; i<node->children.size();)
    {
        //取得路径的第一个节点
        qDebug()<<"i = "<<i<<node->children.size();
        TargetNode *child_lvl1 = node->children[i].data();
        if(!child_lvl1) continue;
        if(child_lvl1 == routeNode)
        {
            //路径点保留
            i++;
            continue;
        }
        //将节点移除
        QSharedPointer<TargetNode> topNode = node->children.takeAt(i);
        if(topNode)
        {
            topNode.data()->cog_confirmed = false;
            int rect_num = getCurrentRectNum();
            if(topNode.data()->rect)
            {
                topNode.data()->rect->set_rectnumber(rect_num);
                qDebug()<<"make new now from ununsed route rect:"<<node<<node->rect<<node->rect->rectnumber();
            }
            mTargetNodeMap.insert(rect_num, topNode);
        }
    }
}

void zchxRadarTargetTrack::deleteExpiredNode()
{
    //清理目标,删除超时未更新的目标或者预推次数太多的目标
    double time_of_day = timeOfDay();
    QList<int> allKeys = mTargetNodeMap.keys();
    foreach (int key, allKeys) {
        QSharedPointer<TargetNode> node = mTargetNodeMap[key];
        if(!node) continue;
        double node_time = node->time_of_day;
        double delta_time = time_of_day - node_time;
        if(delta_time < 0) delta_time += (3600 * 24);
        bool reason1 = (delta_time > mClearTrackTime);
        bool reason2 = (node->est_count >= mMaxEstCount);
        if( reason1 || reason2)
        {
            qDebug()<<"now:"<<time_of_day<<" node time:"<<node_time<<" delta_time:"<<delta_time<<" clear time:"<<mClearTrackTime;
            qDebug()<<"remove node:"<<node.data()->rect->rectnumber()<<" reason:"<<(reason1 == true ? "time expired": (reason2 == true ? "max estmation" : "" ));
            mTargetNodeMap.remove(key);
            continue;
        }
    }
}

void zchxRadarTargetTrack::updateTrackPointWithNode(zchxRadarSurfaceTrack& list, TargetNode *node)
{
    if(!node) return;
    TargetNode *child = node->getLastChild();
    if(!child) return;


    int rect_num = node->rect->rectnumber();
    zchxTrackPoint *trackObj = list.add_trackpoints();
    if(!trackObj) return;
    zchxRadarRectDef *target = child->rect;
    if(!node->cog_confirmed)
    {
        target = node->rect;
    }

    LatLong startLatLong(mCenter.lon, mCenter.lat);
    //编号
    trackObj->set_tracknumber(rect_num);
    //速度
    trackObj->set_sog(target->sog());
    //方向
    trackObj->set_cog(target->cog());
    if(!node->cog_confirmed)
    {
        trackObj->set_sog(0.0);
        trackObj->set_cog(0);
        trackObj->set_trackconfirmed(false);
    } else
    {
        trackObj->set_trackconfirmed(true);
    }

    //经纬度
    trackObj->set_wgs84poslat(target->centerlatitude());
    trackObj->set_wgs84poslong(target->centerlongitude());
    //笛卡尔坐标
    double x,y;
    double lat = trackObj->wgs84poslat();
    double lon = trackObj->wgs84poslong();
    getDxDy(startLatLong, lat, lon, x, y);
    trackObj->set_cartesianposx(x);
    trackObj->set_cartesianposy(y);
    //other
    trackObj->set_timeofday(target->timeofday());
    trackObj->set_systemareacode(0);
    trackObj->set_systemidentificationcode(1);
    trackObj->set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
    trackObj->set_tracklastreport(false);
    trackObj->set_cartesiantrkvel_vx(0);
    trackObj->set_cartesiantrkvel_vy(0);
    //直径
    trackObj->set_diameter(target->diameter());

//    qDebug()<<trackObj->has_systemidentificationcode()<<trackObj->systemidentificationcode()<<trackObj->has_tracknumber()<<trackObj->tracknumber();
}

void zchxRadarTargetTrack::updateRectMapWithNode(zchxRadarRectMap &map, TargetNode *node)
{
    if(!node) return;
    int rect_num = node->rect->rectnumber();

    //开始更新余晖数据
    zchxRadarRect rect;
    rect.set_estcount(node->est_count);
    rect.set_dirconfirmed(node->cog_confirmed);
    if(node->cog_confirmed)
    {
        //需要更新历史数据
        TargetNode * latest_node = node->getLastChild();
        if(!latest_node) return;
        //当前的矩形数据
        rect.mutable_currentrect()->CopyFrom(*(latest_node->rect));
        rect.mutable_currentrect()->set_rectnumber(rect_num);
        //历史的矩形数据
        TargetNode* work_node = node;
        QList<TargetNode*> route_list;      //遍历的路径
        while (work_node) {
            route_list.prepend(work_node); //从后面开始添加路径,时间越靠近的在最前面
            if(work_node == latest_node) break;
            if(work_node->children.size())
            {
                work_node = work_node->children.first().data();
            } else
            {
                break;
            }
        }
        for(int i=0; i<route_list.size(); i++)
        {
            zchxRadarRectDef *his_rect = rect.add_historyrects();
            his_rect->CopyFrom(*(route_list[i]->rect));
        }
    } else
    {
        //不需要更新历史数据,只更新目标的最初数据
        rect.mutable_currentrect()->CopyFrom(*(node->rect));
#if 0
        // 这里便于调试,将所有的节点的预推区域也加上
        for(int i=0; i<node->children.size(); i++)
        {
            TargetNode *worknode = node->children[i].data();
            while (worknode)
            {
                if(worknode->rect->predictionareas_size() > 0)
                {
                    rect.mutable_currentrect()->add_predictionareas()->CopyFrom(worknode->rect->predictionareas(0));
                }
                if(worknode->children.size() == 0) break;
                worknode = worknode->children[0].data();
            }
        }
#endif
    }
    map[rect_num] = rect;
}

void zchxRadarTargetTrack::outputTargets()
{
    zchxRadarSurfaceTrack   track_list;         //雷达目标数据输出
    track_list.set_flag(1);
    track_list.set_sourceid("240");
    track_list.set_utc(QDateTime::currentMSecsSinceEpoch());

    zchxRadarRectMap        rect_map;           //目标回波输出
    //遍历目标进行数据组装
    for(TargetNodeMap::iterator it = mTargetNodeMap.begin(); it != mTargetNodeMap.end(); it++)
    {
        TargetNode *node = it->data();
        if(!node) continue;
        zchxRadarRectDef *top_rect = node->rect;
        if(!top_rect) continue;

        //只输出确认的点迹和轨迹
        if(node->isMotionlessObj() || node->cog_confirmed)
        {
            //先构造目标数据
            updateTrackPointWithNode(track_list, node);
            //添加余晖矩形数据
            updateRectMapWithNode(rect_map, node);
        }
    }
    track_list.set_length(track_list.trackpoints_size());
    if(track_list.trackpoints_size())
    {
//        qDebug()<<"data to be send time:"<<QDateTime::currentDateTime();
        emit signalSendTracks(track_list);
    }

    if(rect_map.size()) emit signalSendRectData(rect_map);
}

void zchxRadarTargetTrack::appendNode(TargetNode *node, int source)
{
    if(!node) return;
    node->cog_confirmed = false;
    int rect_num = getCurrentRectNum();
    if(!node->rect) return;
    node->rect->set_rectnumber(rect_num);
    if(source == 0)
    {
        qDebug()<<"make new now from orignal rect:"<<node<<node->rect<<node->rect->rectnumber();
    } else
    {
        qDebug()<<"make new now from ununsed route rect:"<<node<<node->rect<<node->rect->rectnumber();
    }
    mTargetNodeMap.insert(rect_num, QSharedPointer<TargetNode>(node));
    if(track_debug) qDebug()<<"new node maked now:"<<node->rect->rectnumber()<<node->rect->timeofday();
}

int zchxRadarTargetTrack::getCurrentRectNum()
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
    double list_time = task.first().timeofday();

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
            int    rect_num = it->currentrect().rectnumber();
            double old_time = it.value().currentrect().timeofday();
            double old_sog = it.value().currentrect().sog();
            double old_lat = it.value().currentrect().centerlatitude();
            double old_lon = it.value().currentrect().centerlongitude();

            //检查目标的态势是否已经确定.目标态势确定的方法就是最近出现了连续三笔的数据的方向大体一致  目标没有回头的情况
            //目标态势确定的情况,已经存在的目标的方向角就是已经经过平均估计的方向角
            //否则就是目标的实际方向角
            double est_cog = it->currentrect().cog();
            //计算当前的时间间隔, 根据时间间隔算预估位置点
            float delta_time = list_time - old_time;

            zchxRadarRectDefList merge_list;
            int cur_est_cnt = it->estcount();
            //这里开始进行位置的预估判断,
            //目标的态势已经确定,则根据的预推位置确定目标,目标没有找到,则将预推位置估计为目标位置
            //目标的态势未确定,则根据距离来判断目标的可能位置
            if(it->dirconfirmed())
            {
                if(track_debug) qDebug()<<"current target dir is confirmed."<<rect_num<<" cog:"<<est_cog;
                //计算当前目标可能的预估位置
                double est_lat = 0.0, est_lon = 0.0;
                if(delta_time < 0) delta_time += (3600* 24);
                float est_distance = old_sog * delta_time;
                distbearTolatlon1(old_lat, old_lon, est_distance, est_cog, &est_lat, &est_lon);
                QGeoCoordinate last_pos(old_lat, old_lon);
                //从最新的目标矩形框中寻找预估位置附件的点列
                for(int k = 0; k<temp_list.size();)
                {
                    zchxRadarRectDef next = temp_list[k];
                    //计算两者的距离
                    double distance = getDisDeg(est_lat, est_lon, next.centerlatitude(),next.centerlongitude());
                    if(distance < target_merge_distance)
                    {
                        //计算二者的方向角,待选取的点必须在这个点的前方
                        QGeoCoordinate cur_pos(next.centerlatitude(), next.centerlongitude());
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
                    fakeData.CopyFrom(it->currentrect());
                    fakeData.set_timeofday(list_time);
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
                    double distance = getDisDeg(old_lat, old_lon, next.centerlatitude(),next.centerlongitude());
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
                if(track_debug) qDebug()<<"next target not found. now next loop"<<rect_num<<" dir confirmed:"<<it->dirconfirmed();
                continue;
            }
            if(track_debug) qDebug()<<"target need to be merge with size:"<<merge_list.size();
            //根据找到的目标进行位置更新
            //过滤筛选符合方位角要求的目标
            zchxRadarRectDefList target_list;
            if(it->historyrects_size() == 0)
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
                    QGeoCoordinate cur_pos(cur.centerlatitude(), cur.centerlongitude());
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
                it->mutable_currentrect()->set_timeofday(list_time);
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
            total.set_dirconfirmed(it->dirconfirmed());  //初始化目标是否确认方向
            total.set_estcount(cur_est_cnt);
            total.mutable_currentrect()->CopyFrom(target);
            //旧目标添加历史轨迹点的0号位置
            zchxRadarRectDef *single_Rect = total.add_historyrects();
            single_Rect->CopyFrom(it->currentrect());
            //移动就目标的历史轨迹点到新目表的历史轨迹点.历史轨迹点最大数目为20
            int update_his_size = 20 - total.historyrects_size();
            int k = 0;
            for(int i= 0; i<it->historyrects_size(); i++)
            {
                if(k >= update_his_size) break;
                zchxRadarRectDef *history = total.add_historyrects();
                history->CopyFrom(it->historyrects(i));
                k++;
            }
            //更新速度角度
            if(total.historyrects_size() > 0)
            {
                zchxRadarRectDef* last = total.mutable_historyrects(0);
                QGeoCoordinate last_pos(last->centerlatitude(), last->centerlongitude());
                QGeoCoordinate cur_pos(total.currentrect().centerlatitude(), total.currentrect().centerlongitude());
                total.mutable_currentrect()->set_cog(last_pos.azimuthTo(cur_pos));
                //检查新目标的方向是否与原来确定的目标方向相同
                if(it->dirconfirmed())
                {
                    if(isDirectionChange(it->currentrect().cog(), total.currentrect().cog()))
                    {
                        //目标反方向了, 不再继续更新
                        if(track_debug) qDebug()<<"target go to opposite..when update new sog cog  continue yet..";
                        continue;
                    }
                }
                double delta_time = total.currentrect().timeofday() - last->timeofday();
                if(delta_time <= 0) delta_time += (3600 * 24);
                double cal_dis = last_pos.distanceTo(cur_pos);
                total.mutable_currentrect()->set_sog( cal_dis / delta_time);
                if(total.historyrects_size() == 1)
                {
                    //添加第一个历史轨迹点
                    //第一个点的方向还未确定,默认赋值为第二个点的角度
                    last->set_cog(total.currentrect().cog());
                }
                QList<double> history_cog;
                for(int i=0; i<total.historyrects_size(); i++)
                {
                    history_cog.append(total.historyrects(i).cog());
                }
                if(track_debug) qDebug()<<"now history cogs:"<<history_cog;


                //           cout<<current.currentrect().rectnumber()<<"dis:"<<cal_dis<<"time_gap:"<<delta_time<<"speed:"<<current.currentrect().sog()<< current.currentrect().timeofday()<< last.timeofday();
            }
            //目标没有跳跃,更新目标的方位角为平均值
            if(!total.dirconfirmed())
            {
                if(track_debug) qDebug()<<"now check target is dir stable or not...."<<total.currentrect().cog();
                double avg_cog = 0;
                int target_dir = getTargetDirectStatus(total, target_check_num, &avg_cog);
                if( target_dir == TARGET_DIRECTION_STABLE)
                {
                    total.set_dirconfirmed(true);
                    if(cog_avg_adjust)total.mutable_currentrect()->set_cog(avg_cog);
                    if(track_debug) qDebug()<<"taregt is stable now. with new cog:"<<total.currentrect().cog();

                } else if(target_dir == TARGET_DIRECTION_UNSTABLE)
                {

                    //现在进行最后的确认.检查目标是否是来回地跳来跳去,如果是,删除跳来跳去的轨迹点,保留最初的点
                    if(track_debug) qDebug()<<"now start check new target is jumping or not"<<"  time:"<<total.currentrect().timeofday()<< " size:"<<total.historyrects_size();
                    int jump_num = target_check_num + 1;
                    if(isTargetJumping(total, target_merge_distance, jump_num))
                    {
                        //将目标退回到check_num对应的轨迹点
                        int restore_history_index = jump_num - 2;
                        if(restore_history_index > total.historyrects_size())
                        {
                            //异常情况
                            continue;
                        }
                        zchxRadarRectDef now = total.historyrects(restore_history_index);
                        total.mutable_currentrect()->CopyFrom(now);
                        total.mutable_historyrects()->DeleteSubrange(0, restore_history_index + 1);
                        if(track_debug) qDebug()<<"target is jumping... move to old one:"<<total.currentrect().timeofday()<<" size:"<<total.historyrects_size();

                    }
                } else
                {
                    //目标的点列数还不够,难以判断
                }
            } else if(cog_avg_adjust)
            {
                //根据最近的点列的坐标更新当前点的方位角
                zchxRadarRectDefList list;
                list.append(total.currentrect());
                for(int i=0; i<total.historyrects_size(); i++)
                {
                    list.append(total.historyrects(i));
                    if(list.size() == 5) break;
                }
                double avg_cog = calAvgCog(list);
                total.mutable_currentrect()->set_cog(avg_cog);
            }

            if(total.historyrects_size() < target_check_num)
            {
                if(track_debug) qDebug()<<"target is dir not stable  ....for not enough history size";
                total.set_dirconfirmed(false);
            }

            //将就目标的历史轨迹清除,只有新目标保存历史轨迹
            it->clear_historyrects();
            //将新目标更新到队列中
            it->CopyFrom(total);
            //           if(track_debug) qDebug()<<"update exist target:"<<it->currentrect().rectnumber()<<it->currentrect().timeofday()<<target.timeofday();
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
        rect.set_dirconfirmed(false);
        rect.set_estcount(0);
        rect.mutable_currentrect()->CopyFrom(temp_list[i]);
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
        rect.mutable_currentrect()->set_rectnumber(mRectNum++);
        mRadarRectMap[rect.currentrect().rectnumber()] = rect;
        if(track_debug) qDebug()<<"new rect maked now:"<<rect.currentrect().rectnumber()<<rect.currentrect().timeofday();
    }

//    counter.reset("make new target");

    checkTargetRectAfterUpdate(target_merge_distance);

//    counter.reset("check target after new");

    //   if(0){
    //       zchxRadarRectDefList list;
    //       foreach (zchxRadarRect rect, mRadarRectMap) {
    //           list.append(rect.currentrect());
    //       }
    //       exportRectDef2File(list, QString("%1_rect").arg(QDateTime::currentMSecsSinceEpoch()));
    //   }

    //清理目标,删除超时未更新的目标
    double time_of_day = timeOfDay();
    QList<int> allKeys = mRadarRectMap.keys();
    foreach (int key, allKeys) {
        zchxRadarRect obj = mRadarRectMap[key];
        if((time_of_day - obj.currentrect().timeofday()) > mClearTrackTime)
        {
            mRadarRectMap.remove(key);
            continue;
        }
        if(obj.estcount() >= 10)
        {
            mRadarRectMap.remove(key);
        }
    }

//    counter.reset("remove old target");

    //检查目标之间的距离值
    dumpTargetDistance("all target with new ", target_merge_distance);

//    counter.reset("check all target");

    //连续存在3笔数据的认为是目标，然后输出
    //目标构造
    zchxRadarSurfaceTrack list;
    list.set_flag(1);
    list.set_sourceid("240");
    list.set_utc(QDateTime::currentMSecsSinceEpoch());
    foreach (zchxRadarRect rectObj, mRadarRectMap)
    {

        zchxRadarRectDef target;
        target.CopyFrom(rectObj.currentrect());
        zchxTrackPoint *trackObj = list.add_trackpoints();
        LatLong startLatLong(mCenter.lon, mCenter.lat);
        //编号
        trackObj->set_tracknumber(target.rectnumber());
        //速度
        trackObj->set_sog(target.sog());
        //方向
        trackObj->set_cog(target.cog());
        if(!rectObj.dirconfirmed())
        {
            trackObj->set_sog(0.0);
            trackObj->set_trackconfirmed(false);
            trackObj->set_cog(0);
        } else
        {
            trackObj->set_trackconfirmed(true);
        }

        //经纬度
        trackObj->set_wgs84poslat(target.centerlatitude());
        trackObj->set_wgs84poslong(target.centerlongitude());
        //笛卡尔坐标
        double x,y;
        double lat = trackObj->wgs84poslat();
        double lon = trackObj->wgs84poslong();
        getDxDy(startLatLong, lat, lon, x, y);
        trackObj->set_cartesianposx(x);
        trackObj->set_cartesianposy(y);
        //other
        trackObj->set_timeofday(target.timeofday());
        trackObj->set_systemareacode(0);
        trackObj->set_systemidentificationcode(1);
        trackObj->set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
        trackObj->set_tracklastreport(false);
        trackObj->set_cartesiantrkvel_vx(0);
        trackObj->set_cartesiantrkvel_vy(0);
        //直径
        trackObj->set_diameter(target.diameter());
    }
    list.set_length(list.trackpoints_size());
    emit signalSendTracks(list);
    emit signalSendRectData(mRadarRectMap);
//    counter.reset("make track object");
}


void zchxRadarTargetTrack::dumpTargetDistance(const QString &tag, double merge_dis)
{
    if(track_debug) qDebug()<<"dump target distance now:"<<tag;
    QList<int> keys = mRadarRectMap.keys();
    for(int i=0; i<keys.size(); i++)
    {
        zchxRadarRect cur = mRadarRectMap[keys[i]];
        double cur_lat = cur.currentrect().centerlatitude();
        double cur_lon = cur.currentrect().centerlongitude();
        for(int k = i+1; k<keys.size(); k++)
        {
            zchxRadarRect next = mRadarRectMap[keys[k]];
            double next_lat = next.currentrect().centerlatitude();
            double next_lon = next.currentrect().centerlongitude();
            double distance = getDisDeg(cur_lat, cur_lon, next_lat, next_lon);
            if(distance < merge_dis)
            {
                if(track_debug) qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.currentrect().timeofday()<<next.currentrect().timeofday();
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
        double cur_lat = cur.currentrect().centerlatitude();
        double cur_lon = cur.currentrect().centerlongitude();
        for(int k = i+1; k<keys.size(); k++)
        {
            if(!mRadarRectMap.contains(keys[k])) continue;
            zchxRadarRect next = mRadarRectMap[keys[k]];
            double next_lat = next.currentrect().centerlatitude();
            double next_lon = next.currentrect().centerlongitude();
            double distance = getDisDeg(cur_lat, cur_lon, next_lat, next_lon);
            if(distance < merge_dis)
            {
                if(cur.dirconfirmed() ^ next.dirconfirmed())
                {
                    if(track_debug) qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" confirmed:"<<cur.dirconfirmed()<<next.dirconfirmed()<<" remove not confirmed one.";

                    if(cur.dirconfirmed() && !next.dirconfirmed())
                    {
                        mRadarRectMap.remove(keys[k]);
                        continue;
                    }
                    if((!cur.dirconfirmed()) && next.dirconfirmed())
                    {
                        mRadarRectMap.remove(keys[i]);
                        break;
                    }
                } else
                {
                    if(track_debug) qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.currentrect().timeofday()<<next.currentrect().timeofday()<<" remove old one.";

                    if(next.currentrect().timeofday() <= cur.currentrect().timeofday())
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

