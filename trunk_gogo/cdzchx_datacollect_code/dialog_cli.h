#ifndef DIALOG_CLI_H
#define DIALOG_CLI_H

#include <QDialog>

namespace Ui {
class Dialog_cli;
}

class Dialog_cli : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_cli(QWidget *parent = 0);
    ~Dialog_cli();
public slots:
    void slotUpdateClientTable(const QString& ip, const QString& name, int port, int inout);
private:
    Ui::Dialog_cli *ui;
    QStringList       mClientList;
};

#endif // DIALOG_CLI_H
