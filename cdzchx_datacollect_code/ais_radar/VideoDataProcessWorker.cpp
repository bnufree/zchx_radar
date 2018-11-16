#include "VideoDataProcessWorker.h"
#include "BR24.hpp"
#include "common.h"
#include "zchxfunction.h"

using namespace BR24::Constants;
#define SPOKES (4096)
static uint8_t BR24MARK[] = {0x00, 0x44, 0x0d, 0x0e};

VideoDataProcessWorker::VideoDataProcessWorker(RadarConfig* cfg, QObject *parent) :         QObject(parent),
    mRadarCfg(cfg),
    mExtract(new TargetExtractionWorker(cfg))
{
    qRegisterMetaType<ITF_VideoFrame>("const ITF_VideoFrame&");
    qRegisterMetaType<ITF_VideoFrameList>("const ITF_VideoFrameList&");
    connect(this, SIGNAL(signalSendVideoFrameDataList(ITF_VideoFrameList)), mExtract, SLOT(slotRecvRawVideoDataList(ITF_VideoFrameList)));
    connect(mExtract, SIGNAL(signalSendTrackPoint(QList<TrackPoint>)), this, SIGNAL(signalSendTrackPoint(QList<TrackPoint>)));
    moveToThread(&mThread);
    mThread.start();
}

void VideoDataProcessWorker::slotRecvVideoRawData(const QByteArray &raw)
{
    int len = raw.size();
    if(!mRadarCfg) return;
    QElapsedTimer  timer;
    timer.start();
    int uLineNum = mRadarCfg->getShaftEncodingMax()+1;
    int uCellNum = mRadarCfg->getGateCountMax();
    int uHeading = mRadarCfg->getHead();

    radar_frame_pkt *packet = (radar_frame_pkt *)raw.data();//正常大小是17160

    //radar_frame_pkt大小是固定的64328 定的120个spoke(536*120)+frame_hdr(8)
    if (len < (int)sizeof(packet->frame_hdr)) {
        LOG_FUNC_DBG<<"The packet is so small it contains no scan_lines, quit!";
        return;
    }
    //decide scanline number  (17160 - 8) / 536 = 32
    int scanlines_in_packet = (len - sizeof(packet->frame_hdr)) / sizeof(radar_line);
    if (scanlines_in_packet != 32) {
        LOG_FUNC_DBG<<"The scan line or spoke num is not same as 32, quit!";
        return;
    }
    int time_of_day = TimeStamp::timeOfDay();
    //雷达振幅收集,初始化回波数据结构
    ITF_VideoFrameList videoList;
    int     next_spoke = -1;
    for (int scanline = 0; scanline < scanlines_in_packet; scanline++)
    {
        radar_line *line = &packet->line[scanline];
        // Validate the spoke,低位在前高位在后
        int spoke = line->common.scan_number[0] | (line->common.scan_number[1] << 8);
        //扫描线的头长度检查.正常是24
        bool check_flag = true;
        if (line->common.headerLen != 0x18)
        {
            LOG_FUNC_DBG<<"strange header length:" << line->common.headerLen;
            check_flag = false;
        }
        //状态检查
        if (check_flag && line->common.status != 0x02 && line->common.status != 0x12) {
            LOG_FUNC_DBG<<"strange status:" << line->common.status;
            check_flag = false;
        }
        next_spoke = (spoke + 1) % SPOKES;
        if(!check_flag) continue;


        int range_raw = 0;
        int angle_raw = 0;
        short int heading_raw = (line->common.heading[1] << 8) | line->common.heading[0];
        double range_meters = 0;
        //检查雷达的类型
        if (memcmp(line->br24.mark, BR24MARK, sizeof(BR24MARK)) == 0)
        {
            // BR24 and 3G mode
            range_raw = ((line->br24.range[2] & 0xff) << 16 | (line->br24.range[1] & 0xff) << 8 | (line->br24.range[0] & 0xff));
            angle_raw = (line->br24.angle[1] << 8) | line->br24.angle[0];
            range_meters = (int)((double)range_raw * 10.0 / sqrt(2.0));
            LOG_FUNC_DBG<<"recv br24 data:" << range_raw<<angle_raw<<range_meters;
        } else {
            // 4G mode
            short int large_range = (line->br4g.largerange[1] << 8) | line->br4g.largerange[0];
            short int small_range = (line->br4g.smallrange[1] << 8) | line->br4g.smallrange[0];
            angle_raw = (line->br4g.angle[1] << 8) | line->br4g.angle[0];
            if (large_range == 0x80) {
                if (small_range == -1) {
                    range_raw = 0;  // Invalid range received
                } else {
                    range_raw = small_range;
                }

            } else {
                range_raw = large_range * 256;
            }
            range_meters = range_raw / 4;
            LOG_FUNC_DBG<<"recv br24 data:" << range_raw<<angle_raw<<range_meters<<large_range<<small_range;
        }

        int azimuth_cell = uLineNum;
        bool radar_heading_valid = (((heading_raw) & ~(HEADING_TRUE_FLAG | (azimuth_cell - 1))) == 0);
        double heading;
        if (radar_heading_valid)
        {
            double heading_rotation = (((heading_raw) + 2 * azimuth_cell) % azimuth_cell);
            double heading_degrees = ((heading_rotation) * (double)DEGREES_PER_ROTATION / azimuth_cell);
            heading = (fmod(heading_degrees + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION));
        }else
        {
            //ZCHXLOG_DEBUG("radar_heading_valid=" << radar_heading_valid );
        }

        angle_raw = MOD_ROTATION2048(angle_raw / 2);
        double start_range = 0.0 ;
        double range_factor = range_meters/uCellNum;
        LOG_FUNC_DBG<<"range_meter:"<<range_meters<<" cellNum:"<<uCellNum<<" range_factor"<<range_factor;

        double dAzimuth = angle_raw*(360.0/(uLineNum/2))+uHeading;
        LOG_FUNC_DBG<<"dAzimuth:"<<dAzimuth<<"angle_raw"<<angle_raw<<"uHeading"<<uHeading; //1_扫描方位,angle_raw(0-2047),uHeading(180)
        //更新雷达回波数据
        ITF_VideoFrame video;
        video.set_systemareacode(1);
        video.set_systemidentificationcode(1);
        video.set_msgindex(spoke);
        video.set_azimuth(angle_raw);
        video.set_heading(uHeading);
        video.set_startrange(start_range);
        video.set_rangefactor(range_factor);
        video.set_bitresolution(::com::zhichenhaixin::proto::RES(4));
        video.set_timeofday(time_of_day);
        for (int range = 0; range < uCellNum; range++)
        {
            int value =  (int)(line->data[range]);
            video.set_amplitude(range, value);
        }
        videoList.append(video);
    }

    //发送给算法线程进行回波->VIDEO->BINARYVIDEO处理
    if(videoList.size() > 0)
    {
        emit signalSendVideoFrameDataList(videoList);
    }

    LOG_FUNC_DBG<<" end:"<<timer.elapsed();

}
