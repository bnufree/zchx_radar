///**************************************************************************
//* @File: TSPIConfig.h
//* @Description:  TSPI配置接口
//* @Copyright: Copyright (c) 2017
//* @Company: 深圳置辰海信科技有限公司
//* @WebSite: http://www.szcenterstar.com/
//* @author 李鹭
//* @Revision History
//*
//* <pre>
//* ----------------------------------------------------------------------
//*   Ver     Date       Who             Comments
//*  ----- ----------  --------  ---------------------------------------
//*   1.0  2017/05/06    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/
#ifndef ZCHX_RADAR_MESSAGES_TSPICONFIG_H // -*- C++ -*-
#define ZCHX_RADAR_MESSAGES_TSPICONFIG_H

#include <inttypes.h>
#include <cmath>
#include <string>

#include <Messages/MessagesGlobal.h>

class QDomElement;
class QString;

namespace ZCHX {
namespace Messages {

/** Simple container class that holds attributes relating to a particular tspi in a configuration file.
 */
class TSPIConfig
{
public:


    /** Load in a new radar configuration from an XML DOM node

        \param config the DOM node containing the configuration

        \return true if successful
    */
    static bool Load(const QDomElement& config);


    static void Load(const std::string& name, const std::string& type, const std::string& domain,
                     const std::string& nativeHost, const std::string& host, uint16_t port, const std::string& transport);

    /** Obtain the current zeromq configuration name

        \return config name
    */
    static const std::string& GetName();
    static const std::string& GetType();
    static const std::string& GetDomain();
    static const std::string& GetFullName();
    static const std::string& GetNativeHost();
    static const std::string& GetHost();
    static uint16_t GetPort();
    static const std::string& GetTransport();





    static void SetPort(const uint16_t &port);

private:

    static std::string name_;
    static std::string type_;
    static std::string domain_;
    static std::string fullName_;
    static std::string nativeHost_;
    static std::string host_;
    static uint16_t port_;
    static std::string transport_;

};

} // end namespace Messages
} // end namespace ZCHX

/** \file
 */

#endif
