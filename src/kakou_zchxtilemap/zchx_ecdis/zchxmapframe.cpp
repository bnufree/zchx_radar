#include "zchxmapframe.h"
#include <QDebug>
#include "zchxmaploadthread.h"
#include <QPainter>
#include "profiles.h"

using namespace qt;

zchxMapFrameWork::zchxMapFrameWork(double center_lat, double center_lon, int zoom, int width, int height, int source,  int min_zoom, int max_zoom, QObject *parent) : QObject(parent),
  mViewWidth(0),
  mViewHeight(0),
  mSource(source),
  mStyle(MapStyleEcdisDayBright),
  mMinZoom(min_zoom),
  mMaxZoom(max_zoom),
  mMapThread(0),
  mOffset(0, 0),
  mRangeInit(false)
{
    SetZoom(zoom);
    UpdateCenter(center_lon, center_lat);
    SetViewSize(width, height);

    mMapThread = new zchxMapLoadThread;
    connect(mMapThread, SIGNAL(signalSendCurPixmap(QPixmap,int,int)), this, SLOT(append(QPixmap,int,int)));
    connect(mMapThread, SIGNAL(signalSendNewMap(double, double, int, bool)), this, SLOT(slotRecvNewMap(double,double,int, bool)));
    connect(mMapThread, SIGNAL(signalSendImgList(TileImageList, MapRangeData)), this, SLOT(append(TileImageList, MapRangeData)));
    mMapThread->start();
}

zchxMapFrameWork::~zchxMapFrameWork()
{
     qDebug()<<__FUNCTION__<<__LINE__;
}

void zchxMapFrameWork::setOffSet(int offset_x, int offset_y)
{
    mOffset.setWidth(offset_x);
    mOffset.setHeight(offset_y);
}

void zchxMapFrameWork::SetViewSize(int width, int height)
{
    mViewHeight = height;
    mViewWidth = width;
    UpdateDisplayRange();
}

void zchxMapFrameWork::SetSource(int source)
{
    if(mSource != source)
    {
        mSource = source;
        UpdateDisplayRange();
    }
}

void zchxMapFrameWork::SetZoom(int zoom)
{
    if(zoom < mMinZoom || zoom > mMaxZoom) return;
    mCurZoom = zoom;
    UpdateDisplayRange(zchxMapDataUtils::calResolution(zoom));
}

void zchxMapFrameWork::SetMinZoom(int zoom)
{
    mMinZoom = zoom;
}

void zchxMapFrameWork::SetMaxZoom(int zoom)
{
    mMaxZoom = zoom;
}

int zchxMapFrameWork::Zoom() const
{
    return mCurZoom;
}

void zchxMapFrameWork::UpdateCenter(double lon, double lat)
{
    mCenter.lat = lat;
    mCenter.lon = lon;
    //重新计算当前视窗的显示范围
    UpdateDisplayRange();
}

void zchxMapFrameWork::UpdateCenterAndZoom(const ZCHX::Data::LatLon &ll, int zoom)
{
    mCenter = ll;
    if(zoom == -1) zoom = Zoom();
    SetZoom(zoom);
}

void zchxMapFrameWork::Drag(int x, int y)
{
    UpdateCenter(ZCHX::Data::Point2D(0.5*mViewWidth + x, 0.5*mViewHeight + y));
}

void zchxMapFrameWork::UpdateCenter(const ZCHX::Data::Point2D &point)
{
    UpdateCenter(Pixel2LatLon(point));
}

ZCHX::Data::Point2D zchxMapFrameWork::Mercator2Pixel(const ZCHX::Data::Mercator &mct)
{
    double x = mct.mX - mMapRange.mLowerLeft.mX;
    double y = mct.mY - mMapRange.mTopRight.mY;
    ZCHX::Data::Point2D res;
    res.x = x / mMapRange.mUnitLenth + mOffset.width();
    res.y = 0 - y / mMapRange.mUnitLenth + mOffset.height();
    return res;
}

ZCHX::Data::Point2D zchxMapFrameWork::LatLon2Pixel(const ZCHX::Data::LatLon &ll)
{
    //获取当前经纬度对应的墨卡托坐标
    ZCHX::Data::Mercator mct = zchxMapDataUtils::wgs84LonlatToMercator(ll);
    //通过墨卡托坐标换算屏幕坐标
    return Mercator2Pixel(mct);
}

