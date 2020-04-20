#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& app = QString(), QWidget *parent = 0);
    ~MainWindow();
    void     setAppName(const QString& appName){mAppName = appName;}
public slots:
    void slotTimeOut();
private slots:
    void start();

private:
    Ui::MainWindow *ui;
    QString  mAppName;
    QString  mDogFile;
    qint64   mExeID;
};

#endif // MAINWINDOW_H
