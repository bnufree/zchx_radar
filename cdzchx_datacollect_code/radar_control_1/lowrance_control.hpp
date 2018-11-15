#ifndef LOWRANCE_CONTROL_HPP
#define LOWRANCE_CONTROL_HPP

#include "ZCHXRadarControl.pb.h"
#include "radar_control.hpp"

namespace zchx {
namespace radarproxy {


class Lowrance_control : public radar_control
{
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
public:
	typedef  zchx_utils::safe_map<int, int> status_map_type;
	typedef  zchx_utils::safe_map<std::string, control_element> radar_control_element_map_type;

public:
	Lowrance_control(std::string srcID, int localId, boost::shared_ptr<zchx::asio::service> socket, boost::shared_ptr<zchx::dds::zmq::device> zmq_server);
	void Init(const radar_config & config);
	bool ProcessReport(const unsigned char*buf , size_t len);
	void RadarOFF();
	void RadarON();
	void RadarStayAlive();
	void UpdateValue(CONTROL_TYPE controlType, int value);
	void SetControlValue(CONTROL_TYPE controlType, int value);
private:
	void Report();
	void UpdateReport();
	void AddControl(std::string element_name, control_element element);
	bool isControlPower;
	com::zchxlab::radar::protobuf::RadarReport m_radar_report;
	radar_control_element_map_type m_radar_control_map;
	


	char radarStatus; 
	status_map_type report_status_map;
	status_map_type set_status_map;
	long long setUtc;
};


}
}


#endif // 