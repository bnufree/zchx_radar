#include "zchxradartargettrack.h"
#include <QDebug>
#include <QGeoCoordinate>

const bool track_debug = false;

zchxRadarTargetTrack::zchxRadarTargetTrack(int id, const Latlon& ll, int clear_time, QObject *parent)
    : QThread(parent)
    , mCenter(ll)
    , mClearTrackTime(clear_time)
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
    task = mTaskList.last();
    if(track_debug) qDebug()<<"delete unprocessed task size:"<<mTaskList.size() - 1;
    mTaskList.clear();
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

void zchxRadarTargetTrack::process(const zchxRadarTrackTask &task)
{
    if(task.size() == 0) return;
    zchxTimeElapsedCounter counter("target track");
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

