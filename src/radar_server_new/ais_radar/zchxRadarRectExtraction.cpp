#include "zchxRadarRectExtraction.h"
#include <QDebug>
#include <QDateTime>
#include <QList>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <QPainter>
#include <QApplication>
#include <QGeoCoordinate>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QVector2D>


using namespace std;
using namespace cv;


cv::Mat QImage2cvMat(QImage image)
{
    cv::Mat mat;
    qDebug() << image.format();
    switch(image.format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat, CV_BGR2RGB);
        break;
    case QImage::Format_Indexed8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
        break;
    }
    return mat;
}

QImage cvMat2QImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        qDebug() << "CV_8UC4";
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}

void saveImg(const QString& dir, const QString& name, InputArray mat)
{
    QDir path(QString("%1/temp/%2").arg(QApplication::applicationDirPath()).arg(dir));
    if(!path.exists())
    {
        path.mkpath(path.absolutePath());
    }
    QString filename = QString("%1/%2.png").arg(path.absolutePath()).arg(name);
    imwrite(filename.toStdString(), mat);
}

void parseLatlonJsonArray(const QJsonArray& array, zchxLatlonList& list )
{
    list.clear();
    for(int j = 0; j < array.size(); ++j)
    {
        QJsonArray cellAraay = array.at(j).toArray();
        double dLon = cellAraay.at(0).toDouble();
        double dLat = cellAraay.at(1).toDouble();
        list.append(Latlon(dLat, dLon));
    }
}



zchxRadarRectExtraction::zchxRadarRectExtraction(double lat, double lon, int id, QObject *parent) : QObject(parent)
{
    mCentreLat = lat;
    mCentreLon = lon;
    mImageWidth = 0;
    mImageHeight = 0;
    mRangeFactor = 0.0;
    mMinTargetLength = 0.0;
    mMaxTargetLength = LONG_MAX;
    mMinTargetArea = 0.0;
    mMaxTargetArea = LONG_MAX;
    mIsFilterAreaEnabled = false;
}

zchxRadarRectExtraction::~zchxRadarRectExtraction()
{

}

void zchxRadarRectExtraction::setFilterAreaEnabled(bool sts)
{
    mIsFilterAreaEnabled = sts;
}

void zchxRadarRectExtraction::setFilterAreaData(const QList<zchxMsg::filterArea> &list)
{
    QMutexLocker locker(&mFilterAreaMutex);
    mFilterAreaLatlonList = list;
    transferLatlonArea2PixelArea(false);
}

void zchxRadarRectExtraction::setRadarLL(double lat, double lon)
{
    bool change = false;
    if(mCentreLat != lat)
    {
        mCentreLat = lat;
        change = true;
    }
    if(mCentreLon != lon)
    {
        mCentreLon = lon;
        change = true;
    }
    if(change)
    {
        transferLatlonArea2PixelArea(true);
    }
}

