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
	@author Mike Walters
	@brief Implementation of NthEdgeBurstTrigger
	@ingroup triggers
 */

#include "scopehal.h"
#include "NthEdgeBurstTrigger.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

/**
	@brief Initialize the trigger

	@param scope	The instrument this trigger is being created for
 */
NthEdgeBurstTrigger::NthEdgeBurstTrigger(Oscilloscope* scope)
	: Trigger(scope)
	, m_edgetype(m_parameters["Slope"])
	, m_idletime(m_parameters["Idle time"])
	, m_edgenumber(m_parameters["Edge number"])
{
	CreateInput("din");

	m_edgetype = FilterParameter(FilterParameter::TYPE_ENUM, Unit(Unit::UNIT_COUNTS));
	m_edgetype.AddEnumValue("Rising", EDGE_RISING);
	m_edgetype.AddEnumValue("Falling", EDGE_FALLING);

	m_idletime = FilterParameter(FilterParameter::TYPE_INT, Unit(Unit::UNIT_FS));

	m_edgenumber = FilterParameter(FilterParameter::TYPE_INT, Unit(Unit::UNIT_COUNTS));

}

NthEdgeBurstTrigger::~NthEdgeBurstTrigger()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

///@brief Returns the constant trigger name "Nth Edge Burst"
string NthEdgeBurstTrigger::GetTriggerName()
{
	return "Nth Edge Burst";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input validation

bool NthEdgeBurstTrigger::ValidateChannel(size_t i, StreamDescriptor stream)
{
	//We only can take one input
	if(i > 0)
		return false;

	//There has to be a signal to trigger on
	auto schan = dynamic_cast<OscilloscopeChannel*>(stream.m_channel);
	if(!schan)
		return false;

	//It has to be from the same instrument we're trying to trigger on
	if(schan->GetScope() != m_scope)
		return false;

	return true;
}
