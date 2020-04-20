#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <windows.h>
#include <tlhelp32.h>// for CreateToolhelp32Snapshot
#include <psapi.h>   // for GetModuleFileNameEx
#include <QTimer>
#include <QProcess>
#include <QDateTime>
#include <QSystemTrayIcon>
#include <QDir>

#define FORMAT_PATH(path) path.replace('\\','/').toLower()

const QString myAppName = "zchxMapTest.exe";

QString GetPathByProcessID(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess)
    {
        //QMessageBox::warning(NULL,"GetPathByProcessID","无权访问该进程");
        return "";
    }
    WCHAR filePath[MAX_PATH];
    DWORD ret= GetModuleFileNameEx(hProcess, NULL, filePath, MAX_PATH) ;
    QString file = QString::fromStdWString( filePath );
    //QMessageBox::warning(NULL,"GetPathByProcessID ret=", QString::number(ret)+":"+file);
    CloseHandle(hProcess);
    return ret==0?"":file;
}

QString getAllAppPidList(QMap<QString,qint64>& app_pid)
{
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(pe32);
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(hProcessSnap == INVALID_HANDLE_VALUE)
    {
        return QString("CreateToolhelp32Snapshot failed");
    }
    BOOL bMore = Process32First(hProcessSnap,&pe32);
    while(bMore)
    {
        //printf("进程名称：%s\n",pe32.szExeFile);
        //printf("进程ID：%u\n\n",pe32.th32ProcessID);

        QString exeName = (QString::fromUtf16(reinterpret_cast<const unsigned short *>(pe32.szExeFile)));
        //        QString exePath = GetPathByProcessID( pe32.th32ProcessID );
        //        exePath = FORMAT_PATH( exePath );
        //        if( exePath.isEmpty() )
        //        {
        //            qDebug()<<("获取进程 " + exeName + " 路径失败");
        //        }
        //        else
        //        {
        //            qDebug()<<"get exe:"<<exeName<<exePath;
        //            app_pid[exePath] = pe32.th32ProcessID;
        //        }
        app_pid[exeName] = pe32.th32ProcessID;
        bMore = Process32Next(hProcessSnap,&pe32);
    }
    CloseHandle(hProcessSnap);
    return QString();
}

bool KillProcess(DWORD ProcessId)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE,FALSE,ProcessId);
    if(hProcess==NULL)
        return false;
    if(!TerminateProcess(hProcess,0))
        return false;
    return true;
}


MainWindow::MainWindow(const QString& app, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mAppName(app),
    mExeID(-1)
{
    ui->setupUi(this);
    if(mAppName.isEmpty()) mAppName = myAppName;
    mDogFile = mAppName;
    int index = mDogFile.indexOf(".exe");
    if(index >= 0)
    {
        mDogFile.replace(index, 4, ".txt");
    }
//    this->setWindowFlags(windowFlags() ^ Qt::WindowMaximizeButtonHint);
    setWindowFlags(Qt::SubWindow);

    QTimer *timer = new QTimer(this);
    timer->setInterval(3000 * 10);
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimeOut()));
    timer->start();
    slotTimeOut();
}

MainWindow::~MainWindow()
{
    if(mExeID > 0)
    {
        KillProcess(mExeID);
    }
    delete ui;
}

void MainWindow::slotTimeOut()
{
    QMap<QString,qint64> app_pid;
    QString str = getAllAppPidList(app_pid);
    if(!app_pid.contains(mAppName))
    {
        //目标还没有启动???
        start();
    } else
    {
        mExeID = app_pid[mAppName];
        //检查目标是死是活
        QDir dir(QApplication::applicationDirPath() + QString("/watchdog"));
        if(!dir.exists())
        {
            dir.mkpath(dir.path());
        }

        QString fileName = QString("%1/%2").arg(dir.path()).arg(mDogFile);
        QFileInfo fileInfo(fileName);
        //检查文件的更新时间
        QDateTime now = QDateTime::currentDateTime();
        if(fileInfo.lastModified().secsTo(now) >= 40)
        {
            //目标已经死了,关掉这个进程,然后重新启动
             if(KillProcess(mExeID))
             {
                 start();
             }

        }
    }
}


void MainWindow::start()
{
    QProcess::startDetached(mAppName);
    ui->listWidget->addItem(QString("%1 VHF started..").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    if(ui->listWidget->count() == 10)
    {
        QListWidgetItem* item = ui->listWidget->takeItem(0);
        if(item) delete item;
    }
}