ZCHX::Data::Point2D zchxMapFrameWork::LatLon2Pixel(double lat, double lon)
{
    return LatLon2Pixel(ZCHX::Data::LatLon(lat, lon));
}



ZCHX::Data::LatLon zchxMapFrameWork::Pixel2LatLon(const ZCHX::Data::Point2D& pos)
{
    double x = pos.x;
    double y = mViewHeight - pos.y;
    //获取当前指定位置对应的墨卡托坐标
    ZCHX::Data::Mercator target;
    target.mX = mMapRange.mLowerLeft.mX + mMapRange.mUnitLenth * x;
    target.mY = mMapRange.mLowerLeft.mY + mMapRange.mUnitLenth * y;

    return zchxMapDataUtils::mercatorToWgs84LonLat(target);
}

void zchxMapFrameWork::UpdateDisplayRange(double unit_lenth)
{
    if(unit_lenth > 0 && !mRangeInit)
    {
        mMapRange.mUnitLenth = unit_lenth;
    }
    if(mViewWidth == 0 || mViewHeight == 0) return;
    //计算当前中心经纬度对应的墨卡托坐标
    ZCHX::Data::Mercator center_mct = zchxMapDataUtils::wgs84LonlatToMercator(mCenter);
    //计算当前视窗对应的墨卡托坐标的显示范围
    MapRangeData destRange;
    double wkUnit = mMapRange.mUnitLenth;
    if(unit_lenth > 0) wkUnit = unit_lenth;
    destRange.mLowerLeft.mX = center_mct.mX - wkUnit * mViewWidth / 2.0;
    destRange.mLowerLeft.mY = center_mct.mY - wkUnit * mViewHeight / 2.0;
    destRange.mTopRight.mX = center_mct.mX + wkUnit * mViewWidth / 2.0;
    destRange.mTopRight.mY = center_mct.mY + wkUnit * mViewHeight / 2.0;
    destRange.mUnitLenth = wkUnit;

    MapLoadSetting setting;
    setting.mMapRange = destRange;
    setting.mSource = mSource;
    if(setting.mSource == ZCHX::TILE_GOOGLE_ONLINE)
    {
        setting.mTilePos = TILE_ORIGIN_TOPLEFT;
    } else
    {
        setting.mTilePos = TILE_ORIGIN_BOTTEMLEFT;
    }
    setting.mResolution = destRange.mUnitLenth;
    setting.mZoom = Zoom();
    setting.mCenter = mCenter;
    if(mMapThread) mMapThread->appendTask(setting);
}

void zchxMapFrameWork::ZoomIn()
{
    int zoom = mCurZoom;
    SetZoom(++zoom);
}

void zchxMapFrameWork::ZoomOut()
{
    int zoom = mCurZoom;
    SetZoom(--zoom);
}

void zchxMapFrameWork::Update()
{

}

PPATH zchxMapFrameWork::convert2QtPonitList(const GPATH& path)
{
    PPATH pts;
    for(int i = 0; i < path.size(); ++i)
    {
        GPNT ll = path[i];
        pts.push_back(LatLon2Pixel(ll.first,ll.second).toPointF());
    }
    return pts;
}

void zchxMapFrameWork::updateEcdis(QPainter *painter, bool img_num)
{
    if(!painter) return;
    foreach(TileImage data, mDataList)
    {
        int x = data.mPosX + mOffset.width();
        int y = data.mPosY + mOffset.height();
        painter->drawPixmap(x, y, data.mImg);
        if(img_num)painter->drawText(x, y, data.mName);
    }
}

void zchxMapFrameWork::slotRecvNewMap(double lon, double lat, int zoom, bool sync)
{
    emit signalSendCurMapinfo(lat, lon, zoom);
    if(!sync) clear();
}

void zchxMapFrameWork::append(const QPixmap &img, int x, int y)
{
    mDataList.append(TileImage(img, x, y));
}

void zchxMapFrameWork::append(const TileImageList &list, const MapRangeData& range)
{
    mDataList = list;
    mMapRange = range;
    mRangeInit = true;
    setOffSet(0, 0);
}
