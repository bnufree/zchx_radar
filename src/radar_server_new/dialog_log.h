#ifndef DIALOG_LOG_H
#define DIALOG_LOG_H

#include <QDialog>

namespace Ui {
class Dialog_log;
}

class Dialog_log : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_log(QWidget *parent = 0);
    ~Dialog_log();
public slots:
    void receiveLogSlot(qint64 time, const QString& name, const QString& content);//打印数据日志槽函数

private:
    Ui::Dialog_log *ui;
};

#endif // DIALOG_LOG_H
