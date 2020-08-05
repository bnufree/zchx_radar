//**************************************************************************
//* @File: DataItemBits.h
//* @Description: DataItemBits类 声明
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
//*   1.0  2014/10/15    李惟谦		  初始化创建,基于Damir Salantic修改
//*                                 增加函数setBits str_cast_bin
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef DATAITEMBITS_H_
#define DATAITEMBITS_H_

#include <string>
#include <list>
//#include <boost/lexical_cast.hpp>

class BitsValue
{
public:
  BitsValue(int val) { m_nVal = val; }

  int m_nVal;
  std::string m_strDescription;
};

class DataItemBits : public DataItemFormat
{
public:
  DataItemBits();
  virtual
  ~DataItemBits();

  std::string m_strShortName;
  std::string m_strName;
  int m_nFrom;
  int m_nTo;

  typedef enum
  {
    DATAITEM_ENCODING_UNSIGNED = 0,
    DATAITEM_ENCODING_SIGNED,
    DATAITEM_ENCODING_SIX_BIT_CHAR,
    DATAITEM_ENCODING_HEX_BIT_CHAR,
    DATAITEM_ENCODING_OCTAL,
    DATAITEM_ENCODING_ASCII
  } _eEncoding;

  _eEncoding m_eEncoding;

  bool m_bIsConst; //!< if true this is fixed value (from m_nConst)
  int m_nConst;

  std::string m_strUnit;
  double m_dScale;
  bool m_bMaxValueSet;
  double m_dMaxValue;
  bool m_bMinValueSet;
  double m_dMinValue;

  bool m_bExtension; //!< item extension (used only in Variable length items)
  int m_nPresenceOfField; //!< presence of compound data item (used only in Compound items)
  std::list<BitsValue*> m_lValue;

  bool m_bFiltered; //! item shall be printed when filter is applied
 
  bool m_Filtering;

  bool getText(
    std::string& strResult,
    std::string& strHeader,
    const unsigned int formatType,
    unsigned char* pData,
    long nLength); // appends value to strResult
  bool setValue(
    std::string& strValue,
    std::string& itemName,
    int index,
    std::string& strBuff,
    long* pLength);
  bool setBits(std::string& strVal, unsigned char* pData, int bytes);
  std::string printDescriptors(std::string header); // print items format descriptors
  bool filterOutItem(const char* name); // mark item for filtering
  bool isFiltered(const char*) {return m_bFiltered; } // true if item is in filter (shall be printed)
  bool isBits() 		{ return true; }; // true if this is Bits description format

#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  fulliautomatix_definitions* getWiresharkDefinitions();
  fulliautomatix_data* getData(unsigned char* pData, long len, int byteoffset);
#endif

  long getLength(const unsigned char* pData);
  void addBits(DataItemBits* pBits);

private:
  void str_cast_bin(std::string& str_val);
  unsigned char* getBits(unsigned char* pData, int bytes, int frombit, int tobit);
  unsigned long getUnsigned(unsigned char* pData, int bytes, int frombit, int tobit);
  signed long getSigned(unsigned char* pData, int bytes, int frombit, int tobit);
  unsigned char* getSixBitString(unsigned char* pData, int bytes, int frombit, int tobit);
  unsigned char* getHexBitString(unsigned char* pData, int bytes, int frombit, int tobit);
  unsigned char* getOctal(unsigned char* pData, int bytes, int frombit, int tobit);
};

#endif /* DATAITEMBITS_H_ */
