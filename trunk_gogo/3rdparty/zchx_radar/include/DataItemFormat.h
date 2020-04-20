///**************************************************************************
//* @File: DataItemFormat.h
//* @Description: DataItemFormat类 声明
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
//*                               增加函数setValue
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef DATAITEMFORMAT_H_
#define DATAITEMFORMAT_H_

#include <string>
#include <list>
#include "Utils.h"

class DataItemBits;

class DataItemFormat
{
public:
  DataItemFormat();
  virtual
  ~DataItemFormat();

  std::list<DataItemFormat*> m_lSubItems; //!< List of subitem formats in this item

  DataItemFormat* m_pParentFormat; //! Pointer to parent format (used only in XML parsing)

#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  static int m_nLastPID; //!< static used for allocation of m_nPID
#endif

  virtual long getLength(const unsigned char* pData) = 0;
  virtual void addBits(DataItemBits* pBits) = 0;
  virtual bool getText(
    std::string& strResult,
    std::string& strHeader,
    const unsigned int formatType,
    unsigned char* pData,
    long nLength) = 0; // appends value to strResult
  virtual bool setValue(
    std::string& strValue,
    std::string& itemName,
    int index,
    std::string& strBuff,
    long* pLength) = 0;
  virtual std::string printDescriptors(std::string header) = 0; // print items format descriptors
  virtual bool filterOutItem(const char* name) = 0; // mark item for filtering
  virtual bool isFiltered(const char* name) = 0; // is item filtered

  virtual bool isFixed() 		{ return false; }; // true if this is Fixed format
  virtual bool isRepetitive() 	{ return false; }; // true if this is Repetitive format
  virtual bool isVariable() 	{ return false; }; // true if this is Variable format
  virtual bool isExplicit() 	{ return false; }; // true if this is Explicit format
  virtual bool isCompound() 	{ return false; }; // true if this is Compound format
  virtual bool isBits() 		{ return false; }; // true if this is Bits description format


};

#endif /* DATAITEMFORMAT_H_ */
