//**************************************************************************
//* @File: asterixhdlcparsing.h
//* @Description: asterixhdlcparsing类 声明
//* @Copyright: Copyright (c) 2014
//* @Company: 深圳置辰海信科技有限公司
//* @WebSite: http://www.szcenterstar.com/
//* @author 李惟谦
//* @Revision History
//*
//* <pre>
//* ----------------------------------------------------------------------
//*   Ver     Date       Who             Comments
//*  ----- ----------  --------  ---------------------------------------
//*   1.0  2014/9/20   李惟谦		  初始化创建,基于Damir Salantic修改
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef ASTERIXHDLCPARSING_H_
#define ASTERIXHDLCPARSING_H_

// defines
#define MAX_RXBUF 0x1000                        // 4K
//#define MAX_RXBUF 90 //test
#define MAX_CBUF  0x10000                       // 64K
#define MAX_FRM   0x200                         // 2*256 byte


// data
extern unsigned char RxBuf[];       // buffer za ucitavanje podatka s COM

// fixd by heron lee
#ifdef __cplusplus
extern "C" {
#endif

int copy_to_cbuf(unsigned char *RxBuf, int Cnt);

int get_hdlc_frame(int iF, int MinLen);

unsigned short test_hdlc_fcs(int iF, int iL);

unsigned char *get_next_hdlc_frame(int *len);

int GetAndResetFailedBytes();

#ifdef __cplusplus
}
#endif

// functions

#endif /* ASTERIXHDLCPARSING_H_ */
