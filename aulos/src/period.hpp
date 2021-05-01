// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "shaper.hpp"

namespace aulos
{
	// A wave period consists of two parts:
	//
	// 0                  S/F
	// +~~~~~~~~~~~~~~~~~~~+> periodLength
	// :                   :
	// :---------*---------:---------*---------> +amplitude
	// :       / :\        :        / \
	// : (1) /   : \       :   (1) /   \
	// :   /     :  \      :      /     \
	// : /       :   \     :     /       \
	// *---------:----\----:----/---------\----> 0
	// :         :     \   :   /           \
	// :         :      \  :  /             \
	// :         :   (2) \ : /           (2) \
	// :         :        \:/                 \
	// :---------:---------*-------------------* -amplitude
	//           :         :
	//           +~~~~~~~~~+> asymmetry
	//           0         1
	//
	// The first part of a period (+1) starts at minimum amplitude of the previous period and advances towards the maximum.
	// The second part of a period (-1) starts at maximum amplitude and advances towards the minimum.
	class WavePeriod
	{
	public:
		constexpr bool advance(float samples) noexcept
		{
			_currentRemaining -= samples;
			assert(_currentRemaining > -1);
			if (_currentRemaining > 0)
				return true;
			if (_nextLength == 0)
				return false;
			assert(_rightAmplitude > 0);
			_currentLength = _nextLength;
			_leftAmplitude = _rightAmplitude;
			_rightAmplitude = -_rightAmplitude;
			_nextLength = 0;
			_currentRemaining += _currentLength;
			return _currentRemaining > 0;
		}

		auto maxAdvance() const noexcept
		{
			return static_cast<unsigned>(std::ceil(_currentRemaining));
		}

		constexpr ShaperData currentShaperData(float oscillation, float shapeParameter) const noexcept
		{
			assert(oscillation >= 0 && oscillation <= 1);
			const auto deltaY = (_rightAmplitude - _leftAmplitude) * (1 - oscillation);
			return {
				_rightAmplitude - deltaY,
				deltaY,
				_currentLength,
				shapeParameter,
				_currentLength - _currentRemaining,
			};
		}

		constexpr void startFirst(float periodLength, float amplitude, float asymmetry, bool fromCurrent) noexcept
		{
			assert(periodLength > 0);
			assert(amplitude > 0);
			assert(asymmetry >= 0 && asymmetry <= 1);
			const auto firstPartLength = periodLength * (1 + asymmetry) / 2;
			const auto secondPartLength = periodLength - firstPartLength;
			if (!fromCurrent)
			{
				_currentLength = firstPartLength;
				_leftAmplitude = 0;
				_rightAmplitude = amplitude;
				_nextLength = secondPartLength;
				_currentRemaining = _currentLength;
			}
			else
			{
				assert(_currentRemaining > 0);
				const auto remainingRatio = _currentRemaining / _currentLength;
				if (_rightAmplitude > 0)
				{
					// Ascending part.
					_currentLength = firstPartLength;
					_leftAmplitude = _leftAmplitude < 0 ? -amplitude : 0;
					_rightAmplitude = amplitude;
					_nextLength = secondPartLength;
				}
				else
				{
					// Descending part.
					assert(_nextLength == 0);
					_currentLength = secondPartLength;
					_leftAmplitude = amplitude;
					_rightAmplitude = -amplitude;
				}
				_currentRemaining = _currentLength * remainingRatio;
			}
		}

		void startNext(float periodLength, float amplitude, float asymmetry) noexcept
		{
			assert(periodLength > 0);
			assert(amplitude >= 0);
			assert(asymmetry >= 0 && asymmetry <= 1);
			assert(_nextLength == 0);
			assert(_currentRemaining > -1 && _currentRemaining <= 0);
			const auto firstPartLength = periodLength * (1 + asymmetry) / 2;
			const auto secondPartLength = periodLength - firstPartLength;
			for (;;)
			{
				_currentRemaining += firstPartLength;
				if (_currentRemaining > 0)
				{
					_currentLength = firstPartLength;
					_leftAmplitude = -std::abs(_rightAmplitude);
					_rightAmplitude = amplitude;
					_nextLength = amplitude > 0 ? secondPartLength : 0;
					break;
				}
				_currentRemaining += secondPartLength;
				if (_currentRemaining > 0)
				{
					_currentLength = secondPartLength;
					_leftAmplitude = amplitude;
					_rightAmplitude = -amplitude;
					_nextLength = 0;
					break;
				}
			}
		}

	private:
		float _currentLength = 0;
		float _leftAmplitude = 0;
		float _rightAmplitude = 1;
		float _nextLength = 0;
		float _currentRemaining = 0;
	};
}
