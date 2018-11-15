//**************************************************************************
//* @File: category.h
//* @Description: category类 声明
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

#ifndef CATEGORY_H_
#define CATEGORY_H_

#include <string>
#include <list>

#include "DataItemDescription.h"
#include "UAP.h"

class Category
{
public:
  Category(int id);
  virtual
  ~Category();

  unsigned int m_id;
  bool m_bFiltered; //! at least one item of category shall be printed when filter is applied

  std::string m_strName;
  std::string m_strVer;

  std::list<DataItemDescription*> m_lDataItems;
  std::list<UAP*> m_lUAPs;

  DataItemDescription* getDataItemDescription(std::string id); //!< returns requested data item or creates new if not existing
  UAP* newUAP(); //!< creates and returns new UAP
  UAP* getUAP(const unsigned char* data, unsigned long len); // get UAP that matches condition
  std::string printDescriptors(); // print item descriptors
  bool filterOutItem(std::string item, const char* name);
  bool isFiltered(std::string item, const char* name);

};

#endif /* CATEGORY_H_ */
