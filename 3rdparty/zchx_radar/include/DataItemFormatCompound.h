///**************************************************************************
//* @File: DataItemFormatCompound.h
//* @Description: DataItemFormatCompound类 声明
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

#ifndef DATAITEMFORMATCOMPOUND_H_
#define DATAITEMFORMATCOMPOUND_H_

#include "DataItemFormat.h"
#include "DataItemFormatVariable.h"

class DataItemFormatCompound : public DataItemFormat
{
public:
  DataItemFormatCompound();
  virtual
  ~DataItemFormatCompound();

//  DataItemFormatVariable* m_pCompoundPrimary;

  long getLength(const unsigned char* pData);
  void addBits(DataItemBits* pBits);
  bool getText(
    std::string& strResult,
    std::string& strHeader,
    const unsigned int formatType,
    unsigned char* pData,
    long nLength); // appends value description to strResult
  bool setValue(
    std::string& strValue,
    std::string& itemName,
    int index,
    std::string& strBuff,
    long* pLength);
  std::string printDescriptors(std::string header); // print items format descriptors
  bool filterOutItem(const char* name); // mark item for filtering
  bool isFiltered(const char* name);
  bool isCompound() 	{ return true; }; // true if this is Compound format

#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  fulliautomatix_definitions* getWiresharkDefinitions();
  fulliautomatix_data* getData(unsigned char* pData, long len, int byteoffset);
#endif

};

#endif /* DATAITEMFORMATCOMPOUND_H_ */
