//**************************************************************************
//* @File: Utils.h
//* @Description: 常用函数
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
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <stdlib.h>

//#include <boost/numeric/conversion/cast.hpp>
//#include <boost/date_time.hpp>
//#include <boost/foreach.hpp>
//#include <boost/lexical_cast.hpp>



std::string format_arg_list(const char *fmt, va_list args);
std::string format(const char *fmt, ...);
void strConvertBinary(std::string& str_val);

#endif /* UTILS_H_ */