//range_factor  每像素多少米
//debug模式下输出opencv的图像处理过程
void zchxRadarRectExtraction::parseVideoPieceFromImage(QImage& result, zchxRadarRectDefList& list, const QImage &img, double range_factor, int video_index, bool output)
{
    quint32 time_of_day = QDateTime::currentDateTime().toTime_t();
    double timestamp = QDateTime::currentMSecsSinceEpoch();
    int scaled_width = 255;
    int scaled_height = 255;
    //遍历获取到的外形点列,取得目标的中心点
    QList<parseTarget> parseTargetList;
    QList<QPolygonF> skip_poly_list;

    QTime t;
    t.start();
    //检查图片的大小和距离因子是否发生了变化
    bool param_change = (mImageHeight != img.height());
    param_change |= (mImageWidth != img.width());
    param_change |= (mRangeFactor != range_factor);
    mImageHeight = img.width();
    mImageWidth = img.height();
    mRangeFactor = range_factor;
    if(param_change) transferLatlonArea2PixelArea(true);

    zchxPosConverter posConverter(QPointF(mImageWidth/2.0, mImageHeight/2.0),
                                  Latlon(mCentreLat, mCentreLon), mRangeFactor);
    //使用opencv的方法将图片抽出目标的外形框
    QString time = QString::number(QDateTime::currentMSecsSinceEpoch());
    cv::Mat src = QImage2cvMat(img);
    if (src.empty()){
        qDebug("colud not load image!!!!");
        return;
    }
    // 均值降噪.这里的Size的参数影响灰度画处理.数值过大,会使不相干的图片连成一片  干扰目标解析
    Mat blurImg;
    GaussianBlur(src, blurImg, Size(3, 3), 0, 0);

    // 二值化
    Mat binary, gray_src;
    cvtColor(blurImg, gray_src, COLOR_RGB2GRAY);
    threshold(gray_src, binary, 0, 255, THRESH_BINARY /*| THRESH_TRIANGLE*/);

    // 获取最大轮廓
    vector<vector<Point>> contours;
    findContours(binary, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
    if(output)
    {
        Mat connImage = Mat ::zeros(src.size(),CV_8U);
        drawContours(connImage, contours, -1, Scalar(255, 0, 255));
        saveImg(time, "result", connImage);
        saveImg(time, "src", src);
        saveImg(time, "blur", blurImg);
        saveImg(time, "gray", gray_src);
        saveImg(time, "binary", binary);
    }
    for(int i=0; i<contours.size(); i++)
    {
        double area = 0;
        QPointF grave_pnt;
        QPolygonF poly;
        //将opencv的点列转换成Qt
        vector<Point> pnts_opencv = contours[i];
        for(int k=0; k<pnts_opencv.size(); k++)
        {
            Point2i pnt = pnts_opencv[k];
            poly.append(QPointF(pnt.x, pnt.y));
        }
        //检查回波图形是否是在禁止区域内,或者禁止区域重叠
        if(mIsFilterAreaEnabled && isVideoPolygonNotAvailable(poly))
        {
            skip_poly_list.append(poly);
            continue;
        }
        //计算回波图形的面积,目标的长度和重心位置,检查目标的设定是否设定的阈值范围内
        area = contourArea(contours[i]);
        //检查面积是否符合要求.将面积转换成米
        area = area * mRangeFactor * mRangeFactor;
        if(area <= mMinTargetArea || area >= mMaxTargetArea)
        {
            skip_poly_list.append(poly);
            continue;
        }
        //计算雷达目标的长度和宽度,将矩形块二者的最大者作为目标的长度
        RotatedRect rect = minAreaRect(contours[i]);
        double width = rect.size.width;
        double height = rect.size.height;
        double target_length = qMax(width, height) * mRangeFactor;
        if(target_length <= mMinTargetLength || target_length >= mMaxTargetLength)
        {
            skip_poly_list.append(poly);
            continue;
        }
        //计算重心
        Moments M = moments(contours[i]);
        if(area <= 0.0) continue;
        int cx = int(M.m10/M.m00);
        int cy =  int(M.m01/M.m00);
        grave_pnt.setX(cx);
        grave_pnt.setY(cy);

        parseTarget target;
        target.mLength = target_length;
        target.mAngle = rect.angle;
        target.mTime = timestamp;
        target.mID = -1;
        target.mCenter = grave_pnt;
        target.mPolygons.append(poly);
        parseTargetList.append(target);
        //            qDebug()<<"gravity point:"<<grave_pnt<<gravity_list.size();
    }

    //开始生成新的回波图形
    result = img.copy();
    QPainter painter;
    painter.begin(&result);

    //开始将不符合要求的回波图形删除
    if(skip_poly_list.size() > 0)
    {
        QPainter::CompositionMode old = painter.compositionMode();
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.setPen(Qt::blue);
        painter.setBrush(Qt::transparent);
        foreach (QPolygonF poly, skip_poly_list) {
            painter.drawPolygon(poly);
        }
        painter.setCompositionMode(old);
    }
    //添加目标图形外框
    painter.setPen(QPen(Qt::red, 1.0, Qt::DashDotDotLine));
    for(int i=0; i<parseTargetList.size(); i++)
    {
        parseTarget &target = parseTargetList[i];
        painter.setBrush(Qt::transparent);
        painter.drawPolygon(target.mPolygons);
#if 0
        //添加目标中心点
        painter.setBrush(Qt::yellow);
        painter.drawEllipse(target.mCenter, 2, 2);
#endif
        QTransform transform = painter.transform();
//        transform.scale(0.4, 0.4);
        transform.translate(-target.mCenter.x(), -target.mCenter.y());
        target.mScaledPolygons = transform.map(target.mPolygons);
    }
    //添加屏蔽区域
    if(/*mIsLimitAvailable*/0)
    {
        QMutexLocker locker(&mFilterAreaMutex);
        if(mInFilterAreaPixelList.size() > 0)
        {
            painter.setPen(QPen(Qt::green, 3.0, Qt::DashDotDotLine));
            foreach (QPolygonF poly, mInFilterAreaPixelList) {
                painter.drawPolygon(poly);
            }

        }

        if(mOutFilterAreaPixelList.size() > 0)
        {
            painter.setPen(QPen(Qt::red, 3.0, Qt::DashDotDotLine));
            foreach (QPolygonF poly, mOutFilterAreaPixelList) {
                painter.drawPolygon(poly);
            }

        }
    }

    painter.end();
    if(output)
    {
        saveImg(time, "final", QImage2cvMat(result));
    }
//    qDebug()<<"total size:"<<contours.size()<<" skip size:"<<skip_poly_list.size()<<" target size:"<<parseTargetList.size();
    //开始进行目标合并,主要是将设定距离内的目标合并成一个
    //开始根据抽取的目标生成目标的矩形
    int i = 0;
    foreach (parseTarget target, parseTargetList)
    {
        if(!target.mPolygons.size()) continue;
        //先生成一个空白的数据
        i++;
        QRectF bound = target.mPolygons.boundingRect();
        com::zhichenhaixin::proto::RadarRectDef rectDef;
        rectDef.set_realdata(true);
        foreach (QPointF point, target.mPolygons) {
            zchxLatlon *block = rectDef.add_outline();
            Latlon ll = posConverter.pixel2Latlon(point);
            block->set_latitude(ll.lat);
            block->set_longitude(ll.lon);
        }
        QSizeF fixSize = target.mScaledPolygons.boundingRect().size();
        rectDef.mutable_fixedimg()->set_width(int(fixSize.width()+0.5));
        rectDef.mutable_fixedimg()->set_height(int(fixSize.height()+0.5));
        foreach (QPointF point, target.mScaledPolygons) {
            com::zhichenhaixin::proto::PixelPoint *pnt = rectDef.mutable_fixedimg()->add_points();
            pnt->set_x(int(point.x() + 0.5));
            pnt->set_y(int(point.y() + 0.5));
        }


        //取最长距离的2点
        double longestDistance = 0;//最长距离
        QPointF startPoint, endPoint;
        for(int i = 0; i < target.mPolygons.size(); i++)
        {
            for(int k = i+1; k < target.mPolygons.size(); k++)
            {
                if(i == k)continue;
                QVector2D vec(target.mPolygons[i] - target.mPolygons[k]);
                double distance = vec.lengthSquared();
                if(distance > longestDistance)
                {
                    longestDistance = distance;
                    startPoint = target.mPolygons[i];
                    endPoint = target.mPolygons[k];
                }
            }
        }
        rectDef.set_rectnumber(i);
        rectDef.set_updatetime(time_of_day);

        Latlon ll = posConverter.pixel2Latlon(bound.topLeft());
        rectDef.mutable_boundrect()->mutable_topleft()->set_latitude(ll.lat);
        rectDef.mutable_boundrect()->mutable_topleft()->set_longitude(ll.lon);

        ll = posConverter.pixel2Latlon(bound.bottomRight());
        rectDef.mutable_boundrect()->mutable_bottomright()->set_latitude(ll.lat);
        rectDef.mutable_boundrect()->mutable_bottomright()->set_longitude(ll.lon);

        double diameter = getDisDeg(rectDef.boundrect().topleft().latitude(),
                                    rectDef.boundrect().topleft().longitude(),
                                    rectDef.boundrect().bottomright().latitude(),
                                    rectDef.boundrect().bottomright().longitude());
        rectDef.mutable_boundrect()->set_diameter(diameter);

        ll = posConverter.pixel2Latlon(target.mCenter);
        rectDef.mutable_center()->set_latitude(ll.lat);
        rectDef.mutable_center()->set_longitude(ll.lon);

        ll = posConverter.pixel2Latlon(startPoint);
        rectDef.mutable_seg()->mutable_start()->set_latitude(ll.lat);
        rectDef.mutable_seg()->mutable_start()->set_longitude(ll.lon);

        ll = posConverter.pixel2Latlon(endPoint);
        rectDef.mutable_seg()->mutable_end()->set_latitude(ll.lat);
        rectDef.mutable_seg()->mutable_end()->set_longitude(ll.lon);

        QGeoCoordinate coord1(rectDef.seg().start().latitude(),rectDef.seg().start().longitude());
        QGeoCoordinate coord2(rectDef.seg().end().latitude(),rectDef.seg().end().longitude());
        double angle = coord1.azimuthTo(coord2);
//        //cout<<"角度"<<angle;
//        if(angle > 180)
//            rectDef.set_angle(angle - 180 + 90);
//        else
//            rectDef.set_angle(angle + 90);
        rectDef.mutable_seg()->set_angle(angle);

        //cout<<"angle"<<angle;
        rectDef.set_sogms(0.0);
        rectDef.set_cog(0.0);
        rectDef.set_sogknot(0.0);
        rectDef.set_videocycleindex(video_index);
        list.append(rectDef);
    }
    qDebug()<<__FUNCTION__<<" elapsed:"<<t.elapsed();
}

void zchxRadarRectExtraction::transferLatlonArea2PixelArea( bool lock)
{
    QMutexLocker *locker = 0;
    if(lock) locker = new QMutexLocker(&mFilterAreaMutex);

    zchxPosConverter posConverter(QPointF(mImageWidth/2.0, mImageHeight/2.0),
                                  Latlon(mCentreLat, mCentreLon), mRangeFactor);

    if(mImageWidth == 0 || mImageHeight == 0  || mRangeFactor < 0.0001) goto END;

    mOutFilterAreaPixelList.clear();
    mInFilterAreaPixelList.clear();
    for(int i=0; i<mFilterAreaLatlonList.size(); i++)
    {
        zchxMsg::filterArea data = mFilterAreaLatlonList[i];

        QPolygonF poly;
        foreach (zchxMsg::Latlon ll, data.area) {
            poly.append(posConverter.Latlon2Pixel(Latlon(ll.lat, ll.lon)));
        }
        if(poly.size() > 0)
        {

            if(data.type == 0)
            {
                //目标在这些区域要过滤掉，有效目标在这些区域的外面

                mOutFilterAreaPixelList.append(poly);

            } else
            {
                //目标在这些区域要保留，有效目标在这些区域的里面
                mInFilterAreaPixelList.append(poly);
            }
        }

    }
//    {
//        foreach (zchxMsg::filterArea area, mFilterAreaLatlonList) {
//            QPolygonF poly;
//            foreach (zchxMsg::Latlon ll, area.area) {
//                poly.append(QPointF(ll.lon, ll.lat));
//            }

//            qDebug()<<"latlon poly:"<<poly;

//        }
//        foreach (QPolygonF poly, mInFilterAreaPixelList) {
//            qDebug()<<"in poly:"<<poly;
//        }
//        foreach (QPolygonF poly, mOutFilterAreaPixelList) {
//            qDebug()<<"out poly:"<<poly;
//        }
//    }
END:
    if(locker) delete locker;
}

//区域限制功能
bool zchxRadarRectExtraction::isVideoPolygonNotAvailable(const QPolygonF& poly)
{
    QMutexLocker locker(&mFilterAreaMutex);
    //区域在指定的海域,区域没有在陆地上
    bool in_sea = false, in_land = false;
    if(mInFilterAreaPixelList.size() == 0) in_sea = true;
    foreach (QPolygonF src, mInFilterAreaPixelList) {
        if(src.intersected(poly).size() > 0 && poly.subtracted(src).size() == 0)
        {
            in_sea = true;
            break;
        }
    }
    if(!in_sea) return true;            //没有在海里,直接返回不符合要求
    if(mOutFilterAreaPixelList.size() == 0) return false;       //没有陆地限制, 直接符合要求
    foreach (QPolygonF src, mOutFilterAreaPixelList) {
        if(src.intersected(poly).size() > 0)
        {
            in_land = true;
            break;
        }
    }
    return in_land;
}

void  zchxRadarRectExtraction::setTargetAreaRange(double min, double max)
{
    if(max < 1.0)
    {
        mMaxTargetArea = ULONG_MAX;
    } else
    {
        mMaxTargetArea = max;
    }
    mMinTargetArea = min;
}
void  zchxRadarRectExtraction::setTargetLenthRange(double min, double max)
{
    if(max < 1.0)
    {
        mMaxTargetLength = ULONG_MAX;
    } else
    {
        mMaxTargetLength = max;
    }

    mMinTargetLength = min;
}

//注意这里需要将实际的距离单位(m)转换成像素距离
void zchxRadarRectExtraction::mergeTargetInDistance(QList<parseTarget> &list, double target_merge_distance)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qDebug()<<"merge distance:"<<target_merge_distance;
    qDebug()<<"before merge list size:"<<list.size();
    //目标预处理.将距离太近的目标进行合并,合并的中心点取二者的重点,目标取面积较大者
    for(int i=0; i<list.size();)
    {
        parseTarget &cur = list[i];
        QVector2D src(cur.mCenter);
        //获取和当前目标距离较近的目标,目标可能多个,这里暂且设定为list,
        QList<parseTarget> merge_list;
        for(int k = i+1; k<list.size();)
        {
            parseTarget next = list[k];
            //计算两者的距离
            QVector2D dest(next.mCenter);
            double distance = src.distanceToPoint(dest);
            if(distance < target_merge_distance)
            {
                //目标存在,从原来的队列里边删除
                merge_list.append(next);
                list.removeAt(k);
                continue;
            }
            k++;
        }
        if(merge_list.size() == 0)
        {
            i++;
            continue;
        }
        //合并目标添加目标自己本身
        merge_list.append(cur);
        //目标进行合并
        getMergeTarget(cur, merge_list, true);
        //这里目标的位置进行了移动,需要和后续目标继续进行合并,所以保持下标不变
    }
    qDebug()<<"after merge list size:"<<list.size();
}

