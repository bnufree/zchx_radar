#include "zchxradarrectthread.h"
#include <QDebug>
#include <QFile>
#include "qt/zchxMapDatautils.h"

using namespace ZCHX_RADAR_RECEIVER;
using namespace qt;

ZCHXRadarRectThread::ZCHXRadarRectThread(const ZCHX_RadarRect_Param& param, QObject *parent)
    : ZCHXReceiverThread(ZCHX_RECV_RADAR_RECT, param.mSetting, parent)
    , mRectParam(param)
{
    qRegisterMetaType<PROTOBUF_RadarRectList>("PROTOBUF_RadarRectList");
    qRegisterMetaType<PROTOBUF_RadarRectDef>("PROTOBUF_RadarRectDef");
    qRegisterMetaType<QList<ZCHX::Data::ITF_RadarRect>>("const QList<ZCHX::Data::ITF_RadarRect>&");
}

//解析每一次发送的数据帧
void ZCHXRadarRectThread::parseRecvData(const QByteArrayList &list)
{
    if(list.size() == 0) return;
    zchxFuntionTimer t(mRectParam.mSetting.m_sTopicList.join(","));

    PROTOBUF_RadarRectList objRadarRect;
    if(!objRadarRect.ParseFromArray(list.last().data(), list.last().size())) return;

    QList<ZCHX::Data::ITF_RadarRect> dstlist;
    convertZMQ2ZCHX(dstlist, objRadarRect);
    emit sendVideoMsg(mRadarCommonSettings.m_sSiteID, dstlist);
}
//这里将目标最大定位到1000米,图像大小为255*255
void zchxRectVideoFunc::run()
{
    if(!mRect) return;
    int    img_size = 255;
    int     pix_size = 500;
    double center_lat = mRect->mCurrentRect.centerlatitude;
    double center_lon = mRect->mCurrentRect.centerlongitude;
    //将经纬度点对应到图片上.图元的中心经纬度对应图片的中心
    //1)计算中心点对应的墨卡托坐标
    ZCHX::Data::Mercator cen_mct = zchxMapDataUtils::wgs84LatLonToMercator(center_lat, center_lon);
    //2)开始构建图片的二维数组点列
    char *img = new char[img_size * img_size];
    memset(img, 0, img_size * img_size);

    for(int i=0; i<mRect->mCurrentRect.blocks.size(); i++)
    {
        double lat = mRect->mCurrentRect.blocks[i].latitude;
        double lon = mRect->mCurrentRect.blocks[i].longitude;
        ZCHX::Data::Mercator circle_mct = zchxMapDataUtils::wgs84LatLonToMercator(lat, lon);
        //计算当前点对应中心点之间的距离
        double sub_mct_x = circle_mct.mX - cen_mct.mX;
        double sub_mct_y = circle_mct.mY - cen_mct.mY;
        //距离超处理图片的范围,不处理
        if(fabs(sub_mct_x) > pix_size / 2 || fabs(sub_mct_y) > pix_size / 2) continue;

        //开始计算对应的二维数组的下标
        int x = qFloor(sub_mct_x * (img_size / 2) / (pix_size / 2)); //[-127~127]
        int y = 0 - qFloor(sub_mct_y * (img_size / 2) / (pix_size / 2));//[-127~127]  直角坐标的Y向上变为绘图坐标系的向下
//        qDebug()<<sub_mct_x<<sub_mct_y <<x<<y;
        //转换到[0-254]范围
        x += (img_size / 2);
        y += (img_size / 2);
        //给数组赋值
        *(img+ y* img_size + x) = 1;
    }

    //3)遍历二维数组进行填充,统计每一行的开始点和结束点
    QPolygon left, right;
    int pre_start = -1, pre_end = -1;
    //首先获取最后有效的行
    int line_end = -1, line_start = -1;
    for(int i=img_size-1; i>=0; i--)
    {
        for(int j=0; j<img_size; j++)
        {
            if(*(img+i*img_size+j) == 1)
            {
                line_end = i;
                break;
            }
        }
        if(line_end >= 0) break;
    }

    for(int i=0; i<=line_end; i++)
    {
        int cur_start = -1, cur_end = -1;
        for(int j=0; j<img_size; j++)
        {
            if(*(img+i*img_size+j) == 1)
            {
                if(cur_start == -1) cur_start = j;
                cur_end = j;
                if(line_start == -1) line_start = i;
            }
        }
        if(cur_start == -1 && pre_start >= 0) cur_start = pre_start;
        if(cur_end == -1 && pre_end >= 0) cur_end = pre_end;

        //检查这两个点是否是相距太近,如果是,合并
        if(cur_start != -1 && fabs(cur_start - cur_end) < 5)
        {
            cur_start = cur_end;
        }

        if(cur_start == -1 && cur_end == -1) continue;
        if(pre_start == -1 && pre_end == -1)
        {
            //第一个点进来了,这里不管什么情况直接将开始和结束点分别插入左右侧
            left.append(QPoint(cur_start, i));
            right.append(QPoint(cur_end, i));
        } else
        {
            //计算最新的点相对于前一排的两个点的位置关系
            int start_d1 = cur_start - pre_start;
            int start_d2 = cur_start - pre_end;
            int end_d1 = cur_end - pre_start;
            int end_d2 = cur_end - pre_end;
            bool start_flag = false;
            bool end_flag = false;
            if(start_d1 <= 0)
            {
                //开始点再前一个开始点的左侧
                left.append(QPoint(cur_start, i));
                start_flag = true;
            }
            if(end_d2 >= 0)
            {
                //结束点再前一个结束点的右侧
                right.append(QPoint(cur_end, i));
                end_flag = true;
            }
            if((!start_flag) && start_d1 > 0)
            {
                //开始点再前一个开始点的右侧
                if(fabs(start_d1) < fabs(start_d2))
                {
                    left.append(QPoint(cur_start, i));
                    start_flag = true;
                }
            }

            if((!end_flag) && end_d2 < 0)
            {
                //结束点再前一个结束点的左侧
                if(fabs(end_d2) < fabs(end_d1))
                {
                    right.append(QPoint(cur_end, i));
                    end_flag = true;
                }
            }
        }

        if(left.size() > 0)pre_start = left.last().x();
        if(right.size() > 0) pre_end = right.last().x();
    }

    //左右点列进行合并
    for(int i=right.size()-1; i>=0; i--)
    {
        QPoint target = right[i];
        if(left.contains(target)) continue;
        left.append(target);
    }
    //对点列进行检查,去除锐角钝化
    for(int i=1; i<left.size()-1; )
    {
        QPoint pre = left[i-1];
        QPoint cur = left[i];
        if(i+1 > left.size() - 1) break;
        QPoint next = left[i+1];
        //检查当前点和前后的点是否是锐角
        //先将所有点的坐标变成直角坐标系.正方向X右Y上
        pre.setY(-1 * pre.y());
        cur.setY(-1 * cur.y());
        next.setY(-1 * next.y());
        //求两个向量的夹角
        //cos(@) = b•c / (|b| |c|)
        QVector2D p1(pre.x() - cur.x(), pre.y() - cur.y());
        QVector2D p2(next.x()- cur.x(), next.y()- cur.y());
        double dot = QVector2D::dotProduct(p1, p2);
        double len1 = p1.length();
        double len2 = p2.length();
        if(len1 * len2 > 0)
        {
            double angle = acos(dot / (len1 * len2));
            if( angle < 0.5 * GLOB_PI)
            {
                //锐角,删除
                left.removeAt(i);
                if(i >= 2)i--;
                continue;
            }
        }
#if 0
        //如果两个点相距太近,也删除
        if(len1 <= 2 || len2 <= 1)
        {
            left.removeAt(i);
            if(i >= 2)i--;
            continue;
        }
#endif
        i++;

    }
    //4)将数组点列画到图片上
    if(img)
    {
        delete []img;
        img = 0;
    }

    mRect->mCurrentRect.pixPoints = left;
//    mRect->mCurrentRect.pixWidth = img_size;
//    mRect->mCurrentRect.pixHeight = img_size;
}

