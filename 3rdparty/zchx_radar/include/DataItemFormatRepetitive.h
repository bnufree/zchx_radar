///*************************************************************************
//* @File: DataItemRepetitive.h
//* @Description: DataItemRepetitive 声明
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
//*   1.0  2014/10/17   李惟谦		  初始化创建,基于Damir Salantic修改
//*                               增加函数setValue
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef DATAITEMFORMATREPETITIVE_H_
#define DATAITEMFORMATREPETITIVE_H_
#include "DataItemFormatFixed.h"

class DataItemFormatRepetitive : public DataItemFormat
{
public:
  DataItemFormatRepetitive();
  virtual
  ~DataItemFormatRepetitive();

  long getLength(const unsigned char* pData);
  void addBits(DataItemBits* pBits);
  bool getText(std::string& strResult, std::string& strHeader, unsigned int formatType, unsigned char* pData, long nLength); // appends value description to strResult
  bool setValue(std::string& strValue, std::string& itemName, int index, std::string& strBuff, long* pLength);
  std::string printDescriptors(std::string header); // print items format descriptors
  bool filterOutItem(const char* name); // mark item for filtering
  bool isFiltered(const char* name);
  bool isRepetitive() 	{ return true; }; // true if this is Repetitive format

#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  fulliautomatix_definitions* getWiresharkDefinitions();
  fulliautomatix_data* getData(unsigned char* pData, long len, int byteoffset);
#endif
};

#endif /* DATAITEMFORMATREPETITIVE_H_ */
