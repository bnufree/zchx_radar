#ifndef ZCHXHOSTSETTING_H
#define ZCHXHOSTSETTING_H

#include <QWidget>

namespace Ui {
class zchxHostSetting;
}

class zchxHostSetting : public QWidget
{
    Q_OBJECT

public:
    explicit zchxHostSetting(QWidget *parent = 0);
    ~zchxHostSetting();

private:
    Ui::zchxHostSetting *ui;
};

#endif // ZCHXHOSTSETTING_H
