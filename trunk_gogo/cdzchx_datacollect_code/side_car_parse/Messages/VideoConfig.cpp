///**************************************************************************
//* @File: VideoConfig.cpp
//* @Description:  Video配置类
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
#include <QLoggingCategory>
#include <QtCore/QString>
#include <QtXml/QDomElement>

#include "FilePath.h"
#include "zchxRadarUtils.h"

#include "VideoConfig.h"

using namespace ZCHX::Messages;

//std::string VideoConfig::name_ = "Default";
//std::string VideoConfig::type_ = "Video";
//std::string VideoConfig::domain_ = "local";
//std::string VideoConfig::fullName_ = "Default.local";
//std::string VideoConfig::nativeHost_ = "heron-VirtualBox";
//std::string VideoConfig::host_ = "192.168.60.22";
//uint16_t VideoConfig::port_ = 4648;
//std::string VideoConfig::transport_ = "_zeromq";

const std::string &VideoConfig::GetName() const
{
    return name_;
}

const std::string &VideoConfig::GetType() const
{
    return type_;
}

const std::string &VideoConfig::GetDomain() const
{
    return domain_;
}

const std::string &VideoConfig::GetFullName() const
{
    return fullName_;
}

const std::string &VideoConfig::GetNativeHost() const
{
    return nativeHost_;
}

const std::string &VideoConfig::GetHost() const
{
    return host_;
}

uint16_t VideoConfig::GetPort() const
{
    return port_;
}

const std::string & VideoConfig::GetTransport() const
{
    return transport_;
}

void VideoConfig::SetPort(const uint16_t &port)
{
    port_ = port;
}


void VideoConfig::Load( const std::string &name, const std::string &type, const std::string &domain,
                   const std::string &nativeHost, const std::string &host, uint16_t port, const std::string &transport )
{
    name_ = name;
    type_ = type;
    domain_ = domain;
    fullName_ = name_ + "." + domain_;
    nativeHost_ = nativeHost;
    host_ = host;
    port_ = port;
    transport_ = transport;
}

VideoConfig::VideoConfig()
{
}

bool VideoConfig::Load( const QDomElement &config )
{
    name_ = config.attribute( "name" ).toStdString();
    type_ = config.attribute( "type" ).toStdString();
    domain_ = config.attribute( "domain" ).toStdString();
    fullName_ = name_ + "." + domain_;
    nativeHost_ = config.attribute( "nativeHost" ).toStdString();
    host_ = config.attribute( "host" ).toStdString();
    port_ = config.attribute( "port" ).toUInt();
    transport_ = config.attribute( "transport" ).toStdString();
    return true;
}
