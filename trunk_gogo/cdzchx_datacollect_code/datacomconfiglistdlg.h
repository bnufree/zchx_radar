#ifndef DATACOMCONFIGLISTDLG_H
#define DATACOMCONFIGLISTDLG_H

#include <QDialog>
#include <QTableWidget>

namespace Ui {
class DataComConfigListDlg;
}

class MyTableWidgetItem : public QTableWidgetItem
{
public:
    MyTableWidgetItem(const QString& text)
        :QTableWidgetItem(text)
    {
        setTextAlignment(Qt::AlignCenter);
//        QFont font = this->font();
//        font.setPointSize(10);
//        setFont(font);
    }

    ~MyTableWidgetItem()
    {

    }

};

class DataComConfigListDlg : public QDialog
{
    Q_OBJECT

public:
    explicit DataComConfigListDlg(QWidget *parent = 0);
    ~DataComConfigListDlg();
    void init();

private slots:
    void on_addbtn_clicked();

    void on_delbtn_clicked();

    void on_modbtn_clicked();

private:
    Ui::DataComConfigListDlg *ui;
};

#endif // DATACOMCONFIGLISTDLG_H
