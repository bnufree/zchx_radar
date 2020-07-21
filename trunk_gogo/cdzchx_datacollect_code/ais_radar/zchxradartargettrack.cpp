#include "zchxradartargettrack.h"
#include <QDebug>
#include <QGeoCoordinate>

const bool track_debug = true;

const double point_near_line = 200;
const double ship_max_speed = 5;            //5m/s ~~ 10节  10m/s 就是20节

#define     PATH_CONFIRM_NODE_COUNT         5

//double getDeltaTime(float now, float old)
//{
//    double delta = now - old;
//    if(delta < 0)
//    {
//        delta += 3600 * 24;
//    }
//    return delta;
//}

zchxRadarTargetTrack::zchxRadarTargetTrack(int id, const Latlon& ll, int clear_time, double predictionWidth, bool route, QObject *parent)
    : QThread(parent)
    , mCenter(ll)
    , mClearTrackTime(clear_time)
    , mProcessWithRoute(route)
    , mMaxEstCount(5)
    , mRangeFactor(10)
    , mPredictionWidth(predictionWidth)
{
    mDirectionInvertThresholdVal = 60.0;
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

zchxRadarRectDefList zchxRadarTargetTrack::getDirDeterminTargets(bool& isTargetInVideo, zchxRadarRectDefList &list, zchxRadarRectDef* src, bool cog_usefull)
{
    isTargetInVideo = false;
    zchxRadarRectDefList result;
    if(list.size() == 0 || !src) return result;
    quint32 list_time = list.first().updatetime();
    quint32 old_time = src->updatetime();
    double old_sog = src->sog();
    double old_lat = src->centerlatitude();
    double old_lon = src->centerlongitude();
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
            src->clear_predictionareas();
            QList<Latlon> area = line.getPredictionAreaLL();
            foreach (Latlon ll, area) {
                com::zhichenhaixin::proto::singleVideoBlock* block = src->mutable_predictionareas()->add_area();
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
                double dir = Mercator::angle(old_lat, old_lon, next.centerlatitude(), next.centerlongitude());
                //目标的方向已经有了,也就是目标也不是初次出现,需要检查方向.
                if(cog_usefull && isDirectionChange(old_cog, dir)) break;
                //再次检查速度,速度不能超过目标的最大速度,如果超过了,就将位置更新到最大速度的位置
                Mercator now = latlonToMercator(next.centerlatitude(), next.centerlongitude());
                //计算点到预估位置的距离
                double distance = now.distanceToPoint(old_lat, old_lon);
                if(distance / delta_time > ship_max_speed)
                {
                    //更新next的经纬度
                    distance = delta_time * ship_max_speed;
                    QGeoCoordinate est_pos = QGeoCoordinate(old_lat, old_lon).atDistanceAndAzimuth(distance, dir);
                    next.set_centerlatitude(est_pos.latitude());
                    next.set_centerlongitude(est_pos.longitude());
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
    for(int i=0; i<rect.blocks_size();i++)
    {
        zchxSingleVideoBlock block = rect.blocks(i);
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
    if(!topNode || !topNode->cog_confirmed) return;
    //当前的时间确认
    if(left_list.size() == 0) return;
    quint32 list_time = left_list.first().updatetime();
    //获取最后一次更新的节点
    TargetNode *last_update_node = topNode->getLastChild();
    //没有路径节点的情况
    if(!last_update_node || last_update_node == topNode) return;


    //根据已知的方向和速度预测目标的当前位置
    zchxRadarRectDef* pre_rect = last_update_node->rect;
    if(!pre_rect) return;

    int    rect_num = pre_rect->rectnumber();
    quint32 old_time = pre_rect->updatetime();
    double old_sog = pre_rect->sog();
    double old_lat = pre_rect->centerlatitude();
    double old_lon = pre_rect->centerlongitude();
    double old_cog = pre_rect->cog();
    //计算当前的时间间隔, 根据时间间隔算预估位置点
    quint32 delta_time = list_time - old_time;

    int cur_est_cnt = topNode->est_count;
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
        cur_est_cnt = 0;
        if(track_debug) qDebug()<<"next target found, distance to estmation:"<<rect_num;

    } else
    {        
        if(isTargetInVideo)
        {
            qDebug()<<"now target is in a video area with center at a opposite direction. make is as not move. only update its update_time";
            pre_rect->set_updatetime(list_time);
            topNode->update_time = list_time;
            topNode->est_count = 0;
            pre_rect->set_sog(0);  //认为目标在当前位置静止
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
        cur_est_cnt++;
    }

    //更新目标开始
    topNode->est_count = cur_est_cnt;
    topNode->update_time = list_time;
    pre_rect->set_updatetime(list_time);

    double dir = Mercator::angle(old_lat, old_lon, dest.centerlatitude(), dest.centerlongitude());
    //目标的方向已经有了,也就是目标也不是初次出现,需要检查方向.
    if(isDirectionChange(old_cog, dir))
    {
        qDebug()<<"taregt direction should be same as old one, but now found a oppsite one. abnormal...";
        return;
    }


    //计算新目标和就目标之间的距离
    double distance = getDisDeg(old_lat, old_lon, dest.centerlatitude(), dest.centerlongitude());
    if(distance < 1.0)
    {
        //目标的距离太近,认为目标没有移动, 不进行处理
        if(track_debug) qDebug()<<"new destination too closed. not update. continue..."<<distance;
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
    quint32 list_time = left_list.first().updatetime();

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
            qDebug()<<"child number:"<<node_number<<" time:"<<pre_rect->updatetime()<<" sog:"<<pre_rect->sog()<<" cog:"<<pre_rect->cog()<<" route id:"<<i;

            //根据分支节点的速度和角度取得待更新的矩形单元,这里的矩形单元数只有一个
            bool isTargetInVideo = false;
            zchxRadarRectDefList result = getDirDeterminTargets(isTargetInVideo, left_list, pre_rect, true);
            if(result.size() == 0)
            {
                if(isTargetInVideo)
                {
                    pre_rect->set_updatetime(list_time);
                    topNode->update_time = list_time;
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

            double cog = Mercator::angle(pre_rect->centerlatitude(), pre_rect->centerlongitude(), now_rect.centerlatitude(), now_rect.centerlongitude());
            quint32 delta_time = list_time - pre_rect->updatetime();
            double cal_dis = Mercator::distance(pre_rect->centerlatitude(), pre_rect->centerlongitude(), now_rect.centerlatitude(), now_rect.centerlongitude());
            //检查目标的距离和方向
            if(cal_dis < mRangeFactor)
            {
                //目标没有移动处理
                pre_rect->set_updatetime(list_time);
                topNode->update_time = list_time;
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
            if(pre_rect->sog() > 0 && sog < 0.001)
            {
//                sog = pre_rect->sog();
//                cog = pre_rect->cog();
            }
            now_rect.set_sog(sog);
            now_rect.set_cog(cog);
            last_child->children.append(QSharedPointer<TargetNode>(new TargetNode(now_rect)));
            topNode->update_time = list_time;
            if(track_debug) qDebug()<<"update root node:"<<pre_rect->rectnumber()<<" with possible route index"<<i<<"  cog "<<cog<<" sog:"<<sog;
        }
    } else
    {
        //分支目标不存在,再次更新根节点
         bool isTargetInvideo = false;
        zchxRadarRectDefList result = getDirDeterminTargets(isTargetInvideo, left_list, topNode->rect, false);
        pre_rect = topNode->rect;
        bool root_update = false;
        for(int i=0; i<result.size(); i++)
        {
            zchxRadarRectDef total = result[i];
            total.set_realdata(true);
            total.set_rectnumber(node_number);
            double cog = Mercator::angle(pre_rect->centerlatitude(), pre_rect->centerlongitude(), total.centerlatitude(), total.centerlongitude());
            quint32 delta_time = list_time - pre_rect->updatetime();
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
                if(track_debug) qDebug()<<"update root node:"<<pre_rect->rectnumber()<<" with possible child:(distance, cog, sog, time) "<<cal_dis<<cog<<sog<<total.updatetime();
                root_update = true;
            }
        }
        //将目标对象的更新时间更新到列表时间
        if(result.size() > 0 || isTargetInvideo)   topNode->update_time = list_time;
        if((result.size() > 0 && !root_update) || isTargetInvideo)
        {
            //所有目标都距离太近, 证明目标此时没有移动,作为静止目标处理
            topNode->rect->set_updatetime(list_time);
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
        //第3个节点存在
        if(child_lvl2->children.size() == 0) continue;
        TargetNode *child_lvl3 = child_lvl2->children[0].data();
        if(!child_lvl3) continue;
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
//    zchxTimeElapsedCounter counter(__FUNCTION__);
    zchxRadarRectDefList temp_list(task);             //保存的未经处理的所有矩形单元
    quint32 now_time = task.first().updatetime();
    qDebug()<<"now process list time:"<<QDateTime::fromTime_t(task.first().updatetime()).toString("yyyy-MM-dd hh:mm:ss");

    QList<AreaNodeTable> areaTableList = calculateTargetTrackMode(10.0, task.first().updatetime(), 3.2);
    QList<int>  used_index_list;
    qDebug()<<"exist prediction area size:"<<areaTableList.size();
    if(areaTableList.size() > 0)
    {
        //先检查每一个区域内落入了哪些目标
        for(int i=0; i<areaTableList.size(); i++)
        {
            AreaNodeTable &table = areaTableList[i];
            for(int k=0; k<temp_list.size(); k++)
            {
                zchxRadarRectDef def = temp_list[k];
                Mercator pos = latlonToMercator(def.centerlatitude(), def.centerlongitude());
                if(table.mArea.containsPoint(pos.toPointF(), Qt::OddEvenFill))
                {
                    table.mRectList.append(def);
                }
            }

        }
        //开始统计整理，每一个节点现在有多少个待选节点
        QMap<TargetNode*, zchxRadarRectDefList> counter_map;
        foreach (AreaNodeTable table, areaTableList) {
            foreach (TargetNode* node, table.mNodeList) {
                counter_map[node].append(table.mRectList);
            }
        }
        //开始将新的目标更新到旧目标对应的路径
        QMap<TargetNode*, zchxRadarRectDefList>::iterator it = counter_map.begin();
        for(; it != counter_map.end(); it++)
        {
            TargetNode* node = it.key();
            if(!node) continue;
            zchxRadarRectDefList targetList = it.value();
            if(targetList.size() > 0)
            {
                //从候选目标中移除距离值介于（0~距离因子）的目标点。剩余点就是符合要求的点
                //如果存在多个目标点，选择距离预推运动点最近的目标
                int    index = -1;
                double delta_time = now_time - node->rect->updatetime();
                QGeoCoordinate source(node->rect->centerlatitude(), node->rect->centerlongitude());
                //计算目标的预推位置
                double delta_dis = node->rect->sog() * delta_time;
                QGeoCoordinate dest = source.atDistanceAndAzimuth(delta_dis, node->rect->cog());
                int    min_dis = INT32_MAX;
                for(int i=0; i<targetList.size();i++)
                {
                    zchxRadarRectDef def = targetList[i];
                    double dis = dest.distanceTo(QGeoCoordinate(def.centerlatitude(), def.centerlongitude()));
                    if(dis > 1.0 && dis < mRangeFactor)
                    {
                        qDebug()<<"abnoraml target found. all target distance is smaller than range factor, but not zero"<<dis<<mRangeFactor;
                        continue;
                    }
                    if(dis < min_dis)
                    {
                        min_dis = dis;
                        index = i;
                    }
                }
                if(index == -1)
                {
                    //所有目标的都不符合要求

                    targetList.clear();
                } else
                {
                    zchxRadarRectDef def =  targetList.takeAt(index);
                    targetList.clear();
                    targetList.append(def);
                }
            }

            if(targetList.size() == 0)
            {
                //检查当前目标是否落在了某一个回波图形上
                for(int i=0; i<temp_list.size(); i++)
                {
                    zchxRadarRectDef rect = temp_list[i];
                    if(isRectAreaContainsPoint(rect, node->rect->centerlatitude(), node->rect->centerlongitude()))
                    {
                        targetList.append(rect);
                        break;
                    }
                }
                if(targetList.size() > 0)
                {
                    //将节点自动移动到当前的回波图形重心。
                    zchxRadarRectDef target;
                    target.CopyFrom(targetList[0]);
                    if(!used_index_list.contains(target.rectnumber())) used_index_list.append(target.rectnumber());

                    target.set_realdata(true);
                    target.set_rectnumber(node->rect->rectnumber());

                    if(node->parent)
                    {
                        //取得节点对应的父节点
                        QGeoCoordinate source(node->parent->rect->centerlatitude(), node->parent->rect->centerlongitude());
                        QGeoCoordinate dest(target.centerlatitude(),target.centerlongitude());
                        double cog = source.azimuthTo(dest);
                        double distance = source.distanceTo(dest);
                        int delta_time = target.updatetime() - node->parent->rect->updatetime();
                        double sog = 0.0;
                        if(delta_time > 0) sog = distance / delta_time;
                        //开始平均速度和角度
                        double refer_sog = node->getReferenceSog();
                        if(refer_sog > 0.1)
                        {
                            sog = (sog + refer_sog) / 2.0;
                        }
//                        double refer_cog = node->getReferenceCog();
//                        if(refer_cog > 0.1)
//                        {
//                            cog = (cog + refer_cog) / 2.0;
//                        }

                        //
                        target.set_cog(cog);
                        target.set_sog(sog);
                    } else
                    {
                        //节点本身就是根节点，不用计算速度角度
                        target.set_sog(0.0);
                        target.set_cog(0.0);
                    }

                    node->rect->CopyFrom(target);
                }

                continue;
            }

            //获取选择的目标，更新速度等
            zchxRadarRectDef target;
            target.CopyFrom(targetList.first());
            if(!used_index_list.contains(target.rectnumber())) used_index_list.append(target.rectnumber());

            target.set_realdata(true);
            target.set_rectnumber(node->rect->rectnumber());
            QGeoCoordinate source(node->rect->centerlatitude(), node->rect->centerlongitude());
            QGeoCoordinate dest(target.centerlatitude(), target.centerlongitude());
            double delta_time = now_time - node->rect->updatetime();
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
//            double refer_cog = node->getReferenceCog();
//            if(refer_cog > 0.1)
//            {
//                cog = (cog + refer_cog) / 2.0;
//            }

            //
            target.set_cog(cog);
            target.set_sog(sog);
            node->children.append(QSharedPointer<TargetNode>(new TargetNode(target, node)));
        }
    }


    //将未更新的矩形作为新目标添加
    for(int i=0; i<temp_list.size(); i++)
    {
        zchxRadarRectDef rect = temp_list[i];
        if(used_index_list.contains(rect.rectnumber())) continue;
        TargetNode *node = new TargetNode(rect);
        appendNode(node, 0);
    }
    //开始检查已经存在的目标的更新情况，如果路径节点数大于N，则目标确认输出。目标位置变化不大，就是静止，否则就是运动
    QList<int> exist_numbers = mTargetNodeMap.keys();
    foreach (int key, exist_numbers)
    {
        QSharedPointer<TargetNode> node = mTargetNodeMap[key];
        if(!node || node->children.size() == 0) continue;
        //检查目标信息,查看目标的可能路径是否已经满足确定条件，
        if(node->status  != TargetNode::TARGET_POINT) continue;  //目标的路径信息已经确定，不处理
        int size = node->children.size();
        int path_index = -1;
        for(int i=0; i<size; i++)
        {
            //从这个子节点开始遍历，开始构造路径
            QList<TargetNode*> list;
            list.append(node.data());
            TargetNode* current = node->children[i].data();
            if(!current) continue;
            while (current) {
                list.append(current);
                if(current->children.size() > 0)
                {
                    current = current->children.first().data();

                } else
                {
                    break;
                }
            }
            TargetNode::TargetStatus sts = checkRoutePathSts(list);
            if(sts == TargetNode::TARGET_MOVING)
            {
                path_index = i;
                node->status = sts;
                list.last()->status = sts;
                if(sts == TargetNode::TARGET_MOVING)
                {
                    node->cog_confirmed = true;
                } else
                {
                    node->not_move_cnt = 5;
                }
            }
            if(path_index >= 0) break;
        }
        if(path_index < 0) continue;
        //开始将确认的没有确认的路径进行重新分离编号
        splitAllRoutesIntoTargets(node.data(), node->children[path_index].data());
        node->updateRouteNodePathStatus(TargetNode::TARGET_MOVING);
    }
    //路径节点的时间重新更新
    foreach (QSharedPointer<TargetNode> node, mTargetNodeMap)
    {
        if(!node) continue;
        node->setAllNodeRectNum(node->rect->rectnumber());
        QList<TargetNode*> children = node->getAllBranchLastChild();
        foreach (TargetNode* child, children) {
            if(node->update_time < child->update_time)
            {
                node->update_time = child->update_time;
            }
        }
    }


    //删除很久没有更新的目标点
    deleteExpiredNode();
    //现在将目标进行输出
    outputTargets();
//    outputRoutePath();
    qDebug()<<"process end now";
#endif
}

TargetNode::TargetStatus zchxRadarTargetTrack::checkRoutePathSts(QList<TargetNode *> path)
{
    if(path.size() < PATH_CONFIRM_NODE_COUNT) return TargetNode::TARGET_POINT;
    quint32 start_time = path.first()->rect->updatetime();
    quint32 end_time = path.last()->rect->updatetime();
    quint32 delta_time = end_time - start_time;
    if(delta_time == 0) return TargetNode::TARGET_POINT;
    QGeoCoordinate start_pos(path.first()->rect->centerlatitude(), path.first()->rect->centerlongitude());
    QGeoCoordinate end_pos(path.last()->rect->centerlatitude(), path.last()->rect->centerlongitude());
    double distance = start_pos.distanceTo(end_pos);
    double sog = distance / delta_time;
    if(sog < 1.0) return TargetNode::TARGET_POINT;
    return TargetNode::TARGET_MOVING;
}

QList<AreaNodeTable> zchxRadarTargetTrack::calculateTargetTrackMode(double max_speed, quint32 now, double scan_time)
{
    QList<AreaNodeTable> areaNodeTableList;
    if(mTargetNodeMap.size() == 0) return areaNodeTableList;
    //通过计算目标的预推区域来计算区域是否存在相交，追越， 平行相遇等模型
    QList<PredictionNode> result_list;
    //1)先计算预推区域
    foreach (QSharedPointer<TargetNode> node, mTargetNodeMap)
    {
        if(!node) continue;

        //检查目标是否已经存在子类末端节点
        QList<TargetNode*> node_list = node->getAllBranchLastChild();
        if(node_list.size() == 0) node_list.append(node.data());
        foreach (TargetNode* tail, node_list)
        {
            double old_speed = tail->rect->sog();
            double old_lat = tail->rect->centerlatitude();
            double old_lon = tail->rect->centerlongitude();
            double old_cog = tail->rect->cog();
            double distance = old_speed * (now - node->update_time);
            if(tail->rect->sog() < 0.1)
            {
                distance = max_speed * scan_time;
            }

            zchxTargetPrediction *prediction = 0;
            if(tail->rect->sog() < 0.1)
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
                tail->rect->clear_predictionareas();
                QList<Latlon> area = prediction->getPredictionAreaLL();
                foreach (Latlon ll, area) {
                    com::zhichenhaixin::proto::singleVideoBlock* block = tail->rect->mutable_predictionareas()->add_area();
                    block->set_latitude(ll.lat);
                    block->set_longitude(ll.lon);
                }
                PredictionNode res;
                res.mNode = tail;
                res.mPrediction = prediction;
                result_list.append(res);
            }
        }
    }
    //开始计算目标预推区域和节点的对应关系
    //对应相交追越相遇模型计算他们的共同区域，并且抽出来，区域被重新分割，然后进行重新赋值
    for(int i=0; i<result_list.size(); i++)
    {
        PredictionNode cur = result_list[i];
        //分别计算相交 追越  相遇的预推区域
        bool found = false;
        QGeoCoordinate cur_geo(cur.mNode->rect->centerlatitude(), cur.mNode->rect->centerlongitude());
        for(int k=i+1; k<result_list.size(); k++)
        {
            if(i==k) continue;
            PredictionNode next = result_list[k];
            //先计算两者之间的实际距离值，距离太远(100m)，就不考虑二者的位置关系
            QGeoCoordinate next_geo(next.mNode->rect->centerlatitude(), next.mNode->rect->centerlongitude());
            if(cur_geo.distanceTo(next_geo) >= 100) continue;
            //相交
            QPolygonF andPoly = cur.mPrediction->getPredictionAreaMC().intersected(next.mPrediction->getPredictionAreaMC());
            if(andPoly.size() > 0)
            {
                found = true;
                AreaNodeTable table;
                table.mArea = andPoly;
                table.mNodeList.append(cur.mNode);
                table.mNodeList.append(next.mNode);
                areaNodeTableList.append(table);
                QPolygonF subPoly = cur.mPrediction->getPredictionAreaMC().subtracted(next.mPrediction->getPredictionAreaMC());
                if(subPoly.size() > 0)
                {
                    table.mArea = subPoly;
                    table.mNodeList.append(cur.mNode);
                    areaNodeTableList.append(table);
                }
                continue;
            }
            //追越或者相遇。计算二者运动方向的夹角，如果是小于5度，将预推区域旋转到同一方向，如果很近或者相交，则满足条件
            double sub_angle = cur.mNode->rect->cog() - next.mNode->rect->cog();
            //同向的判断 二者的角度差不多大小，就是相差5度  如果一个在0度左边，一个在0度右边 就是相差355
            if(fabs(sub_angle) <= 5.0 || fabs(sub_angle) >= 355.0)
            {
                AreaNodeTable table;
                table.mArea = andPoly;
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
                    table.mArea = andPoly;
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

    return areaNodeTableList;
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
    quint32 time_of_day = QDateTime::currentDateTime().toTime_t();
    QList<int> allKeys = mTargetNodeMap.keys();
    foreach (int key, allKeys) {
        QSharedPointer<TargetNode> node = mTargetNodeMap[key];
        if(!node) continue;
        quint32 node_time = node->update_time;
        quint32 delta_time = time_of_day - node_time;
        bool reason1 = (delta_time > /*mClearTrackTime*/120);
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
    trackObj->set_sog(target->sog() * 3.6 / 1.852);
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
    trackObj->set_timeofday(timeOfDay(target->updatetime()));
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
            if(rect.historyrects_size() == 20) break;
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
                if(worknode->rect->has_predictionareas())
                {
                    rect.mutable_currentrect()->mutable_predictionareas()->CopyFrom(worknode->rect->predictionareas());
                }
                if(worknode->children.size() == 0) break;
                worknode = worknode->children[0].data();
            }
        }
#endif
    }
    map[rect_num] = rect;
}

void zchxRadarTargetTrack::outputRoutePath()
{
    static qint64 count = 0;
    zchxRadarRouteNodes nodeList;
    //遍历目标进行数据组装
    for(TargetNodeMap::iterator it = mTargetNodeMap.begin(); it != mTargetNodeMap.end(); it++)
    {
        TargetNode *node = it->data();
        if(!node) continue;
        com::zhichenhaixin::proto::RouteNode* route = nodeList.add_nodes();
        route->mutable_topnode()->CopyFrom(*(node->rect));
        for(int i=0; i<node->children.size();i++)
        {
            TargetNode* child = node->children.at(i).data();
            com::zhichenhaixin::proto::RoutePath *path = route->add_pathlist();
            path->add_path()->CopyFrom(*(child->rect));
            while (child->children.size() > 0) {
                child = child->children[0].data();
                path->add_path()->CopyFrom(*(child->rect));
            }
        }
    }
    if(nodeList.nodes_size() > 0 && count <= 10)
    {
        emit signalSendRoutePath(nodeList);
    }
    count++;
    if(count >= 10000) count = 10000;
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
    if(track_debug) qDebug()<<"new node maked now:"<<node->rect->rectnumber()<<node->rect->updatetime();
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
            int    rect_num = it->currentrect().rectnumber();
            quint32 old_time = it.value().currentrect().updatetime();
            double old_sog = it.value().currentrect().sog();
            double old_lat = it.value().currentrect().centerlatitude();
            double old_lon = it.value().currentrect().centerlongitude();

            //检查目标的态势是否已经确定.目标态势确定的方法就是最近出现了连续三笔的数据的方向大体一致  目标没有回头的情况
            //目标态势确定的情况,已经存在的目标的方向角就是已经经过平均估计的方向角
            //否则就是目标的实际方向角
            double est_cog = it->currentrect().cog();
            //计算当前的时间间隔, 根据时间间隔算预估位置点
            quint32 delta_time = list_time - old_time;

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
                if(delta_time < 0) continue;
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
                it->mutable_currentrect()->set_updatetime(list_time);
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
                quint32 delta_time = total.currentrect().updatetime() - last->updatetime();
                if(delta_time <= 0) continue;
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
                    if(track_debug) qDebug()<<"now start check new target is jumping or not"<<"  time:"<<total.currentrect().updatetime()<< " size:"<<total.historyrects_size();
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
                        if(track_debug) qDebug()<<"target is jumping... move to old one:"<<total.currentrect().updatetime()<<" size:"<<total.historyrects_size();

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
        if(track_debug) qDebug()<<"new rect maked now:"<<rect.currentrect().rectnumber()<<rect.currentrect().updatetime();
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
    quint32 time_of_day = QDateTime::currentDateTime().toTime_t();
    QList<int> allKeys = mRadarRectMap.keys();
    foreach (int key, allKeys) {
        zchxRadarRect obj = mRadarRectMap[key];
        if((time_of_day - obj.currentrect().updatetime()) > mClearTrackTime)
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
        trackObj->set_timeofday(timeOfDay(target.updatetime()));
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
                if(track_debug) qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.currentrect().updatetime()<<next.currentrect().updatetime();
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
                    if(track_debug) qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.currentrect().updatetime()<<next.currentrect().updatetime()<<" remove old one.";

                    if(next.currentrect().updatetime() <= cur.currentrect().updatetime())
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

