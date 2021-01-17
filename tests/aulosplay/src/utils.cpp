// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/src/backend.hpp>
#include <aulosplay/src/utils.hpp>

#include <algorithm>
#include <array>
#include <numeric>

#include <doctest.h>

TEST_CASE("utils_mono_to_stereo")
{
	constexpr size_t kTestFrames = 17;
	alignas(aulosplay::kSimdAlignment) std::array<float, kTestFrames * 2> stereo{};
	alignas(aulosplay::kSimdAlignment) std::array<float, kTestFrames> mono{};
	std::iota(mono.begin(), mono.end(), 1.f);
	aulosplay::monoToStereo(stereo.data(), mono.data(), kTestFrames);
	for (size_t i = 0; i < stereo.size(); ++i)
		CHECK(stereo[i] == mono[i / 2]);
}
