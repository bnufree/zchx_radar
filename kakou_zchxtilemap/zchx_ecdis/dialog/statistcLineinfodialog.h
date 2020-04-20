#ifndef STATISTCLINEINFODIALOG_H
#define STATISTCLINEINFODIALOG_H

#include <QDialog>

namespace Ui {
class StatistcLineInfoDialog;
}
namespace qt {
class StatistcLineInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatistcLineInfoDialog(QWidget *parent = 0);
    ~StatistcLineInfoDialog();

    QString getName();
    int getEnterStatus();
    QString getColor();

private slots:
    void on_colorPushButton_clicked();

    void on_comfirmButton_clicked();

    void on_cancelButton_clicked();

private:
    Ui::StatistcLineInfoDialog *ui;
    QString m_color;
};
}

#endif // STATISTCLINEINFODIALOG_H
