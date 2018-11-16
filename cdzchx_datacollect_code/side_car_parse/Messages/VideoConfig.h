///**************************************************************************
//* @File: VideoConfig.h
//* @Description:  Video配置接口
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
//*   1.0  2017/05/02    李鹭      初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/
#ifndef ZCHX_RADAR_MESSAGES_VIDEOCONFIG_H // -*- C++ -*-
#define ZCHX_RADAR_MESSAGES_VIDEOCONFIG_H

#include <inttypes.h>
#include <cmath>
#include <string>

#include <Messages/MessagesGlobal.h>

class QDomElement;
class QString;

namespace ZCHX {
namespace Messages {

/** Simple container class that holds attributes relating to a particular video in a configuration file.
 */
class VideoConfig
{
public:
    VideoConfig();

    /** Load in a new radar configuration from an XML DOM node
        \param config the DOM node containing the configuration
        \return true if successful
    */
    bool Load(const QDomElement& config);
    void Load(const std::string& name, const std::string& type, const std::string& domain,
              const std::string& nativeHost, const std::string& host, uint16_t port, const std::string& transport);

    /** Obtain the current zeromq configuration name
        \return config name
    */
    const std::string& GetName() const;
    const std::string& GetType() const;
    const std::string& GetDomain() const;
    const std::string& GetFullName() const;
    const std::string& GetNativeHost() const;
    const std::string& GetHost() const;
    uint16_t GetPort() const;
    const std::string& GetTransport() const;

    void SetPort(const uint16_t &port);
    void SetHost(const std::string& host) {host_ = host;}

private:
    std::string name_;
    std::string type_;
    std::string domain_;
    std::string fullName_;
    std::string nativeHost_;
    std::string host_;
    uint16_t port_;
    std::string transport_;
};

} // end namespace Messages
} // end namespace ZCHX

/** \file
 */

#endif
