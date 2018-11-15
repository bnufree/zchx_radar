#include "lowrance_control.hpp"

namespace zchx
{
	namespace radarproxy
	{

		Lowrance_control::Lowrance_control(std::string srcID
			, int localId
			, boost::shared_ptr<zchx::asio::service> socket
			, boost::shared_ptr<zchx::dds::zmq::device> zmq_server
			)
		{
			radarSourceId = srcID;
			radarLocalId = localId;
			m_asio_server = socket;
			m_ptr_zmq_server = zmq_server;
			//
			m_radar_report.set_timeofday(0);
			m_radar_report.set_type(1);
			m_radar_report.set_id(1);
			isControlPower = false;
			radarStatus = 0;
			setUtc = 0;
		}

		void Lowrance_control::Init(const radar_config & config)
		{
			int elelment_size = config.m_control_element_map.size();
			std::stringstream os;
			BOOST_FOREACH(BOOST_TYPEOF(*config.m_control_element_map.begin()) iter, config.m_control_element_map)
			{
				std::string element_name = iter.first;
				control_element element = iter.second;
				os << element_name << ": Unit="<< element .valueUnit<< ",min=" << element.min << ",max=" << element.max<<"\n";
				AddControl(element_name, element);
			}


			ZCHXLOG_DEBUG("=====Lowrance_radar_control_config=====\n"
				<< "id: " << config.id << "\n"
				<< "local_id: " << config.local_id << "\n"
				<< "protocol: " << config.protocol << "\n"
				<< "category: " << config.category << "\n"
				<< "elelment_size: " << elelment_size<<"\n"
				<< os.str()
			);
		}

		bool Lowrance_control::ProcessReport(const unsigned char * buf, size_t len)
		{

			//ZCHXLOG_DEBUG("Lowrance_control::ProcessReport len: "<< len);

			ZCHXLOG_INFO("ProcessReport len: "<<len);
			if (buf[1] == 0xC4)
			{
				switch ((len << 8) + buf[0])
				{
					case (18 << 8) + 0x01:
					{
						RadarReport_01C4_18 *s = (RadarReport_01C4_18 *)buf;
						if (radarStatus != s->radar_status)
						{
							radarStatus = s->radar_status;
							switch (buf[2])
							{
							case 0x01:
								ZCHXLOG_DEBUG("reports status RADAR_STANDBY");
								UpdateValue(CT_POWER,0);
								break;
							case 0x02:
								ZCHXLOG_DEBUG("reports status TRANSMIT");
								UpdateValue(CT_POWER,1);
								break;
							case 0x05:
								ZCHXLOG_DEBUG("reports status WAKING UP");
								break;
							default:
								break;
							}
						}
						break;
					}
					case (99 << 8) + 0x02: // length 99, 02 C4,contains gain,rain,interference rejection,sea
						//target_boost, target_expansion,range
					{
						RadarReport_02C4_99 *s = (RadarReport_02C4_99 *)buf;
						//gain
						if (s->field8 == 1)        // 1 for auto
							UpdateValue(CT_GAIN,-1);
						else 
							UpdateValue(CT_GAIN, s->gain * 100 / 255);
						//sea
						if (s->field13 == 0x01) 
							UpdateValue(CT_SEA,-1);  // auto sea
						else 
							UpdateValue(CT_SEA,s->sea * 100 / 255);
						//rain
						UpdateValue(CT_RAIN, s->rain * 100 / 255);
						//target boost
						UpdateValue(CT_TARGET_BOOST, s->target_boost);
						//s->interference rejection
						UpdateValue(CT_INTERFERENCE_REJECTION, s->interference_rejection);
						//target expansion
						UpdateValue(CT_TARGET_EXPANSION, s->target_expansion);
						//range
						UpdateValue(CT_RANG, s->range / 10);
						break;
					}
					case (129 << 8) + 0x03: // 129 bytes starting with 03 C4
					{
						RadarReport_03C4_129 *s = (RadarReport_03C4_129 *)buf;
						switch (s->radar_type) {
						case 0x0f:
							//ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_BR24");
							break;
						case 0x08:
							//ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_3G");
							break;
						case 0x01:
							//ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: Navico RT_4G");
							break;
						default:
							//ZCHXLOG_DEBUG("BR24radar_pi: Unknown radar_type: " << s->radar_type);
							return false;
						}
						break;
					}
					case (66 << 8) + 0x04: // 66 bytes starting with 04 C4,contains bearing alignment,antenna height
					{
						RadarReport_04C4_66 *data = (RadarReport_04C4_66 *)buf;
						// bearing alignment
						int ba = (int)data->bearing_alignment / 10;
						if (ba > 180) ba = ba - 360;
						UpdateValue(CT_BEARING_ALIGNMENT, ba);
						// antenna height
						UpdateValue(CT_ANTENNA_HEIGHT, data->antenna_height / 1000);
						break;
					}
					case (564 << 8) + 0x05:
					{
						// Content unknown, but we know that BR24 radomes send this
						ZCHXLOG_DEBUG("Navico BR24: msg");
						break;
					}
					case (18 << 8) + 0x08: // length 18, 08 C4, 
						//contains scan speed, noise rejection and target_separation and sidelobe suppression,local_interference_rejection
					{
						RadarReport_08C4_18 *s08 = (RadarReport_08C4_18 *)buf;
						UpdateValue(CT_SCAN_SPEED, s08->scan_speed);
						UpdateValue(CT_NOISE_REJECTION, s08->noise_rejection);
						UpdateValue(CT_TARGET_SEPARATION, s08->target_sep);
						if (s08->sls_auto == 1)
							UpdateValue(CT_SIDE_LOBE_SUPPRESSION,-1);
						else 
							UpdateValue(CT_SIDE_LOBE_SUPPRESSION, s08->side_lobe_suppression * 100 / 255);
						UpdateValue(CT_LOCAL_INTERFERENCE_REJECTION, s08->local_interference_rejection);
						break;
					}
					default:
						break;
				}
			}
			else if(buf[1] == 0xF5){
				ZCHXLOG_DEBUG("unknown: buf[1]=0xF5");
			}
			return false;
		}

