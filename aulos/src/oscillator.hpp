// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cmath>
#include <numbers>

namespace aulos
{
	// Y(0) = magnitude
	// Y(period / 2) = 0
	// Y(period) = magnitude
	class TriangleOscillator
	{
	public:
		constexpr explicit TriangleOscillator(float period, float magnitude) noexcept
			: _period{ period }
			, _magnitude{ magnitude }
		{
			assert(_period > 0);
			assert(_magnitude >= 0 && _magnitude <= 1);
		}

		void advance(float duration) noexcept
		{
			assert(duration >= 0);
			_offset = std::fmod(_offset + duration, _period);
		}

		void start(float offset) noexcept
		{
			assert(offset >= 0);
			_offset = std::fmod(offset, _period);
		}

		[[nodiscard]] float value() const noexcept
		{
			return _magnitude * std::abs(1 - 2 * _offset / _period);
		}

	private:
		const float _period;
		const float _magnitude;
		float _offset = 0;
	};
}
