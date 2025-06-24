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
	@brief Implementation of BufferedSwitchMatrixOutputChannel
	@ingroup core
 */

#include "scopehal.h"
#include "BufferedSwitchMatrixOutputChannel.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

/**
	@brief Initialize the output channel

	@param hwname	Hardware name of the channel
	@param parent	Switch matrix the channel is part of
	@param color	Initial display color of the channel
	@param index	Number of the channel
 */
BufferedSwitchMatrixOutputChannel::BufferedSwitchMatrixOutputChannel(
	const string& hwname,
	SwitchMatrix* parent,
	const string& color,
	size_t index)
	: DigitalOutputChannel(hwname, parent, color, index)
{
}

BufferedSwitchMatrixOutputChannel::~BufferedSwitchMatrixOutputChannel()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input management

bool BufferedSwitchMatrixOutputChannel::ValidateChannel(size_t i, StreamDescriptor stream)
{
	//Must be channel 0
	if(i > 0)
		return false;

	//Digital input of same instrument? It's good
	auto din = dynamic_cast<DigitalInputChannel*>(stream.m_channel);
	if(din && (din->GetInstrument() == m_instrument) )
		return true;

	//Digital I/O of same instrument? It's good
	auto dio = dynamic_cast<DigitalIOChannel*>(stream.m_channel);
	if(dio && (dio->GetInstrument() == m_instrument) )
		return true;

	//Nope, doesn't work
	return false;
}

void BufferedSwitchMatrixOutputChannel::OnInputChanged(size_t i)
{
	//no null check needed because constructor takes a SwitchMatrix*
	//but the base class variable is an Instrument*
	auto parent = dynamic_cast<SwitchMatrix*>(m_instrument);

	//get the input channel and set the path appropriately
	auto din = dynamic_cast<DigitalInputChannel*>(GetInput(i).m_channel);
	auto dio = dynamic_cast<DigitalIOChannel*>(GetInput(i).m_channel);
	if(dio)
		parent->SetMuxPath(GetIndex(), dio->GetIndex());
	else if(din)
		parent->SetMuxPath(GetIndex(), din->GetIndex());
	else
		parent->SetMuxPathOpen(GetIndex());
}
