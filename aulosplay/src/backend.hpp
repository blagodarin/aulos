// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulosplay/player.hpp>

#include <atomic>
#include <functional>
#include <numeric>

namespace aulosplay
{
	constexpr auto kBackendChannels = 2u;
	constexpr auto kSimdAlignment = 16u;
	constexpr auto kBackendFrameBytes = kBackendChannels * sizeof(float);
	constexpr auto kBackendFrameAlignment = std::lcm(kSimdAlignment, kBackendFrameBytes) / kBackendFrameBytes;

	class BackendCallbacks
	{
	public:
		virtual ~BackendCallbacks() noexcept = default;

		virtual size_t onDataExpected(float* output, size_t maxFrames, float* monoBuffer) noexcept = 0;
		virtual void onDataProcessed() = 0;
		virtual void onErrorReported(PlaybackError) = 0;
		virtual void onErrorReported(const char* function, int code, const std::string& description) = 0;
	};

	void runBackend(BackendCallbacks&, unsigned samplingRate, const std::atomic<bool>& stopFlag);
}
