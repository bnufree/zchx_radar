///**************************************************************************
//* @File: DataItemDescription.h
//* @Description: DataItem描述类 声明
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

#ifndef DATAITEMDESCRIPTION_H_
#define DATAITEMDESCRIPTION_H_

#include "DataItemFormat.h"

class DataItemDescription
{
public:
  DataItemDescription(std::string id);
  virtual
  ~DataItemDescription();

  std::string m_strID;

  void setName(char* name) { m_strName = name; }
  void setDefinition(char* definition) { m_strDefinition = definition; }
  void setFormat(char* format) { m_strFormat = format; }
  bool setValue(
    std::string& strValue,
    std::string& itemName,
    int index,
    std::string& strBuff,
    long* pLength);
  bool getText(
    std::string& strResult,
    std::string& strHeader,
    const unsigned int formatType,
    unsigned char* pData,
    long nLength) // appends value to strResult
  {
    return m_pFormat->getText(strResult, strHeader, formatType, pData, nLength);
  };


  std::string m_strName;
  std::string m_strDefinition;
  std::string m_strFormat;
  std::string m_strNote;

  DataItemFormat* m_pFormat;

  typedef enum
  {
    DATAITEM_UNKNOWN = 0,
    DATAITEM_OPTIONAL,
    DATAITEM_MANDATORY,
    DATAITEM_COMPRESS
  } _eRule;

  _eRule m_eRule;

};

#endif /* DATAITEMDESCRIPTION_H_ */
