#include "new_gettrack.h"
#include <QDebug>
#include <math.h>
#include <QMutex>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QGeoCoordinate>
#include <QCoreApplication>
#include <QtCore/qmath.h>
#include <synchapi.h>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

new_gettrack::new_gettrack(QObject *parent) : QObject(parent)
{
    connect(this,SIGNAL(new_gettrack_signal(Afterglow)),this,SLOT(new_gettrack_slot(Afterglow)));//利用信号槽开启线程
    moveToThread(&m_workThread);
    m_workThread.start();
}
new_gettrack::~new_gettrack()
{
    if(m_workThread.isRunning()) {
        m_workThread.quit();
    }

    m_workThread.terminate();
}
void new_gettrack::new_gettrack_slot(const Afterglow& dataAfterglow)//新目标识别算法
{
    //Sleep(10000);
    //cout<< "传进来的扫描线_1"<<dataAfterglow.m_RadarVideo.size();
    QMap<int,RADAR_VIDEO_DATA> RadarVideo = dataAfterglow.m_RadarVideo;
    QList<int> amplitudeList;
    QList<int> indexList;
    QMap<int,RADAR_VIDEO_DATA>::iterator it;
    int a = 0;
    static int s_32 = 0;
    int bl = s_32;
    while(1)
    {
        a++;
        RADAR_VIDEO_DATA data = RadarVideo.value(s_32);
        double dAzimuth = data.m_uAzimuth * (360.0 / data.m_uLineNum) + data.m_uHeading;
        double dArc = dAzimuth  * PI / 180.0;
        amplitudeList = data.m_pAmplitude;
        indexList = data.m_pIndex;
        //cout<< "m_uAzimuth"<<data.m_uAzimuth<<"spoke"<<s_32;
        int i = 0 ;
        while(i < amplitudeList.size())
        {
            int position = indexList[i];
            int value = amplitudeList[i];
            //取所有黄色的坐标点,然后进行距离判断,距离在2以内的都视作同一目标
            if(RadarVideo.size()>0)
            {
                if(value == 255 && amplitudeList.size() < 200)
                {
                    QPoint p(position*sin(dArc),-position*cos(dArc));
                    yel_pot_1 << p;
                }
            }
            i++;
        }
        ++s_32;
        if(s_32 > 4095)
        {
            s_32 = 0;
            break;
        }
        if(s_32 - bl == 31)
            break;
    }
    static int b = 0;
    b++;
    if(b >63)
        b=0;
    cout<<"黄色回波"<<yel_pot_1.size()<<"第几批32条扫描线"<< b;
    if(yel_pot_1.size() != 62)
    {
       //cout<<"yel_pot_1"<<yel_pot_1.size();
        for(int a=0; a<yel_pot_1.size(); a++)
        {
            if(yel_pot_1[a].x() ==0 && yel_pot_1[a].y() ==0)
            {
            }
            else
            {
                yel_pot_2 << yel_pot_1[a];
            }
        }
        yel_pot_1.clear();
        for(int a=0; a<yel_pot_2.size()-1; a++)
        {
            if(a == 0 || a ==yel_pot_2.size()-1)//第一个点和最后一个点加入
               yel_pot_3 << yel_pot_2[a];
            else if(0 < a && a < yel_pot_2.size())
            {
                int dis_p = qSqrt(qPow(yel_pot_2[a].x() - yel_pot_2[a-1].x(), 2) + qPow(yel_pot_2[a].y() - yel_pot_2[a-1].y(), 2));
                int dis_f = qSqrt(qPow(yel_pot_2[a].x() - yel_pot_2[a+1].x(), 2) + qPow(yel_pot_2[a].y() - yel_pot_2[a+1].y(), 2));
                if(dis_p > 2 || dis_f>2)
                {
                    bool same = false;
                    for(int b=0; b<yel_pot_3.size(); b++)
                    {
                        if(yel_pot_3[b] == yel_pot_2[a])
                        {
                            same = true;
                        }
                    }
                    if(same == false)
                        yel_pot_3<<yel_pot_2[a];

                }
            }
        }

        //cout<<"第一次判断后"<<yel_pot_2.size();
        //cout<<"第二次判断后"<<yel_pot_3.size();//<<yel_pot_3;
    }
    emit show_newtrack_signal(yel_pot_3);//发送显示新算法识别的目标
    if(b == 0)
    {
        cout<<"清楚pot3";
        yel_pot_3.clear();
    }
    yel_pot_2.clear();
}
