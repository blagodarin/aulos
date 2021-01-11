// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <string>

namespace aulosplay
{
	class Source
	{
	public:
		virtual ~Source() noexcept = default;

		virtual bool isStereo() const noexcept = 0;
		virtual size_t onRead(float* buffer, size_t maxFrames) noexcept = 0;
	};

	class PlayerCallbacks
	{
	public:
		virtual ~PlayerCallbacks() noexcept = default;

		virtual void onPlaybackError(std::string&& message) = 0;
		virtual void onPlaybackStarted() = 0;
		virtual void onPlaybackStopped() = 0;
	};

	class Player
	{
	public:
		[[nodiscard]] static std::unique_ptr<Player> create(PlayerCallbacks&, unsigned samplingRate);

		virtual ~Player() noexcept = default;

		virtual void play(const std::shared_ptr<Source>&) = 0;
		[[nodiscard]] virtual unsigned samplingRate() const noexcept = 0;
		virtual void stop() = 0;
	};
}
