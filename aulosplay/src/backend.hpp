// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <functional>
#include <numeric>

namespace aulosplay
{
	constexpr auto kChannels = 2u;
	constexpr auto kSimdAlignment = 16u;
	constexpr auto kFrameBytes = kChannels * sizeof(float);
	constexpr auto kFrameAlignment = std::lcm(kSimdAlignment, kFrameBytes) / kFrameBytes;

	class BackendCallbacks
	{
	public:
		virtual ~BackendCallbacks() noexcept = default;

		virtual size_t onDataRequested(float* output, size_t maxFrames, float* monoBuffer) noexcept = 0;
		virtual void onErrorReported(const char* function, unsigned code, const std::string& description) = 0;
		virtual void onStateChanged(bool playing) = 0;
	};

	void runBackend(BackendCallbacks&, unsigned samplingRate, const std::atomic<bool>& stopFlag);
}
