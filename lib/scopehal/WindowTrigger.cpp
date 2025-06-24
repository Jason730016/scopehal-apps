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
	@brief Implementation of WindowTrigger
	@ingroup triggers
 */

#include "scopehal.h"
#include "WindowTrigger.h"
#include "TektronixOscilloscope.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

/**
	@brief Creates a new window trigger

	@param scope	The oscilloscope the trigger is going to be used with
 */
WindowTrigger::WindowTrigger(Oscilloscope* scope)
	: TwoLevelTrigger(scope)
	, m_width(m_parameters["Time Limit"])
	, m_crossingType(m_parameters["Edge"])
	, m_windowType(m_parameters["Condition"])
{
	CreateInput("din");

	if(dynamic_cast<TektronixOscilloscope*>(scope))
	{
		m_width = FilterParameter(FilterParameter::TYPE_INT, Unit(Unit::UNIT_FS));

		m_crossingType = FilterParameter(FilterParameter::TYPE_ENUM, Unit(Unit::UNIT_COUNTS));
		m_crossingType.AddEnumValue("Upper", CROSS_UPPER);
		m_crossingType.AddEnumValue("Lower", CROSS_LOWER);
		m_crossingType.AddEnumValue("Either", CROSS_EITHER);
		m_crossingType.AddEnumValue("None", CROSS_NONE);

		m_windowType = FilterParameter(FilterParameter::TYPE_ENUM, Unit(Unit::UNIT_COUNTS));
		m_windowType.AddEnumValue("Enter", WINDOW_ENTER);
		m_windowType.AddEnumValue("Exit", WINDOW_EXIT);
		m_windowType.AddEnumValue("Exit (timed)", WINDOW_EXIT_TIMED);
		m_windowType.AddEnumValue("Enter (timed)", WINDOW_ENTER_TIMED);
	}

	//Mark the extra parameters as hidden
	else
	{
		m_width.MarkHidden();
	}
}

WindowTrigger::~WindowTrigger()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

///@brief Return the constant trigger name "Window"
string WindowTrigger::GetTriggerName()
{
	return "Window";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input validation

bool WindowTrigger::ValidateChannel(size_t i, StreamDescriptor stream)
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

	//It has to be analog or external trigger, digital inputs make no sense
	if( (stream.GetType() != Stream::STREAM_TYPE_ANALOG) && (stream.GetType() != Stream::STREAM_TYPE_TRIGGER) )
		return false;

	return true;
}
