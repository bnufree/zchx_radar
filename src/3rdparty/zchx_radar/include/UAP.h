//**************************************************************************
//* @File: UAP.h
//* @Description: UAP类 声明
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

#ifndef UAP_H_
#define UAP_H_

#include "UAPItem.h"

class UAP
{
public:
  UAP();
  virtual
  ~UAP();

  unsigned long m_nUseIfBitSet;
  unsigned long m_nUseIfByteNr;
  unsigned char m_nIsSetTo;
  std::list<UAPItem*> m_lUAPItems;

  UAPItem* newUAPItem(); //!< creates and returns new UAP Item
#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
  fulliautomatix_definitions* getWiresharkDefinitions();
#endif
  std::string getDataItemIDByUAPfrn(int uapfrn);
};

#endif /* UAP_H_ */
