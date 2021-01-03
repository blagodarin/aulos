// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "shaper.hpp"

namespace aulos
{
	// A wave period consists of two parts:
	//
	// *               * +A
	// |\             /|
	// | \ (1)       / |
	// |  \         /  |
	// +---\-------/---+> 0
	// |    \     /    |
	// |     \   / (2) |
	// |      \ /      |
	// |       *       | -A
	//
	// The first part (+1) starts at maximum amplitude and advances towards the minimum.
	// and the second part (-1) starts at minimum amplitude and advances towards the maximum.
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
			assert(_currentAmplitude > 0);
			_currentLength = _nextLength;
			_currentAmplitude = -_currentAmplitude;
			_nextLength = 0;
			_currentRemaining += _currentLength;
			return _currentRemaining > 0;
		}

		auto maxAdvance() const noexcept
		{
			return static_cast<unsigned>(std::ceil(_currentRemaining));
		}

		void restart(float periodLength, float asymmetry) noexcept
		{
			assert(periodLength > 0);
			assert(asymmetry >= 0 && asymmetry <= 1);
			assert(_currentRemaining > -1 && _currentRemaining <= 0 && _nextLength == 0);
			const auto firstPartLength = periodLength * (1 + asymmetry) / 2;
			const auto secondPartLength = periodLength - firstPartLength;
			const auto amplitude = std::abs(_currentAmplitude);
			for (;;)
			{
				_currentRemaining += firstPartLength;
				if (_currentRemaining > 0)
				{
					_currentLength = firstPartLength;
					_currentAmplitude = amplitude;
					_nextLength = secondPartLength;
					break;
				}
				_currentRemaining += secondPartLength;
				if (_currentRemaining > 0)
				{
					_currentLength = secondPartLength;
					_currentAmplitude = -amplitude;
					_nextLength = 0;
					break;
				}
			}
		}

		constexpr ShaperData currentShaperData(float oscillation, float shapeParameter) const noexcept
		{
			assert(oscillation >= 0 && oscillation <= 1);
			return {
				_currentAmplitude,
				-2 * _currentAmplitude * (1 - oscillation),
				_currentLength,
				shapeParameter,
				_currentLength - _currentRemaining,
			};
		}

		constexpr void start(float periodLength, float amplitude, float asymmetry, bool fromCurrent) noexcept
		{
			assert(periodLength > 0);
			assert(amplitude > 0);
			assert(asymmetry >= 0 && asymmetry <= 1);
			const auto firstPartLength = periodLength * (1 + asymmetry) / 2;
			const auto secondPartLength = periodLength - firstPartLength;
			if (!fromCurrent)
			{
				_currentLength = firstPartLength;
				_currentAmplitude = amplitude;
				_nextLength = secondPartLength;
				_currentRemaining = _currentLength;
			}
			else
			{
				assert(_currentRemaining > 0);
				const auto remainingRatio = _currentRemaining / _currentLength;
				if (_currentAmplitude > 0)
				{
					_currentLength = firstPartLength;
					_currentAmplitude = amplitude;
					_nextLength = secondPartLength;
				}
				else
				{
					assert(_nextLength == 0);
					_currentLength = secondPartLength;
					_currentAmplitude = -amplitude;
				}
				_currentRemaining = _currentLength * remainingRatio;
			}
		}

	private:
		float _currentLength = 0;
		float _currentAmplitude = 1;
		float _nextLength = 0;
		float _currentRemaining = 0;
	};
}
