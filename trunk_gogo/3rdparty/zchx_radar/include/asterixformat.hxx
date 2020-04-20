//**************************************************************************
//* @File: asterixformat.h
//* @Description: asterixformat类 声明
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
//*   1.0  2015/11/26   李鹭     CreateFormatDescriptor函数 增加配置文件参数传入
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/

#ifndef ASTERIXFORMAT_HXX__
#define ASTERIXFORMAT_HXX__

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "baseformat.hxx"



#define NOTARRAY -1

// 与MSVC兼容
#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif


class CBaseDevice;
class CBaseFormatDescriptor;

/**
 * @class CAsterixFormat
 *
 * @brief The Asterix formatter class.
 */
class ZCHX_API CAsterixFormat : public CBaseFormat
{
public:

  /**
   * Public enumerator for supported formats identification. Should be used by
   * public methods which require format type as a parameter.
   */
  enum {
  ERaw=0,         // Raw Asterix format
  EPcap,          // PCAP file format
  ETxt,           // textual output (human readable)
  EFinal,         // Final format
  EXML,           // XML
  EJSON,          // JSON (JavaScript Object Notation) format, compact form
  EJSONH,         // JSON (JavaScript Object Notation) format, human readable form
  EHDLC,          // HDLC format
  EOradisRaw,     // Raw Asterix format with ORADIS header
  EOradisPcap,    // PCAP file format with ORADIS header
  EOut,           // textual output (one line text, easy for parsing)
  ETotalFormats
  };

 private:

    /**
     * Basic structure (packet) for all format conversions
     */
//    CAsterixPacket _packet;

public:

    /**
     * Default class constructor
     */
    CAsterixFormat(const char* asterixDefinitionsFile)
      : CBaseFormat()
      , m_AsterixDefinitionsFile(asterixDefinitionsFile)
      , m_pFormatDescriptor(NULL) { }

    /**
     * Default class destructor.
     */
    virtual ~CAsterixFormat() {}

    virtual bool ReadPacket(CBaseFormatDescriptor& formatDescriptor, CBaseDevice &device,
        const unsigned int formatType, bool &discard);

    virtual bool WritePacket(CBaseFormatDescriptor& formatDescriptor, CBaseDevice &device,
        const unsigned int formatType, bool &discard);

    virtual bool ProcessPacket(CBaseFormatDescriptor& formatDescriptor, CBaseDevice &device,
        const unsigned int formatType, bool &discard);

    virtual bool HeartbeatProcessing(CBaseFormatDescriptor& formatDescriptor, CBaseDevice &device,
        const unsigned int formatType);

    virtual CBaseFormatDescriptor* CreateFormatDescriptor
        (const unsigned int formatType, const char* sFormatDescriptor);

    virtual bool GetFormatNo(const char *formatName, unsigned int &formatNo);

    virtual int GetStatus(CBaseDevice &device,
        const unsigned int formatType, int query = 0);

    virtual bool OnResetInputChannel(CBaseFormatDescriptor& formatDescriptor);

    virtual bool OnResetOutputChannel(unsigned int channel, CBaseFormatDescriptor& formatDescriptor);


private:  

  // Supported formats name string list
  static const char* _FormatName[ETotalFormats];

  CBaseFormatDescriptor* m_pFormatDescriptor;

  const char*        m_AsterixDefinitionsFile;

};

#endif
