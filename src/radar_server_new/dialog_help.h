#ifndef DIALOG_HELP_H
#define DIALOG_HELP_H

#include <QWidget>
#include <QMessageBox>

namespace Ui {
class dialog_help;
}

class dialog_help : public QWidget
{
    Q_OBJECT

public:
    explicit dialog_help(QWidget *parent = 0);
    ~dialog_help();
    void ChangePicture();

private slots:
    void on_next_pushButton_clicked();

    void on_front_pushButton_clicked();

private:
    Ui::dialog_help *ui;
    int index;
};

#endif // DIALOG_HELP_H