void  zchxRadarRectExtraction::trackTarget(QList<parseTarget>& list, double target_merge_distance, bool merge_target)
{
#if 0
    if(list.size() == 0) return;
    quint64 list_time = list.first().mTime;
    //将距离太近的目标按照设定的合并距离进行合并
    if(merge_target) mergeTargetInDistance(list, merge_distance);
    //跟踪旧的目标,并且将已经使用的数据从数据源中删除
    int target_check_num = 4;  //需要有稳定的4个目标点才能确定目标已经稳定
    if(mTargetTrackMap.size() != 0)
    {
        //雷达目标已经存在了,现在开始找可能的下一个目标点
        QMap<int, parseTargetList>::iterator it = mTargetTrackMap.begin();
        for(; it != mTargetTrackMap.end(); it++)
        {
            if(it->size() == 0) continue;
            //检查目标的态势是否已经确定.目标态势确定的方法就是最近出现了连续三笔的数据的方向大体一致  目标没有回头的情况
            double est_cog = 0.0; //预估的目标运动方向
            bool target_stable = isTargetDirectStable(est_cog, it.value().mid(1), target_check_num);
            if(!target_stable)
            {
                est_cog = it->first().mRealCog;
            }
            //计算当前的时间间隔, 根据时间间隔算预估位置点
            float delta_time = list_time - it->first().mTime;

            parseTargetList merge_list;
            //计算当前目标可能的预估位置
            float est_distance = it->first().mSpeed * delta_time;
            //将方位角的值变换到绘图坐标系
            double est_angle = est_cog - 90;
            double est_x = it->first().mCenter.x() + est_distance * cos(est_angle / GLOB_PI);
            double est_y = it->first().mCenter.y() + est_distance * sin(est_angle / GLOB_PI);
            QVector2D est_pnt(est_x, est_y);

            //从最新的目标矩形框中寻找预估位置附件的点列
            for(int k = 0; k<list.size();)
            {
                parseTarget next = temp_list[k];
                //计算两者的距离
                double distance = est_pnt.distanceToPoint(QVector2D(next.mCenter));
                if(distance < 2)
                {
                    //目标存在,从原来的队列里边删除
                    merge_list.append(next);
                    list.removeAt(k);
                    continue;
                }
                k++;
            }
            if(merge_list.size() == 0)
            {
                QVector2D last_pos(it->first().mCenter);
                //预估位置附近没有找到,直接计算目标范围之类的点
                for(int k = 0; k<temp_list.size();)
                {
                    parseTarget next = temp_list[k];
                    //计算两者的距离
                    double distance = last_pos.distanceToPoint(QVector2D(next.mCenter));
                    if(distance < target_merge_distance)
                    {
                        //目标存在,从原来的队列里边删除
                        merge_list.append(next);
                        temp_list.removeAt(k);
                        continue;
                    }
                    k++;
                }
            }
            //没有找到符合要求的目标,目标不更新
            if(merge_list.size() == 0) continue;
            //根据找到的目标进行位置更新
            //过滤筛选符合方位角要求的目标
            parseTargetList target_list;
            if(it->size() == 1)
            {
                //如果目标还是第一个点,则方向角还未确定,则所有目标的均值作为下一个目标
                target_list.append(merge_list);
            } else
            {
                //如果目标的方向角已经存在了,则优先使用和目标方向值大体方向相同的所有目标的均值作为下一个目标,
                //如果相同的不存在,则使用所有的均值作为下一个目标
                QPointF last_pos(it->first().mCenter);
                for(int i=0; i < merge_list.size(); i++)
                {
                    parseTarget cur = merge_list[i];
                    double cog = calCog(last_pos, cur.mCenter);
                    if(qAbs(cog - est_cog) < 90)
                    {
                        target_list.append(cur);
                    }
                }
                if(target_list.size() == 0)
                {
                    target_list.append(merge_list);
                }

            }
            //计算目标合并后的中心位置
            parseTarget new_target;
            getMergeTarget(new_target, target_list, true);
            //计算新目标和就目标之间的距离
            QVector2D last_pos(it->first().mCenter);
            double distance = last_pos.distanceToPoint(QVector2D(new_target.mCenter));
            if(distance < 1) //像素距离小于1,认为目标未移动
            {
                //目标的距离太近,认为目标没有移动, 不进行处理
                continue;
            }
            //计算目标的速度和方向
            new_target.mSpeed = distance * 1000 / delta_time;
            //更新前一个点的实际运动方向
            it->first().mRealCog = calCog(it->first().mCenter, new_target.mCenter);
            it->first().mRealDefined = true;
            double refer_cog = 0.0;
            bool is_direction_ok = isTargetDirectStable(refer_cog, it.value(), 4);
            new_target.mEstCog = it->first().mRealCog;
            if(is_direction_ok)
            {
                //方向已经固定
                new_target.mEstCog = refer_cog;
            }
            //新目标对应的矩形单元确定,开始尝试将新目标单元添加到运动轨迹中
            it->prepend(new_target);

            //更新速度角度
            if(total.historyrects_size() > 0)
            {
                zchxRadarRectDef* last = total.mutable_historyrects(0);
                QGeoCoordinate last_pos(last->centerlatitude(), last->centerlongitude());
                QGeoCoordinate cur_pos(total.currentrect().centerlatitude(), total.currentrect().centerlongitude());
                total.mutable_currentrect()->set_cog(last_pos.azimuthTo(cur_pos));
                //检查新目标的方向是否与原来确定的目标方向相同
                if(it->refcount())
                {
                    if(qAbs(it->currentrect().cog() - total.currentrect().cog()) >= 90)
                    {
                        //目标反方向了, 不再继续更新
                        continue;
                    }
                }
                double delta_time = total.currentrect().timeofday() - last->timeofday();
                if(delta_time <= 0) delta_time += (3600 * 24);
                double cal_dis = last_pos.distanceTo(cur_pos);
                total.mutable_currentrect()->set_sog( cal_dis / delta_time);
                if(total.historyrects_size() == 0)
                {
                    //添加第一个历史轨迹点
                    //第一个点的方向还未确定,默认赋值为第二个点的角度
                    last->set_cog(total.currentrect().cog());
                }


                //           cout<<current.currentrect().rectnumber()<<"dis:"<<cal_dis<<"time_gap:"<<delta_time<<"speed:"<<current.currentrect().sog()<< current.currentrect().timeofday()<< last.timeofday();
            }
            //现在进行最后的确认.检查目标是否是来回地跳来跳去,如果是,删除跳来跳去的轨迹点,保留最初的点
            if(isTargetJumping(total, target_merge_distance, target_check_num))
            {
                //将目标退回到check_num对应的轨迹点
                int restore_history_index = target_check_num - 2;
                if(restore_history_index > total.historyrects_size())
                {
                    //异常情况
                    continue;
                }
                zchxRadarRectDef now = total.historyrects(restore_history_index);
                total.mutable_currentrect()->CopyFrom(now);
                total.mutable_historyrects()->DeleteSubrange(0, restore_history_index + 1);
            }

            if(total.historyrects_size() <= target_check_num -1 )
            {
                it->set_refcount(0);
            }

            //将就目标的历史轨迹清除,只有新目标保存历史轨迹
            it->clear_historyrects();
            //将新目标更新到队列中
            it->CopyFrom(total);
 //           qDebug()<<"update exist target:"<<it->currentrect().rectnumber()<<it->currentrect().timeofday()<<target.timeofday();
        }
    }

    checkTargetRectAfterUpdate(target_merge_distance);
    //检查目标之间的距离值
    dumpTargetDistance("old target after update", target_merge_distance);
    //未使用的数据作为新的目标添加到目标队列
    foreach (parseTarget target, list) {
        //目标的编号确定
        if(mTargetNum > mMaxTargettNum)
        {
            mTargetNum = mMinTargetNum;
        }

        //检查当前是否还存在空闲的
        while (mTargetNum <= mMaxTargettNum)
        {
            if(mTargetTrackMap.contains(mTargetNum))
            {
                mTargetNum++;
                continue;
            }
            break;
        }
        target.mID = mTargetNum;
        target.mCog = 0.0;          //初始状态下目标的方位角和速度都设定为默认值,未确定
        target.mSpeed = 0.0;
        parseTargetList resList;
        resList.append(target);
        mTargetTrackMap[mTargetNum] = resList;
    }
#endif

}

