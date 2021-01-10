// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "utils.hpp"

namespace aulosplay
{
	void monoToStereo(float* output, const float* input, size_t frames) noexcept
	{
		for (const auto end = output + 2 * frames; output < end; ++input)
		{
			*output++ = *input;
			*output++ = *input;
		}
	}
}
