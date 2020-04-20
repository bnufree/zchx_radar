#include "testrotatewidget.h"
#include "ui_testrotatewidget.h"
#include <QPainter>
#include <QDebug>
#include <QVector2D>
#include "qt/zchxutils.hpp"
#include <QStringList>

#define         GLOB_PI                                 (3.14159265358979323846)

testRotateWidget::testRotateWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::testRotateWidget)
{
    ui->setupUi(this);
    test = new MyWidget(this);
    ui->frame->layout()->addWidget(test);
    ui->x->setRange(-10, 10);
    ui->y->setRange(-10, 10);
    ui->x->setValue(0);
    ui->x->setValue(0);
    ui->x->setPageStep(1);
    ui->y->setPageStep(1);
    connect(ui->x, SIGNAL( valueChanged(int)), test, SLOT(setX(int)));
    connect(ui->y, SIGNAL( valueChanged(int)), test, SLOT(setY(int)));
}

testRotateWidget::~testRotateWidget()
{
    delete ui;
}

MyWidget::MyWidget(QWidget *parent) :
    QWidget(parent),
    mPress(false),
    mX(0),
    mY(0)
{
#if 1
    mTimer = new QTimer;
    mTimer->setInterval(1000);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(update()));
    mTimer->start();
#else
    on_calc_clicked();
#endif
}



void MyWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    QPoint p1 = this->rect().center();
    double angle = GLOB_PI / 4;
    QPoint p2 = mStart;
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::red);
    painter.drawEllipse(p1, 5, 5);
    painter.drawText(p1, "O");

    if(!mPress) return;
    painter.drawEllipse(p2, 5,5);
    painter.drawText(p2, "P2");
    QPoint center((p1.x() + p2.x())/ 2, (p1.y() + p2.y())/ 2);
    QVector2D sub(p2- p1);
    angle = atan2(sub.y(), sub.x());
    qDebug()<<"origin angle:"<<angle<<sub;
    int old_rotate = angle * 180 / GLOB_PI;
    if(angle < 0) angle += (GLOB_PI * 2);
    double rotate = angle * 180 / GLOB_PI;
    qDebug()<<"angle:"<<angle<<rotate;
    double len = sub.length();

//    painter.translate(-center.x(), -center.y());
    QRect rect(200, 50, len, 60);
    painter.save();
//    painter.translate(center.x(), center.y());
//    painter.rotate(rotate);
//    rect.moveCenter(QPoint(0, 0));
    painter.setBrush(Qt::green);
    painter.drawRoundRect(rect, mX * 100, mY* 100);

    return;
    QLinearGradient line(rect.topLeft(), rect.bottomRight());
    line.setColorAt(0, Qt::red);
    line.setColorAt(0.2, Qt::yellow);
    line.setColorAt(0.8, Qt::yellow);
    line.setColorAt(1, Qt::red);

    painter.shear(mX, mY);
    qDebug()<<"mX:"<<mX<<" mY:"<<mY;
    painter.setBrush(line);
    painter.drawRect(rect);
    painter.setPen(QPen(Qt::darkMagenta, 5));
    painter.drawText(rect.center(), QString("rotate angle is %1 --- mX = %2, mY = %3").arg(rotate).arg(mX).arg(mY));
    QPolygon shape = painter.transform().mapToPolygon(rect);
    painter.restore();
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    for(int i=0; i<shape.size(); i++)
    {
        painter.drawEllipse(shape[i], 2, 2);
        painter.drawText(shape[i], QString("%1, %2").arg(shape[i].x()).arg(shape[i].y()));
    }






}

void MyWidget::mousePressEvent(QMouseEvent *e)
{
    mPress = true;
    mStart = e->pos();
}

void MyWidget::mouseReleaseEvent(QMouseEvent *e)
{
    mStart = e->pos();
}

void MyWidget::mouseMoveEvent(QMouseEvent *e)
{
    mStart = e->pos();
}

void MyWidget::on_calc_clicked()
{
    QString src = "{22.212637,113.122392},"
                  "{22.211724,113.121148},"
                  "{22.211247,113.120504},"
                  "{22.210730,113.119860},"
                  "{22.210134,113.119174},"
                  "{22.209618,113.118358},"
                  "{22.208744,113.117243},"
                  "{22.207750,113.115784},"
                  "{22.206837,113.114281},"
                  "{22.205923,113.112908},"
                  "{22.204930,113.111621},"
                  "{22.203698,113.110076},"
                  "{22.202824,113.109089},"
                  "{22.202228,113.107973},"
                  "{22.201314,113.106557},"
                  "{22.200440,113.105827},"
                  "{22.199605,113.104497},"
                  "{22.198612,113.103510},"
                  "{22.197738,113.102995},"
                  "{22.196903,113.102480},"
                  "{22.196109,113.102094},"
                  "{22.195314,113.101922},"
                  "{22.194082,113.101536},"
                  "{22.192373,113.101021},"
                  "{22.191976,113.100334},"
                  "{22.190228,113.098102},"
                  "{22.189632,113.096858},"
                  "{22.188797,113.095742},"
                  "{22.188042,113.094626},"
                  "{22.187247,113.093296},"
                  "{22.186572,113.092223},";
    QStringList src_list = src.split(QRegExp("[\{\, \}]"), QString::SkipEmptyParts);
    QList<ZCHX::Data::LatLon> ll_list;
    double angle = 5;
    double dis = 100;
    QStringList res_list;
    for(int i=0; i<src_list.size()-2; i = i+2)
    {
        ZCHX::Data::LatLon ll = {src_list[i].toDouble(), src_list[i+1].toDouble()};
        ZCHX::Data::LatLon dest = ZCHX::Utils::distbear_to_latlon(ll.lat, ll.lon, dis, angle);
        angle += 3;
        res_list.append(QString("{%1},{%2}").arg(QString("%1,%2").arg(ll.lat, 0, 'r', 6).arg(ll.lon, 0, 'r', 6))
                .arg(QString("%1,%2").arg(dest.lat, 0, 'r', 6).arg(dest.lon, 0, 'r', 6)));
    }

    qDebug()<<res_list;

}

void testRotateWidget::on_reset_clicked()
{
    test->reset();
    ui->x->setValue(0);
    ui->y->setValue(0);
}