//计算目标的方位角  方位角使用地图坐标系  Y轴正向为0, 顺时针旋转为正,逆时针旋转为负
//使用前注意两点不能相等
double zchxRadarRectExtraction::calCog(const QPointF &start, const QPointF &end)
{
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double rad =  atan2(dy, dx) + GLOB_PI / 2;
    return rad * 180 / GLOB_PI;
}

bool zchxRadarRectExtraction::getMergeTarget(parseTarget& target, const QList<parseTarget> &list, bool area_weight)
{
    if(list.size() == 0) return false;
    //找出面积最大的目标作为最终的外形目标
    //合并后的目标的中心坐标按照面积比进行加权显示
    double sum_area = 0, max_area = 0;
    int    max_area_index = -1;
    for(int i=0; i<list.size(); i++)
    {
        parseTarget target = list[i];
        if(target.mArea > max_area)
        {
            max_area_index = i;
            max_area = target.mArea;
        }
        sum_area += target.mArea;
    }
    if(max_area_index == -1) return false;
    target = list[max_area_index];
    if(area_weight && sum_area > 0)
    {
        QPointF p(0, 0);
        for(int i=0; i<list.size(); i++)
        {
            parseTarget target = list[i];
            p += (target.mCenter * target.mArea / sum_area);
        }
        QPointF sub = p - target.mCenter;
        target.mCenter = p;
        //同步平移外形点列
        target.mPolygons.translate(sub);
    }

    return true;
}

