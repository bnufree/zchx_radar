//**************************************************************************
//* @File: UAPItem.h
//* @Description: UAPItem类 声明
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

#ifndef UAPITEM_H_
#define UAPITEM_H_

#include "Tracer.h"
#include "DataItemFormat.h"


class UAPItem : public DataItemFormat
{
public:
  UAPItem();
  virtual
  ~UAPItem();

  int m_nBit; // <!ATTLIST UAPItem bit CDATA #REQUIRED>
  int m_nFRN; // <!ATTLIST UAPItem frn CDATA #REQUIRED>
  bool m_bFX; // <!ATTLIST UAPItem fx CDATA "0">
  int m_nLen; // <!ATTLIST UAPItem len CDATA "-">
  std::string m_strItemID; // <!ELEMENT UAPItem (#PCDATA)>

  long getLength(const unsigned char*) { Tracer::Error("Function should not be called!"); return 0; }
  void addBits(DataItemBits*) { Tracer::Error("Function should not be called!"); }
  bool getText(std::string&, std::string&, const unsigned int, unsigned char*, long)
  { Tracer::Error("Function should not be called!"); return false;} // appends description to strDescription
  bool setValue(std::string& strValue, std::string& itemName, int index, std::string& strBuff, long* pLength)
  {Tracer::Error("Function should not be called!"); return false;}
 	std::string printDescriptors(std::string) { Tracer::Error("Function should not be called!"); return ""; }; // print items format descriptors
  bool filterOutItem(const char*) { Tracer::Error("Function should not be called!"); return false; }; // mark item for filtering
  bool isFiltered(const char*) { Tracer::Error("Function should not be called!"); return false; }; // mark item for filtering

#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  fulliautomatix_definitions* getWiresharkDefinitions();
  fulliautomatix_data* getData(unsigned char* pData, long len, int byteoffset);
#endif
};

#endif /* UAPITEM_H_ */
