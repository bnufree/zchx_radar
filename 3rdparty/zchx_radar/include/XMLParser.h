//**************************************************************************
//* @File: xmlparser.h
//* @Description: 解析xml描述文件
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

#ifndef XMLPARSER_H_
#define XMLPARSER_H_

#include "expat_external.h"
#include "expat.h"


#include "AsterixDefinition.h"
#include "DataItemFormatFixed.h"
#include "DataItemFormatVariable.h"
#include "DataItemFormatCompound.h"
#include "DataItemFormatExplicit.h"
#include "DataItemFormatRepetitive.h"
#include "UAP.h"

//! parsing buffer size
#define BUFFSIZE        8192

class XMLParser
{
public:
  XMLParser();
  virtual
  ~XMLParser();
  bool Parse(FILE* pFile, AsterixDefinition* pDefinition, const char* filename);

  bool m_bErrorDetectedStopParsing; //!< Flag to stop parsing if error detected

  AsterixDefinition* m_pDef;
  Category* m_pCategory; //<! Currently parsed <Category>
  DataItemDescription* m_pDataItem; //!< Currently parsed <DataItemDescription>
  DataItemFormat* m_pFormat; //!< Currently parsed <Format>
  BitsValue* m_pBitsValue; //!< Currently parsed <BitsValue>
  UAPItem* m_pUAPItem; //!< Currently parsed UAPItem
  UAP* m_pUAP; //!< Currently parsed UAP

  // pointer to string to which to assign next CDATA
  std::string *m_pstrCData;

  // pointer to int to which to assign next CDATA
  int* m_pintCData;

  // current parsed file name
  const char* m_pFileName;

  void GetCData(std::string *pstr) { m_pstrCData = pstr; }
  void GetCData(int *pint) { m_pintCData = pint; m_pstrCData = NULL; }

  static void XMLCALL
  ElementHandlerStart(void *data, const char *el, const char **attr);

  static void XMLCALL
  ElementHandlerEnd(void *data, const char *el);

  static void XMLCALL
  CharacterHandler(void *userData, const XML_Char *s, int len);

  void Error(const char* errstr); //!< print error message with line number
  void Error(const char* errstr, const char* param1);

private:
  XML_Parser m_Parser;

  char m_pBuff[BUFFSIZE];

  bool GetAttribute(const char* elementName, const char* attrName, std::string* ptrString);
  bool GetAttribute(const char* elementName, const char* attrName, int* ptrInt);
};

#endif /* XMLPARSER_H_ */