		void Lowrance_control::RadarOFF()
		{
			ZCHXLOG_DEBUG("Lowrance_control::RadarOFF");
			if (isControlPower)
			{
				unsigned char Down[6] = { 0x00, 0xc1, 0x01, 0x01, 0xc1, 0x00 }; //00 c1 01  /01 c1 00
				m_asio_server->send_peer_message(radarSourceId, (char *)(Down + 0), 3);
				m_asio_server->send_peer_message(radarSourceId, (char *)(Down + 3), 3);
			}
		}
		void Lowrance_control::RadarON()
		{
			ZCHXLOG_DEBUG("Lowrance_control::RadarON");
			if (isControlPower) 
			{
				unsigned char Boot[6] = { 0x00, 0xc1, 0x01, 0x01, 0xc1, 0x01 }; //00c101/01c101		
				m_asio_server->send_peer_message(radarSourceId, (char *)(Boot + 0), 3);
				m_asio_server->send_peer_message(radarSourceId, (char *)(Boot + 3), 3);
			}
		}
		void Lowrance_control::RadarStayAlive()
		{
			static boost::posix_time::ptime const epoch(boost::gregorian::date(1970, 1, 1));
			boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
			boost::posix_time::time_duration recv_time = now - epoch;
			long long curUtc = recv_time.total_milliseconds();

			if (curUtc - setUtc > 20*1000)
			{
				//ZCHXLOG_DEBUG("Lowrance_control::RadarStayAlive");
				unsigned char heartbeat_[8] = { 0Xa0, 0xc1, 0x03, 0xc2, 0x04, 0xc2, 0x05, 0xc2 };
				for (int i = 0; i < 4; i++)
				{
					unsigned char sendC[2] = { 0 };
					memcpy(sendC, heartbeat_ + i * 2, 2);
					m_asio_server->send_peer_message(radarSourceId, (char *)sendC, 2);
				}
				setUtc = curUtc;
 			}
			Report();
		}

