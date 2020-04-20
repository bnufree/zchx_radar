#include "zchxmapscaleslider.h"
#include "ui_zchxmapscaleslider.h"

zchxMapScaleSlider::zchxMapScaleSlider(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::zchxMapScaleSlider)
{
    ui->setupUi(this);
}

zchxMapScaleSlider::~zchxMapScaleSlider()
{
    delete ui;
}
