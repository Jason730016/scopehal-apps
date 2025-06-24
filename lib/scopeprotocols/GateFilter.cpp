/***********************************************************************************************************************
*                                                                                                                      *
* libscopeprotocols                                                                                                    *
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

#include "../scopehal/scopehal.h"
#include "GateFilter.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

GateFilter::GateFilter(const string& color)
	: Filter(color, CAT_MATH)
	, m_mode("Mode")
{
	AddStream(Unit(Unit::UNIT_VOLTS), "out", Stream::STREAM_TYPE_ANALOG);

	m_parameters[m_mode] = FilterParameter(FilterParameter::TYPE_ENUM, Unit(Unit::UNIT_COUNTS));
	m_parameters[m_mode].AddEnumValue("Gate", MODE_GATE);
	m_parameters[m_mode].AddEnumValue("Latch", MODE_LATCH);
	m_parameters[m_mode].SetIntVal(MODE_LATCH);

	CreateInput("data");
	CreateInput("enable");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool GateFilter::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if( (i == 0) && (stream.GetType() == Stream::STREAM_TYPE_ANALOG) )
		return true;

	if( (i == 1) && (stream.GetType() == Stream::STREAM_TYPE_ANALOG_SCALAR) )
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string GateFilter::GetProtocolName()
{
	return "Gate";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void GateFilter::Refresh(vk::raii::CommandBuffer& /*cmdBuf*/, std::shared_ptr<QueueHandle> /*queue*/)
{
	//Make sure we've got valid inputs
	auto din = GetInput(0);
	auto en = GetInput(1);
	if(!din || !en)
	{
		SetData(nullptr, 0);
		return;
	}

	//TODO: handle sparse case
	//for now, drop anything other than uniform
	auto udin = dynamic_cast<UniformAnalogWaveform*>(din.GetData());
	if(!udin)
	{
		SetData(nullptr, 0);
		return;
	}

	//If gating, nothing to output
	auto mode = m_parameters[m_mode].GetIntVal();
	if(!en.GetScalarValue())
	{
		if(mode == MODE_GATE)
			SetData(nullptr, 0);
		//else if latch keep existing data
		return;
	}

	//Not gating, echo input to output
	auto cap = SetupEmptyUniformAnalogOutputWaveform(udin, 0);
	cap->m_timescale = udin->m_timescale;
	cap->m_startTimestamp = udin->m_startTimestamp;
	cap->m_startFemtoseconds = udin->m_startFemtoseconds;
	cap->m_triggerPhase = udin->m_triggerPhase;
	cap->m_flags = udin->m_flags;
	cap->m_revision ++;
	cap->m_samples.CopyFrom(udin->m_samples);
}
