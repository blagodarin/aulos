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
			, _doubleMagnitude{ 2 * magnitude }
		{
			assert(period > 0);
			assert(magnitude >= 0 && magnitude <= 1);
		}

		[[nodiscard]] float value(float offset) const noexcept
		{
			float dummy;
			return _doubleMagnitude * std::abs(std::modf(offset / _period, &dummy) - .5f);
		}

	private:
		const float _period;
		const float _doubleMagnitude;
	};
}
