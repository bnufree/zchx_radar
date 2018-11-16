///**************************************************************************
//* @File: TSPIConfig.cpp
//* @Description:  TSPI配置类
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
#include <QLoggingCategory>
#include <QtCore/QString>
#include <QtXml/QDomElement>

#include "FilePath.h"
#include "zchxRadarUtils.h"

#include "TSPIConfig.h"

using namespace ZCHX::Messages;

//std::string TSPIConfig::name_ = "Default";
//std::string TSPIConfig::type_ = "TSPI";
//std::string TSPIConfig::domain_ = "local";
//std::string TSPIConfig::fullName_ = "Default.local";
//std::string TSPIConfig::nativeHost_ = "heron-VirtualBox";
//std::string TSPIConfig::host_ = "192.168.60.22";
//uint16_t TSPIConfig::port_ = 4646;
//std::string TSPIConfig::transport_ = "_zeromq";

const std::string&
TSPIConfig::GetName() { return name_; }

const std::string&
TSPIConfig::GetType() { return type_; }

const std::string&
TSPIConfig::GetDomain() { return domain_; }

const std::string&
TSPIConfig::GetFullName() { return fullName_; }

const std::string&
TSPIConfig::GetNativeHost() { return nativeHost_; }

const std::string&
TSPIConfig::GetHost() { return host_; }

uint16_t
TSPIConfig::GetPort() { return port_; }

const std::string&
TSPIConfig::GetTransport() { return transport_; }

void TSPIConfig::SetPort(const uint16_t &port)
{
    port_ = port;
}


void
TSPIConfig::Load(const std::string& name, const std::string& type, const std::string& domain,
                     const std::string& nativeHost, const std::string& host, uint16_t port, const std::string& transport)
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



bool
TSPIConfig::Load(const QDomElement& config)
{
    qCDebug(radarmsg) << "TSPIConfig::Load" ;

    name_ = config.attribute("name").toStdString();
    type_ = config.attribute("type").toStdString();
    domain_ = config.attribute("domain").toStdString();
    fullName_ = name_ + "." + domain_;
    nativeHost_ = config.attribute("nativeHost").toStdString();
    host_ = config.attribute("host").toStdString();
    port_ = config.attribute("port").toUInt();
    transport_ = config.attribute("transport").toStdString();

    return true;
}