void ZCHXRadarRectThread::convertZMQ2ZCHX(QList<ZCHX::Data::ITF_RadarRect> &res, const PROTOBUF_RadarRectList &src)
{
    res.clear();
    for (int i = 0; i < src.rect_list_size(); i++)
    {
        PROTOBUF_RadarRect obj = src.rect_list(i);
        ZCHX::Data::ITF_RadarRect rect;
        rect.mIsEstObj = obj.cur_est_count() == 0? false : true;
        rect.mRadarSiteId = mRadarCommonSettings.m_sSiteID;
        rect.mNodeNumber = obj.node_num();
        rect.mCurrentRect.rectNumber = obj.current_rect().rectnumber();
        rect.mCurrentRect.topLeftlatitude = obj.current_rect().topleftlatitude();
        rect.mCurrentRect.topLeftlongitude = obj.current_rect().topleftlongitude();
        rect.mCurrentRect.bottomRightlatitude = obj.current_rect().bottomrightlatitude();
        rect.mCurrentRect.bottomRightlongitude = obj.current_rect().bottomrightlongitude();
        rect.mCurrentRect.centerlatitude = obj.current_rect().centerlatitude();
        rect.mCurrentRect.centerlongitude = obj.current_rect().centerlongitude();
        rect.mCurrentRect.updateTime = obj.current_rect().updatetime();
        rect.mCurrentRect.diameter = obj.current_rect().diameter();
        rect.mBlockColor.setNamedColor(mRectParam.m_sCurColor);
        rect.mBlockEdgeColor.setNamedColor(mRectParam.m_sCurColor);
        rect.mHisBlockColor.setNamedColor(mRectParam.m_sHistoryColor);
        rect.mHistoryBackgroundColor.setNamedColor(mRectParam.m_sHistoryColor);
        rect.mCurrentRect.startlatitude = obj.current_rect().startlatitude();
        rect.mCurrentRect.startlongitude = obj.current_rect().startlongitude();
        rect.mCurrentRect.endlatitude = obj.current_rect().endlatitude();
        rect.mCurrentRect.endlongitude = obj.current_rect().endlongitude();
        rect.mCurrentRect.angle = obj.current_rect().cog();
        rect.mCurrentRect.isRealData = obj.current_rect().realdata();

        //添加预推区域
        if(obj.current_rect().has_predictionareas())
        {
            com::zhichenhaixin::proto::predictionArea area(obj.current_rect().predictionareas());
            for(int m =0; m<area.area_size(); m++)
            {
                ZCHX::Data::ITF_SingleVideoBlock block;
                block.latitude = area.area(m).latitude();
                block.longitude = area.area(m).longitude();
                rect.mCurrentRect.predictionArea.append(block);
            }
        }

        for (int j = 0; j < obj.current_rect().blocks_size(); j++)
        {
            PROTOBUF_SingleVideoBlock block = obj.current_rect().blocks(j);

            ZCHX::Data::ITF_SingleVideoBlock item;
            item.latitude = block.latitude();
            item.longitude = block.longitude();
            rect.mCurrentRect.blocks.append(item);
        }
//        qDebug()<<"shape_pnts:"<<obj.currentrect().pixelpnts_size();
        for(int i=0; i<obj.current_rect().pixelpnts_size(); i++)
        {
            com::zhichenhaixin::proto::pixelPoint pnt = obj.current_rect().pixelpnts(i);
            rect.mCurrentRect.pixPoints.append(QPoint(pnt.x(), pnt.y()));
        }

        for (int j = 0; j < obj.history_rect_list_size(); j++)
        {
            PROTOBUF_RadarRectDef historyObj = obj.history_rect_list(j);

            ZCHX::Data::ITF_RadarRectDef hisRect;
            hisRect.rectNumber = historyObj.rectnumber();
            hisRect.topLeftlatitude = historyObj.topleftlatitude();
            hisRect.topLeftlongitude = historyObj.topleftlongitude();
            hisRect.bottomRightlatitude = historyObj.bottomrightlatitude();
            hisRect.bottomRightlongitude = historyObj.bottomrightlongitude();
            hisRect.centerlatitude = historyObj.centerlatitude();
            hisRect.centerlongitude = historyObj.centerlongitude();
            hisRect.updateTime = historyObj.updatetime();
            hisRect.startlatitude = historyObj.startlatitude();
            hisRect.startlongitude = historyObj.startlongitude();
            hisRect.endlatitude = historyObj.endlatitude();
            hisRect.endlongitude = historyObj.endlongitude();
            hisRect.angle = historyObj.angle();
            hisRect.isRealData = historyObj.realdata();
            for(int k=0; k<historyObj.blocks_size(); k++)
            {
                com::zhichenhaixin::proto::singleVideoBlock node = historyObj.blocks(k);
                ZCHX::Data::ITF_SingleVideoBlock item;
                item.latitude = node.latitude();
                item.longitude = node.longitude();
                hisRect.blocks.append(item);
            }

            for(int i=0; i<historyObj.pixelpnts_size(); i++)
            {
                com::zhichenhaixin::proto::pixelPoint pnt = historyObj.pixelpnts(i);
                hisRect.pixPoints.append(QPoint(pnt.x(), pnt.y()));
            }

            if(historyObj.has_predictionareas())
            {
                com::zhichenhaixin::proto::predictionArea area(historyObj.predictionareas());
                for(int m =0; m<area.area_size(); m++)
                {
                    ZCHX::Data::ITF_SingleVideoBlock block;
                    block.latitude = area.area(m).latitude();
                    block.longitude = area.area(m).longitude();
                    hisRect.predictionArea.append(block);
                }
            }

            rect.mHistoryRects.append(hisRect);
        }

        res.append(rect);
    }
}
