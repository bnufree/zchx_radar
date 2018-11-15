///**************************************************************************
//* @File: radar_config.hpp
//* @Description: 雷达配置类
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
//*   1.0  2017/01/10    李鹭           初始化创建
//* ----------------------------------------------------------------------
//* </pre>
//**************************************************************************/


#ifndef ZCHX_RADAR_CONFIG_HPP
#define ZCHX_RADAR_CONFIG_HPP


#include <iostream>
#include <boost/optional.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include "safe_map.h"
#include "log_utils.h"

namespace zchx {
namespace radarproxy  {


struct control_element {
	std::string		valueUnit;		//单位
	int				value;			//值
	int				min;			//最小值
	int				max;			//最大值
};


struct radar_config
{

public:
	typedef  std::map<std::string, zchx::radarproxy::control_element>  control_element_type;
	explicit radar_config(boost::property_tree::ptree radar_conf);

	std::string           id;				//数据来源标识
	int					local_id;		//雷达id
	int                   SAC;			//系统区域代码
	int                   SIC;			//系统识别代码
	std::string           protocol;		//协议 (Asterix/Simrad)
	std::string           category;		//分类
	int                   azimuth_cell;	//方位角门
	int                   range_cell;		//距离门
	double				latitude;		//			雷达纬度
	double				longitude;		//			雷达经度
	double				startCog;		//			雷达初始方位角

	control_element_type m_control_element_map;

};


inline radar_config::radar_config(boost::property_tree::ptree radar_conf)
{
	//
	id = radar_conf.get<std::string>("id");
	SAC = radar_conf.get<int>("SAC");
	SIC= radar_conf.get<int>("SIC");
	local_id = radar_conf.get<int>("local_id");
	protocol= radar_conf.get<std::string>("protocol");
	category= radar_conf.get<std::string>("category");
	azimuth_cell= radar_conf.get<int>("azimuth_cell");
	range_cell= radar_conf.get<int>("range_cell");
	latitude= radar_conf.get<double>("latitude");
	longitude= radar_conf.get<double>("longitude");
	startCog= radar_conf.get<double>("startCog");
	BOOST_AUTO(elements, radar_conf.get_child_optional("elements"));
	if (elements)
	{
		std::cout << id<<" have element size: " << (*elements).size() << "\n";
		BOOST_FOREACH(BOOST_TYPEOF(*(*elements).begin()) element_iter, (*elements))
		{
			boost::property_tree::ptree element_conf = element_iter.second;
			std::string controlT;
			int min=0, max = 0;
			BOOST_FOREACH(boost::property_tree::ptree::value_type &v3, element_conf)
			{
				if (v3.first == "<xmlattr>" )
				{
					BOOST_FOREACH(boost::property_tree::ptree::value_type &vAttr, v3.second)
					{		
						if (vAttr.first == "type")
						{
							controlT = vAttr.second.data();
							break;
						}						
					}
					break;
				}
			}
			min = element_conf.get<int>("minValue");
			max = element_conf.get<int>("maxValue");
			std::string Unit = element_conf.get<std::string>("BitsUnit");
			int value = 0;
			control_element temp_element; 
			temp_element.valueUnit = Unit;
			temp_element.value = 0;
			temp_element.min = min;
			temp_element.max = max;
			
			m_control_element_map.insert(std::pair<std::string, control_element>(controlT, temp_element));
		}
		//std::cout << "have elements\n";
	}
	else {
		//std::cout << "no elements\n";
	}
}

//struct radar_config
//{
//public:
//	explicit radar_config(std::string& id
//	    ,int SAC
//	    ,int SIC
//	    ,std::string& protocol
//	    ,std::string& category
//	    ,int azimuth_cell
//	    ,int range_cell
//	    ,double latitude
//	    ,double longitude
//	 ,double startCog
//	    );
//
//
//	std::string				id;				//数据来源标识
//	int						local_id;		//雷达id
//	int						SAC;			//系统区域代码
//	int						SIC;			//系统识别代码
//	std::string				protocol;		//协议 (Asterix/Simrad)
//	std::string				category;		//分类
//	int						azimuth_cell;	//方位角门
//	int						range_cell;		//距离门
//	double					latitude;		//			雷达纬度
//	double					longitude;		//			雷达经度
//
//}; // struct radar_config

//inline radar_config::radar_config(std::string& the_id
//      , int the_SAC
//      , int the_SIC
//      , std::string& the_protocol
//      , std::string& the_category
//      , int the_azimuth_cell
//      , int the_range_cell
//      ,double the_latitude
//      ,double the_longitude
//      ,double the_startCog
//    )
//  : id(the_id)
//  , SAC(the_SAC)
//  , SIC(the_SIC)
//  , protocol(the_protocol)
//  , category(the_category)
//  , azimuth_cell(the_azimuth_cell)
//  , range_cell(the_range_cell)
//  , latitude(the_latitude)
//  , longitude(the_longitude)
//  , startCog(the_startCog)
//{
// 
//  BOOST_ASSERT_MSG(!the_id.empty(), "id must not be empty");
//  BOOST_ASSERT_MSG(!the_protocol.empty(), "protocol must not be empty");
//  BOOST_ASSERT_MSG(!the_category.empty(), "category must not be empty");
//
//  BOOST_ASSERT_MSG(the_azimuth_cell > 0, "azimuth_cell must be > 0");
//  BOOST_ASSERT_MSG(the_range_cell > 0, "range_cell must be > 0");
//  BOOST_ASSERT_MSG(the_latitude > 0, "latitude must be > 0");
//  BOOST_ASSERT_MSG(the_longitude > 0, "longitude must be > 0");
//
//}


} // namespace radarproxy
} // namespace zchx

#endif // ZCHX_RADAR_CONFIG_HPP
