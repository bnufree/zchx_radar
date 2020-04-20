#include "zchxmapmonitorthread.h"
#include <QDebug>
#include <windows.h>
#include <tlhelp32.h>// for CreateToolhelp32Snapshot
#include <psapi.h>   // for GetModuleFileNameEx
#include <QTimer>
#include <QProcess>
#include <QDateTime>
#include <QDir>
#include <QApplication>

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
        QString exeName = (QString::fromUtf16(reinterpret_cast<const unsigned short *>(pe32.szExeFile)));
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




zchxMapMonitorThread::~zchxMapMonitorThread()
{
    if(mAppID > 0)
    {
        KillProcess(mAppID);
    }
    if(mSub)
    {
        delete mSub;
    }
}

void zchxMapMonitorThread::run()
{
    while (1)
    {
        QMap<QString,qint64> app_pid;
        getAllAppPidList(app_pid);
        if(!app_pid.contains(mAppName))
        {
            //目标还没有启动???
            runApp();
        } else
        {
            mAppID = app_pid[mAppName];
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
                if(KillProcess(mAppID))
                {
                    runApp();
                }

            }
        }
        sleep(30);
    }
}


void zchxMapMonitorThread::runApp()
{
    if(mSub) delete mSub;
    mSub = new QProcess;
    mSub->start(mAppName);
//    QProcess::startDetached(mAppName);
}


zchxMapMonitorThread::zchxMapMonitorThread(const QString& app, QObject *parent)
    : QThread(parent)
    , mAppName(app)
    , mAppID(-1)
    , mSub(0)
{
    if(mAppName.isEmpty()) mAppName = myAppName;
    mDogFile = mAppName;
    int index = mDogFile.indexOf(".exe");
    if(index >= 0)
    {
        mDogFile.replace(index, 4, ".txt");
    }
}
