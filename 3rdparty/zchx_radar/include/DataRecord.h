///**************************************************************************
//* @File: datarecord.h
//* @Description: datarecord类 声明
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

#ifndef DATARECORD_H_
#define DATARECORD_H_
#include <bitset>
#include "DataItem.h"
//#include "log_utils.h"

class DataRecord
{
public:
  DataRecord(Category* cat, int id, unsigned long len, const unsigned char* data, unsigned long nTimestamp);
  DataRecord(Category* cat, int id, unsigned long nTimestamp);
  virtual
  ~DataRecord();

  Category* m_pCategory;
  int           m_nID;
  unsigned long m_nLength;
  unsigned long m_nFSPECLength;
  unsigned char* m_pFSPECData;
  unsigned long m_nTimestamp; // Date and time when this packet was captured. This value is in seconds since January 1, 1970 00:00:00 GMT

  bool m_bFormatOK;
  std::list<DataItem*> m_lDataItems;

  int getCategory() { return (m_pCategory) ? m_pCategory->m_id : 0; }

  bool getText(std::string& strResult, std::string& strHeader, const unsigned int formatType); // appends value to strResult in formatType format
  bool getBinary(std::string& strResult);
  DataItem* getItem(std::string itemid);
  bool setValue(std::string& strValue, std::string itemId, std::string itemName, int index = -1);
#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  fulliautomatix_data* getData(int byteoffset);
#endif
};

#endif /* DATARECORD_H_ */
