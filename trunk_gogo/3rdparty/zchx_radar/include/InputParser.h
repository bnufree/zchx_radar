//**************************************************************************
//* @File: inputparser.h
//* @Description: inputparser类 声明
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

#ifndef INPUTPARSER_H_
#define INPUTPARSER_H_

#include "AsterixDefinition.h"
#include "AsterixData.h"

#include "osmacros.h"

class ZCHX_API InputParser
{
public:
    InputParser(AsterixDefinition* pDefinition);
    AsterixData* parsePacket(const unsigned char* m_pBuffer, unsigned int m_nBufferSize, unsigned long nTimestamp = 0);
    AsterixData* newDataBlock(unsigned char nCategory, unsigned char recordsCount);
    std::string printDefinition();
    bool filterOutItem(int cat, std::string item, const char* name);
    bool isFiltered(int cat, std::string item, const char* name);
private:
    AsterixDefinition* m_pDefinition; // Asterix definitions

};

#endif /* INPUTPARSER_H_ */