bool zchxRadarRectExtraction::isTargetDirectStable(double &avg_cog, const parseTargetList& src, int min_target_num)
{
    parseTargetList list = src.mid(0, min_target_num);
    int size = list.size();
    if(size < min_target_num) return false;
    if(size < 2) return false;

    //找出最后一个确定点的方位角
    if(!list.last().mRealDefined) return false;
    double pre_cog = list.last().mRealCog;
    QPointF start = list.last().mCenter;
    int same_num = 0, diff_num = 0;
    double sum_cog = 0; //从起始点计算目标可能的方向角

    for(int i=list.size()-2; i>=0; i--)
    {
        if(list[i].mRealDefined == false) return false;
        double cur_cog = list[i].mRealCog;
        double delta_cog = cur_cog - pre_cog;
        if(qAbs(delta_cog) < 90)
        {
            //在同一个方向
            same_num++;
        } else
        {
            diff_num++;
        }
        sum_cog += calCog(start, list[i].mCenter);
    }
    if(diff_num) return false;

    avg_cog = sum_cog / (size - 1);
    return true;

}

#if 0
void ZCHXAnalysisAndSendRadar::changeTargetLL(const Latlon &ll, zchxRadarRectDef &cur)
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
        singleVideoBlock *block = cur.mutable_blocks(i);
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



