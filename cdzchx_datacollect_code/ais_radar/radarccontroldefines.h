#ifndef RADARCCONTROLDEFINES_H
#define RADARCCONTROLDEFINES_H

#include <QObject>
#include <QString>
#include <basetsd.h>

//------------------------------------------------------------------------------------------------
// Info Type 消息类型
//------------------------------------------------------------------------------------------------
//-|num |-------------Status Discription--------------------|------------中文-------------------
// | 1  | Power                                             |  雷达电源控制
// | 2  | Scan speed                                        |  扫描速度
// | 3  | Antenna height                                    |  天线高度
// | 4  | Bearing alignment                                 |  方位校准
// | 5  | Rang                                              |  半径
// | 6  | Gain                                              |  增益
// | 7  | Sea clutter                                       |  海杂波
// | 8  | Rain clutter                                      |  雨杂波
// | 9  | Noise rejection                                   |  噪声抑制
// | 10 | Side lobe suppression                             |  旁瓣抑制
// | 11 | Interference rejection                            |  抗干扰
// | 12 | Local interference rejection                      |  本地抗干扰
// | 13 | Target expansion                                  |  目标扩展
// | 14 | Target boost                                      |  目标推进
// | 15 | Target separation                                 |  目标分离
//------------------------------------------------------------------------------------------------

enum INFOTYPE {
    UNKNOWN 									= 0,
    POWER                               		= 1,
    SCAN_SPEED                           		= 2,
    ANTENNA_HEIGHT								= 3,
    BEARING_ALIGNMENT							= 4,
    RANG                          				= 5,
    GAIN                           				= 6,
    SEA_CLUTTER                   				= 7,
    RAIN_CLUTTER                     			= 8,
    NOISE_REJECTION                           	= 9,
    SIDE_LOBE_SUPPRESSION                      	= 10,
    INTERFERENCE_REJECTION	             		= 11,
    LOCAL_INTERFERENCE_REJECTION             	= 12,
    TARGET_EXPANSION  							= 13,
    TARGET_BOOST                   				= 14,
    TARGET_SEPARATION                     		= 15,
    RESVERED,
};

//enum CONTROL_TYPE
//{
//    CT_POWER = 1,								//  "Power"								电源
//    CT_SCAN_SPEED = 2,							//  "Scan speed"					扫描速度
//    CT_ANTENNA_HEIGHT = 3,						//  "Antenna height"				天线高度
//    CT_BEARING_ALIGNMENT = 4,					//  "Bearing alignment"				方位校准
//    CT_RANG = 5,								//  "Rang"							半径
//    CT_GAIN = 6,								//  "Gain"							增益
//    CT_SEA = 7,									//  "Sea clutter"					海杂波
//    CT_RAIN = 8,								//  "Rain clutter"					雨滴杂波
//    CT_NOISE_REJECTION = 9,						//  "Noise rejection"				噪声抑制
//    CT_SIDE_LOBE_SUPPRESSION = 10,				//  "Side lobe suppression"			旁瓣抑制
//    CT_INTERFERENCE_REJECTION = 11,				//  "Interference rejection"		抗干扰
//    CT_LOCAL_INTERFERENCE_REJECTION = 12,		//  "Local interference rejection"  本地干扰抑制
//    CT_TARGET_EXPANSION = 13,					//	"Target expansion"				目标扩展
//    CT_TARGET_BOOST = 14,						//  "Target boost"					目标提升
//    CT_TARGET_SEPARATION = 15					//  "Target separation"				目标分割
//};

//------------------------------------------------------------------------------------------------
// Value unit 数据类型
//------------------------------------------------------------------------------------------------
//-|num|-------------Status Discription---------------------|------------中文-------------------
// | 1  | Unknown                             				| 无单位
// | 2  | Bool                             				  	| false=关,true=开
// | 3  | Degree                             		  	  	| 度
// | 4  | Meter                             		  	  	| 米
//------------------------------------------------------------------------------------------------

enum VALUEUNIT {
    UNIT_UNKNOWN										= 1,
    UNIT_BOOL                               				= 2,
    UNIT_DEGREE                           				= 3,
    UNIT_METER											= 4,
};

#define         STR_MODE_CHI            0
#define         STR_MODE_ENG            1

struct RadarStatus
{
    INFOTYPE            id;					// 消息类型
    VALUEUNIT			unit;               // 值类型
    int 	  			value;				// 当前值
    int                 min;				// value可设置的最小值
    int                 max;				// value可设置的最大值

    RadarStatus(INFOTYPE ptype = INFOTYPE::UNKNOWN, int pmin = INT_MIN, int pmax = INT_MAX)
    {
        id = ptype;
        unit = VALUEUNIT::UNIT_UNKNOWN;
        value = -1;
        min = pmin;
        max = pmax;
    }

