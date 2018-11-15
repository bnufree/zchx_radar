#ifndef RADAR_CONTROL_HPP
#define RADAR_CONTROL_HPP

#include <sstream>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/lexical_cast.hpp>

//
#include <asio/service.hpp>
#include <zmq/device.hpp>


#include "log_utils.h"
#include "safe_map.h"

#include "radar_config.hpp"

namespace zchx {
namespace radarproxy  {


	//Hardware
enum CONTROL_TYPE
{
	CT_POWER = 1,								//  "Power"								电源			
	CT_SCAN_SPEED = 2,							//  "Scan speed"					扫描速度
	CT_ANTENNA_HEIGHT = 3,						//  "Antenna height"				天线高度
	CT_BEARING_ALIGNMENT = 4,					//  "Bearing alignment"				方位校准
	CT_RANG = 5,								//  "Rang"							半径
	CT_GAIN = 6,								//  "Gain"							增益		
	CT_SEA = 7,									//  "Sea clutter"					海杂波
	CT_RAIN = 8,								//  "Rain clutter"					雨滴杂波
	CT_NOISE_REJECTION = 9,						//  "Noise rejection"				噪声抑制
	CT_SIDE_LOBE_SUPPRESSION = 10,				//  "Side lobe suppression"			旁瓣抑制
	CT_INTERFERENCE_REJECTION = 11,				//  "Interference rejection"		抗干扰
	CT_LOCAL_INTERFERENCE_REJECTION = 12,		//  "Local interference rejection"  本地干扰抑制
	CT_TARGET_EXPANSION = 13,					//	"Target expansion"				目标扩展	
	CT_TARGET_BOOST = 14,						//  "Target boost"					目标提升	
	CT_TARGET_SEPARATION = 15					//  "Target separation"				目标分割
};



class radar_control
{
public:
	radar_control() {}
	virtual void Init(const radar_config & config) = 0;
	virtual void RadarOFF() = 0;
	virtual void RadarON() = 0;
	virtual void RadarStayAlive() = 0;
	virtual void UpdateValue(CONTROL_TYPE controlType, int value) = 0;
	virtual void SetControlValue(CONTROL_TYPE controlType, int value) = 0;
	virtual bool ProcessReport(const unsigned char*buf, size_t len) = 0;
//private:
	std::string		radarSourceId;
	int				radarLocalId;
	boost::shared_ptr<zchx::asio::service>		m_asio_server;
	boost::shared_ptr<zchx::dds::zmq::device>      m_ptr_zmq_server;
};



}// namespace radarproxy
}// namespace zchx
#endif // 