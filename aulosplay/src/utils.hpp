// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <primal/intrinsics.hpp>

#include <cstddef>

namespace aulosplay
{
	inline void monoToStereo(float* output, const float* input, size_t frames) noexcept
	{
#if PRIMAL_INTRINSICS_SSE
		for (; frames > 4; frames -= 4)
		{
			const auto mono = _mm_load_ps(input);
			_mm_store_ps(output, _mm_shuffle_ps(mono, mono, 0b01010000));
			_mm_store_ps(output + 4, _mm_shuffle_ps(mono, mono, 0b11111010));
			input += 4;
			output += 8;
		}
#endif
		for (; frames > 0; --frames)
		{
			*output++ = *input;
			*output++ = *input;
			++input;
		}
	}
}