//判断目标当前是否是在一定范围内跳动,方法
//1)目标的点数超过3个点
//2)连续点的方向不统一,每一个目标距离判定的起始点的距离都在合并的目标距离范围之内.
bool ZCHXAnalysisAndSendRadar::isTargetJumping(const zchxRadarRect& rect, double merge_dis, int jump_target_num)
{
    double avg_cog = 0;
    if(isTargetDirectStable(avg_cog, rect, jump_target_num)) return false;
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

//发送回波矩形-------------------------目标构造--------------------------------------------------------------------------------------------------
//目标出现连续3次的方向相同,则认为目标的方向和速度确定.目标确定之前,目标的速度和方向值都是0
//这个时候通过距离阈值将目标可能点保存,构造路径
void ZCHXAnalysisAndSendRadar::sendVideoRects(const zchxRadarRectDefList& src_list)
{
   signalSendRecvedContent(QDateTime::currentMSecsSinceEpoch(), "TargetExtraction", "sendVideoRects");
   if(src_list.size() == 0) return;
   qDebug()<<"start process video rects...............";
   zchxRadarRectDefList temp_list(src_list);
   m_radarPointMap_1.clear();
   double list_time = src_list.first().timeofday();

   double target_merge_distance = mTargetMergeRadius;
   double target_min_speed = 3.0;
   double target_max_speed = 14.0;
   //首先合并距离太近的目标
   mergeRectTargetInDistance(temp_list,target_merge_distance);

   dumpTargetDistance("old target before update", target_merge_distance);

   //开始确定雷达目标点
   int target_check_num = 4;
   if(m_radarRectMap.size() != 0)
   {
       //雷达目标已经存在了,现在开始找可能的下一个目标点
       zchxRadarRectMap::iterator it = m_radarRectMap.begin();
       for(; it != m_radarRectMap.end(); it++)
       {
           int rect_num = it.key();
           double old_time = it.value().currentrect().timeofday();
           double old_sog = it.value().currentrect().sog();
           double old_cog = it.value().currentrect().cog();
           double old_lat = it.value().currentrect().centerlatitude();
           double old_lon = it.value().currentrect().centerlongitude();

           //检查目标的态势是否已经确定.目标态势确定的方法就是最近出现了连续三笔的数据的方向大体一致  目标没有回头的情况
           double est_cog = 0.0;
           bool target_stable = isTargetDirectStable(est_cog, it.value(), target_check_num);
           if(!target_stable)
           {
               est_cog = old_cog;
           } else
           {
               it->set_refcount(1); //证明目标已经确定下来
           }
           //计算当前的时间间隔, 根据时间间隔算预估位置点
           float delta_time = list_time - old_time;

           zchxRadarRectDefList merge_list;
           if(0)
           {
               //计算当前目标可能的预估位置
               double est_lat = 0.0, est_lon = 0.0;
               if(delta_time < 0) delta_time += (3600* 24);
               float est_distance = old_sog * delta_time;
               distbearTolatlon1(old_lat, old_lon, est_distance, est_cog, &est_lat, &est_lon);

               //从最新的目标矩形框中寻找预估位置附件的点列
               for(int k = 0; k<temp_list.size();)
               {
                   zchxRadarRectDef next = temp_list[k];
                   //计算两者的距离
                   double distance = getDisDeg(est_lat, est_lon, next.centerlatitude(),next.centerlongitude());
                   if(distance < target_merge_distance)
                   {
                       //目标存在,从原来的队列里边删除
                       merge_list.append(next);
                       temp_list.removeAt(k);
                       continue;
                   }
                   k++;
               }
           } else
           {
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
           }
           //没有找到符合要求的目标,目标不更新
           if(merge_list.size() == 0) continue;
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
                   if(qAbs(cog - est_cog) < 90)
                   {
                       target_list.append(cur);
                   }
               }
               if(target_list.size() == 0)
               {
                   target_list.append(merge_list);
               }

           }
           //计算目标合并后的中心位置
           Latlon ll = getMergeTargetLL(target_list);
           //计算新目标和就目标之间的距离
           double distance = getDisDeg(old_lat, old_lon, ll.lat, ll.lon);
           if(distance < 10)
           {
               //目标的距离太近,认为目标没有移动, 不进行处理
               continue;
           }
           //新目标对应的矩形单元确定
           zchxRadarRectDef target = target_list.first();
           target.set_rectnumber(it.key());
           changeTargetLL(ll, target);
//           if(target.timeofday() == 0)
//           {
//               qDebug()<<"is time_of_day set:"<<target.has_timeofday();
//               for(int i=0; i<target_list.size(); i++)
//               {
//                   qDebug()<<"target timeof day:"<<target_list[i].timeofday();
//               }
//           }

           //确定新目标
           zchxRadarRect total;
           total.set_refcount(it->refcount());  //当前目标轨迹是否输出的标志
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
               if(it->refcount())
               {
                   if(qAbs(it->currentrect().cog() - total.currentrect().cog()) >= 90)
                   {
                       //目标反方向了, 不再继续更新
                       continue;
                   }
               }
               double delta_time = total.currentrect().timeofday() - last->timeofday();
               if(delta_time <= 0) delta_time += (3600 * 24);
               double cal_dis = last_pos.distanceTo(cur_pos);
               total.mutable_currentrect()->set_sog( cal_dis / delta_time);
               if(total.historyrects_size() == 0)
               {
                   //添加第一个历史轨迹点
                   //第一个点的方向还未确定,默认赋值为第二个点的角度
                   last->set_cog(total.currentrect().cog());
               }


               //           cout<<current.currentrect().rectnumber()<<"dis:"<<cal_dis<<"time_gap:"<<delta_time<<"speed:"<<current.currentrect().sog()<< current.currentrect().timeofday()<< last.timeofday();
           }
           //现在进行最后的确认.检查目标是否是来回地跳来跳去,如果是,删除跳来跳去的轨迹点,保留最初的点
           if(isTargetJumping(total, target_merge_distance, target_check_num))
           {
               //将目标退回到check_num对应的轨迹点
               int restore_history_index = target_check_num - 2;
               if(restore_history_index > total.historyrects_size())
               {
                   //异常情况
                   continue;
               }
               zchxRadarRectDef now = total.historyrects(restore_history_index);
               total.mutable_currentrect()->CopyFrom(now);
               total.mutable_historyrects()->DeleteSubrange(0, restore_history_index + 1);
           }

           if(total.historyrects_size() <= target_check_num -1 )
           {
               it->set_refcount(0);
           }

           //将就目标的历史轨迹清除,只有新目标保存历史轨迹
           it->clear_historyrects();
           //将新目标更新到队列中
           it->CopyFrom(total);
//           qDebug()<<"update exist target:"<<it->currentrect().rectnumber()<<it->currentrect().timeofday()<<target.timeofday();
       }
   }

   checkTargetRectAfterUpdate(target_merge_distance);
   //检查目标之间的距离值
   dumpTargetDistance("old target after update", target_merge_distance);
   //将剩余的矩形目标作为单独的目标添加
   //目标点为空,所有的雷达目标点自动生成初始化的矩形点
   for(int i=0; i<temp_list.size(); i++)
   {
       zchxRadarRect rect;
       rect.set_refcount(0);
       rect.mutable_currentrect()->CopyFrom(temp_list[i]);
       //更新编号,没有找到重复的点,直接添加
       if(rectNum > maxNum)
       {
           rectNum = objNum;
       }

       //检查当前是否还存在空闲的
       while (rectNum <= maxNum) {
           if(m_radarRectMap.contains(rectNum)){
               rectNum++;
               continue;
           }
           break;
       }
       rect.mutable_currentrect()->set_rectnumber(rectNum++);
       m_radarRectMap[rect.currentrect().rectnumber()] = rect;
//       qDebug()<<"new rect maked now:"<<rect.currentrect().rectnumber()<<rect.currentrect().timeofday();
   }
   if(0){
       zchxRadarRectDefList list;
       foreach (zchxRadarRect rect, m_radarRectMap) {
           list.append(rect.currentrect());
       }
       exportRectDef2File(list, QString("%1_rect").arg(QDateTime::currentMSecsSinceEpoch()));
   }

   int ClearTrack_Time = Utils::Profiles::instance()->value(str_radar,"ClearTrack_Time").toInt();
   //清理目标,删除超时未更新的目标
   double time_of_day = timeOfDay();
   QList<int> allKeys = m_radarRectMap.keys();
   foreach (int key, allKeys) {
       zchxRadarRect obj = m_radarRectMap[key];
       if((time_of_day - obj.currentrect().timeofday()) > ClearTrack_Time)
       {
           m_radarRectMap.remove(key);
       }
   }
   //检查目标之间的距离值
   dumpTargetDistance("all target with new ", target_merge_distance);

   //连续存在3笔数据的认为是目标，然后输出
   //目标构造
   foreach (zchxRadarRect rectObj, m_radarRectMap)
   {

       zchxRadarRectDef target;
#if 0
       if(rectObj.refcount() == 0) continue;

#endif
       target.CopyFrom(rectObj.currentrect());
       TrackPoint trackObj;
       LatLong startLatLong(m_dCentreLon,m_dCentreLat);
       //编号
       trackObj.set_tracknumber(target.rectnumber());
       //速度
       trackObj.set_sog(target.sog());
       //方向
       trackObj.set_cog(target.cog());

       //经纬度
       trackObj.set_wgs84poslat(target.centerlatitude());
       trackObj.set_wgs84poslong(target.centerlongitude());
       //笛卡尔坐标
       double x,y;
       double lat = trackObj.wgs84poslat();
       double lon = trackObj.wgs84poslong();
       getDxDy(startLatLong, lat, lon, x, y);
       trackObj.set_cartesianposx(x);
       trackObj.set_cartesianposy(y);
       //other
       trackObj.set_timeofday(target.timeofday());
       trackObj.set_systemareacode(0);
       trackObj.set_systemidentificationcode(1);
       trackObj.set_messagetype(static_cast<com::zhichenhaixin::proto::MSGTYP>(1));
       trackObj.set_tracklastreport(false);
       trackObj.set_cartesiantrkvel_vx(0);
       trackObj.set_cartesiantrkvel_vy(0);
       //直径
       trackObj.set_diameter(target.diameter());
       //完成
       m_radarPointMap_1[trackObj.tracknumber()] = trackObj;
   }
   //根据回波块中心确定目标
   m_radarPointMap = m_radarPointMap_1;


   cout<<"大小-缓存 rect size:"<<m_radarRectMap.size()<<" taregt size:"<<m_radarPointMap.size();

   sendRadarTrack();//小雷达发送目标
   signalVideoRects(m_radarRectMap);//绘制矩形块
   sendRadarRectPixmap();
}

