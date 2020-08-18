// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

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
			: _samplingRate{ static_cast<float>(samplingRate) }
		{
		}

		constexpr void advance(float samples, float nextFrequency, float nextAsymmetry) noexcept
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
		const float _samplingRate;
		float _stageLength = 0;
		float _stageRemainder = 0;
		float _amplitudeSign = 1;
	};
}
