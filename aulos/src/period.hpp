// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "shaper.hpp"

namespace aulos
{
	// A wave period consists of two parts.
	// The first part starts at minimum amplitude of the previous period and advances towards the maximum.
	// The second part starts at maximum amplitude and advances towards the minimum.
	//
	// 0                  S/F
	// +~~~~~~~~~~~~~~~~~~~+> periodLength
	// :                   :
	// :---------*---------:---------*---------> +amplitude
	// :       / :\        :        / \        :
	// : (1) /   : \       :   (1) /   \       :
	// :   /     :  \      :      /     \      :
	// : /       :   \     :     /       \     :
	// *---------:----\----:----/---------\----> 0
	// :         :     \   :   /           \   :
	// :         :      \  :  /             \  :
	// :         :   (2) \ : /           (2) \ :
	// :         :        \:/                 \:
	// :---------:---------*-------------------* -amplitude
	//           :         :
	//           +~~~~~~~~~+> asymmetry
	//           0         1
	//
	class WavePeriod
	{
	public:
		constexpr void advance(float samples) noexcept
		{
			assert(_currentRemaining - samples > -1);
			_currentRemaining -= samples;
			if (_currentRemaining > 0)
				return;
			if (_nextLength == 0)
				return;
			assert(_rightAmplitude > 0);
			_currentLength = _nextLength;
			_leftAmplitude = _rightAmplitude;
			_rightAmplitude = -_rightAmplitude;
			_nextLength = 0;
			_currentRemaining += _currentLength;
		}

		[[nodiscard]] constexpr ShaperData currentShaperData(float oscillation, float shapeParameter) const noexcept
		{
			assert(oscillation >= 0 && oscillation <= 1);
			assert(_currentLength > 0); // Otherwise the shaper will produce garbage.
			const auto deltaY = (_rightAmplitude - _leftAmplitude) * (1 - oscillation);
			return {
				_rightAmplitude - deltaY,
				deltaY,
				_currentLength,
				shapeParameter,
				_currentLength - _currentRemaining,
			};
		}

		[[nodiscard]] constexpr auto maxAdvance() const noexcept
		{
			return _currentRemaining;
		}

		void start(float periodLength, float amplitude, float asymmetry, bool stop) noexcept
		{
			assert(periodLength > 0);
			assert(amplitude > 0);
			assert(asymmetry >= 0 && asymmetry <= 1);
			assert(stopped());
			const auto firstPartLength = periodLength * (1 + asymmetry) / 2;
			const auto secondPartLength = periodLength - firstPartLength;
			for (;;)
			{
				_currentRemaining += firstPartLength;
				if (_currentRemaining > 0)
				{
					_currentLength = firstPartLength;
					_leftAmplitude = -std::abs(_rightAmplitude);
					_rightAmplitude = stop ? 0.f : amplitude;
					_nextLength = stop ? 0.f : secondPartLength;
					break;
				}
				_currentRemaining += secondPartLength;
				if (_currentRemaining > 0)
				{
					_currentLength = secondPartLength;
					_leftAmplitude = amplitude;
					_rightAmplitude = stop ? 0.f : -amplitude;
					_nextLength = 0;
					break;
				}
			}
		}

		[[nodiscard]] constexpr bool stopped() const noexcept
		{
			return _nextLength == 0 && _currentRemaining > -1 && _currentRemaining <= 0;
		}

	private:
		float _currentLength = 1;
		float _leftAmplitude = 0;
		float _rightAmplitude = 0;
		float _nextLength = 0;
		float _currentRemaining = 0;
	};
}
