#include "Log.h"
#include <QMutex>
#include <QMutexLocker>
#include <QDir>
#include <QTime>
#include <QMap>
#include <QApplication>
#include "dataserverutils.h"
#include <QDebug>
/*
WriteLog: 写日志函数---如果日志文件超过指定大小 则从0开始
*/

#define         LOG_PATH            QDir::currentPath() + "/data"

int g_nLogLevel = LOG_DEBUG;
int g_nLogFileSize = MAX_LOG_FILE_SIZE;



char g_szFileName[LOG_FILE_PATH] = { 0 };
char g_szDataFileName[LOG_FILE_PATH] = { 0 };
QMutex g_cs;
QMutex g_avcs;
bool g_bInit=false;
QMap<QString, QString>      mLastOptFileName;

void InitLog()
{

    QString path;
    QDir dir;
    path = dir.currentPath();

    sprintf_s(g_szFileName,"%s//datacollect.log",path.toLocal8Bit().data());

    g_nLogLevel = LOG_RTM;
    g_nLogFileSize = MAX_LOG_FILE_SIZE;
    g_bInit = true;
}


QString getOldFile(const QString& topic)
{
    return mLastOptFileName.value(topic, QString());
}

QString getNewFile(const QString& topic)
{
    //在当前目录检索文件
    QDir dir;
    dir.setPath(LOG_PATH);
    if(!dir.exists())
    {
        dir.mkpath(LOG_PATH);
    }

    //文件不存在
    QDateTime cur = QDateTime::currentDateTime();
    QString fileName  = QString("%1/%2%3.data").arg(dir.path()).arg(topic).arg(cur.toString("yyyyMMddhhmmss"));
    mLastOptFileName[topic] = fileName;
    return fileName;
}

void SetLogParam(int nLogLevel, int nLogFileSize)
{
    g_nLogLevel = nLogLevel;
    g_nLogFileSize = nLogFileSize;
}

void LOG(const QString& topic, char* format, ...)
{
    g_nLogFileSize = MAX_LOG_FILE_SIZE;
    QMutexLocker locker(&g_avcs);
    QString fileName = getOldFile(topic);
    if(fileName.length() == 0)
    {
        fileName = getNewFile(topic);
    }

    sprintf_s(g_szDataFileName,"%s",fileName.toLocal8Bit().data());

    FILE *pFile = NULL;
    int dwFileSize = 0;
    va_list arg;
    //获取当前时间
    QString strTime = DataServerUtils::currentTimeString(false);
    fopen_s(&pFile, g_szDataFileName, "a+");
    if (pFile == NULL) return;

    //限制大小
    fseek(pFile, 0, SEEK_END);
    dwFileSize = ftell(pFile);
    //qDebug()<<g_szDataFileName<<" size:"<<dwFileSize<< " limit:"<<g_nLogFileSize;
    if (dwFileSize > g_nLogFileSize)
    {
        fclose(pFile);
        pFile = NULL;
        fileName = getNewFile(topic);
        sprintf_s(g_szDataFileName,"%s",fileName.toLocal8Bit().data());
        fopen_s(&pFile, g_szDataFileName, "w");
        dwFileSize = 0;
        if (pFile == NULL) return;
    }


    fprintf(pFile, "%s\t", strTime.toStdString().c_str());
    //处理参数
    va_start(arg, format);
    vfprintf(pFile, format, arg);
    fprintf(pFile, "\n");
    fflush(pFile);
    va_end(arg);
    fclose(pFile);
}


void LOG(LOG_TYPE t, char* format, ...)
{

    if( false == g_bInit ) InitLog();
    g_cs.lock();

    FILE *pFile = NULL;
    int dwFileSize = 0;
    va_list arg;
    //char szFileName[LOG_FILE_PATH];
    //SYSTEMTIME tm;
    char uchStrTime[LOG_FILE_PATH] = { 0 };
    char szMsg[1024] = { 0 };
    //获取当前时间
    QString strTime = DataServerUtils::currentTimeString(false);

    if (t > g_nLogLevel) goto END_LOG;

    /* 文件处理 */
    //sprintf_s(szFileName, LOG_FILE_PATH,"RTSPLOG.log");

    fopen_s(&pFile, g_szFileName, "a+");
    if (pFile == NULL)
    {
        goto END_LOG;
    }

    //限制大小
    fseek(pFile, 0, SEEK_END);
    dwFileSize = ftell(pFile);
    if (dwFileSize > g_nLogFileSize)
    {
        fclose(pFile);
        pFile = NULL;
        fopen_s(&pFile, g_szFileName, "w");
        dwFileSize = 0;
        if (pFile == NULL)
        {
            goto END_LOG;
        }
    }


    fprintf(pFile, "%s\t", strTime.toStdString().c_str());
    switch (t)
    {
        case LOG_NONE:
            break;
        case LOG_ERROR:
            fprintf(pFile, "[ERR]");
            break;
        case LOG_RTM:
            fprintf(pFile, "[RTM]");
            break;
        case LOG_DEBUG:
            fprintf(pFile, "[DBG]");
            break;
    }
    //处理参数
    va_start(arg, format);
    vfprintf(pFile, format, arg);
    fprintf(pFile, "\n");
    fflush(pFile);
    va_end(arg);
    fclose(pFile);
END_LOG:
    g_cs.unlock();
}
