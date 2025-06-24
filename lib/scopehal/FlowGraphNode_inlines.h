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
	@brief Inline functions for FlowGraphNode which reference OscilloscopeChannel class
	@ingroup core

	Since OscilloscopeChannel is derived from FlowGraphNode, FlowGraphNode.h cannot include inline functions which
	rely on the full class definition.
 */
#ifndef FlowGraphNode_inlines_h
#define FlowGraphNode_inlines_h

/**
	@brief Gets the waveform attached to the specified input.

	This function is safe to call on a null input and will return null in that case.

	@param i	Input index

	@return		Data from the channel if applicable, null if no data or no input connected
 */
inline WaveformBase* FlowGraphNode::GetInputWaveform(size_t i)
{
	auto chan = m_inputs[i].m_channel;
	if(chan == nullptr)
		return nullptr;
	return chan->GetData(m_inputs[i].m_stream);
}

/**
	@brief Get the type of stream (if connected). Returns STREAM_TYPE_ANALOG if null.
 */
inline Stream::StreamType StreamDescriptor::GetType()
{
	if(m_channel == nullptr)
		return Stream::STREAM_TYPE_ANALOG;
	return m_channel->GetType(m_stream);
}

#endif
