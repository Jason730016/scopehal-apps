/***********************************************************************************************************************
*                                                                                                                      *
* libscopeprotocols                                                                                                    *
*                                                                                                                      *
* Copyright (c) 2012-2024 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

#include "../scopehal/scopehal.h"
#include "EthernetProtocolDecoder.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

EthernetProtocolDecoder::EthernetProtocolDecoder(const string& color)
	: PacketDecoder(color, CAT_SERIAL)
{
	//Set up channels
	CreateInput("din");
}

EthernetProtocolDecoder::~EthernetProtocolDecoder()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool EthernetProtocolDecoder::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if( (i == 0) && (stream.GetType() == Stream::STREAM_TYPE_ANALOG) )
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

vector<string> EthernetProtocolDecoder::GetHeaders()
{
	vector<string> ret;
	ret.push_back("Dest MAC");
	ret.push_back("Src MAC");
	ret.push_back("VLAN");
	ret.push_back("Ethertype");
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual protocol decoding

void EthernetProtocolDecoder::BytesToFrames(
		vector<uint8_t>& bytes,
		vector<uint64_t>& starts,
		vector<uint64_t>& ends,
		EthernetWaveform* cap,
		bool suppressedPreambleAndFCS)
{
	Packet* pack = new Packet;

	EthernetFrameSegment segment;
	segment.m_type = EthernetFrameSegment::TYPE_INVALID;
	size_t start = 0;
	size_t len = bytes.size();
	size_t crcstart = 0;
	uint32_t crc_expected = 0;
	uint32_t crc_actual = 0;

	//If we are suppressing the preamble, jump straight into the data
	if(suppressedPreambleAndFCS)
		segment.m_type = EthernetFrameSegment::TYPE_DST_MAC;

	for(size_t i=0; i<len; i++)
	{
		switch(segment.m_type)
		{
			case EthernetFrameSegment::TYPE_INVALID:

				//In between frames. Look for a preamble
				if(bytes[i] != 0x55)
				{
					//LogDebug("EthernetProtocolDecoder: Skipping unknown byte %02x\n", bytes[i]);
				}

				//Got a valid 55. We're now in the preamble
				else
				{
					start = starts[i];
					segment.m_type = EthernetFrameSegment::TYPE_PREAMBLE;
					segment.m_data.clear();
					segment.m_data.push_back(0x55);

					//Start a new packet
					pack->m_offset = starts[i];
				}
				break;

			case EthernetFrameSegment::TYPE_PREAMBLE:

				//Look for the SFD
				if(bytes[i] == 0xd5)
				{
					//Save the preamble
					cap->m_offsets.push_back(start / cap->m_timescale);
					cap->m_durations.push_back( (starts[i] - start) / cap->m_timescale);
					cap->m_samples.push_back(segment);

					//Save the SFD
					start = starts[i];
					cap->m_offsets.push_back(start / cap->m_timescale);
					cap->m_durations.push_back( (ends[i] - starts[i]) / cap->m_timescale);
					segment.m_type = EthernetFrameSegment::TYPE_SFD;
					segment.m_data.clear();
					segment.m_data.push_back(0xd5);
					cap->m_samples.push_back(segment);

					//Set up for data
					segment.m_type = EthernetFrameSegment::TYPE_DST_MAC;
					segment.m_data.clear();

					crcstart = i+1;
				}

				//No SFD, just add the preamble byte
				else if(bytes[i] == 0x55)
					segment.m_data.push_back(0x55);

				//Garbage (TODO: handle this better)
				else
				{
					//LogDebug("EthernetProtocolDecoder: Skipping unknown byte %02x\n", bytes[i]);
				}

				break;

			case EthernetFrameSegment::TYPE_DST_MAC:

				//Start of MAC? Record start time
				if(segment.m_data.empty())
				{
					start = starts[i];
					cap->m_offsets.push_back(start / cap->m_timescale);
				}

				//Add the data
				segment.m_data.push_back(bytes[i]);

				//Are we done? Add it
				if(segment.m_data.size() == 6)
				{
					cap->m_durations.push_back( (ends[i] - start) / cap->m_timescale );
					cap->m_samples.push_back(segment);

					//Format the content for display
					char tmp[64];
					snprintf(tmp, sizeof(tmp), "%02x:%02x:%02x:%02x:%02x:%02x",
						segment.m_data[0],
						segment.m_data[1],
						segment.m_data[2],
						segment.m_data[3],
						segment.m_data[4],
						segment.m_data[5]);
					pack->m_headers["Dest MAC"] = tmp;

					//Reset for next block of the frame
					segment.m_type = EthernetFrameSegment::TYPE_SRC_MAC;
					segment.m_data.clear();
				}

				break;

			case EthernetFrameSegment::TYPE_SRC_MAC:

				//Start of MAC? Record start time
				if(segment.m_data.empty())
				{
					start = starts[i];
					cap->m_offsets.push_back(start / cap->m_timescale);
				}

				//Add the data
				segment.m_data.push_back(bytes[i]);

				//Are we done? Add it
				if(segment.m_data.size() == 6)
				{
					cap->m_durations.push_back( (ends[i] - start) / cap->m_timescale);
					cap->m_samples.push_back(segment);

					//Format the content for display
					char tmp[64];
					snprintf(tmp, sizeof(tmp),"%02x:%02x:%02x:%02x:%02x:%02x",
						segment.m_data[0],
						segment.m_data[1],
						segment.m_data[2],
						segment.m_data[3],
						segment.m_data[4],
						segment.m_data[5]);
					pack->m_headers["Src MAC"] = tmp;

					//Reset for next block of the frame
					segment.m_type = EthernetFrameSegment::TYPE_ETHERTYPE;
					segment.m_data.clear();
				}

				break;

			case EthernetFrameSegment::TYPE_ETHERTYPE:

				//Start of Ethertype? Record start time
				if(segment.m_data.empty())
				{
					start = starts[i] ;
					cap->m_offsets.push_back(start / cap->m_timescale);
				}

				//Add the data
				segment.m_data.push_back(bytes[i]);

				//Are we done? Add it
				if(segment.m_data.size() == 2)
				{
					cap->m_durations.push_back( (ends[i] - start) / cap->m_timescale);
					cap->m_samples.push_back(segment);

					/*
						Format the content for display
						Colors from ColorBrewer 11-class Paired.

						#e31a1c
						#ff7f00
						#cab2d6
						#6a3d9a
					 */
					uint16_t ethertype = (segment.m_data[0] << 8) | segment.m_data[1];
					if(ethertype < 1500)
					{
						//Default to unknown LLC
						pack->m_headers["Ethertype"] = "LLC";
						pack->m_displayBackgroundColor = "#33a02c";
						pack->m_displayForegroundColor = "#000000";

						//Look up the LLC LSAP address to see what it is
						if( (i+1) < bytes.size() )
						{
							if(bytes[i+1] == 0x42)
							{
								pack->m_headers["Ethertype"] = "STP";
								pack->m_displayBackgroundColor = "#fdbf6f";
								pack->m_displayForegroundColor = "#000000";
							}
						}
					}
					else
					{
						char tmp[64];
						switch(ethertype)
						{
							case 0x0800:
								pack->m_headers["Ethertype"] = "IPv4";
								pack->m_displayBackgroundColor = "#a6cee3";
								pack->m_displayForegroundColor = "#000000";
								break;

							case 0x0806:
								pack->m_headers["Ethertype"] = "ARP";
								pack->m_displayBackgroundColor = "#ffff99";
								pack->m_displayForegroundColor = "#000000";
								break;

							//TODO: decoder inner ethertype too?
							case 0x8100:
								pack->m_headers["Ethertype"] = "802.1q";
								pack->m_displayBackgroundColor = "#b2df8a";
								pack->m_displayForegroundColor = "#000000";
								break;

							case 0x86DD:
								pack->m_headers["Ethertype"] = "IPv6";
								pack->m_displayBackgroundColor = "#1f78b4";
								pack->m_displayForegroundColor = "#ffffff";
								break;

							case 0x88cc:
								pack->m_headers["Ethertype"] = "LLDP";
								pack->m_displayBackgroundColor = "#5e4fa2";
								pack->m_displayForegroundColor = "#ffffff";
								break;

							default:
								snprintf(tmp, sizeof(tmp), "%02x%02x",
								segment.m_data[0],
								segment.m_data[1]);
								pack->m_headers["Ethertype"] = tmp;
								pack->m_displayBackgroundColor = "#fb9a99";
								pack->m_displayForegroundColor = "#000000";
								break;
						}
					}

					//Reset for next block of the frame
					segment.m_type = EthernetFrameSegment::TYPE_PAYLOAD;
					segment.m_data.clear();

					//It's an 802.1q tag, decode the VLAN header
					if(ethertype == 0x8100)
						segment.m_type = EthernetFrameSegment::TYPE_VLAN_TAG;
				}

				break;

			case EthernetFrameSegment::TYPE_VLAN_TAG:

				//Start of tag? Record start time
				if(segment.m_data.empty())
				{
					start = starts[i];
					cap->m_offsets.push_back(start / cap->m_timescale);
				}

				//Add the data
				segment.m_data.push_back(bytes[i]);

				//Are we done? Add it
				if(segment.m_data.size() == 2)
				{
					cap->m_durations.push_back( (ends[i] - start) / cap->m_timescale);
					cap->m_samples.push_back(segment);

					uint16_t tag = (segment.m_data[0] << 8) | segment.m_data[1];

					//Reset for the internal ethertype
					segment.m_type = EthernetFrameSegment::TYPE_ETHERTYPE;
					segment.m_data.clear();

					//Format the content for display
					char tmp[64];
					snprintf(tmp, sizeof(tmp),"%d", tag & 0xfff);
					pack->m_headers["VLAN"] = tmp;
				}

				break;

			case EthernetFrameSegment::TYPE_PAYLOAD:

				//Add a data element
				//For now, each byte is its own payload blob
				start = starts[i];
				cap->m_offsets.push_back(start / cap->m_timescale);
				cap->m_durations.push_back( (ends[i] - start) / cap->m_timescale);
				segment.m_type = EthernetFrameSegment::TYPE_PAYLOAD;
				segment.m_data.clear();
				segment.m_data.push_back(bytes[i]);
				cap->m_samples.push_back(segment);

				pack->m_data.push_back(bytes[i]);

				//If almost at end of packet, next 4 bytes are FCS
				if(suppressedPreambleAndFCS)
				{
					if(i == bytes.size()-1)
					{
						pack->m_len = ends[i] - pack->m_offset;
						m_packets.push_back(pack);
						return;
					}
				}
				else if(i == bytes.size() - 5)
				{
					segment.m_data.clear();
					segment.m_type = EthernetFrameSegment::TYPE_FCS_GOOD;
				}
				break;

			case EthernetFrameSegment::TYPE_FCS_GOOD:

				//Start of FCS? Record start time
				if(segment.m_data.empty())
				{
					crc_expected = CRC32(&bytes[0], crcstart, i-1);

					start = starts[i];
					cap->m_offsets.push_back(start / cap->m_timescale);
				}

				//Add the data
				segment.m_data.push_back(bytes[i]);
				crc_actual = (crc_actual << 8) | bytes[i];

				//Are we done? Add it
				if(segment.m_data.size() == 4)
				{
					//Validate CRC
					if(crc_actual != crc_expected)
					{
						segment.m_type = EthernetFrameSegment::TYPE_FCS_BAD;
						pack->m_displayBackgroundColor = m_backgroundColors[PROTO_COLOR_ERROR];
						pack->m_displayForegroundColor = "#ffffff";
						LogTrace("Frame CRC is %08x, expected %08x\n", crc_actual, crc_expected);
					}

					cap->m_durations.push_back( (ends[i] - start)/ cap->m_timescale);
					cap->m_samples.push_back(segment);

					pack->m_len = ends[i] - pack->m_offset;
					m_packets.push_back(pack);
					return;
				}

				break;

			default:
				break;
		}
	}

	//If we get here it wasn't a valid frame
	delete pack;
}

