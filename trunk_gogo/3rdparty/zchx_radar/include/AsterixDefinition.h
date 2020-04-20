//**************************************************************************
//* @File: asterixdefinition.cpp
//* @Description: asterixdefinition类 声明
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

#ifndef ASTERIXDEFINITION_H_
#define ASTERIXDEFINITION_H_

#include "Category.h"

#define MAX_CATEGORIES  256

class AsterixDefinition
{
public:
  AsterixDefinition();
  virtual
  ~AsterixDefinition();

  Category* getCategory(int i);
  bool CategoryDefined(int i);
  std::string printDescriptors();
  bool filterOutItem(int cat, std::string item, const char* name);
  bool isFiltered(int cat, std::string item, const char* name);

private:
  Category *m_pCategory[MAX_CATEGORIES];
};

#endif /* ASTERIXDEFINITION_H_ */
