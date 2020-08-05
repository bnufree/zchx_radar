#ifndef ZCHXMAPSCALESLIDER_H
#define ZCHXMAPSCALESLIDER_H

#include <QWidget>

namespace Ui {
class zchxMapScaleSlider;
}

class zchxMapScaleSlider : public QWidget
{
    Q_OBJECT

public:
    explicit zchxMapScaleSlider(QWidget *parent = 0);
    ~zchxMapScaleSlider();

private:
    Ui::zchxMapScaleSlider *ui;
};

#endif // ZCHXMAPSCALESLIDER_H