std::string EthernetWaveform::GetColor(size_t i)
{
	switch(m_samples[i].m_type)
	{
		//Preamble/SFD: gray (not interesting)
		case EthernetFrameSegment::TYPE_INBAND_STATUS:
		case EthernetFrameSegment::TYPE_PREAMBLE:
		case EthernetFrameSegment::TYPE_SFD:
			return StandardColors::colors[StandardColors::COLOR_PREAMBLE];

		//MAC addresses (src or dest)
		case EthernetFrameSegment::TYPE_DST_MAC:
		case EthernetFrameSegment::TYPE_SRC_MAC:
			return StandardColors::colors[StandardColors::COLOR_ADDRESS];

		//Control codes
		case EthernetFrameSegment::TYPE_ETHERTYPE:
		case EthernetFrameSegment::TYPE_VLAN_TAG:
			return StandardColors::colors[StandardColors::COLOR_CONTROL];

		case EthernetFrameSegment::TYPE_FCS_GOOD:
			return StandardColors::colors[StandardColors::COLOR_CHECKSUM_OK];
		case EthernetFrameSegment::TYPE_FCS_BAD:
			return StandardColors::colors[StandardColors::COLOR_CHECKSUM_BAD];

		//Signal has entirely disappeared, or fault condition reported
		case EthernetFrameSegment::TYPE_NO_CARRIER:
		case EthernetFrameSegment::TYPE_TX_ERROR:
		case EthernetFrameSegment::TYPE_REMOTE_FAULT:
		case EthernetFrameSegment::TYPE_LOCAL_FAULT:
		case EthernetFrameSegment::TYPE_LINK_INTERRUPTION:
			return StandardColors::colors[StandardColors::COLOR_ERROR];

		//Payload
		default:
			return StandardColors::colors[StandardColors::COLOR_DATA];
	}
}

