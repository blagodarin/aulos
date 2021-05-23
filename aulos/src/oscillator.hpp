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
		constexpr explicit TriangleOscillator(float frequency, float magnitude) noexcept
			: _frequency{ frequency }
			, _doubleMagnitude{ 2 * magnitude }
		{
			assert(frequency > 0);
			assert(magnitude >= 0 && magnitude <= 1);
		}

		[[nodiscard]] float value(float offset) const noexcept
		{
			float dummy;
			return _doubleMagnitude * std::abs(std::modf(offset * _frequency, &dummy) - .5f);
		}

	private:
		const float _frequency;
		const float _doubleMagnitude;
	};
}
