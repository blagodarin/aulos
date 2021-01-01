// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cmath>

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
			assert(_currentSign == 1);
			_currentLength = _nextLength;
			_currentSign = -1;
			_nextLength = 0;
			_currentRemaining += _currentLength;
			return _currentRemaining > 0;
		}

		auto maxAdvance() const noexcept
		{
			return static_cast<unsigned>(std::ceil(_currentRemaining));
		}

		constexpr void restart(float periodLength, float asymmetry) noexcept
		{
			assert(periodLength > 0);
			assert(asymmetry >= 0 && asymmetry <= 1);
			assert(_currentRemaining > -1 && _currentRemaining <= 0 && _nextLength == 0);
			const auto firstPartLength = periodLength * (1 + asymmetry) / 2;
			const auto secondPartLength = periodLength - firstPartLength;
			for (;;)
			{
				_currentRemaining += firstPartLength;
				if (_currentRemaining > 0)
				{
					_currentLength = firstPartLength;
					_currentSign = 1;
					_nextLength = secondPartLength;
					break;
				}
				_currentRemaining += secondPartLength;
				if (_currentRemaining > 0)
				{
					_currentLength = secondPartLength;
					_currentSign = -1;
					_nextLength = 0;
					break;
				}
			}
		}

		constexpr auto currentPartLength() const noexcept
		{
			return _currentLength;
		}

		constexpr auto currentPartOffset() const noexcept
		{
			return _currentLength - _currentRemaining;
		}

		constexpr auto currentPartSign() const noexcept
		{
			return _currentSign;
		}

		constexpr void start(float periodLength, float asymmetry, bool fromCurrent) noexcept
		{
			assert(periodLength > 0);
			assert(asymmetry >= 0 && asymmetry <= 1);
			const auto firstPartLength = periodLength * (1 + asymmetry) / 2;
			const auto secondPartLength = periodLength - firstPartLength;
			if (!fromCurrent)
			{
				_currentLength = firstPartLength;
				_currentSign = 1;
				_nextLength = secondPartLength;
				_currentRemaining = _currentLength;
			}
			else
			{
				assert(_currentRemaining > 0);
				const auto remainingRatio = _currentRemaining / _currentLength;
				if (_currentSign > 0)
				{
					_currentLength = firstPartLength;
					_nextLength = secondPartLength;
				}
				else
				{
					assert(_nextLength == 0);
					_currentLength = secondPartLength;
				}
				_currentRemaining = _currentLength * remainingRatio;
			}
		}

	private:
		float _currentLength = 0;
		float _currentSign = 1;
		float _nextLength = 0;
		float _currentRemaining = 0;
	};
}
