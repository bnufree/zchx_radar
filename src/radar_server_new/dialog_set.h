#ifndef DIALOG_SET_H
#define DIALOG_SET_H

#include <QDialog>

namespace Ui {
class Dialog_set;
}

class Dialog_set : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_set(int id,QWidget *parent = 0);
    ~Dialog_set();
signals:
    void signalRangeFactorChanged_1(double val);
    void set_change_signal_1();
    void signalGetGpsData(double, double);
public slots:
    void slotUpdateRealRangeFactor(double,double);
private:
    Ui::Dialog_set *ui;
};

#endif // DIALOG_SET_H
