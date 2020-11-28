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
			, _sourceDistance{ trackProperties._sourceDistance }
			, _sourceWidth{ trackProperties._sourceWidth }
			, _sourceOffset{ trackProperties._sourceOffset }
		{}

		int stereoDelay(Note note) const noexcept
		{
			constexpr int kLastNoteIndex = kNoteCount - 1;
			const auto noteAngle = static_cast<float>(2 * static_cast<int>(note) - kLastNoteIndex) / (2 * kLastNoteIndex);   // [-0.5, 0.5]
			const auto doubleSin = 2 * std::sin((noteAngle * _sourceWidth + _sourceOffset) * std::numbers::pi_v<float> / 2); // [-2.0, 2.0]
			const auto left = std::sqrt(1 + _sourceDistance * (_sourceDistance + doubleSin));
			const auto right = std::sqrt(1 + _sourceDistance * (_sourceDistance - doubleSin));
			const auto delta = left - right; // [-|doubleSin|, |doubleSin|] <= [-2, 2]
			return static_cast<int>(_headHalfDelay * delta);
		}

	private:
		const float _headHalfDelay = 0.f;
		const float _sourceDistance = 0.f;
		const float _sourceWidth = 0.f;
		const float _sourceOffset = 0.f;
	};
}