void ZCHXAnalysisAndSendRadar::dumpTargetDistance(const QString &tag, double merge_dis)
{
    qDebug()<<"dump target distance now:"<<tag;
    QList<int> keys = m_radarRectMap.keys();
    for(int i=0; i<keys.size(); i++)
    {
        zchxRadarRect cur = m_radarRectMap[keys[i]];
        double cur_lat = cur.currentrect().centerlatitude();
        double cur_lon = cur.currentrect().centerlongitude();
        for(int k = i+1; k<keys.size(); k++)
        {
            zchxRadarRect next = m_radarRectMap[keys[k]];
            double next_lat = next.currentrect().centerlatitude();
            double next_lon = next.currentrect().centerlongitude();
            double distance = getDisDeg(cur_lat, cur_lon, next_lat, next_lon);
            if(distance < merge_dis)
            {
                qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.currentrect().timeofday()<<next.currentrect().timeofday();
            }
        }
    }
}

void ZCHXAnalysisAndSendRadar::checkTargetRectAfterUpdate(double merge_dis)
{
    QList<int> keys = m_radarRectMap.keys();
    for(int i=0; i<keys.size(); i++)
    {
        if(!m_radarRectMap.contains(keys[i])) continue;
        zchxRadarRect cur = m_radarRectMap[keys[i]];
        double cur_lat = cur.currentrect().centerlatitude();
        double cur_lon = cur.currentrect().centerlongitude();
        for(int k = i+1; k<keys.size(); k++)
        {
            if(!m_radarRectMap.contains(keys[k])) continue;
            zchxRadarRect next = m_radarRectMap[keys[k]];
            double next_lat = next.currentrect().centerlatitude();
            double next_lon = next.currentrect().centerlongitude();
            double distance = getDisDeg(cur_lat, cur_lon, next_lat, next_lon);
            if(distance < merge_dis)
            {
                qDebug()<<"found abormal targets:"<<keys[i]<<keys[k]<<"  distance:"<<distance<<" time:"<<cur.currentrect().timeofday()<<next.currentrect().timeofday()<<" remove old one.";
                if(cur.refcount() && !next.refcount())
                {
                    m_radarRectMap.remove(keys[k]);
                    continue;
                }
                if((!cur.refcount()) && next.refcount())
                {
                    m_radarPointMap.remove(keys[i]);
                    break;
                }
                if(next.currentrect().timeofday() <= cur.currentrect().timeofday())
                {
                    m_radarRectMap.remove(keys[k]);
                } else
                {
                    m_radarPointMap.remove(keys[i]);
                    break;
                }


            }
        }
    }
}


#endif







