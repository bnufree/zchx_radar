#ifndef DBGCRASH_H
#define DBGCRASH_H

#include <Windows.h>
#include <DbgHelp.h>
#include <iostream>
#include <vector>
// 崩溃信息
//
using namespace std;
const int MAX_ADDRESS_LENGTH = 32;
const int MAX_NAME_LENGTH = 1024;

struct CrashInfo
{
    CHAR ErrorCode[MAX_ADDRESS_LENGTH];
    CHAR Address[MAX_ADDRESS_LENGTH];
    CHAR Flags[MAX_ADDRESS_LENGTH];
};

// CallStack信息
//
struct CallStackInfo
{
    CHAR ModuleName[MAX_NAME_LENGTH];
    CHAR MethodName[MAX_NAME_LENGTH];
    CHAR FileName[MAX_NAME_LENGTH];
    CHAR LineNumber[MAX_NAME_LENGTH];
};

void SafeStrCpy(char* szDest, size_t nMaxDestSize, const char* szSrc);
CrashInfo GetCrashInfo(const EXCEPTION_RECORD *pRecord);
vector<CallStackInfo> GetCallStack(const CONTEXT *pContext);
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException);
void printStack( void );

#endif // DBGCRASH_H
