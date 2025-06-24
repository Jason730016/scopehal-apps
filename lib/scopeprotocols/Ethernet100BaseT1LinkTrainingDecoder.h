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
	@brief Declaration of Ethernet100BaseT1LinkTrainingDecoder
 */
#ifndef Ethernet100BaseT1LinkTrainingDecoder_h
#define Ethernet100BaseT1LinkTrainingDecoder_h

#include "Ethernet100BaseT1Decoder.h"

class Ethernet100BaseT1LinkTrainingSymbol
{
public:

	enum type_t
	{
		TYPE_SEND_Z,
		TYPE_SEND_I_UNLOCKED,
		TYPE_SEND_I_LOCKED,
		TYPE_SEND_N,
		TYPE_ERROR,
	} m_type;

	Ethernet100BaseT1LinkTrainingSymbol()
	{}

	Ethernet100BaseT1LinkTrainingSymbol(type_t type)
	 : m_type(type)
	{}

	bool operator== (const Ethernet100BaseT1LinkTrainingSymbol& s) const
	{
		return (m_type == s.m_type);
	}
};

class Ethernet100BaseT1LinkTrainingWaveform : public SparseWaveform<Ethernet100BaseT1LinkTrainingSymbol>
{
public:
	Ethernet100BaseT1LinkTrainingWaveform () : SparseWaveform<Ethernet100BaseT1LinkTrainingSymbol>() {};
	virtual std::string GetText(size_t) override;
	virtual std::string GetColor(size_t) override;
};

class Ethernet100BaseT1LinkTrainingDecoder : public Filter
{
public:
	Ethernet100BaseT1LinkTrainingDecoder(const std::string& color);
	virtual ~Ethernet100BaseT1LinkTrainingDecoder();

	virtual void Refresh(vk::raii::CommandBuffer& cmdBuf, std::shared_ptr<QueueHandle> queue) override;

	static std::string GetProtocolName();

	virtual bool ValidateChannel(size_t i, StreamDescriptor stream) override;

	PROTOCOL_DECODER_INITPROC(Ethernet100BaseT1LinkTrainingDecoder)

protected:
	std::string m_scrambler;
};

#endif