		void Lowrance_control::UpdateValue(CONTROL_TYPE controlType, int value)
		{
			ZCHXLOG_INFO("UpdateValue T = " << controlType << ", V = " << value);
			report_status_map.insert(controlType, value);

			switch (controlType)
			{
				case CT_POWER: // 1
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Power");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Power", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_SCAN_SPEED: // 2
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Scan speed");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Scan speed", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_ANTENNA_HEIGHT: // 3
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Antenna height");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Antenna height", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_BEARING_ALIGNMENT: // 4
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Bearing alignment");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Bearing alignment", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_RANG: // 5
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Rang");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Rang", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_GAIN: // 6
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Gain");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Gain", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_SEA: // 7
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Sea clutter");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Sea clutter", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_RAIN: // 8
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Rain clutter");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Rain clutter", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_NOISE_REJECTION: // 9
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Noise rejection");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Noise rejection", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_SIDE_LOBE_SUPPRESSION: // 10
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Side lobe suppression");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Side lobe suppression", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_INTERFERENCE_REJECTION: // 11
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Interference rejection");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Interference rejection", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_LOCAL_INTERFERENCE_REJECTION:  // 12
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Local interference rejection");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Local interference rejection", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_TARGET_EXPANSION: // 13
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Target expansion");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Target expansion", *radar_ctrl);
							UpdateReport();
						}
					}

