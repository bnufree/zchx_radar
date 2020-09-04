#include "mainprocess.h"
#include <QDebug>
#include <QDateTime>
#include "ais_radar/zchxanalysisandsendradar.h"

MainProcess* MainProcess::m_pInstance = 0;
bool output_log_std = false;

MainProcess::MainProcess(QObject *parent) : QObject(parent)
{
    mStartFlag = false;
}

MainProcess* MainProcess::instance()
{
    if(!m_pInstance) m_pInstance = new MainProcess;
    return m_pInstance;
}


void MainProcess::start()
{
    mStartFlag = true;
    //获取配置文件
    //开启数据数据服务
    //开启雷达数据接收
    //开启数据服务交互服务
}

void MainProcess::initConfig()
{

}

void MainProcess::apendRadarAnalysisServer(int site_id, ZCHXAnalysisAndSendRadar *server)
{
    if(!server) return;
    if(mRadarAnalysisMap.contains(site_id))  mRadarAnalysisMap[site_id] = server;
    else mRadarAnalysisMap.insert(site_id, server);
}

bool MainProcess::processFilterAreaMsg(int cmd, const zchxMsg::filterArea &area)
{
    //获取当前过滤区域对应的雷达ID号，然后找到对应的回波分析程序
    int siteID = area.site;
    if(siteID <= 0)
    {
        qDebug()<<" filter area site id not specfied"<<area.name<<QDateTime::fromMSecsSinceEpoch(area.time);
        return false;
    }
    if(mRadarAnalysisMap.contains(siteID))
    {
        ZCHXAnalysisAndSendRadar *server = mRadarAnalysisMap[siteID];
        if(server)
        {
            if(cmd == Msg_Edit_FilterArea) return server->addOrEditFilterArea(area);
            if(cmd == Msg_Delete_FilterArea) return server->removeFilterArea(area.id);
        }
    }
    qDebug()<<" filter area operation with unexpected error now";
    return false;
}
