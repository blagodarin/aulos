// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace aulos
{
	// Supported channel layouts.
	enum class ChannelLayout
	{
		Mono = 1,   // 1 channel.
		Stereo = 2, // 2 channels (interleaved left-right).
	};

	// Rendered audio data format.
	class AudioFormat
	{
	public:
		constexpr AudioFormat(unsigned samplingRate, ChannelLayout channelLayout) noexcept
			: _samplingRate{ samplingRate }, _channelLayout{ channelLayout } {}

		[[nodiscard]] constexpr auto bytesPerFrame() const noexcept { return static_cast<unsigned char>(static_cast<unsigned>(_channelLayout) * sizeof(float)); }
		[[nodiscard]] constexpr auto channelCount() const noexcept { return static_cast<unsigned char>(_channelLayout); }
		[[nodiscard]] constexpr auto channelLayout() const noexcept { return _channelLayout; }
		[[nodiscard]] constexpr auto samplingRate() const noexcept { return _samplingRate; }

	private:
		unsigned _samplingRate = 0;
		ChannelLayout _channelLayout = ChannelLayout::Mono;
	};

	constexpr bool operator==(const AudioFormat& left, const AudioFormat& right) noexcept
	{
		return left.samplingRate() == right.samplingRate() && left.channelLayout() == right.channelLayout();
	}
}
