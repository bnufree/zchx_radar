#include "drawaistrack.h"
#include <QThread>
#include <QFile>
#include <QString>
#include <QDialog>
#include <QFileDialog>
#include <QDebug>
#include <QPainter>
#include "profiles.h"


drawaistrack::drawaistrack(QObject *parent) : QObject(parent)
{
    pix = QPixmap(850, 600);
    moveToThread(&m_threadWork);
    QObject::connect(this,SIGNAL(signalDrawAisTrack(int, double, double)),this,SLOT(slotDrawAisTrack(int, double, double)));
    connect(this,SIGNAL(signalGetPixmap(QPixmap, int, int)),this,SLOT(slotGetPixmap(QPixmap, int, int)));
    connect(&m_threadWork,&QThread::finished,this,&QObject::deleteLater);
    m_threadWork.start();
}

drawaistrack::~drawaistrack()
{
    if(m_threadWork.isRunning())
    {
        m_threadWork.quit();
    }

    m_threadWork.terminate();
}
//画轨迹
void drawaistrack::slotDrawAisTrack(int a, double lon, double lat)
{
    //qDebug()<<"线程画轨迹";
    double mLuLon = Utils::Profiles::instance()->value("Ais_Latlon", "LeftLon").toDouble();
    double mLuLat = Utils::Profiles::instance()->value("Ais_Latlon", "LeftLat").toDouble();
    double mRdLon = Utils::Profiles::instance()->value("Ais_Latlon", "RightLon").toDouble();
    double mRdLat = Utils::Profiles::instance()->value("Ais_Latlon", "RightLat").toDouble();
    LatLong startLatLong(mLuLon,mLuLat);
    double posx = 0, posy = 0;
    getDxDy(startLatLong, mRdLat, mRdLon, posx, posy);
    QPointF pos_1(posy,-posx);
    double kx = pos_1.x() / xLevel;
    double ky = pos_1.y() / yLevel;

    double pax = 0, pay = 0, pbx = 0, pby = 0;
    getDxDy(startLatLong, lat, lon, pax, pay);
    pbx = pay / kx;
    pby = -pax/ ky;
    QPointF pos_2(pbx, pby);
    //cout<<"坐标点a"<<pos_2;

    QPainter painter(&pix);// 画的设备
    QPen pen;
    pen.setWidth(5); //设置宽度
    pen.setColor(Qt::red);
    //cout<<"pos_1"<<pos_2;
    painter.setPen(pen);
    painter.drawPoint(pos_2);

    QPixmap pixmap_1 = pix;
    emit signalSendAisPix(pixmap_1);
}
//画网格
void drawaistrack::drawTrackPixmap()
{
    pix.fill(Qt::gray);
    QPainter painter(&pix);// 画的设备
    QPen pen;
    pen.setWidth(2); //设置宽度
    pen.setBrush(Qt::black); //设置颜色
    painter.setPen(pen); //选中画笔
    painter.drawRect(0, 0,pix.width(),pix.height());
    int i;
    for(i = 0;i<pix.width();i+=50)//x 轴
    {
         painter.drawLine(i+50,pix.height(),i+50,0); //绘制x轴上的点
         painter.drawText(i,pix.height()-2,QString::number(i)); //绘制文本

    }
    for(i=0;i<pix.height();i+=50)//y 轴
    {
         painter.drawLine(0,i,pix.width(),i); //绘制y轴上的点
         painter.drawText(0,i,QString::number(pix.height()-i)); //绘制文本
    }
    //画图
    QPixmap pixmap_1 = pix;
    emit signalSendAisPix(pixmap_1);

}
//获取放大缩小的pix,以及坐标系数
void drawaistrack::slotGetPixmap(QPixmap p, int x, int y)
{
    qDebug()<<"线程获取放大缩小的pix";
    pix = p;
    xLevel = x;
    yLevel = y;
    drawTrackPixmap();
}
