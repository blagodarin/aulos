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

#pragma once

#include <cassert>
#include <cmath>

namespace aulos
{
	// A wave period is represented by two stages.
	// The first stage (+1) starts at maximum amplitude and advances towards the minimum,
	// and the second stage (-1) starts at minimum amplitude and advances towards the maximum.
	// Oscillator holds the state of a current stage and controls switching between stages.
	class Oscillator
	{
	public:
		constexpr explicit Oscillator(unsigned samplingRate) noexcept
			: _samplingRate{ samplingRate }
		{
		}

		constexpr void advance(unsigned samples, float nextFrequency, float nextAsymmetry) noexcept
		{
			auto remaining = _stageRemainder - samples;
			assert(remaining > -1);
			while (remaining <= 0)
			{
				_amplitudeSign = -_amplitudeSign;
				resetStage(nextFrequency, nextAsymmetry);
				remaining += _stageLength;
			}
			_stageRemainder = remaining;
		}

		auto maxAdvance() const noexcept
		{
			return static_cast<unsigned>(std::ceil(_stageRemainder));
		}

		constexpr auto samplingRate() const noexcept
		{
			return _samplingRate;
		}

		constexpr auto stageLength() const noexcept
		{
			return _stageLength;
		}

		constexpr auto stageOffset() const noexcept
		{
			return _stageLength - _stageRemainder;
		}

		constexpr auto stageSign() const noexcept
		{
			return _amplitudeSign;
		}

		constexpr void start(float frequency, float asymmetry, bool fromCurrent) noexcept
		{
			assert(!fromCurrent || _stageRemainder > 0);
			if (!fromCurrent)
				_amplitudeSign = 1;
			const auto ratio = fromCurrent ? _stageRemainder / _stageLength : 1;
			resetStage(frequency, asymmetry);
			_stageRemainder = _stageLength * ratio;
		}

	private:
		constexpr void resetStage(float frequency, float asymmetry) noexcept
		{
			assert(frequency > 0);
			assert(asymmetry >= -1 && asymmetry <= 1);
			auto orientedAsymmetry = _amplitudeSign * asymmetry;
			if (orientedAsymmetry == -1)
			{
				_amplitudeSign = -_amplitudeSign;
				orientedAsymmetry = 1;
			}
			const auto partLength = _samplingRate * (1 + orientedAsymmetry) / (2 * frequency);
			assert(partLength > 0);
			_stageLength = partLength;
		}

	private:
		const unsigned _samplingRate;
		float _stageLength = 0;
		float _stageRemainder = 0;
		float _amplitudeSign = 1;
	};
}
