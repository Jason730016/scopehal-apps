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
	@brief Declaration of Load
	@ingroup core
 */

#ifndef Load_h
#define Load_h

/**
	@brief Base class for all electronic load drivers
	@ingroup core
 */
class Load : public virtual Instrument
{
public:
	Load();
	virtual ~Load();

	virtual unsigned int GetInstrumentTypes() const override;

	virtual bool AcquireData() override;

	//New object model does not have explicit query methods for channel properties.
	//Instead, call AcquireData() then read scalar channel state

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Operating modes

	///@brief Operating modes for the load
	enum LoadMode
	{
		///@brief Draw a constant current regardless of supplied voltage
		MODE_CONSTANT_CURRENT,

		///@brief Draw as much current as needed for the input voltage to drop to the specified level
		MODE_CONSTANT_VOLTAGE,

		///@brief Emulate a fixed resistance
		MODE_CONSTANT_RESISTANCE,

		///@brief Draw a constant power regardless of supplied voltage
		MODE_CONSTANT_POWER
	};

	/**
		@brief Returns the operating mode of the load

		@param channel	Index of the channel to query
	 */
	virtual LoadMode GetLoadMode(size_t channel) =0;

	/**
		@brief Sets the operating mode of the load

		@param channel	Channel index
		@param mode		Operating mode
	 */
	virtual void SetLoadMode(size_t channel, LoadMode mode) =0;

	static std::string GetNameOfLoadMode(LoadMode mode);
	static LoadMode GetLoadModeOfName(const std::string& name);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Range selection

	/**
		@brief Returns a sorted list of operating ranges for the load's current scale, in amps

		For example, returning [1, 10] means the load supports one mode with 1A full scale range and one with 10A range.

		@param channel	Index of the channel to query
	 */
	virtual std::vector<float> GetLoadCurrentRanges(size_t channel) =0;

	/**
		@brief Returns the index of the load's selected current range, as returned by GetLoadCurrentRanges()

		@param channel	Index of the channel to query
	 */
	virtual size_t GetLoadCurrentRange(size_t channel) =0;

	/**
		@brief Returns a sorted list of operating ranges for the load's voltage scale, in volts

		For example, returning [10, 250] means the load supports one mode with 10V full scale range and one with
		250V range.

		@param channel	Index of the channel to query
	 */
	virtual std::vector<float> GetLoadVoltageRanges(size_t channel) =0;

	/**
		@brief Returns the index of the load's selected voltage range, as returned by GetLoadVoltageRanges()

		@param channel	Index of the channel to query
	 */
	virtual size_t GetLoadVoltageRange(size_t channel) =0;

	/**
		@brief Select the voltage range to use

		@param channel		Channel index
		@param rangeIndex	Index of the range, as returned by GetLoadVoltageRanges()
	 */
	virtual void SetLoadVoltageRange(size_t channel, size_t rangeIndex) =0;

	/**
		@brief Select the current range to use

		@param channel		Channel index
		@param rangeIndex	Index of the range, as returned by GetLoadCurrentRanges()
	 */
	virtual void SetLoadCurrentRange(size_t channel, size_t rangeIndex) =0;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Channel control

	/**
		@brief Returns true if the load is enabled (sinking power) and false if disabled (no load)

		@param channel	Index of the channel to query
	 */
	virtual bool GetLoadActive(size_t channel) =0;

	/**
		@brief Turns the load on or off

		@param channel	Index of the channel to query
		@param active	True to turn the load on, false to turn it off
	 */
	virtual void SetLoadActive(size_t channel, bool active) =0;

	/**
		@brief Gets the set point for the channel

		Units vary depending on operating mode: amps (CC), volts (CV), ohms (CR), watts (CP).

		@param channel	Index of the channel to query
	 */
	virtual float GetLoadSetPoint(size_t channel) =0;

	/**
		@brief Sets the set point for the channel

		Units vary depending on operating mode: amps (CC), volts (CV), ohms (CR), watts (CP).

		@param channel	Index of the channel to query
		@param target	Desired operating current/voltage/resistance/power depending on the operating mode
	 */
	virtual void SetLoadSetPoint(size_t channel, float target) =0;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Readback
	// Typically called by AcquireData() and cached in the channel object, not used directly by applications

protected:

	/**
		@brief Get the measured voltage of the load (uncached instantaneous measurement)

		@param channel	Index of the channel to query
	 */
	virtual float GetLoadVoltageActual(size_t channel) =0;

	/**
		@brief Get the measured current of the load (uncached instantaneous measurement)

		@param channel	Index of the channel to query
	 */
	virtual float GetLoadCurrentActual(size_t channel) =0;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration storage

protected:
	void DoSerializeConfiguration(YAML::Node& node, IDTable& table);
	void DoLoadConfiguration(int version, const YAML::Node& node, IDTable& idmap);
	void DoPreLoadConfiguration(int version, const YAML::Node& node, IDTable& idmap, ConfigWarningList& list);
};

#endif