					break;
				}
				break;
				case CT_TARGET_BOOST: // 14 
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Target boost");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Target boost", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				case CT_TARGET_SEPARATION: // 15
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Target separation");
					if (radar_ctrl)
					{
						if (radar_ctrl->value != value) {
							radar_ctrl->value = value;
							m_radar_control_map.set("Target separation", *radar_ctrl);
							UpdateReport();
						}
					}
					break;
				}
				default:
					break;
			}

		}
		void Lowrance_control::SetControlValue(CONTROL_TYPE controlType, int value)
		{
			ZCHXLOG_INFO("Lowrance_control::SetControlValue, T = "<< controlType<<", V = "<<value);
			set_status_map.insert(controlType, value);
			switch (controlType)
			{
			case CT_POWER: // 1
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Power");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							if (value == 0)
								RadarOFF();
							else
								RadarON();
						}else{
							ZCHXLOG_ERROR("Power control error,value=" << value);
						}
					}
					break;
				}		
			case CT_SCAN_SPEED: // 2
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Scan speed");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							UINT8 cmd[] = { 0x0f, 0xc1, (UINT8)value };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Scan speed value: " << value);
						}

					}
					break;
				}		
			case CT_ANTENNA_HEIGHT: // 3
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Antenna height");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							int v = value * 1000;  // radar wants millimeters, not meters
							int v1 = v / 256;
							int v2 = v & 255;
							UINT8 cmd[10] = { 0x30, 0xc1, 0x01, 0, 0, 0, (UINT8)v2, (UINT8)v1, 0, 0 };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Antenna height value: " << value);
						}
					}
					break;
				}
			case CT_BEARING_ALIGNMENT: // 4
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Bearing alignment");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							if (value < 0)  value += 360;
							int v = value * 10;
							int v1 = v / 256;
							int v2 = v & 255;
							UINT8 cmd[4] = { 0x05, 0xc1, (UINT8)v2, (UINT8)v1 };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Bearing alignment value: " << value);
						}
					}
					break;
				}	
			case CT_RANG: // 5
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Rang");
					if (radar_ctrl)
					{
						if (value >= 50 && value <= 72704)
						{
							unsigned int decimeters = (unsigned int)value * 10;
							UINT8 cmd[] = { 0x03,0xc1,
								(UINT8)((decimeters >> 0) & 0XFFL),
								(UINT8)((decimeters >> 8) & 0XFFL),
								(UINT8)((decimeters >> 16) & 0XFFL),
							};
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Rang value: " << value);
						}
					}
					break;
				}
			case CT_GAIN: // 6
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Gain");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							if (value < 0) {  // AUTO gain
								UINT8 cmd[] = {
									0x06, 0xc1, 0, 0, 0,   0,
									0x01, 0,    0, 0, 0xad  // changed from a1 to ad
								};
								m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
							}else {  // Manual Gain
								int v = (value + 1) * 255 / 100;
								if (v > 255) v = 255;
								UINT8 cmd[] = { 0x06, 0xc1, 0, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
								m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
							}
						}else{
							ZCHXLOG_ERROR("set Gain value: " << value);
						}
					}
					break;
				}
			case CT_SEA: // 7
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Sea clutter");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							if (value < 0) {  // Sea Clutter - Auto
								UINT8 cmd[11] = { 0x06, 0xc1, 0x02, 0, 0, 0, 0x01, 0, 0, 0, 0xd3 };
								m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
							}
							else {  // Sea Clutter
								int v = (value + 1) * 255 / 100;
								if (v > 255) v = 255;
								UINT8 cmd[] = { 0x06, 0xc1, 0x02, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
								m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
							}
						}else{
							ZCHXLOG_ERROR("set Sea clutter value: " << value);
						}
					}
					break;
				}		
			case CT_RAIN: // 8
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Rain clutter");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							int v = (value + 1) * 255 / 100;
							if (v > 255) v = 255;
							UINT8 cmd[] = { 0x06, 0xc1, 0x04, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Rain clutter value: " << value);
						}
					}
					break;
				}	
			case CT_NOISE_REJECTION: // 9
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Noise rejection");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							UINT8 cmd[] = { 0x21, 0xc1, (UINT8)value };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Noise rejection value: " << value);
						}
					}
					break;
				}	
			case CT_SIDE_LOBE_SUPPRESSION: // 10
				{ 
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Side lobe suppression");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							if (value < 0) {
								UINT8 cmd[] = {// SIDE_LOBE_SUPPRESSION auto
									0x06, 0xc1, 0x05, 0, 0, 0, 0x01, 0, 0, 0, 0xc0 };
								m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
							}else {
								int v = (value + 1) * 255 / 100;
								if (v > 255) v = 255;
								UINT8 cmd[] = { 0x6, 0xc1, 0x05, 0, 0, 0, 0, 0, 0, 0, (UINT8)v };
								m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
							}
						}else{
							ZCHXLOG_DEBUG("set Side lobe suppression value: " << value);
						}
					}
					break;
				} 	
			case CT_INTERFERENCE_REJECTION: // 11
				{  
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Interference rejection");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							UINT8 cmd[] = { 0x08, 0xc1, (UINT8)value };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Interference rejection value: " << value);
						}
					}
					break; 
				}
			case CT_LOCAL_INTERFERENCE_REJECTION:  // 12
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Local interference rejection");
					if (radar_ctrl)
					{
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max)
						{
							if (value < 0) value = 0;
							if (value > 3) value = 3;
							UINT8 cmd[] = { 0x0e, 0xc1, (UINT8)value };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Local interference rejection value: " << value);
						}
					}
					break;
				}		
			case CT_TARGET_EXPANSION: // 13
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Target expansion");
					if (radar_ctrl){
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max){
							UINT8 cmd[] = { 0x09, 0xc1, (UINT8)value };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_ERROR("set Target expansion value: " << value);
						}
					}
					break;
				}
				break;
			case CT_TARGET_BOOST: // 14 
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Target boost");
					if (radar_ctrl){
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max){
							UINT8 cmd[] = { 0x0a, 0xc1, (UINT8)value };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_DEBUG("set Target boost value: " << value);
						}
					}
					break;
				}
			case CT_TARGET_SEPARATION: // 15
				{
					boost::optional<control_element> radar_ctrl = m_radar_control_map.at("Target separation");
					if (radar_ctrl){
						if (value >= radar_ctrl->min &&  value <= radar_ctrl->max){
							UINT8 cmd[] = { 0x22, 0xc1, (UINT8)value };
							m_asio_server->send_peer_message(radarSourceId, (char *)cmd, sizeof(cmd));
						}else{
							ZCHXLOG_DEBUG("set Target separation value: "<<value);
						}
					}
					break;
				}
			default:
				break;
			}
		
		}

		void Lowrance_control::Report()
		{
			ZCHXLOG_DEBUG("Lowrance_control::Report");

			static boost::posix_time::ptime const epoch(boost::gregorian::date(1970, 1, 1));//一个基准点。
			boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
			boost::posix_time::time_duration recivetime = now - epoch;
			long long utc = recivetime.total_milliseconds();


			boost::posix_time::time_duration td = now.time_of_day();
			int time_of_day = td.total_seconds();

			m_radar_report.set_id(radarLocalId);
			m_radar_report.set_timeofday(time_of_day);
			m_radar_report.set_type(2);

			std::string topic = "radar_report";
			std::string category = "radar_report_category";
			std::string reportStr;			//  = "report_msg";

			m_radar_report.SerializeToString(&reportStr);

			zchx::dds::zmq::zmsg *message = new zchx::dds::zmq::zmsg();
			//
			message->append(topic.c_str());//add topic frame
			message->append((boost::lexical_cast<std::string>(utc)).c_str());//add utc frame
			message->push_back(reportStr.c_str(), reportStr.length());//add context frame

			ZCHXLOG_DEBUG("===send=== \n"
				<< "topic_name: " << topic << "\n"
				<< "utc:" << utc << "\n"
				<< "data.size: "<< reportStr.length()<<"\n"
				<< "control_s: "<< m_radar_report.report_size()
			);
	
			m_ptr_zmq_server->send(topic, category, message);
	

		}
		void Lowrance_control::UpdateReport()
		{
			ZCHXLOG_INFO("UpdateReport...");
			m_radar_report.Clear();
			BOOST_FOREACH(BOOST_TYPEOF(*m_radar_control_map.begin()) iter, m_radar_control_map)
			{
				std::string elementName = iter.first;
				control_element element = iter.second;
				AddControl(elementName, element);
			}
		}
		void Lowrance_control::AddControl(std::string element_name, control_element element)
		{	
			//ZCHXLOG_DEBUG("Lowrance_control::AddControl: "<< element_name);


			com::zchxlab::radar::protobuf::RadarStatus radar_control_element;
			radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(0));
			//
			if (element_name == "Power") // 1
			{		
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(1)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(2)); //值类型
				isControlPower = true;
			}
			else if (element_name == "Scan speed") // 2
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(2)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型
			}
			else if (element_name == "Antenna height") // 3
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(3)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(4)); //值类型
			}
			else if (element_name == "Bearing alignment") // 4
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(4)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(3)); //值类型
			}
			else if (element_name == "Rang") // 5
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(5)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(3)); //值类型
			}
			else if (element_name == "Gain") // 6
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(6)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			}
			else if (element_name == "Sea clutter") // 7
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(7)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			} 
			else if (element_name == "Rain clutter") // 8
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(8)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			}
			else if (element_name == "Noise rejection") // 9
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(9)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			} 
			else if (element_name == "Side lobe suppression") // 10
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(10)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			}
			else if (element_name == "Interference rejection") // 11
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(11)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			}
			else if (element_name == "Local interference rejection") // 12
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(12)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			}
			else if (element_name == "Target expansion") // 13
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(13)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			}
			else if (element_name == "Target boost") // 14
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(14)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			}
			//Target separation
			else if (element_name == "Target separation") // 15
			{
				radar_control_element.set_id(boost::numeric_cast<com::zchxlab::radar::protobuf::INFOTYPE, int>(15)); //控制类型
				radar_control_element.set_valuetype(boost::numeric_cast<com::zchxlab::radar::protobuf::VALUEUNIT, int>(1)); //值类型 
			}
			//
			int infoT = radar_control_element.id();
			if (infoT != 0)
			{
				//
				radar_control_element.set_value(element.value);
				radar_control_element.set_min(element.min);
				radar_control_element.set_max(element.max);
				//
				com::zchxlab::radar::protobuf::RadarStatus* ptr_radar_control_element;
				ptr_radar_control_element = m_radar_report.add_report();
				ptr_radar_control_element->CopyFrom(radar_control_element);
				m_radar_control_map.insert(element_name, element);
			}	
		}

	}
}
