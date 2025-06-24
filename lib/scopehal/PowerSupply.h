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

#ifndef PowerSupply_h
#define PowerSupply_h

/**
	@brief A generic power supply
 */
class PowerSupply : public virtual Instrument
{
public:
	PowerSupply();
	virtual ~PowerSupply();

	virtual unsigned int GetInstrumentTypes() const override;

	//Device capabilities
	/**
		@brief Determines if the power supply supports soft start

		If this function returns false, IsSoftStartEnabled() will always return false,
		and SetSoftStartEnabled() is a no-op.
	 */
	virtual bool SupportsSoftStart();

	/**
		@brief Determines if the power supply supports switching individual output channels

		If this function returns false, GetPowerChannelActive() will always return true,
		and SetPowerChannelActive() is a no-op.
	 */
	virtual bool SupportsIndividualOutputSwitching();

	/**
		@brief Determines if the power supply supports ganged master switching of all outputs

		If this function returns false, GetMasterPowerEnable() will always return true,
		and SetMasterPowerEnable() is a no-op.
	 */
	virtual bool SupportsMasterOutputSwitching();

	/**
		@brief Determines if the power supply supports shutdown rather than constant-current mode on overcurrent

		If this function returns false, GetPowerOvercurrentShutdownEnabled() and GetPowerOvercurrentShutdownTripped() will always return false,
		and SetPowerOvercurrentShutdownEnabled() is a no-op.
	 */
	virtual bool SupportsOvercurrentShutdown();

	/**
		@brief Determines if the power supply supports voltage/current control for the given channel.

		If this function returns false, GetPowerVoltage* and GetPowerCurrent* will always return zero,
		and SetPowerCurrent*, and SetPowerVoltage* are no-ops.
	 */
	virtual bool SupportsVoltageCurrentControl(int chan);

	virtual bool AcquireData() override;

	//Read sensors
	virtual double GetPowerVoltageActual(int chan) =0;				//actual voltage after current limiting
	virtual double GetPowerVoltageNominal(int chan) =0;				//set point
	virtual double GetPowerCurrentActual(int chan) =0;				//actual current drawn by the load
	virtual double GetPowerCurrentNominal(int chan) =0;				//current limit
	virtual bool GetPowerChannelActive(int chan);

	//Configuration
	virtual bool GetPowerOvercurrentShutdownEnabled(int chan);	//shut channel off entirely on overload,
																//rather than current limiting
	virtual void SetPowerOvercurrentShutdownEnabled(int chan, bool enable);
	virtual bool GetPowerOvercurrentShutdownTripped(int chan);
	virtual void SetPowerVoltage(int chan, double volts) =0;
	virtual void SetPowerCurrent(int chan, double amps) =0;
	virtual void SetPowerChannelActive(int chan, bool on);

	virtual bool IsPowerConstantCurrent(int chan) =0;				//true = CC, false = CV

	virtual bool GetMasterPowerEnable();
	virtual void SetMasterPowerEnable(bool enable);

	//Soft start
	virtual bool IsSoftStartEnabled(int chan);
	virtual void SetSoftStartEnabled(int chan, bool enable);

	/**
		@brief Gets the ramp time for use with soft-start mode

		@param chan	Channel index

		@return	Ramp time, in femtoseconds
	 */
	virtual int64_t GetSoftStartRampTime(int chan);

	/**
		@brief Sets the ramp time for use with soft-start mode

		@param chan	Channel index
		@param time	Ramp time, in femtoseconds
	 */
	virtual void SetSoftStartRampTime(int chan, int64_t time);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Serialization

protected:
	/**
		@brief Serializes this oscilloscope's configuration to a YAML node.
	 */
	void DoSerializeConfiguration(YAML::Node& node, IDTable& table);

	/**
		@brief Load instrument and channel configuration from a save file
	 */
	void DoLoadConfiguration(int version, const YAML::Node& node, IDTable& idmap);

	/**
		@brief Validate instrument and channel configuration from a save file
	 */
	void DoPreLoadConfiguration(int version, const YAML::Node& node, IDTable& idmap, ConfigWarningList& list);
};

#endif
