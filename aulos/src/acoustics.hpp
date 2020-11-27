// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/common.hpp>

#include <cmath>
#include <numbers>

namespace aulos
{
	struct CircularAcoustics
	{
		const float _headRadius = 0.f;   // In samples.
		const float _sourceRadius = 0.f; // In head radiuses.
		const float _sourceSize = 0.f;   // In right angles.
		const float _sourceOffset = 0.f; // In right angles, zero is forward, positive is right.

		constexpr CircularAcoustics() noexcept = default;

		constexpr CircularAcoustics(const TrackProperties& trackProperties, unsigned samplingRate) noexcept
			: _headRadius{ static_cast<float>(samplingRate) * trackProperties._headRadius / 1'000 }
			, _sourceRadius{ trackProperties._sourceRadius }
			, _sourceSize{ trackProperties._sourceSize }
			, _sourceOffset{ trackProperties._sourceOffset }
		{}

		int stereoDelay(Note note) const noexcept
		{
			constexpr int kLastNoteIndex = kNoteCount - 1;
			const auto noteAngle = static_cast<float>(2 * static_cast<int>(note) - kLastNoteIndex) / (2 * kLastNoteIndex);  // [-0.5, 0.5]
			const auto doubleSin = 2 * std::sin((noteAngle * _sourceSize + _sourceOffset) * std::numbers::pi_v<float> / 2); // [-2.0, 2.0]
			const auto left = std::sqrt(1 + _sourceRadius * (_sourceRadius + doubleSin));
			const auto right = std::sqrt(1 + _sourceRadius * (_sourceRadius - doubleSin));
			const auto delta = left - right; // [-|doubleSin|, |doubleSin|]
			return static_cast<int>(_headRadius * delta);
		}
	};
}
