#include "mainprocess.h"
#include <QDebug>

MainProcess* MainProcess::m_pInstance = 0;
bool output_log_std = false;

MainProcess::MainProcess(QObject *parent) : QObject(parent)
{
    mStartFlag = false;
}

MainProcess* MainProcess::instance()
{
    if(m_pInstance) m_pInstance = new MainProcess;
    return m_pInstance;
}


void MainProcess::start()
{
    mStartFlag = true;

    //开启数据服务交互服务
    //开启数据数据服务
    //开启雷达数据接收

}
