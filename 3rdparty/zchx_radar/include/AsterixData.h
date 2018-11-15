//**************************************************************************
//* @File: asterixdata.h
//* @Description: asterixdata类 申明
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
//*   1.0  2014/9/20   李惟谦		  初始化创建,基于Damir Salantic修改
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef ASTERIXDATA_H_
#define ASTERIXDATA_H_

#include <map>

#include "AsterixDefinition.h"
#include "DataBlock.h"

class AsterixData
{
public:
  AsterixData();
  virtual
  ~AsterixData();

  std::list<DataBlock*> m_lDataBlocks;

#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  fulliautomatix_data* getData();
#endif

    bool getBinary(std::string& strResult);
    bool getText(std::string& strResult,
                 const unsigned int formatType); // appends value to strResult in formatType format
};

#endif /* ASTERIXDATA_H_ */
