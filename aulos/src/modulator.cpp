//
// This file is part of the Aulos toolkit.
// Copyright (C) 2020 Sergei Blagodarin.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "modulator.hpp"

namespace aulos
{
	ModulationData::ModulationData(const VoiceData& voice, unsigned samplingRate) noexcept
		: _amplitudeEnvelope{ voice._amplitudeEnvelope, samplingRate }
		, _frequencyEnvelope{ voice._frequencyEnvelope, samplingRate }
		, _asymmetryEnvelope{ voice._asymmetryEnvelope, samplingRate }
		, _oscillationEnvelope{ voice._oscillationEnvelope, samplingRate }
	{
	}

	Modulator::Modulator(const ModulationData& data) noexcept
		: _amplitudeModulator{ data._amplitudeEnvelope }
		, _frequencyModulator{ data._frequencyEnvelope }
		, _asymmetryModulator{ data._asymmetryEnvelope }
		, _oscillationModulator{ data._oscillationEnvelope }
	{
	}

	void Modulator::advance(unsigned samples) noexcept
	{
		_amplitudeModulator.advance(samples);
		_frequencyModulator.advance(samples);
		_asymmetryModulator.advance(samples);
		_oscillationModulator.advance(samples);
	}

	void Modulator::start() noexcept
	{
		_amplitudeModulator.start(!_amplitudeModulator.stopped());
		_frequencyModulator.start(false);
		_asymmetryModulator.start(false);
		_oscillationModulator.start(false);
	}
}
