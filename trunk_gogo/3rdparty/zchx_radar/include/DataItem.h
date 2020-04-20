///**************************************************************************
//* @File: dataitem.h
//* @Description: dataitem类 声明
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

#ifndef DATAITEM_H_
#define DATAITEM_H_

#include <string>

#include "DataItemDescription.h"

class DataItem
{
public:
  DataItem(DataItemDescription* pDesc);
  virtual
  ~DataItem();

  DataItemDescription* m_pDescription;

  bool getText(std::string& strResult, std::string& strHeader, const unsigned int formatType); // appends value to strResult in formatType format
  bool getBinary(std::string& strResult); // add by flab
  long parse(const unsigned char* pData, long len);
  bool setValue(std::string& strValue, std::string& itemName, int index=-1);
  long getLength() { return m_nLength; }
  unsigned char* m_pData;
  long m_nLength;
#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  fulliautomatix_data* getData(int byteoffset);
#endif
private:

};

#endif /* DATAITEM_H_ */
