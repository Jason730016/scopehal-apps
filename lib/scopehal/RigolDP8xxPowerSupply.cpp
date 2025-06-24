/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal v0.1                                                                                                     *
*                                                                                                                      *
* Copyright (c) 2012-2023 Andrew D. Zonenberg and contributors                                                         *
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

#include "scopehal.h"
#include "RigolDP8xxPowerSupply.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

RigolDP8xxPowerSupply::RigolDP8xxPowerSupply(SCPITransport* transport)
	: SCPIDevice(transport)
	, SCPIInstrument(transport)
{
	//Figure out how many channels we have
	int nchans = m_model.c_str()[strlen("DP8")] - '0';
	LogDebug("m_model = %s, nchans = %d\n", m_model.c_str(), nchans);

	for(int i=0; i<nchans; i++)
	{
		m_channels.push_back(
			new PowerSupplyChannel(string("CH") + to_string(i+1), this, "#808080", i));

		GetPowerOvercurrentShutdownEnabled(i); // Initialize map
	}
}

RigolDP8xxPowerSupply::~RigolDP8xxPowerSupply()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device info

string RigolDP8xxPowerSupply::GetDriverNameInternal()
{
	return "rigol_dp8xx";
}

uint32_t RigolDP8xxPowerSupply::GetInstrumentTypesForChannel(size_t /*i*/) const
{
	return INST_PSU;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device capabilities

bool RigolDP8xxPowerSupply::SupportsSoftStart()
{
	return false;
}

bool RigolDP8xxPowerSupply::SupportsIndividualOutputSwitching()
{
	return true;
}

bool RigolDP8xxPowerSupply::SupportsMasterOutputSwitching()
{
	return false;
}

bool RigolDP8xxPowerSupply::SupportsOvercurrentShutdown()
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual hardware interfacing

#define CHNAME(ch) ("CH" + to_string(ch + 1))
#define SOURCENAME(ch) ("SOURCE" + to_string(ch + 1))

bool RigolDP8xxPowerSupply::IsPowerConstantCurrent(int chan)
{
	return m_transport->SendCommandQueuedWithReply("OUTPUT:CVCC? " + CHNAME(chan)) == "CC";
}

double RigolDP8xxPowerSupply::GetPowerVoltageActual(int chan)
{
	return stof(m_transport->SendCommandQueuedWithReply("MEASURE:VOLTAGE? " + CHNAME(chan)));
}

double RigolDP8xxPowerSupply::GetPowerVoltageNominal(int chan)
{
	return stof(m_transport->SendCommandQueuedWithReply(SOURCENAME(chan) + ":VOLTAGE?"));
}

double RigolDP8xxPowerSupply::GetPowerCurrentActual(int chan)
{
	return stof(m_transport->SendCommandQueuedWithReply("MEASURE:CURRENT? " + CHNAME(chan)));
}

double RigolDP8xxPowerSupply::GetPowerCurrentNominal(int chan)
{
	return stof(m_transport->SendCommandQueuedWithReply(SOURCENAME(chan) + ":CURRENT?"));
}

bool RigolDP8xxPowerSupply::GetPowerChannelActive(int chan)
{
	return m_transport->SendCommandQueuedWithReply("OUTPUT? " + CHNAME(chan)) == "ON";
}

void RigolDP8xxPowerSupply::SetPowerOvercurrentShutdownEnabled(int chan, bool enable)
{
	m_overcurrentProtectionEnabled[chan] = enable;
	m_transport->SendCommandQueued(SOURCENAME(chan) + ":CURRENT:PROTECTION:STATE " + (enable ? "ON" : "OFF"));

	if (enable)
	{
		SetPowerCurrent(chan, GetPowerCurrentNominal(chan)); // Make sure OVP level is set
	}
	else
	{
		m_transport->SendCommandQueued(SOURCENAME(chan) + ":CURRENT:PROTECTION:CLEAR");
	}
}

bool RigolDP8xxPowerSupply::GetPowerOvercurrentShutdownEnabled(int chan)
{
	if (m_overcurrentProtectionEnabled.find(chan) == m_overcurrentProtectionEnabled.end())
	{
		m_overcurrentProtectionEnabled[chan] =
			m_transport->SendCommandQueuedWithReply(SOURCENAME(chan) + ":CURRENT:PROTECTION:STATE?") == "ON";
	}

	return m_overcurrentProtectionEnabled[chan];
}

bool RigolDP8xxPowerSupply::GetPowerOvercurrentShutdownTripped(int chan)
{
	return m_transport->SendCommandQueuedWithReply(SOURCENAME(chan) + ":CURRENT:PROTECTION:TRIPPED?") == "YES";
}

void RigolDP8xxPowerSupply::SetPowerVoltage(int chan, double volts)
{
	m_transport->SendCommandQueued(SOURCENAME(chan) + ":VOLTAGE " + to_string(volts));
}

void RigolDP8xxPowerSupply::SetPowerCurrent(int chan, double amps)
{
	if (GetPowerOvercurrentShutdownEnabled(chan))
	{
		// Unclear if there is a better way to do this
		m_transport->SendCommandQueued(SOURCENAME(chan) + ":CURRENT:PROTECTION:LEVEL " + to_string(amps));
	}

	m_transport->SendCommandQueued(SOURCENAME(chan) + ":CURRENT " + to_string(amps));
}

void RigolDP8xxPowerSupply::SetPowerChannelActive(int chan, bool on)
{
	m_transport->SendCommandQueued("OUTPUT " + CHNAME(chan) + "," + (on ? "ON" : "OFF"));

	if (on)
	{
		m_transport->SendCommandQueued(SOURCENAME(chan) + ":CURRENT:PROTECTION:CLEAR");
	}
}
