#include "aisLabel.h"
#include <qdebug.h>
#include <QPainter>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

aisLabel::aisLabel(QWidget *parent) : QLabel(parent)
{
    this->resize(850, 600);
    mPix = QPixmap(850, 600);
    drawTrackPixmap();

}
//滑动鼠标
void aisLabel::wheelEvent(QWheelEvent *event)
{

}
//按下鼠标
void aisLabel::mousePressEvent(QMouseEvent *event)
{

    int x_pos = event->pos().x();
    int y_pos = event->pos().y();
    cout<<"坐标点"<<x_pos<<y_pos;
    QPoint pot(x_pos, y_pos);
    QPainter painter(&mPix);// 画的设备
    QPen pen;
    pen.setWidth(8); //设置宽度
    pen.setBrush(Qt::red); //设置颜色
    painter.setPen(pen); //选中画笔
    painter.drawPoint(pot);

    aisPolygon<<pot;
    if(aisPolygon.size() > 1)
    {
        for(int i=0; i<aisPolygon.size() - 1; i++)
        {
            painter.save();
            pen.setWidth(3); //设置宽度
            pen.setBrush(Qt::white); //设置颜色
            painter.setPen(pen); //选中画笔
            painter.drawLine(aisPolygon[i], aisPolygon[i+1]);
            painter.restore();
        }
        painter.drawPoints(aisPolygon);
    }

   this->setPixmap(mPix);
}

//获得画板
void aisLabel::slotCreatBtnClicked()
{
    cout<<"生成数据按钮按下";
//    if(aisPolygon.size() > 1)
//    {
//        QPainter painter(&mPix);// 画的设备
//        QPen pen;
//        pen.setWidth(8); //设置宽度
//        pen.setBrush(Qt::white); //设置颜色
//        painter.setPen(pen); //选中画笔
//        painter.drawText(aisPolygon.first().x(), aisPolygon.first().y(),QString::number(412502570)); //绘制文本
//    }
//    this->setPixmap(mPix);
    emit signalAisPolygon(aisPolygon);
    aisPolygon.clear();
}
//绘制网格坐标系
void aisLabel::drawTrackPixmap()
{
    mPix.fill(Qt::gray);
    QPainter painter(&mPix);// 画的设备
    QPen pen;
    pen.setWidth(2); //设置宽度
    pen.setBrush(Qt::black); //设置颜色
    painter.setPen(pen); //选中画笔
    painter.drawRect(0, 0,mPix.width(),mPix.height());
    int i;
    for(i = 0;i<mPix.width();i+=50)//x 轴
    {
         painter.drawLine(i+50,mPix.height(),i+50,0); //绘制x轴上的点
         painter.drawText(i,mPix.height()-2,QString::number(i)); //绘制文本

    }
    for(i=0;i<mPix.height();i+=50)//y 轴
    {
         painter.drawLine(0,i,mPix.width(),i); //绘制y轴上的点
         painter.drawText(0,i,QString::number(mPix.height()-i)); //绘制文本
    }
    //画图
    QPixmap pixmap_1 = mPix;
    this->setPixmap(pixmap_1);

}

void aisLabel::drawTrackName(QString str)
{
    if(aisPolygon.size() > 1)
    {
        QPainter painter(&mPix);// 画的设备
        QPen pen;
        pen.setWidth(8); //设置宽度
        pen.setBrush(Qt::white); //设置颜色
        painter.setPen(pen); //选中画笔
        painter.drawText(aisPolygon.first().x(), aisPolygon.first().y(),str); //绘制文本
    }
    this->setPixmap(mPix);
}
//根据表格的修改重绘轨迹
void aisLabel::slotRedrawAisTrack(QMap<int,QPolygonF> pmap)
{
    drawTrackPixmap();
    foreach (int k, pmap.keys()) {
        for(int i=0; i<pmap[k].size(); i++)
        {
            int x_pos = pmap[k][i].x();
            int y_pos = pmap[k][i].y();

            QPoint pot(x_pos, y_pos);
            QPainter painter(&mPix);// 画的设备
            QPen pen;
            pen.setWidth(8); //设置宽度
            pen.setBrush(Qt::red); //设置颜色
            painter.setPen(pen); //选中画笔
            painter.drawPoint(pot);

            aisPolygon<<pot;
            if(aisPolygon.size() > 1)
            {
                for(int i=0; i<aisPolygon.size() - 1; i++)
                {
                    painter.save();
                    pen.setWidth(3); //设置宽度
                    pen.setBrush(Qt::white); //设置颜色
                    painter.setPen(pen); //选中画笔
                    painter.drawLine(aisPolygon[i], aisPolygon[i+1]);
                    painter.restore();
                }
                painter.drawPoints(aisPolygon);
            }
        }
        aisPolygon.clear();
    }
    //画图
    QPixmap pixmap_1 = mPix;
    this->setPixmap(pixmap_1);
}
//清除所有的点
void aisLabel::delThePoy()
{
     aisPolygon.clear();
}
