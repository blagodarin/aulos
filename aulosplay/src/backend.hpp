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

	void runBackend(class PlayerCallbacks&, unsigned samplingRate, std::atomic<size_t>& offset, const std::atomic<bool>& stopFlag, const std::function<size_t(float*, size_t)>& mixFunction);
}
