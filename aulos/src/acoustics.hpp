// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/common.hpp>

#include <cmath>
#include <numbers>

namespace aulos
{
	class CircularAcoustics
	{
	public:
		constexpr CircularAcoustics() noexcept = default;

		constexpr CircularAcoustics(const TrackProperties& trackProperties, unsigned samplingRate) noexcept
			: _headHalfDelay{ static_cast<float>(samplingRate) * trackProperties._headDelay / 2'000 }
			, _distance{ trackProperties._sourceDistance }
			, _angularOffset{ static_cast<float>(trackProperties._sourceOffset) }
			, _angularSize{ static_cast<float>(trackProperties._sourceWidth) }
		{}

		int stereoDelay(Note note) const noexcept
		{
			constexpr int kLastNoteIndex = kNoteCount - 1;
			const auto noteAngle = static_cast<float>(2 * static_cast<int>(note) - kLastNoteIndex) / (2 * kLastNoteIndex);      // [-0.5, 0.5]
			const auto doubleSin = 2 * std::sin((_angularOffset + noteAngle * _angularSize) * std::numbers::pi_v<float> / 180); // [-2.0, 2.0]
			const auto left = std::sqrt(1 + _distance * (_distance + doubleSin));
			const auto right = std::sqrt(1 + _distance * (_distance - doubleSin));
			const auto delta = left - right; // [-|doubleSin|, |doubleSin|] <= [-2, 2]
			return static_cast<int>(_headHalfDelay * delta);
		}

	private:
		const float _headHalfDelay = 0.f;
		const float _distance = 0.f;
		const float _angularOffset = 0.f;
		const float _angularSize = 0.f;
	};
}
