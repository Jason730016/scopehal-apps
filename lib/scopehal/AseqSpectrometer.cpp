/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal                                                                                                          *
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

/**
	@file
	@author Andrew D. Zonenberg
	@brief Implementation of AseqSpectrometer
	@ingroup spectrometerdrivers
 */

#ifdef _WIN32
#include <chrono>
#include <thread>
#endif

#include "scopehal.h"
#include "AseqSpectrometer.h"
#include "EdgeTrigger.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Construction / destruction

/**
	@brief Initialize the driver

	@param transport	SCPITransport pointing to a scopehal-aseq-bridge instance
 */
AseqSpectrometer::AseqSpectrometer(SCPITransport* transport)
	: SCPIDevice(transport)
	, SCPIInstrument(transport)
	, m_triggerArmed(false)
	, m_triggerOneShot(false)
{
	//Create the output channel
	auto chan = new AseqSpectrometerChannel(
		this,
		"Spectrum",
		"#e040e0",
		CHAN_SPECTRUM);
	m_channels.push_back(chan);

	//default to reasonable full scale range
	chan->SetVoltageRange(30000, 0);
	chan->SetOffset(-15000, 0);

	chan->SetVoltageRange(30000, 1);
	chan->SetOffset(-15000, 1);

	//Dark frame input
	m_darkframe = new SpectrometerDarkFrameChannel(
		this,
		"Dark Frame",
		"#808080",
		CHAN_DARKFRAME);
	m_channels.push_back(m_darkframe);

	/*
	//Add the external trigger input
	m_extTrigChannel =
		new OscilloscopeChannel(
		this,
		"EX",
		"#808080",
		Unit(Unit::UNIT_FS),
		Unit(Unit::UNIT_COUNTS),
		Stream::STREAM_TYPE_TRIGGER,
		m_channels.size());
	m_channels.push_back(m_extTrigChannel);
	m_extTrigChannel->SetDefaultDisplayName();

	//Configure the trigger
	auto trig = new EdgeTrigger(this);
	trig->SetType(EdgeTrigger::EDGE_RISING);
	trig->SetLevel(0);
	trig->SetInput(0, StreamDescriptor(GetOscilloscopeChannel(0)));
	SetTrigger(trig);
	PushTrigger();
	SetTriggerOffset(10 * 1000L * 1000L);
	*/

	//Figure out the set of wavelengths the spectrometer supports
	//This is going to be inverted, highest wavelength at the lowest pixel index
	size_t npoints = stoi(m_transport->SendCommandQueuedWithReply("POINTS?"));
	auto wavelengths = explode(m_transport->SendCommandQueuedWithReply("WAVELENGTHS?"), ',');
	if(wavelengths.size() < npoints)
		LogFatal("not enough wavelength data\n");
	for(size_t i=0; i<npoints; i++)
		m_wavelengths.push_back(stof(wavelengths[i]) * 1e3);

	//Flatness calibration data
	auto flatcal = explode(m_transport->SendCommandQueuedWithReply("FLATCAL?"), ',');
	if(flatcal.size() < npoints)
		LogFatal("not enough flatcal data\n");
	for(size_t i=0; i<npoints; i++)
		m_flatcal.push_back(stof(flatcal[i]));

	//Absolute irradiance cal
	m_irrcoeff = stof(m_transport->SendCommandQueuedWithReply("IRRCOEFF?"));
	auto irrcal = explode(m_transport->SendCommandQueuedWithReply("IRRCAL?"), ',');
	if(irrcal.size() < npoints)
		LogFatal("not enough irrcal data\n");
	for(size_t i=0; i<npoints; i++)
		m_irrcal.push_back(stof(irrcal[i]));

	//Default to 125ms exposure
	SetIntegrationTime(FS_PER_SECOND * 125e-3);
}

AseqSpectrometer::~AseqSpectrometer()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Accessors

unsigned int AseqSpectrometer::GetInstrumentTypes() const
{
	return Instrument::INST_OSCILLOSCOPE;
}

