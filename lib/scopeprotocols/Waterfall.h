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

/**
	@file
	@author Andrew D. Zonenberg
	@brief Declaration of Waterfall
 */
#ifndef Waterfall_h
#define Waterfall_h

#include "../scopehal/DensityFunctionWaveform.h"

struct WaterfallFilterArgs
{
	uint32_t width;
	uint32_t height;
	uint32_t inlen;
	float vrange;
	float vfs;
	float timescaleRatio;
};

class WaterfallWaveform : public DensityFunctionWaveform
{
public:
	WaterfallWaveform(size_t width, size_t height);
	virtual ~WaterfallWaveform();

	//not copyable or assignable
	WaterfallWaveform(const WaterfallWaveform&) =delete;
	WaterfallWaveform& operator=(const WaterfallWaveform&) =delete;

	virtual void FreeGpuMemory() override
	{}

	virtual bool HasGpuBuffer() override
	{ return false; }

	AcceleratorBuffer<float> m_tempBuf;
};

class Waterfall : public Filter
{
public:
	Waterfall(const std::string& color);

	//not copyable or assignable
	Waterfall(const Waterfall&) =delete;
	Waterfall& operator=(const Waterfall&) =delete;

	virtual void Refresh(vk::raii::CommandBuffer& cmdBuf, std::shared_ptr<QueueHandle> queue) override;

	static std::string GetProtocolName();

	virtual float GetVoltageRange(size_t stream) override;
	virtual float GetOffset(size_t stream) override;
	virtual bool ValidateChannel(size_t i, StreamDescriptor stream) override;
	virtual void ClearSweeps() override;

	void SetWidth(size_t width)
	{ m_width = width; }

	void SetHeight(size_t height)
	{ m_height = height; }

	size_t GetWidth()
	{ return m_width; }

	size_t GetHeight()
	{ return m_height; }

	PROTOCOL_DECODER_INITPROC(Waterfall)

protected:
	double m_offsetHz;

	size_t m_width;
	size_t m_height;

	std::string m_maxwidth;

	ComputePipeline m_computePipeline;
};

#endif
