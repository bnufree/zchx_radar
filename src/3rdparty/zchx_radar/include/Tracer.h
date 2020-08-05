//**************************************************************************
//* @File: tracer.h
//* @Description: tracer类 声明
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
//*   1.0  2014/10/15   李惟谦		  初始化创建,基于Damir Salantic修改
//*                               增加函数getBinary,setValue
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef TRACER_H_
#define TRACER_H_

#include <stdlib.h>

typedef int(*ptExtPrintf)(char const*, ...);
typedef void(*ptExtVoidPrintf)(char const*, ...);

class Tracer
{
public:
  Tracer();
  virtual  ~Tracer();
  static void Error(const char* format, ...);
  static void Configure(ptExtPrintf pFunc);
  static void Configure(ptExtVoidPrintf pFunc);

  static Tracer *g_TracerInstance;
  static Tracer& instance();

  ptExtPrintf pPrintFunc;
  ptExtVoidPrintf pPrintFunc2;
};

#endif /* TRACER_H_ */