    static QString getTypeString(INFOTYPE type, int strMode = STR_MODE_CHI)
    {
        QString res = "";
        switch (type) {
        case UNKNOWN:
            res = (strMode == STR_MODE_CHI? QObject::tr("未知命令") : "Unknown_Command");
            break;
        case POWER:
            res = (strMode == STR_MODE_CHI? QObject::tr("电源"):"Power");
            break;
        case SCAN_SPEED:
            res = (strMode == STR_MODE_CHI? QObject::tr("扫描速度"):"Scan_speed");
            break;
        case ANTENNA_HEIGHT:
            res = (strMode == STR_MODE_CHI? QObject::tr("天线高度"):"Antenna_height");
            break;
        case BEARING_ALIGNMENT:
            res = (strMode == STR_MODE_CHI? QObject::tr("方位校准"):"Bearing_alignment");
            break;
        case RANG:
            res = (strMode == STR_MODE_CHI? QObject::tr("半径"):"Rang");
            break;
        case GAIN:
            res = (strMode == STR_MODE_CHI? QObject::tr("增益"):"Gain");
            break;
        case SEA_CLUTTER:
           res = (strMode == STR_MODE_CHI? QObject::tr("海杂波"):"Sea_clutter");
            break;
        case RAIN_CLUTTER:
           res = (strMode == STR_MODE_CHI? QObject::tr("雨滴杂波"):"Rain_clutter");
            break;
        case NOISE_REJECTION:
           res = (strMode == STR_MODE_CHI? QObject::tr("噪声抑制"):"Noise_rejection");
            break;
        case SIDE_LOBE_SUPPRESSION:
           res = (strMode == STR_MODE_CHI? QObject::tr("旁瓣抑制"):"Side_lobe_suppression");
            break;
        case INTERFERENCE_REJECTION:
           res = (strMode == STR_MODE_CHI? QObject::tr("抗干扰"):"Interference_rejection");
            break;
        case LOCAL_INTERFERENCE_REJECTION:
           res = (strMode == STR_MODE_CHI? QObject::tr("本地干扰抑制"):"Local_interference_rejection");
            break;
        case TARGET_EXPANSION:
           res = (strMode == STR_MODE_CHI? QObject::tr("目标扩展"):"Target_expansion");
            break;
        case TARGET_BOOST:
           res = (strMode == STR_MODE_CHI? QObject::tr("目标提升"):"Target_boost");
            break;
        case TARGET_SEPARATION:
           res = (strMode == STR_MODE_CHI? QObject::tr("目标分割") :"Target_separation");
            break;
        default:
            break;
        }

        return res;
    }
};

struct RadarControl
{
    int 				id;             	// 雷达id
    int 				timeOfDay;			// 当日时间
    INFOTYPE 			type;				// 消息类型
    int	  			    value;				// 设置值
};
struct RadarReport
{
    int 				id;             	// 雷达id
    int 				timeOfDay;			// 当日时间
    int				    type;				// 0=请求雷达状态信息,1=报告雷达状态信息
    QList<RadarStatus>  reportList;				// 雷达报告信息
};

Q_DECLARE_METATYPE(RadarStatus)


#pragma pack(push,1)
struct RadarReport_01C4_18 {  // 01 C4 with length 18
    UINT8	what;                 // 0   0x01
    UINT8	command;              // 1   0xC4
    UINT8	radar_status;         // 2
    UINT8	field3;               // 3
    UINT8	field4;               // 4
    UINT8	field5;               // 5
    UINT16	field6;              // 6-7
    UINT16	field8;              // 8-9
    UINT16	field10;             // 10-11
};

struct RadarReport_02C4_99 {     // length 99
    UINT8	what;                    // 0   0x02
    UINT8	command;                 // 1 0xC4
    UINT32	range;                  //  2-3   0x06 0x09
    UINT16	field4;                 // 6-7    0
    UINT32	field8;                 // 8-11   1
    UINT8	gain;                    // 12
    UINT8	field13;                 // 13  ==1 for sea auto
    UINT8	field14;                 // 14
    UINT16	field15;                // 15-16
    UINT32	sea;                    // 17-20   sea state (17)
    UINT8	field21;                 // 21
    UINT8	rain;                    // 22   rain clutter
    UINT8	field23;                 // 23
    UINT32	field24;                // 24-27
    UINT32	field28;                // 28-31
    UINT8	field32;                 // 32
    UINT8	field33;                 // 33
    UINT8	interference_rejection;  // 34
    UINT8	field35;                 // 35
    UINT8	field36;                 // 36
    UINT8	field37;                 // 37
    UINT8	target_expansion;        // 38
    UINT8	field39;                 // 39
    UINT8	field40;                 // 40
    UINT8	field41;                 // 41
    UINT8	target_boost;            // 42
};

struct RadarReport_03C4_129 {
    UINT8	what;
    UINT8	command;
    UINT8	radar_type;  // I hope! 01 = 4G, 08 = 3G, 0F = BR24
    UINT8	u00[55];     // Lots of unknown
    UINT16	firmware_date[16];
    UINT16	firmware_time[16];
    UINT8	u01[7];
};

struct RadarReport_04C4_66 {  // 04 C4 with length 66
    UINT8	what;                 // 0   0x04
    UINT8	command;              // 1   0xC4
    UINT32	field2;              // 2-5
    UINT16	bearing_alignment;   // 6-7
    UINT16	field8;              // 8-9
    UINT16	antenna_height;      // 10-11
};

struct RadarReport_08C4_18 {           // 08 c4  length 18
    UINT8 what;                          // 0  0x08
    UINT8 command;                       // 1  0xC4
    UINT8 field2;                        // 2
    UINT8 local_interference_rejection;  // 3
    UINT8 scan_speed;                    // 4
    UINT8 sls_auto;                      // 5 installation: sidelobe suppression auto
    UINT8 field6;                        // 6
    UINT8 field7;                        // 7
    UINT8 field8;                        // 8
    UINT8 side_lobe_suppression;         // 9 installation: sidelobe suppression
    UINT16 field10;                      // 10-11
    UINT8 noise_rejection;               // 12    noise rejection
    UINT8 target_sep;                    // 13
};
#pragma pack(pop)

#endif // RADARCCONTROLDEFINES_H
