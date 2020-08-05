#ifndef RADARCCONTROLDEFINES_H
#define RADARCCONTROLDEFINES_H

#include <QObject>
#include <QString>
//#include <basetsd.h>

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

class RadarStatus
{
private:
    INFOTYPE            id;					// 消息类型
    VALUEUNIT			unit;               // 值类型
    int 	  			value;				// 当前值
    int                 min;				// value可设置的最小值
    int                 max;				// value可设置的最大值
    bool                isAuto;
public:
    RadarStatus()
    {
        isAuto = false;
        id = UNKNOWN;
        unit = UNIT_UNKNOWN;
        value = -1;
        min = INT_MIN;
        max = INT_MAX;

    }

    RadarStatus(INFOTYPE ptype, int pmin = INT_MIN, int pmax = INT_MAX)
    {
        id = ptype;
        unit = VALUEUNIT::UNIT_UNKNOWN;
        value = -1;
        min = pmin;
        max = pmax;
        isAuto = false;
        if(min == - 1)
        {
            isAuto= true;
//            min = 0;
        }
    }

    INFOTYPE getId() const {return id;}
    void    setId(INFOTYPE id_){id = id_;}

    VALUEUNIT getUnit() const {return unit;}
    void    setUnit(VALUEUNIT u) {unit = u;}

    int     getValue() const {return value;}
    void    setValue(int v) {value = v;}

    int     getMin() const {return min;}
    void    setMin(int v)
    {
        min = v;
        if(min == - 1)
        {
            isAuto= true;
            min = 0;
        }
    }

    int     getMax() const {return max;}
    void    setMax(int v) {max = v;}

    bool    getAutoAvailable() const {return isAuto;}
    void    setAutoAvailable(bool b) {isAuto = b;}

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
    uint8_t	what;                 // 0   0x01
    uint8_t	command;              // 1   0xC4
    uint8_t	radar_status;         // 2
    uint8_t	field3;               // 3
    uint8_t	field4;               // 4
    uint8_t	field5;               // 5
    uint16_t	field6;              // 6-7
    uint16_t	field8;              // 8-9
    uint16_t	field10;             // 10-11
};

struct RadarReport_02C4_99 {     // length 99
    uint8_t	what;                    // 0   0x02
    uint8_t	command;                 // 1 0xC4
    uint32_t	range;                  //  2-3   0x06 0x09
    uint16_t	field4;                 // 6-7    0
    uint32_t	field8;                 // 8-11   1
    uint8_t	gain;                    // 12
    uint8_t	field13;                 // 13  ==1 for sea auto
    uint8_t	field14;                 // 14
    uint16_t	field15;                // 15-16
    uint32_t	sea;                    // 17-20   sea state (17)
    uint8_t	field21;                 // 21
    uint8_t	rain;                    // 22   rain clutter
    uint8_t	field23;                 // 23
    uint32_t	field24;                // 24-27
    uint32_t	field28;                // 28-31
    uint8_t	field32;                 // 32
    uint8_t	field33;                 // 33
    uint8_t	interference_rejection;  // 34
    uint8_t	field35;                 // 35
    uint8_t	field36;                 // 36
    uint8_t	field37;                 // 37
    uint8_t	target_expansion;        // 38
    uint8_t	field39;                 // 39
    uint8_t	field40;                 // 40
    uint8_t	field41;                 // 41
    uint8_t	target_boost;            // 42
};

struct RadarReport_03C4_129 {
    uint8_t	what;
    uint8_t	command;
    uint8_t	radar_type;  // I hope! 01 = 4G and new 3G, 08 = 3G, 0F = BR24, 00 = HALO
    uint8_t	u00[55];     // Lots of unknown
    uint16_t	firmware_date[16];
    uint16_t	firmware_time[16];
    uint8_t	u01[7];
};

struct RadarReport_04C4_66 {  // 04 C4 with length 66
    uint8_t	what;                 // 0   0x04
    uint8_t	command;              // 1   0xC4
    uint32_t	field2;              // 2-5
    uint16_t	bearing_alignment;   // 6-7
    uint16_t	field8;              // 8-9
    uint16_t	antenna_height;      // 10-11
};

struct RadarReport_08C4_18 {           // 08 c4  length 18
    uint8_t what;                          // 0  0x08
    uint8_t command;                       // 1  0xC4
    uint8_t field2;                        // 2
    uint8_t local_interference_rejection;  // 3
    uint8_t scan_speed;                    // 4
    uint8_t sls_auto;                      // 5 installation: sidelobe suppression auto
    uint8_t field6;                        // 6
    uint8_t field7;                        // 7
    uint8_t field8;                        // 8
    uint8_t side_lobe_suppression;         // 9 installation: sidelobe suppression
    uint16_t field10;                      // 10-11
    uint8_t noise_rejection;               // 12    noise rejection
    uint8_t target_sep;                    // 13
};

struct RadarReport_08C4_21 {
  RadarReport_08C4_18 old;
  uint8_t doppler_state;
  uint16_t doppler_speed;
};

struct RadarReport_12C4_66 {  // 12 C4 with length 66
  // Device Serial number is sent once upon network initialization only
  uint8_t what;          // 0   0x12
  uint8_t command;       // 1   0xC4
  uint8_t serialno[12];  // 2-13 Device serial number at 3G (All?)
};

/*
struct RadarReport_01B2 {
  char serialno[16];  // ASCII serial number, zero terminated
  uint8_t u1[18];
  PackedAddress addr1;   // EC0608201970
  uint8_t u2[4];         // 11000000
  PackedAddress addr2;   // EC0607161A26
  uint8_t u3[10];        // 1F002001020010000000
  PackedAddress addr3;   // EC0608211971
  uint8_t u4[4];         // 11000000
  PackedAddress addr4;   // EC0608221972
  uint8_t u5[10];        // 10002001030010000000
  PackedAddress addr5;   // EC0608231973
  uint8_t u6[4];         // 11000000
  PackedAddress addr6;   // EC0608241974
  uint8_t u7[4];         // 12000000
  PackedAddress addr7;   // EC0608231975
  uint8_t u8[10];        // 10002002030010000000
  PackedAddress addr8;   // EC0608251976
  uint8_t u9[4];         // 11000000
  PackedAddress addr9;   // EC0608261977
  uint8_t u10[4];        // 12000000
  PackedAddress addr10;  // EC0608251978
  uint8_t u11[10];       // 12002001030010000000
  PackedAddress addr11;  // EC0608231979
  uint8_t u12[4];        // 11000000
  PackedAddress addr12;  // EC060827197A
  uint8_t u13[4];        // 12000000
  PackedAddress addr13;  // EC060823197B
  uint8_t u14[10];       // 12002002030010000000
  PackedAddress addr14;  // EC060825197C
  uint8_t u15[10];       // 11000000
  PackedAddress addr15;  // EC060828197D
  uint8_t u16[10];       // 12000000
  PackedAddress addr16;  // EC060825197E
};
*/

#pragma pack(pop)

#endif // RADARCCONTROLDEFINES_H
