///**************************************************************************
//* @File: datablock.h
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
//*                               getItemValue, setItemValue
//*                               getItemBinary, setItemBinary
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef DATABLOCK_H_
#define DATABLOCK_H_

//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>

#include "Category.h"
#include "DataRecord.h"

#include "osmacros.h"

class ZCHX_API DataBlock
{
public:
  DataBlock(
    Category* cat,
    unsigned long len,
    const unsigned char* data,
    unsigned long nTimestamp = 0);
  DataBlock(Category* cat, unsigned char recordsCount, long nTimeStamp = 0);
  virtual
  ~DataBlock();

  Category* m_pCategory;
  unsigned long m_nLength;
  unsigned long m_nTimestamp; // Date and time when this packet was captured. This value is in seconds since January 1, 1970 00:00:00 GMT
  bool m_bFormatOK;

  bool m_Filtering;

  std::list<DataRecord*> m_lDataRecords;
  bool setItemValue(
    unsigned int recordIndex,
    std::string strValue,
    std::string itemId,
    std::string itemName,
    int index = -1);

  bool getItemValue(
    unsigned int recordIndex,
    std::string& strValue,
    std::string itemId,
    std::string itemName,
    int index = -1);

  bool getItemBinary(
    unsigned int recordIndex,
    std::string& strValue,
    std::string itemId);

  bool setItemBinary(
    unsigned int recordIndex,
    std::string& strValue,
    std::string itemId);

  bool getText(std::string& strResult, const unsigned int formatType); // appends value to strResult in formatType format
  bool getBinary(std::string& strResult); // add by flab
};

#endif /* DATABLOCK_H_ */
