#ifndef TESTROTATEWIDGET_H
#define TESTROTATEWIDGET_H

#include <QWidget>
#include <QTimer>.
#include <QMouseEvent>

namespace Ui {
class testRotateWidget;
}

class MyWidget;

class testRotateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit testRotateWidget(QWidget *parent = 0);
    ~testRotateWidget();

private slots:
    void on_reset_clicked();

private:
    Ui::testRotateWidget *ui;
    MyWidget *test;
};

class MyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyWidget(QWidget *parent = 0);
    ~MyWidget() {}

protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
public slots:
    void setX(int x) {mX = x * 0.1;}
    void setY(int y) {mY = y * 0.1;}
    void reset() {mX = 0; mY = 0;}

private slots:
    void on_calc_clicked();

private:
    QPoint  mStart;
    QPoint  mEnd;
    bool    mPress;
    QTimer*  mTimer;
    qreal   mX;
    qreal   mY;
};

#endif // TESTROTATEWIDGET_H
