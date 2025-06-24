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
#include "RMSMeasurement.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

RMSMeasurement::RMSMeasurement(const string& color)
	: Filter(color, CAT_MEASUREMENT)
{
	AddStream(Unit(Unit::UNIT_VOLTS), "trend", Stream::STREAM_TYPE_ANALOG);
	AddStream(Unit(Unit::UNIT_VOLTS), "avg", Stream::STREAM_TYPE_ANALOG_SCALAR);

	//Set up channels
	CreateInput("din");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Factory methods

bool RMSMeasurement::ValidateChannel(size_t i, StreamDescriptor stream)
{
	if(stream.m_channel == NULL)
		return false;

	if(i > 0)
		return false;

	if(stream.GetType() == Stream::STREAM_TYPE_ANALOG)
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string RMSMeasurement::GetProtocolName()
{
	return "RMS";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Actual decoder logic

void RMSMeasurement::Refresh()
{
	//Make sure we've got valid inputs
	if(!VerifyAllInputsOK())
	{
		SetData(NULL, 0);
		return;
	}

	auto din = GetInputWaveform(0);
	din->PrepareForCpuAccess();

	auto uadin = dynamic_cast<UniformAnalogWaveform*>(din);
	auto sadin = dynamic_cast<SparseAnalogWaveform*>(din);

	//Copy input unit to output
	SetYAxisUnits(m_inputs[0].GetYAxisUnits(), 0);
	SetYAxisUnits(m_inputs[0].GetYAxisUnits(), 1);
	auto length = din->size();

	//Calculate the global RMS value
	//Sum the squares of all values after subtracting the DC value
	float temp = 0;
	if(uadin)
	{
		for (size_t i = 0; i < length; i++)
			temp += uadin->m_samples[i] * uadin->m_samples[i];
	}
	else if(sadin)
	{
		for (size_t i = 0; i < length; i++)
			temp += sadin->m_samples[i] * sadin->m_samples[i];
	}

	//Divide by total number of samples and take the square root to get the final AC RMS
	m_streams[1].m_value = sqrt(temp / length);

	//Now we can do the cycle-by-cycle value
	temp = 0;
	vector<int64_t> edges;

	//Auto-threshold analog signals at average value
	//TODO: make threshold configurable?
	float threshold = GetAvgVoltage(sadin, uadin);
	if(uadin)
		FindZeroCrossings(uadin, threshold, edges);
	else if(sadin)
		FindZeroCrossings(sadin, threshold, edges);

	//We need at least one full cycle of the waveform to have a meaningful AC RMS Measurement
	if(edges.size() < 2)
	{
		SetData(NULL, 0);
		return;
	}

	//Create the output as a sparse waveform
	auto cap = SetupEmptySparseAnalogOutputWaveform(din, 0, true);
	cap->PrepareForCpuAccess();

	size_t elen = edges.size();

	for(size_t i = 0; i < (elen - 2); i += 2)
	{
		//Measure from edge to 2 edges later, since we find all zero crossings regardless of polarity
		int64_t start = edges[i] / din->m_timescale;
		int64_t end = edges[i + 2] / din->m_timescale;
		int64_t j = 0;

		//Simply sum the squares of all values in a cycle after subtracting the DC value
		if(uadin)
		{
			for(j = start; (j <= end) && (j < (int64_t)length); j++)
				temp += uadin->m_samples[j] * uadin->m_samples[j];
		}
		else if(sadin)
		{
			for(j = start; (j <= end) && (j < (int64_t)length); j++)
				temp += sadin->m_samples[j] * sadin->m_samples[j];
		}

		//Get the difference between the end and start of cycle. This would be the number of samples
		//on which AC RMS calculation was performed
		int64_t delta = j - start - 1;

		if (delta != 0)
		{
			//Divide by total number of samples for one cycle
			temp /= delta;

			//Take square root to get the final AC RMS Value of one cycle
			temp = sqrt(temp);

			//Push values to the waveform
			cap->m_offsets.push_back(start);
			cap->m_durations.push_back(delta);
			cap->m_samples.push_back(temp);
		}
	}

	SetData(cap, 0);
	cap->MarkModifiedFromCpu();
}