uint32_t AseqSpectrometer::GetInstrumentTypesForChannel(size_t /*i*/) const
{
	return Instrument::INST_OSCILLOSCOPE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Device interface functions

int64_t AseqSpectrometer::GetIntegrationTime()
{
	return m_integrationTime;
}

void AseqSpectrometer::SetIntegrationTime(int64_t t)
{
	m_integrationTime = t;
	m_transport->SendCommandQueued(string("EXPOSURE ") + to_string(t));
}

vector<uint64_t> AseqSpectrometer::GetSampleDepthsNonInterleaved()
{
	vector<uint64_t> ret;
	ret.push_back(GetSampleDepth());
	return ret;
}

OscilloscopeChannel* AseqSpectrometer::GetExternalTrigger()
{
	return nullptr;
}

uint64_t AseqSpectrometer::GetSampleRate()
{
	return 1;
}

uint64_t AseqSpectrometer::GetSampleDepth()
{
	return m_wavelengths.size();
}

void AseqSpectrometer::SetSampleDepth(uint64_t /*depth*/)
{
}

void AseqSpectrometer::SetSampleRate(uint64_t /*rate*/)
{
}

void AseqSpectrometer::Start()
{
	m_transport->SendCommandQueued("START");
	m_transport->FlushCommandQueue();

	m_triggerArmed = true;
	m_triggerOneShot = false;
}

void AseqSpectrometer::StartSingleTrigger()
{
	m_transport->SendCommandQueued("SINGLE");
	m_transport->FlushCommandQueue();

	m_triggerArmed = true;
	m_triggerOneShot = true;
}

void AseqSpectrometer::Stop()
{
	m_transport->SendCommandQueued("STOP");
	m_transport->FlushCommandQueue();

	m_triggerArmed = false;
}

void AseqSpectrometer::ForceTrigger()
{
	m_transport->SendCommandQueued("FORCE");
	m_transport->FlushCommandQueue();

	m_triggerArmed = true;
	m_triggerOneShot = true;
}

string AseqSpectrometer::GetDriverNameInternal()
{
	return "aseq";
}

void AseqSpectrometer::FlushConfigCache()
{
	lock_guard<recursive_mutex> lock(m_cacheMutex);
}

void AseqSpectrometer::PushTrigger()
{
}

void AseqSpectrometer::PullTrigger()
{
}

bool AseqSpectrometer::IsTriggerArmed()
{
	return m_triggerArmed;
}

Oscilloscope::TriggerMode AseqSpectrometer::PollTrigger()
{
	m_transport->FlushCommandQueue();

	//Always report "triggered" so we can block on AcquireData() in ScopeThread
	//TODO: peek function of some sort?
	return TRIGGER_MODE_TRIGGERED;
}

bool AseqSpectrometer::AcquireData()
{
	//Read the blob
	size_t npoints = m_wavelengths.size();
	float* buf = new float[npoints];

	//Pull the data from the server
	if(!m_transport->ReadRawData(npoints * sizeof(float), (uint8_t*)buf))
		return false;

	//Flip the samples around so the lowest wavelength is at the left, then display as a sparse waveform
	double t = GetTime();
	int64_t fs = (t - floor(t)) * FS_PER_SECOND;
	auto rawcap = new SparseAnalogWaveform;
	rawcap->m_timescale = 1;
	rawcap->m_triggerPhase = 0;
	rawcap->m_startTimestamp = floor(t);
	rawcap->m_startFemtoseconds = fs;
	rawcap->Resize(npoints);

	size_t last = npoints-1;
	for(size_t i=0; i<npoints; i++)
	{
		rawcap->m_offsets[i] = m_wavelengths[last - i];

		if(i+1 < npoints)
			rawcap->m_durations[i] = m_wavelengths[last - (i+1)] - m_wavelengths[last - i];
		else
			rawcap->m_durations[i] = 0;

		rawcap->m_samples[i] = buf[last - i];
	}
	rawcap->MarkModifiedFromCpu();

	//We always have raw count data
	SequenceSet s;
	s[StreamDescriptor(GetOscilloscopeChannel(CHAN_SPECTRUM),
		AseqSpectrometerChannel::STREAM_RAW_COUNTS)] = rawcap;

	//Given raw counts, apply dark frame correction and flatness correction coefficients
	//(make sure to invert the ordering as well)
	auto darkframe = m_darkframe->GetInput(0);
	auto darkcap = dynamic_cast<SparseAnalogWaveform*>(darkframe.GetData());
	if(darkcap)
	{
		auto flatcap = new SparseAnalogWaveform;
		flatcap->m_timescale = 1;
		flatcap->m_triggerPhase = 0;
		flatcap->m_startTimestamp = rawcap->m_startTimestamp;
		flatcap->m_startFemtoseconds = fs;
		flatcap->Resize(npoints);

		for(size_t i=0; i<npoints; i++)
		{
			flatcap->m_samples[i] = (rawcap->m_samples[i] - darkcap->m_samples[i]) / m_flatcal[last - i];

			flatcap->m_durations[i] = rawcap->m_durations[i];
			flatcap->m_offsets[i] = rawcap->m_offsets[i];
		}
		flatcap->MarkModifiedFromCpu();

		s[StreamDescriptor(GetOscilloscopeChannel(CHAN_SPECTRUM),
			AseqSpectrometerChannel::STREAM_FLATTENED_COUNTS)] = flatcap;

		//Apply absolute irradiance calibration
		//TODO: only if spectrometer has this data
		auto irrcap = new SparseAnalogWaveform;
		irrcap->m_timescale = 1;
		irrcap->m_triggerPhase = 0;
		irrcap->m_startTimestamp = rawcap->m_startTimestamp;
		irrcap->m_startFemtoseconds = fs;
		irrcap->Resize(npoints);
		for(size_t i=0; i<npoints; i++)
		{
			//Relative irradiance
			float relirr = flatcap->m_samples[i] * m_irrcal[last - i];

			//Absolute irradiance
			float exposureMicroseconds = GetIntegrationTime() / FS_PER_MICROSECOND;
			float absirr = relirr / (exposureMicroseconds * 10 * m_irrcoeff);

			irrcap->m_samples[i] = absirr;

			irrcap->m_durations[i] = rawcap->m_durations[i];
			irrcap->m_offsets[i] = rawcap->m_offsets[i];
		}
		irrcap->MarkModifiedFromCpu();

		s[StreamDescriptor(GetOscilloscopeChannel(CHAN_SPECTRUM),
			AseqSpectrometerChannel::STREAM_ABSOLUTE_IRRADIANCE)] = irrcap;

	}

	m_channels[0]->SetYAxisUnits(Unit::UNIT_W_M2_NM, AseqSpectrometerChannel::STREAM_ABSOLUTE_IRRADIANCE);

	//Save the waveforms to our queue
	m_pendingWaveformsMutex.lock();
	m_pendingWaveforms.push_back(s);
	m_pendingWaveformsMutex.unlock();

	//Done, clean up
	delete[] buf;

	//If this was a one-shot trigger we're no longer armed
	if(m_triggerOneShot)
		m_triggerArmed = false;

	return true;
}
