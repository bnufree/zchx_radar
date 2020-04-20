#pragma once;

#ifndef DLL_IMPLEMENT
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

//typedef struct SHead{
//	int iArea;
//	int iMsg;
//	int iAzm;
//	int iHead;
//	float fR0;
//	float fDr;
//	int iBit;
//	int iTime;
//}SHead;

#define AZM_IN_SCAN  2048
#define DATA_NUM_IN_AZM	512

struct SDataHead{
	int iArea;
	int iSys;
	int iMsg;
	int iAzm;
	int iHead;
	float fR0;
	float fDR;
	int iBit;
        int iTime;
};

struct SDataTail{
        int iAzmCell;
        int iRngCell;
        double dRotateSpd;
        double dLon;
        double dLat;
        //int iUnknown[2]
        //float fUnknown[4];
};

struct SAzmData{
        struct SDataHead sHead;
        int iRawData[DATA_NUM_IN_AZM];
        struct SDataTail sTail;
};

DLL_API int Tracking(struct SAzmData* psScan, int* pSit);