string EthernetWaveform::GetText(size_t i)
{
	char tmp[128];

	auto sample = m_samples[i];
	switch(sample.m_type)
	{
		case EthernetFrameSegment::TYPE_TX_ERROR:
			return "ERROR";

		case EthernetFrameSegment::TYPE_PREAMBLE:
			return "PREAMBLE";

		case EthernetFrameSegment::TYPE_SFD:
			return "SFD";

		case EthernetFrameSegment::TYPE_NO_CARRIER:
			return "NO CARRIER";

		case EthernetFrameSegment::TYPE_DST_MAC:
			{
				if(sample.m_data.size() != 6)
					return "[invalid dest MAC length]";

				snprintf(tmp, sizeof(tmp), "To %02x:%02x:%02x:%02x:%02x:%02x",
					sample.m_data[0],
					sample.m_data[1],
					sample.m_data[2],
					sample.m_data[3],
					sample.m_data[4],
					sample.m_data[5]);
				return tmp;
			}

		case EthernetFrameSegment::TYPE_SRC_MAC:
			{
				if(sample.m_data.size() != 6)
					return "[invalid src MAC length]";

				snprintf(tmp, sizeof(tmp), "From %02x:%02x:%02x:%02x:%02x:%02x",
					sample.m_data[0],
					sample.m_data[1],
					sample.m_data[2],
					sample.m_data[3],
					sample.m_data[4],
					sample.m_data[5]);
				return tmp;
			}

		case EthernetFrameSegment::TYPE_VLAN_TAG:
			{
				uint16_t tag = (sample.m_data[0] << 8) | sample.m_data[1];

				snprintf(tmp, sizeof(tmp), "VLAN %d, PCP %d",
					tag & 0xfff, tag >> 13);
				string sret = tmp;
				if(tag & 0x1000)
					sret += ", DE";
				return sret;
			}
			break;


		case EthernetFrameSegment::TYPE_ETHERTYPE:
			{
				if(sample.m_data.size() != 2)
					return "[invalid Ethertype length]";

				string type = "Type: ";

				uint16_t ethertype = (sample.m_data[0] << 8) | sample.m_data[1];

				//It's not actually an ethertype, it's a LLC frame.
				if(ethertype < 1500)
				{
					//Look at the next segment to get the payload
					if((size_t)i+1 < m_samples.size())
					{
						auto& next = m_samples[i+1];
						if(next.m_data[0] == 0x42)
							type += "STP";
						else
							type += "LLC";
					}
					else
						type += "LLC";
				}

				else
				{
					switch(ethertype)
					{
						case 0x0800:
							type += "IPv4";
							break;

						case 0x0806:
							type += "ARP";
							break;

						case 0x8100:
							type += "802.1q";
							break;

						case 0x86dd:
							type += "IPv6";
							break;

						case 0x88cc:
							type += "LLDP";
							break;

						case 0x88f7:
							type += "PTP";
							break;

						default:
							snprintf(tmp, sizeof(tmp), "0x%04x", ethertype);
							type += tmp;
							break;
					}
				}

				return type;
			}

		case EthernetFrameSegment::TYPE_PAYLOAD:
			{
				string ret;
				for(auto b : sample.m_data)
				{
					snprintf(tmp, sizeof(tmp), "%02x ", b);
					ret += tmp;
				}
				return ret;
			}

		case EthernetFrameSegment::TYPE_INBAND_STATUS:
			{
				int status = sample.m_data[0];

				int up = status & 1;
				int rawspeed = (status >> 1) & 3;
				int duplex = (status >> 3) & 1;

				int speed = 10;
				if(rawspeed == 1)
					speed = 100;
				else if(rawspeed == 2)
					speed = 1000;

				snprintf(tmp, sizeof(tmp), "%s, %s duplex, %d Mbps",
					up ? "up" : "down",
					duplex ? "full" : "half",
					speed
					);
				return tmp;
			}

		case EthernetFrameSegment::TYPE_FCS_GOOD:
		case EthernetFrameSegment::TYPE_FCS_BAD:
			{
				if(sample.m_data.size() != 4)
					return "[invalid FCS length]";

				snprintf(tmp, sizeof(tmp), "CRC: %02x%02x%02x%02x",
					sample.m_data[0],
					sample.m_data[1],
					sample.m_data[2],
					sample.m_data[3]);
				return tmp;
			}

		case EthernetFrameSegment::TYPE_LOCAL_FAULT:
			return "Local Fault";
		case EthernetFrameSegment::TYPE_REMOTE_FAULT:
			return "Remote Fault";
		case EthernetFrameSegment::TYPE_LINK_INTERRUPTION:
			return "Link Interruption";

		default:
			break;
	}

	return "";
}
