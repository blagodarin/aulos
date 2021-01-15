// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "utils.hpp"

#if defined(_M_AMD64) || defined(_M_IX86) || defined(__amd64) || defined(__i386)
#	ifdef _MSC_VER
#		include <intrin.h>
#	else
#		include <x86intrin.h>
#	endif
#	define HAS_SSE 1
#else
#	define HAS_SSE 0
#endif

namespace aulosplay
{
	void monoToStereo(float* output, const float* input, size_t frames) noexcept
	{
#if HAS_SSE
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
