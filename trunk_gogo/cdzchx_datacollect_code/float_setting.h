#ifndef FLOAT_SETTING_H
#define FLOAT_SETTING_H

#include <QWidget>
#include <QMap>
#include <QList>
#include "profiles.h"

namespace Ui {
class float_setting;
}

class float_setting : public QWidget
{
    Q_OBJECT

public:
    explicit float_setting(QWidget *parent = 0);
    ~float_setting();
signals:
    void updateFloatSignal();
private slots:
    void on_add_pushButton_clicked();

    void on_del_pushButton_clicked();

    void on_save_pushButton_clicked();

    //void on_mod_pushButton_clicked();

    //void on_sure_pushButton_clicked();

private:
    Ui::float_setting *ui;
    QMap<int,QStringList> fMap;
};

#endif // FLOAT_SETTING_H
