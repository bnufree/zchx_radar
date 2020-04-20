#ifndef DATACOMCONFIGDLG_H
#define DATACOMCONFIGDLG_H

#include <QDialog>

namespace Ui {
class DataComConfigDlg;
}

class DataComConfigDlg : public QDialog
{
    Q_OBJECT

public:
    explicit DataComConfigDlg(QWidget *parent = 0);
    ~DataComConfigDlg();
    void setParam(const QString& type, const QString& com, const QString& braud, bool chk);
    QString type();
    QString com();
    QString braud();
    bool    check();

private slots:
    void on_cancelbtn_clicked();

    void on_okbtn_clicked();

private:
    Ui::DataComConfigDlg *ui;
};

#endif // DATACOMCONFIGDLG_H
