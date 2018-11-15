#ifndef __LOG_H__
#define __LOG_H__

#include<stdio.h>
#include <QString>
/*******************************************************************
* @版权信息：
* @文件名称：Log.h
* @摘    要：日志操作定义头文件
* @作    者：周勇
* @当前版本：1.0.0
* @日    期：2015年4月2日
* @备    注：线程安全日志类
*******************************************************************/
typedef enum _tagENUM_LOG_TYPE
{
	LOG_NONE,				//不开启日志
	LOG_ERROR,				//错误日志
	LOG_RTM,					//运行日志
	LOG_DEBUG 			//调试日志
}LOG_TYPE;





#define LOG_FILE_PATH		250
#define MAX_LOG_ROW_SIZE	500				//一行日志的大小
#define MAX_LOG_FILE_SIZE (10*1024*1024)	//日志文件大小


//初始化日志
void InitLog();
void InitFilePath(char* topic, bool newFile = false);
QString getOldFile(const QString& topic);
QString getNewFile(const QString& topic);
//设置日志参数
void SetLogParam(int nLogLevel=LOG_RTM, int nLogFileSize = MAX_LOG_FILE_SIZE);
//写日志函数
void LOG(LOG_TYPE t, char* format, ...);
//记录数据文件
void LOG(const QString& topic, char* format, ...);

#endif //__LOG_H__
