// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

namespace aulos
{
	// Contains packed audio data.
	class Composition
	{
	public:
		// Loads a composition from text data.
		[[nodiscard]] static std::unique_ptr<Composition> create(const char* textData);

		virtual ~Composition() noexcept = default;

		// Returns true if the composition has loop markup.
		[[nodiscard]] virtual bool hasLoop() const noexcept = 0;
	};

	// Supported channel layouts.
	enum class ChannelLayout
	{
		Mono,   // 1 channel.
		Stereo, // 2 channels (interleaved left-right).
	};

	// Generates PCM audio for a composition.
	class Renderer
	{
	public:
		static constexpr unsigned kMinSamplingRate = 8'000;
		static constexpr unsigned kMaxSamplingRate = 48'000;

		// Creates a renderer for the composition.
		[[nodiscard]] static std::unique_ptr<Renderer> create(const Composition&, unsigned samplingRate, ChannelLayout, bool looping = false);

		virtual ~Renderer() noexcept = default;

		// Returns the number of bytes in a frame.
		[[nodiscard]] virtual size_t bytesPerFrame() const noexcept = 0;

		// Returns the channel layout.
		[[nodiscard]] virtual aulos::ChannelLayout channelLayout() const noexcept = 0;

		// Returns the start and end frame offsets of the loop.
		[[nodiscard]] virtual std::pair<size_t, size_t> loopRange() const noexcept = 0;

		// Renders the next part of the composition.
		// The composition is rendered in whole frames, where a frame is one sample for each channel.
		// Returns the number of frames written.
		[[nodiscard]] virtual size_t render(float* buffer, size_t maxFrames) noexcept = 0;

		// Restarts rendering from the beginning of the composition.
		virtual void restart() noexcept = 0;

		// Returns the sampling rate.
		[[nodiscard]] virtual unsigned samplingRate() const noexcept = 0;

		// Skips part of the composition.
		// The composition is skipped in whole frames, where a frame is one sample for each channel.
		// Returns the number of frames actually skipped,
		// which may be less than requested if the composition has ended.
		[[nodiscard]] virtual size_t skipFrames(size_t maxFrames) noexcept = 0;
	};
}
